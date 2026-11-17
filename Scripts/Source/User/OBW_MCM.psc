Scriptname OBW_MCM extends SKI_ConfigBase

int _modeOption     = -1
int _bodyOption     = -1
int _femaleOption   = -1
int _maleOption     = -1
int _maleBuildOption = -1
int _scaleOption    = -1
int _orientOption   = -1
int _neckColorOption = -1
int _clothedRefitOption = -1
int _fantasyOption  = -1
int _unusualOption  = -1
int _bUnusualOption = -1
int _athleticOption = -1
int _naturalOption  = -1
int _curvyOption    = -1
int _baseBodyOption = -1
int _raceOption     = -1
int _keyOption      = -1
int _exclKeyOption  = -1
int _exportKeyOption = -1
int _resetKeysOption = -1
int _presetOfTargetOption = -1
int _reprocessOption = -1
int _biasOption     = -1
int _seedOption     = -1
int _reseedOption   = -1
int _debugOption    = -1
int _neuralOption   = -1

string[] _modeLabels
string[] _bodyLabels
string[] _baseLabels

string[] _exclPlugins   ; Exclusions page: plugin name per toggle (parallel to _exclOptions) - current page chunk
int[]    _exclOptions   ; Exclusions page: option ID per plugin
int      _exclPage      ; Exclusions: current page index (0-based)
int      _exclPerPage   ; Exclusions: plugins per page (<128 for the Papyrus array + MCM option cap)
int      _exclPrevOption
int      _exclNextOption

Event OnConfigInit()
    ModName = "OBodyNG Weight"

    Pages = new string[2]
    Pages[0] = "Settings"
    Pages[1] = "Exclusions"

    _modeLabels = new string[2]
    _modeLabels[0] = "Random"
    _modeLabels[1] = "Seeded (Deterministic)"

    _bodyLabels = new string[3]
    _bodyLabels[0] = "Procedural Morphs"
    _bodyLabels[1] = "OBody Sim Weight"
    _bodyLabels[2] = "Procedural Oriented"

    _baseLabels = new string[3]
    _baseLabels[0] = "Auto-detect"
    _baseLabels[1] = "CBBE (3BA)"
    _baseLabels[2] = "BHUNP"
EndEvent

Event OnPlayerLoadGame()
EndEvent

