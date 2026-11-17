// OBodyNG Weight - procedural and learned NPC body generation for Skyrim SE/AE.
// Copyright (C) 2026 Geovane H. F. Pires
//
// This program is free software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
// WARRANTY; see the GNU General Public License for details: <https://www.gnu.org/licenses/>.

#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include "WeightManager.hpp"
#include "BodyNet.hpp"
#include "Config.hpp"
#include "MorphInterface.hpp"

namespace OBW::PapyrusBindings {
bool Register(RE::BSScript::IVirtualMachine*);
void InstallInputSink();   // C++ re-roll/exclude hotkeys (replaces Papyrus RegisterForKey)
}

namespace {
// Independent distribution sink: when an NPC's 3D loads in a PROCEDURAL body mode, watch it so the
// procedural fallback can self-distribute if OBody never fires for it (empty preset library, or an
// NPC OBody's own distribution skips). In OBody-preset mode we stay off (that mode relies on OBody).
class ActorLoadSink final : public RE::BSTEventSink<RE::TESObjectLoadedEvent> {
public:
    static ActorLoadSink* GetSingleton() {
        static ActorLoadSink s;
        return &s;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESObjectLoadedEvent* a_event,
                                          RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override {
        if (!a_event || !a_event->loaded) return RE::BSEventNotifyControl::kContinue;
        auto& wm = OBW::WeightManager::GetSingleton();
        auto* actor = RE::TESForm::LookupByID<RE::Actor>(a_event->formID);
        if (!actor || actor == RE::PlayerCharacter::GetSingleton()) return RE::BSEventNotifyControl::kContinue;
        if (!actor->GetActorBase() || !actor->HasKeywordString("ActorTypeNPC"))
            return RE::BSEventNotifyControl::kContinue;   // humanoid NPCs only (skip creatures/objects)
        // Neck-seam color fix for EVERY humanoid NPC, in ALL body modes (the seam is a pre-existing facegen/RSV
        // issue, not OBW-specific). The deferred re-applies hold it past RSV's late head re-apply.
        wm.ScheduleNeckColor(actor->GetFormID());
        if (wm.GetBodyMode() == OBW::BodyMode::kOBodyPreset) return RE::BSEventNotifyControl::kContinue;
        wm.WatchForFallback(actor->GetFormID());
        return RE::BSEventNotifyControl::kContinue;
    }
};

// Event-driven OBody re-assert (closes the "OBW loses to OBody" race for good). OBody NG announces EVERY
// morph apply with the "Obody_ApplyMorph" ModEvent, sent right AFTER its ApplyBodyMorphs - including its
// TESEquipEvent re-applies (outfit changes), which the 3D-load watch never saw: OBW won on load, then the
// NPC changed clothes, OBody re-applied, and OBody stayed on top until the next load. Now every OBody apply
// enqueues the actor and the OBW drain (deferred Papyrus poll, off OBody's call stack - the safe pattern)
// re-applies on top, so OBW always lands LAST.
// THREADING: in OBody's performance mode this event arrives on OBody's own detached thread -> this sink is
// queue-only (QueueForMorphs is mutex-guarded; no SKEE/engine work here). Gates (exclusion / body mode /
// per-sex toggles) are enforced drain-side in OBW_Quest.ApplyMorphs, where they belong.
// LOOP-SAFE: OBW's re-apply never enters OBody's apply pipeline (SKEE SetMorph only; the preset path
// unassigns with doNotApplyMorphs=true), so it never re-fires this event.
class OBodyApplySink final : public RE::BSTEventSink<SKSE::ModCallbackEvent> {
public:
    static OBodyApplySink* GetSingleton() {
        static OBodyApplySink s;
        return &s;
    }
    RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* a_event,
                                          RE::BSTEventSource<SKSE::ModCallbackEvent>*) override {
        if (!a_event || a_event->eventName != "Obody_ApplyMorph")
            return RE::BSEventNotifyControl::kContinue;
        auto* actor = a_event->sender ? a_event->sender->As<RE::Actor>() : nullptr;
        if (!actor) return RE::BSEventNotifyControl::kContinue;
        if (actor == RE::PlayerCharacter::GetSingleton()) {
            // The player assigned his own body through OBody: that choice wins. Drop any OBW morphs still
            // on him (they STACK with the OBody preset otherwise).
            OBW::WeightManager::GetSingleton().CleanPlayerMorphs();
            return RE::BSEventNotifyControl::kContinue;
        }
        auto& wm = OBW::WeightManager::GetSingleton();
        const RE::FormID id = actor->GetFormID();
        // Re-fire window: an Obody_ApplyMorph landing right after OUR apply is the echo of our own
        // rebuild, not a genuine OBody distribution - re-queuing it looped the re-assert forever
        // (the "bodies visibly re-morph from time to time" report). Only the sink needs this guard:
        // the Papyrus OnActorGenerated path has its own one-shot.
        if (wm.RecentlyApplied(id)) {
            if (OBW::g_debugLog) SKSE::log::info("sink: Obody_ApplyMorph echo for {:08X} suppressed (our own rebuild)", id);
            return RE::BSEventNotifyControl::kContinue;
        }
        if (OBW::g_debugLog) SKSE::log::info("sink: Obody_ApplyMorph -> queue {:08X}", id);
        wm.QueueForMorphs(actor);
        return RE::BSEventNotifyControl::kContinue;
    }
};

// Clothed-refit sink: OBW's own "dressed vs nude" body adjustment (the desirable half of OBody's ORefit, but on
// OBW's own key so it survives the re-assert). A body-slot armor equip/unequip on an OBW-managed actor toggles a
// small negative trim on the soft sliders (dressed) or clears it (nude). LOOP-SAFE: ApplyClothedRefit rebuilds
// with ApplyBodyMorphs (no armor re-equip -> no new equip event), and it self-gates on the cached clothed state,
// so OBW's own preset-path re-equip (same state) is a no-op. Deferred to the game thread (equip events can arrive
// on a worker), where the current worn state is re-read.
class ClothedRefitSink final : public RE::BSTEventSink<RE::TESEquipEvent> {
public:
    static ClothedRefitSink* GetSingleton() {
        static ClothedRefitSink s;
        return &s;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event,
                                          RE::BSTEventSource<RE::TESEquipEvent>*) override {
        if (!a_event || !a_event->actor) return RE::BSEventNotifyControl::kContinue;
        auto* actor = a_event->actor->As<RE::Actor>();
        if (!actor || actor == RE::PlayerCharacter::GetSingleton()) return RE::BSEventNotifyControl::kContinue;
        auto& wm = OBW::WeightManager::GetSingleton();
        if (wm.GetClothedRefit() <= 0.0f) return RE::BSEventNotifyControl::kContinue;   // feature off
        if (!wm.IsManaged(actor->GetFormID())) return RE::BSEventNotifyControl::kContinue;  // OBW-managed only
        // Only BODY-slot (32) armor changes affect the clothed/nude body; ignore weapons/jewelry/etc.
        auto* form = RE::TESForm::LookupByID(a_event->baseObject);
        auto* armo = form ? form->As<RE::TESObjectARMO>() : nullptr;
        if (!armo || !armo->HasPartOf(RE::BGSBipedObjectForm::BipedObjectSlot::kBody))
            return RE::BSEventNotifyControl::kContinue;
        const RE::FormID id = actor->GetFormID();
        if (OBW::g_debugLog) SKSE::log::info("refit: body-slot equip change on {:08X}", id);
        if (auto* task = SKSE::GetTaskInterface()) {
            task->AddTask([id]() {
                auto* a = RE::TESForm::LookupByID<RE::Actor>(id);
                if (!a) return;
                auto& w = OBW::WeightManager::GetSingleton();
                w.ApplyClothedRefit(a, w.IsBodyArmorWorn(a), false);   // self-gates on the cached state change
            });
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
}  // namespace

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);