; SkyUI calls this on EVERY game load (including existing saves). OnPlayerLoadGame does NOT fire on
; Quest scripts, so this is the reliable place to (re)start OBW_Quest's persistent manual-assign poll.
Event OnGameReload()
    parent.OnGameReload()
    OBW_Quest q = Game.GetFormFromFile(0x000800, "OBodyNGWeight.esp") as OBW_Quest
    if q
        q.StartPoll()    ; arms the poll first (no Log dependency inside)
    endif
    OBW_Native.Log("OnGameReload fired, q=" + q)   ; diagnostic LAST (a Log failure can't block the poll)
EndEvent

Event OnPageReset(string page)
    if page == "Exclusions"
        BuildExclusionsPage()
        return
    endif

    SetCursorFillMode(TOP_TO_BOTTOM)

    int gm = OBW_Native.GetBodyMode()    ; 0 Procedural, 1 OBody Presets, 2 Procedural Oriented
    int wm = OBW_Native.GetMode()        ; 0 Random, 1 Seeded
    bool fem = OBW_Native.GetFemaleBodies()
    bool male = OBW_Native.GetMaleBodies()

    ; Grey out what the current mode ignores, so only the relevant options are active.
    ; Procedural distribution dials: procedural modes (0/2) only — OBody Sim Weight (1) keeps the preset shape.
    int procFlag = OPTION_FLAG_DISABLED
    if gm != 1
        procFlag = OPTION_FLAG_NONE
    endif
    ; Female-only procedural dials also need Female bodies on.
    int femProcFlag = OPTION_FLAG_DISABLED
    if gm != 1 && fem
        femProcFlag = OPTION_FLAG_NONE
    endif
    ; Male build: procedural male tuning -> procedural mode + Male bodies on.
    int maleBuildFlag = OPTION_FLAG_DISABLED
    if gm != 1 && male
        maleBuildFlag = OPTION_FLAG_NONE
    endif
    ; Preset orientation: only Procedural Oriented (2) blends toward the preset.
    int orientFlag = OPTION_FLAG_DISABLED
    if gm == 2
        orientFlag = OPTION_FLAG_NONE
    endif
    ; Seed: only meaningful in Seeded weight mode (1).
    int seedFlag = OPTION_FLAG_DISABLED
    if wm == 1
        seedFlag = OPTION_FLAG_NONE
    endif

    ; Base-body selection (0 auto/unknown, 1 CBBE, 2 BHUNP) also selects the matching female BodyNet.
    ; Realism-axis gating: Natural (BHUNP-flavor) is for CBBE
    ; users, Curvy (3BA-flavor) is for BHUNP users. Disable the one that doesn't match the resolved base; show
    ; both when ambiguous. Both still require procedural mode + Female bodies (femProcFlag).
    int base = OBW_Native.GetBaseBody()
    int naturalFlag = femProcFlag
    int curvyFlag = femProcFlag
    if base == 2
        naturalFlag = OPTION_FLAG_DISABLED
    elseif base == 1
        curvyFlag = OPTION_FLAG_DISABLED
    endif

    ; ── LEFT COLUMN: global settings first ──────────────────────────────
    ; Generation: the two mode selectors + the global size dial (apply to everything).
    AddHeaderOption("Generation")
    _bodyOption = AddMenuOption("Generation mode", _bodyLabels[gm])
    _modeOption = AddMenuOption("Weight mode", _modeLabels[wm])
    _biasOption = AddSliderOption("Bias (size)", OBW_Native.GetBias(), "{0}")
    ; BodyNet: greyed out unless at least one model file (bodynet_*.obwnet) actually loaded, so the
    ; toggle can never be switched on into a no-op.
    int neuralFlag = OPTION_FLAG_DISABLED
    if OBW_Native.GetNeuralAvailable()
        neuralFlag = OPTION_FLAG_NONE
    endif
    _neuralOption = AddToggleOption("AI body generation", OBW_Native.GetNeuralBody(), neuralFlag)
    AddEmptyOption()

    ; Sexes: which bodies OBW manages + male-only tuning.
    AddHeaderOption("Sexes")
    _femaleOption = AddToggleOption("Female bodies", fem)
    _maleOption = AddToggleOption("Male bodies", male)
    _maleBuildOption = AddSliderOption("Male build", OBW_Native.GetMaleBuild(), "{2}x", maleBuildFlag)
    AddEmptyOption()

    ; Apply & hotkey.
    AddHeaderOption("Apply")
    _reprocessOption = AddTextOption("Reprocess all loaded NPCs", "[ Click ]")
    _presetOfTargetOption = AddTextOption("Preset of target NPC", "[ Aim + Click ]")
    _keyOption = AddKeyMapOption("Re-roll key", OBW_Native.GetReRollKey())
    _exclKeyOption = AddKeyMapOption("Exclude target key", OBW_Native.GetExcludeKey())
    _exportKeyOption = AddKeyMapOption("Export body preset key", OBW_Native.GetExportKey())
    _resetKeysOption = AddTextOption("Reset keybinds", "[ Click ]")

    ; ── RIGHT COLUMN: per-mode / distribution settings ──────────────────
    ; Procedural variety: the distribution dials for the procedural modes (0 / 2).
    AddHeaderOption("Procedural Variety")
    _scaleOption = AddSliderOption("Morph intensity", OBW_Native.GetMorphScale(), "{2}x", procFlag)
    _fantasyOption = AddSliderOption("Fantasy NPCs", OBW_Native.GetFantasyRatio() * 100.0, "{0}%", procFlag)
    _unusualOption = AddSliderOption("Unusual bodies", OBW_Native.GetUnusualRatio() * 100.0, "{0}%", procFlag)
    _bUnusualOption = AddSliderOption("Unusual breasts", OBW_Native.GetBreastUnusualRatio() * 100.0, "{0}%", femProcFlag)
    _athleticOption = AddSliderOption("Athletic women", OBW_Native.GetAthleticRatio() * 100.0, "{0}%", femProcFlag)
    _naturalOption = AddSliderOption("Natural women", OBW_Native.GetNaturalRatio() * 100.0, "{0}%", naturalFlag)
    _curvyOption = AddSliderOption("Curvy women", OBW_Native.GetCurvyRatio() * 100.0, "{0}%", curvyFlag)
    _baseBodyOption = AddMenuOption("Base body", _baseLabels[OBW_Native.GetBaseBodyPref()])
    _raceOption = AddSliderOption("Race coherence", OBW_Native.GetRaceCoherence() * 100.0, "{0}%", procFlag)
    AddEmptyOption()

    ; OBody preset mode (Procedural Oriented blend strength).
    AddHeaderOption("OBody Preset Mode")
    _orientOption = AddSliderOption("Preset orientation", OBW_Native.GetPresetOrient() * 100.0, "{0}%", orientFlag)
    AddEmptyOption()

    ; Appearance (all modes): neck-seam color match - how strongly the head tint is pulled to the body tone.
    AddHeaderOption("Appearance")
    _neckColorOption = AddSliderOption("Neck color match", OBW_Native.GetNeckColorFix() * 100.0, "{0}%")
    _clothedRefitOption = AddSliderOption("Clothed refit", OBW_Native.GetClothedRefit() * 100.0, "{0}%")

    ; Seed (only meaningful in Seeded weight mode).
    AddHeaderOption("Seed (Seeded Mode)")
    int curSeed = OBW_Native.GetSeed()
    _seedOption   = AddTextOption("Current seed", curSeed as string, seedFlag)
    _reseedOption = AddTextOption("Generate new seed", "[ Click ]", seedFlag)
    AddEmptyOption()

    AddHeaderOption("Debug")
    _debugOption = AddToggleOption("Debug logging", OBW_Native.GetDebugLog())
EndEvent

; Exclusions page: one checkbox per plugin that adds NPCs. Checked = OBW leaves that mod's NPCs alone.
Function BuildExclusionsPage()
    SetCursorFillMode(TOP_TO_BOTTOM)
    if _exclPerPage == 0
        _exclPerPage = 120   ; <128 to fit the Papyrus array + the SkyUI per-page option cap (leave room for nav)
    endif
    int total = OBW_Native.GetNpcPluginCount()
    if total == 0
        AddHeaderOption("Exclude NPCs by plugin")
        AddTextOption("(no NPC-adding plugins found)", "", OPTION_FLAG_DISABLED)
        return
    endif
    int pageCount = ((total - 1) / _exclPerPage) + 1
    if _exclPage >= pageCount
        _exclPage = 0
    endif
    AddHeaderOption("Exclude NPCs by plugin (page " + (_exclPage + 1) + "/" + pageCount + " - " + total + " plugins)")
    if pageCount > 1
        _exclPrevOption = AddTextOption("<<< Prev page", "")
        _exclNextOption = AddTextOption("Next page >>>", "")
    else
        _exclPrevOption = -1
        _exclNextOption = -1
    endif
    _exclPlugins = OBW_Native.GetNpcPluginsPage(_exclPage, _exclPerPage)
    _exclOptions = new int[128]
    int i = 0
    while i < _exclPlugins.Length
        _exclOptions[i] = AddToggleOption(_exclPlugins[i], OBW_Native.IsPluginExcluded(_exclPlugins[i]))
        i += 1
    endwhile
EndFunction

Event OnOptionMenuOpen(int option)
    if option == _modeOption
        SetMenuDialogOptions(_modeLabels)
        SetMenuDialogStartIndex(OBW_Native.GetMode())
        SetMenuDialogDefaultIndex(1)
    elseif option == _bodyOption
        SetMenuDialogOptions(_bodyLabels)
        SetMenuDialogStartIndex(OBW_Native.GetBodyMode())
        SetMenuDialogDefaultIndex(0)
    elseif option == _baseBodyOption
        SetMenuDialogOptions(_baseLabels)
        SetMenuDialogStartIndex(OBW_Native.GetBaseBodyPref())
        SetMenuDialogDefaultIndex(0)
    endif
EndEvent

Event OnOptionMenuAccept(int option, int index)
    if option == _modeOption
        OBW_Native.SetMode(index)
        ForcePageReset()   ; weight mode gates the Seed group -> redraw to recompute flags
    elseif option == _bodyOption
        OBW_Native.SetBodyMode(index)
        ForcePageReset()   ; generation mode gates the procedural / preset / orientation groups
    elseif option == _baseBodyOption
        OBW_Native.SetBaseBodyPref(index)
        SetMenuOptionValue(_baseBodyOption, _baseLabels[index])
        ForcePageReset()   ; base body gates which realism toggle (Natural / Curvy) is enabled
    endif
EndEvent

Event OnOptionSliderOpen(int option)
    if option == _biasOption
        SetSliderDialogStartValue(OBW_Native.GetBias())
        SetSliderDialogDefaultValue(0.0)
        SetSliderDialogRange(-50.0, 50.0)
        SetSliderDialogInterval(1.0)
    elseif option == _maleBuildOption
        SetSliderDialogStartValue(OBW_Native.GetMaleBuild())
        SetSliderDialogDefaultValue(1.0)
        SetSliderDialogRange(0.0, 2.0)
        SetSliderDialogInterval(0.05)
    elseif option == _scaleOption
        SetSliderDialogStartValue(OBW_Native.GetMorphScale())
        SetSliderDialogDefaultValue(1.0)
        SetSliderDialogRange(0.0, 2.5)
        SetSliderDialogInterval(0.05)
    elseif option == _orientOption
        SetSliderDialogStartValue(OBW_Native.GetPresetOrient() * 100.0)
        SetSliderDialogDefaultValue(50.0)
        SetSliderDialogRange(0.0, 100.0)
        SetSliderDialogInterval(5.0)
    elseif option == _neckColorOption
        SetSliderDialogStartValue(OBW_Native.GetNeckColorFix() * 100.0)
        SetSliderDialogDefaultValue(50.0)
        SetSliderDialogRange(0.0, 100.0)
        SetSliderDialogInterval(5.0)
    elseif option == _clothedRefitOption
        SetSliderDialogStartValue(OBW_Native.GetClothedRefit() * 100.0)
        SetSliderDialogDefaultValue(10.0)
        SetSliderDialogRange(0.0, 50.0)
        SetSliderDialogInterval(1.0)
    elseif option == _fantasyOption
        SetSliderDialogStartValue(OBW_Native.GetFantasyRatio() * 100.0)
        SetSliderDialogDefaultValue(15.0)
        SetSliderDialogRange(0.0, 100.0)
        SetSliderDialogInterval(5.0)
    elseif option == _unusualOption
        SetSliderDialogStartValue(OBW_Native.GetUnusualRatio() * 100.0)
        SetSliderDialogDefaultValue(6.0)
        SetSliderDialogRange(0.0, 50.0)
        SetSliderDialogInterval(1.0)
    elseif option == _bUnusualOption
        SetSliderDialogStartValue(OBW_Native.GetBreastUnusualRatio() * 100.0)
        SetSliderDialogDefaultValue(6.0)
        SetSliderDialogRange(0.0, 50.0)
        SetSliderDialogInterval(1.0)
    elseif option == _athleticOption
        SetSliderDialogStartValue(OBW_Native.GetAthleticRatio() * 100.0)
        SetSliderDialogDefaultValue(15.0)
        SetSliderDialogRange(0.0, 100.0)
        SetSliderDialogInterval(5.0)
    elseif option == _naturalOption
        SetSliderDialogStartValue(OBW_Native.GetNaturalRatio() * 100.0)
        SetSliderDialogDefaultValue(20.0)
        SetSliderDialogRange(0.0, 100.0)
        SetSliderDialogInterval(5.0)
    elseif option == _curvyOption
        SetSliderDialogStartValue(OBW_Native.GetCurvyRatio() * 100.0)
        SetSliderDialogDefaultValue(0.0)
        SetSliderDialogRange(0.0, 100.0)
        SetSliderDialogInterval(5.0)
    elseif option == _raceOption
        SetSliderDialogStartValue(OBW_Native.GetRaceCoherence() * 100.0)
        SetSliderDialogDefaultValue(100.0)
        SetSliderDialogRange(0.0, 100.0)
        SetSliderDialogInterval(5.0)
    endif
EndEvent

; Re-roll key rebind. SkyUI handles the "press a key" prompt.
Event OnOptionKeyMapChange(int option, int keyCode, string conflictControl, string conflictName)
    if option == _keyOption
        OBW_Native.SetReRollKey(keyCode)
        SetKeyMapOptionValue(_keyOption, keyCode)
        ; Robust path: re-register the key DIRECTLY on the handler quest. The old
        ; ModEvent route could silently fail (OBW_Quest.OnPlayerLoadGame doesn't fire on
        ; a Quest script, so its mod-event registration may be missing after a reload,
        ; leaving the rebind with no listener). 0x800 = OBW_QuestRecord in the plugin.
        OBW_Quest q = Game.GetFormFromFile(0x000800, "OBodyNGWeight.esp") as OBW_Quest
        if q
            q.BindReRollKey()
        endif
        ; Keep the ModEvent as a fallback for any setup where the form lookup fails.
        int h = ModEvent.Create("OBW_RebindKey")
        ModEvent.Send(h)
    elseif option == _exclKeyOption
        OBW_Native.SetExcludeKey(keyCode)
        SetKeyMapOptionValue(_exclKeyOption, keyCode)
        OBW_Quest q2 = Game.GetFormFromFile(0x000800, "OBodyNGWeight.esp") as OBW_Quest
        if q2
            q2.BindExcludeKey()
        endif
        int h2 = ModEvent.Create("OBW_RebindKey")
        ModEvent.Send(h2)
    elseif option == _exportKeyOption
        OBW_Native.SetExportKey(keyCode)
        SetKeyMapOptionValue(_exportKeyOption, keyCode)
        ; No re-registration needed: the C++ key sink reads the configured key live.
    endif
EndEvent

Event OnOptionSliderAccept(int option, float value)
    if option == _biasOption
        OBW_Native.SetBias(value)
        SetSliderOptionValue(_biasOption, value, "{0}")
    elseif option == _maleBuildOption
        OBW_Native.SetMaleBuild(value)
        SetSliderOptionValue(_maleBuildOption, value, "{2}x")
    elseif option == _scaleOption
        OBW_Native.SetMorphScale(value)
        SetSliderOptionValue(_scaleOption, value, "{2}x")
    elseif option == _orientOption
        OBW_Native.SetPresetOrient(value / 100.0)
        SetSliderOptionValue(_orientOption, value, "{0}%")
    elseif option == _fantasyOption
        OBW_Native.SetFantasyRatio(value / 100.0)
        SetSliderOptionValue(_fantasyOption, value, "{0}%")
    elseif option == _unusualOption
        OBW_Native.SetUnusualRatio(value / 100.0)
        SetSliderOptionValue(_unusualOption, value, "{0}%")
    elseif option == _bUnusualOption
        OBW_Native.SetBreastUnusualRatio(value / 100.0)
        SetSliderOptionValue(_bUnusualOption, value, "{0}%")
    elseif option == _athleticOption
        OBW_Native.SetAthleticRatio(value / 100.0)
        SetSliderOptionValue(_athleticOption, value, "{0}%")
    elseif option == _naturalOption
        OBW_Native.SetNaturalRatio(value / 100.0)
        SetSliderOptionValue(_naturalOption, value, "{0}%")
    elseif option == _curvyOption
        OBW_Native.SetCurvyRatio(value / 100.0)
        SetSliderOptionValue(_curvyOption, value, "{0}%")
    elseif option == _raceOption
        OBW_Native.SetRaceCoherence(value / 100.0)
        SetSliderOptionValue(_raceOption, value, "{0}%")
    elseif option == _neckColorOption
        OBW_Native.SetNeckColorFix(value / 100.0)
        SetSliderOptionValue(_neckColorOption, value, "{0}%")
    elseif option == _clothedRefitOption
        OBW_Native.SetClothedRefit(value / 100.0)
        SetSliderOptionValue(_clothedRefitOption, value, "{0}%")
    endif
EndEvent

Event OnOptionSelect(int option)
    if option == _reseedOption
        OBW_Native.RegenerateSeed()
        int newSeed = OBW_Native.GetSeed()
        SetTextOptionValue(_seedOption, newSeed as string)
        if OBW_Native.GetDebugLog()
            Debug.Notification("OBodyNG Weight: new seed generated.")
        endif
    elseif option == _femaleOption
        bool newFem = !OBW_Native.GetFemaleBodies()
        OBW_Native.SetFemaleBodies(newFem)
        SetToggleOptionValue(_femaleOption, newFem)
        ; Female-only procedural dials follow this toggle (and the mode).
        int ff = OPTION_FLAG_DISABLED
        if newFem && OBW_Native.GetBodyMode() != 1
            ff = OPTION_FLAG_NONE
        endif
        SetOptionFlags(_bUnusualOption, ff)
        SetOptionFlags(_athleticOption, ff)
        ; Natural / Curvy also honor the base-body gate (Natural for CBBE, Curvy for BHUNP).
        int nf = ff
        int cf = ff
        int base = OBW_Native.GetBaseBody()
        if base == 2
            nf = OPTION_FLAG_DISABLED
        elseif base == 1
            cf = OPTION_FLAG_DISABLED
        endif
        SetOptionFlags(_naturalOption, nf)
        SetOptionFlags(_curvyOption, cf)
    elseif option == _maleOption
        bool newVal = !OBW_Native.GetMaleBodies()
        OBW_Native.SetMaleBodies(newVal)
        SetToggleOptionValue(_maleOption, newVal)
        ; Male build follows this toggle (and the mode).
        int mf = OPTION_FLAG_DISABLED
        if newVal && OBW_Native.GetBodyMode() != 1
            mf = OPTION_FLAG_NONE
        endif
        SetOptionFlags(_maleBuildOption, mf)
    elseif option == _neuralOption
        bool newNeu = !OBW_Native.GetNeuralBody()
        OBW_Native.SetNeuralBody(newNeu)
        SetToggleOptionValue(_neuralOption, newNeu)
        ; Bodies only change when an NPC is next processed - point the player at Reprocess.
        if newNeu
            Debug.Notification("OBW: AI body generation on - use Reprocess to apply.")
        else
            Debug.Notification("OBW: AI body generation off - use Reprocess to apply.")
        endif
    elseif option == _debugOption
        bool newDbg = !OBW_Native.GetDebugLog()
        OBW_Native.SetDebugLog(newDbg)
        SetToggleOptionValue(_debugOption, newDbg)
    elseif option == _reprocessOption
        SendModEvent("OBW_Reprocess")   ; OBW_Quest re-queues all loaded NPCs + arms the drain
    elseif option == _presetOfTargetOption
        ; Show which OBody preset shaped the NPC you're aiming at (modes 1 & 2). OBW unassigns the preset in
        ; OBody after taking over, so OBody itself reports "unknown"; OBW kept the name.
        Actor tgt = Game.GetCurrentCrosshairRef() as Actor
        if !tgt
            Debug.MessageBox("OBW: aim at an NPC first, then click this option.")
        else
            string pn = OBW_Native.GetActorPresetName(tgt)
            if pn == ""
                Debug.MessageBox(tgt.GetActorBase().GetName() + ": procedural body (no OBody preset).")
            else
                Debug.MessageBox(tgt.GetActorBase().GetName() + " is using OBody preset:\n" + pn)
            endif
        endif
    elseif option == _resetKeysOption
        ; Restore the defaults: re-roll = [ (DX scancode 26), exclude + export unbound (0).
        ; Fixes "I bound exclude and now can't clear it" - SkyUI's keymap widget can rebind but not unbind.
        OBW_Native.SetReRollKey(26)
        OBW_Native.SetExcludeKey(0)
        OBW_Native.SetExportKey(0)
        SetKeyMapOptionValue(_keyOption, 26)
        SetKeyMapOptionValue(_exclKeyOption, 0)
        SetKeyMapOptionValue(_exportKeyOption, 0)
        int hReset = ModEvent.Create("OBW_RebindKey")   ; re-arm the C++/Papyrus key state to the new values
        ModEvent.Send(hReset)
        Debug.Notification("OBW: keybinds reset (re-roll = [ , exclude and export unbound).")
    elseif _exclPlugins
        ; Exclusions page: page nav first, then the per-plugin checkboxes.
        if _exclPrevOption != -1 && option == _exclPrevOption
            _exclPage -= 1
            if _exclPage < 0
                _exclPage = (OBW_Native.GetNpcPluginCount() - 1) / _exclPerPage   ; wrap to last page
            endif
            ForcePageReset()
            return
        elseif _exclNextOption != -1 && option == _exclNextOption
            int pgs = ((OBW_Native.GetNpcPluginCount() - 1) / _exclPerPage) + 1
            _exclPage += 1
            if _exclPage >= pgs
                _exclPage = 0   ; wrap to first page
            endif
            ForcePageReset()
            return
        endif
        int i = 0
        while i < _exclPlugins.Length
            if option == _exclOptions[i]
                bool nx = !OBW_Native.IsPluginExcluded(_exclPlugins[i])
                OBW_Native.SetPluginExcluded(_exclPlugins[i], nx)
                SetToggleOptionValue(_exclOptions[i], nx)
                return
            endif
            i += 1
        endwhile
    endif
EndEvent

Event OnOptionHighlight(int option)
    if option == _modeOption
        SetInfoText("How bodies are decided.\nRandom: every NPC gets her own random body, kept for this playthrough.\nSeeded: one seed decides everyone - the same seed always gives the same bodies, so it can be shared with a friend for an identical world.")
    elseif option == _bodyOption
        SetInfoText("Where bodies come from.\nProcedural: fully generated bodies - you need no presets.\nOBody Sim Weight: keeps your OBody presets but varies each NPC's size, so NPCs sharing a preset aren't clones.\nProcedural Oriented: generated bodies nudged toward each NPC's OBody preset (blend set below).")
    elseif option == _femaleOption
        SetInfoText("Turn OBW's bodies on or off for women. Off = women are left to OBody or the game. Applies to NPCs that load in from now on.")
    elseif option == _maleOption
        SetInfoText("Turn OBW's bodies on or off for men. Off = men are left to OBody or the game. Applies to NPCs that load in from now on.")
    elseif option == _maleBuildOption
        SetInfoText("Overall size of men's builds. Lower for slimmer men, higher for beefier. Everything scales together so proportions stay natural.")
    elseif option == _scaleOption
        SetInfoText("Overall body size for everyone. 1.0 is the standard look; lower makes bodies smaller, higher makes them bigger.")
    elseif option == _orientOption
        SetInfoText("Only for 'Procedural Oriented' mode: how much each NPC's body leans toward her OBody preset. 0% = fully generated, 100% = the preset itself, in between = a blend.")
    elseif option == _fantasyOption
        SetInfoText("How many NPCs get an exaggerated 'bombshell' body. The rest look natural. 15% = mostly realistic with the occasional bombshell; 100% = everyone exaggerated.")
    elseif option == _unusualOption
        SetInfoText("How many NPCs get an unusual, out-of-the-ordinary body - either very tiny or very heavy, with atypical proportions. Rare by default. 0% turns it off.")
    elseif option == _bUnusualOption
        SetInfoText("How many NPCs get unusual breasts - either very saggy or very perky, beyond the normal range for their size. Rare by default. 0% turns it off.")
    elseif option == _athleticOption
        SetInfoText("How many women have athletic muscle tone - visible abs and definition. The rest are soft or normal. 0% turns it off. (Men get tone from their build automatically.)")
    elseif option == _naturalOption
        SetInfoText("How many women get a more natural, less-exaggerated shape: a moderate, wider, lower, closer bust, a softer waist and belly. A realistic look for those who want it. The rest keep the default curvier look.")
    elseif option == _curvyOption
        SetInfoText("How many women get a fuller, curvier, more exaggerated shape - the opposite of Natural. For BHUNP users who want some of the curvier look. The rest keep the default. Off by default.")
    elseif option == _baseBodyOption
        SetInfoText("Which female body you use. It selects the matching AI generator and decides which realism option is shown: Natural for CBBE / 3BA, Curvy for BHUNP. Leave on Auto-detect unless it guesses wrong, then set it yourself.")
    elseif option == _clothedRefitOption
        SetInfoText("A gentle 'dressed vs nude' adjustment. When an NPC is wearing clothes her body is trimmed a little so the clothes sit better; undressed, she has her full body. 0% = dressed and nude look the same.")
    elseif option == _raceOption
        SetInfoText("How much an NPC's race shapes her body, so the world feels like Tamriel: Orcs bigger and broader, Bosmer petite, Altmer tall and slim, and so on. The full range is still possible, just rarer. 0% = race makes no difference.")
    elseif option == _keyOption
        SetInfoText("Hotkey to give the NPC you're looking at a brand-new random body. Default is the [ key. If it clashes with OBody, set OBody's own key to None.")
    elseif option == _exclKeyOption
        SetInfoText("Hotkey: aim at an NPC and press it to leave that one NPC alone (or include her again). Handy for a follower whose body you want to keep. Unbound by default.")
    elseif option == _exportKeyOption
        SetInfoText("Hotkey: aim at someone and press it to save their current body as a BodySlide preset file (aiming at no one saves your own). The preset appears in BodySlide's preset list, ready to use or share. Unbound by default.")
    elseif option == _biasOption
        SetInfoText("Makes everyone a little heavier (+) or leaner (-). A quick way to shift overall body size across the whole game.")
    elseif option == _seedOption
        SetInfoText("The seed for this playthrough. The same seed always produces the same bodies.")
    elseif option == _reseedOption
        SetInfoText("Rolls a new seed. NPCs you've already met keep their bodies; only NPCs you haven't met yet use the new seed.")
    elseif option == _neuralOption
        SetInfoText("Uses AI to build bodies. Shapes tend to come out better proportioned. Use Reprocess after changing it.")
    elseif option == _debugOption
        SetInfoText("Writes extra detail to the log file. Leave it off - only turn it on if you're troubleshooting a problem.")
    elseif option == _reprocessOption
        SetInfoText("Re-applies bodies to the NPCs around you right now, without waiting to walk between areas. Use it after changing a setting. (In Random mode it also gives them fresh bodies.)")
    elseif option == _presetOfTargetOption
        SetInfoText("In OBody Sim Weight and Procedural Oriented modes, aim your crosshair at an NPC and click this to see which OBody preset their body is based on. (OBW takes the preset over, so OBody's own menu would just say 'unknown'.)")
    elseif option == _resetKeysOption
        SetInfoText("Restores the hotkeys to their defaults: re-roll back to the [ key, and the exclude and export keys unbound. Use this if you bound a key and want to clear it (the rebind widget can change a key but not remove it).")
    elseif _exclPlugins
        SetInfoText("Tick a mod to leave its NPCs alone - OBW won't touch them, so custom-bodied followers keep their look. Your choices are remembered across saves.")
    endif
EndEvent