    // Log
    auto path = SKSE::log::log_directory();
    if (path) {
        *path /= "OBodyNGWeight.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto logger = std::make_shared<spdlog::logger>("OBW", std::move(sink));
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::debug);
        spdlog::set_default_logger(std::move(logger));
    }

    // Read INI defaults (FOMOD-installed preset) before WeightManager is constructed.
    OBW::Config::Load();
    // Optional per-archetype weight/shape overrides (CSV) — after Config, before bodies are built.
    OBW::WeightManager::LoadArchetypeConfig();
    OBW::WeightManager::LoadMaleArchetypeConfig();
    OBW::WeightManager::LoadRaceConfig();
    // Plugin exclusion list (NPCs from these .esp/.esl/.esm are left untouched).
    OBW::Config::LoadExclusions();

    // Papyrus
    auto* papyrus = SKSE::GetPapyrusInterface();
    if (!papyrus) return false;
    papyrus->Register(OBW::PapyrusBindings::Register);

    // Cosave (UID unique across all SKSE plugins)
    auto* serial = SKSE::GetSerializationInterface();
    if (serial) {
        serial->SetUniqueID(OBW::WeightManager::kRecordUID);
        serial->SetSaveCallback(OBW::WeightManager::SaveCallback);
        serial->SetLoadCallback(OBW::WeightManager::LoadCallback);
        serial->SetRevertCallback(OBW::WeightManager::RevertCallback);
    }

    // NOTE: OBW no longer changes the actor's real weight (base->weight). Changing it
    // caused neck seams (the baked head facegen is at the editor weight and can't follow)
    // and outfit/body weight mismatches. Body size now comes purely from morphs. The
    // per-NPC weight value is kept as a "mock weight" that drives those morphs.

    // SKEE (RaceMenu) BodyMorph interface — acquired at kPostPostLoad, when SKEE is ready. Lets the
    // OBody-preset path apply all morphs from C++ (no Papyrus per-slider calls, no 128-array cap).
    if (auto* messaging = SKSE::GetMessagingInterface()) {
        messaging->RegisterListener([](SKSE::MessagingInterface::Message* a_msg) {
            if (a_msg->type == SKSE::MessagingInterface::kPostPostLoad) {
                SKEE::InterfaceExchangeMessage msg;
                const bool dispatched = SKSE::GetMessagingInterface()->Dispatch(
                    SKEE::InterfaceExchangeMessage::kExchangeInterface, &msg,
                    sizeof(SKEE::InterfaceExchangeMessage*), "skee");
                OBW::g_morph = nullptr;
                if (!dispatched) {
                    SKSE::log::error("SKEE interface test failed: exchange dispatch failed. OBW remains active; morph application disabled.");
                } else if (!msg.interfaceMap) {
                    SKSE::log::error("SKEE interface test failed: no interface map returned. OBW remains active; morph application disabled.");
                } else {
                    auto* candidate = static_cast<SKEE::IBodyMorphInterface*>(
                        msg.interfaceMap->QueryInterface("BodyMorph"));
                    if (!candidate) {
                        SKSE::log::error("SKEE interface test failed: BodyMorph not provided. OBW remains active; morph application disabled.");
                    } else {
                        const auto version = candidate->GetVersion();
                        if (version == 0) {
                            SKSE::log::error("SKEE interface test failed: BodyMorph reported version 0. OBW remains active; morph application disabled.");
                        } else {
                            OBW::g_morph = candidate;
                            SKSE::log::info("SKEE interface test passed: BodyMorph version {} acquired", version);
                        }
                    }
                }
            } else if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
                // Independent distribution: hook actor 3D-load so procedural mode works without OBody.
                if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) {
                    holder->AddEventSink<RE::TESObjectLoadedEvent>(ActorLoadSink::GetSingleton());
                    SKSE::log::info("OBW: actor-load sink installed (procedural self-distribution)");
                }
                // Event-driven re-assert: react to OBody's OWN apply announcements (incl. equip re-applies).
                if (auto* modEvents = SKSE::GetModCallbackEventSource()) {
                    modEvents->AddEventSink(OBodyApplySink::GetSingleton());
                    SKSE::log::info("OBW: Obody_ApplyMorph sink installed (event-driven re-assert)");
                }
                // Clothed-refit: OBW's own dressed-vs-nude body trim, driven by body-armor equip changes.
                if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) {
                    holder->AddEventSink<RE::TESEquipEvent>(ClothedRefitSink::GetSingleton());
                    SKSE::log::info("OBW: clothed-refit equip sink installed");
                }
                // C++ hotkeys (re-roll / exclude): work the instant a save loads, immune to VM congestion.
                OBW::PapyrusBindings::InstallInputSink();
            } else if (a_msg->type == SKSE::MessagingInterface::kPostLoadGame) {
                // The player's body belongs to him, unconditionally: strip any stale "OBW" morphs a
                // previous version (or the retired self-re-roll) left on him.
                OBW::WeightManager::GetSingleton().CleanPlayerMorphs();
            }
        });
    }

    // BodyNet: load the learned body model if present. Soft: a missing or malformed file only disables the
        // feature (named reason in the log) - OBW then behaves exactly as it always has.
        {
            using Slot = OBW::BodyNet::Slot;
            const struct { Slot slot; const char* name; const char* path; } kModels[] = {
                // RAW literals on purpose. Written as a plain string, "...OBodyNGWeight\bodynet..." turns
                // \b into a BACKSPACE and \S \P \O silently drop their backslash, so the path resolved to
                // "DataSKSEPluginsOBodyNGWeightodynet_f3ba.obwnet" and both models failed to open with the
                // MCM toggle greyed out (2026-08-31). Keep every model path as a raw literal.
                { Slot::Female,      "female/F3BA", R"(Data\SKSE\Plugins\OBodyNGWeight\bodynet_f3ba.obwnet)"   },
                { Slot::FemaleBHUNP, "female/BHUNP", R"(Data\SKSE\Plugins\OBodyNGWeight\bodynet_fbhunp.obwnet)" },
                { Slot::Male,        "male/HIMBO",   R"(Data\SKSE\Plugins\OBodyNGWeight\bodynet_mhimbo.obwnet)" },
            };
            for (const auto& mdl : kModels) {
                std::string err;
                if (OBW::BodyNet::Load(mdl.slot, mdl.path, err))
                    SKSE::log::info("BodyNet[{}]: loaded ({} sliders, {} archetypes)", mdl.name,
                                    OBW::BodyNet::SliderNames(mdl.slot).size(),
                                    OBW::BodyNet::ArchetypeCount(mdl.slot));
                else
                    SKSE::log::info("BodyNet[{}]: disabled - {}", mdl.name, err);
            }
            // Log the INI default beside the live flag: the two disagreeing is the signature of the
            // config not having been read (an INI saved with LF-only line endings, for one - the Win32
            // profile API wants CRLF), which would silently opt every new game in or out.
            SKSE::log::info("BodyNet: feature is {} (INI default {})",
                            OBW::WeightManager::GetSingleton().GetNeuralBody() ? "ON" : "off (opt-in)",
                            OBW::Config::g_defaultNeuralBody ? "on" : "off");
        }
        SKSE::log::info("OBodyNGWeight loaded");
    return true;
}
