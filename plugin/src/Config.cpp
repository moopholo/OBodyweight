#include "Config.hpp"
#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <windows.h>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

namespace OBW::Config {

namespace {

constexpr const char* kIniPath = R"(.\Data\SKSE\Plugins\OBodyNGWeight.ini)";

float ReadFloat(const char* section, const char* key, float fallback) {
    char buf[64]{};
    const DWORD n = GetPrivateProfileStringA(section, key, "", buf, sizeof(buf), kIniPath);
    if (n == 0) return fallback;
    try {
        return std::stof(std::string(buf, n));
    } catch (...) {
        return fallback;
    }
}

}  // namespace

void Load() {
    g_defaultMorphScale   = std::clamp(ReadFloat("Defaults", "MorphScale", 1.0f), 0.0f, 2.5f);
    g_defaultPresetOrient = std::clamp(ReadFloat("Defaults", "PresetOrient", 0.5f), 0.0f, 1.0f);
    g_defaultFantasyRatio = std::clamp(ReadFloat("Defaults", "FantasyRatio", 0.15f), 0.0f, 1.0f);
    g_defaultUnusualRatio       = std::clamp(ReadFloat("Defaults", "UnusualRatio", 0.06f), 0.0f, 1.0f);
    g_defaultBreastUnusualRatio = std::clamp(ReadFloat("Defaults", "BreastUnusualRatio", 0.06f), 0.0f, 1.0f);
    g_defaultAthleticRatio      = std::clamp(ReadFloat("Defaults", "AthleticRatio", 0.15f), 0.0f, 1.0f);
    g_defaultRaceCoherence      = std::clamp(ReadFloat("Defaults", "RaceCoherence", 1.0f), 0.0f, 1.0f);
    g_defaultNaturalRatio       = std::clamp(ReadFloat("Defaults", "NaturalRatio", 0.20f), 0.0f, 1.0f);
    g_defaultCurvyRatio         = std::clamp(ReadFloat("Defaults", "CurvyRatio", 0.0f), 0.0f, 1.0f);
    g_defaultBaseBody           = std::clamp(static_cast<int>(ReadFloat("Defaults", "BaseBody", 0.0f)), 0, 2);
    g_defaultClothedRefit       = std::clamp(ReadFloat("Defaults", "ClothedRefit", 0.10f), 0.0f, 0.5f);
    g_defaultNeuralBody         = ReadFloat("Defaults", "NeuralBody", 0.0f) >= 0.5f;
    g_defaultReRollKey          = static_cast<int>(ReadFloat("Defaults", "ReRollKey", 26.0f));
    g_excludeKey                = static_cast<int>(ReadFloat("Defaults", "ExcludeKey", 0.0f));
    g_exportKey                 = static_cast<int>(ReadFloat("Defaults", "ExportKey", 0.0f));
    g_defaultFemaleBodies       = GetPrivateProfileIntA("Defaults", "FemaleBodies", 1, kIniPath) != 0;
    g_defaultMaleBodies         = GetPrivateProfileIntA("Defaults", "MaleBodies", 1, kIniPath) != 0;
    g_defaultMaleBuild          = std::clamp(ReadFloat("Defaults", "MaleBuild", 1.0f), 0.0f, 2.0f);
    g_defaultDebugLog           = GetPrivateProfileIntA("Defaults", "DebugLog", 0, kIniPath) != 0;
    g_defaultNeckColorFix       = std::clamp(ReadFloat("Defaults", "NeckColorFix", 0.5f), 0.0f, 1.0f);
    SKSE::log::info("Config: MorphScale={:.2f}, Fantasy={:.2f}, Unusual={:.2f}, BreastUnusual={:.2f}, Athletic={:.2f}, RaceCoherence={:.2f}, FemaleBodies={}, MaleBodies={}, MaleBuild={:.2f}",
                    g_defaultMorphScale, g_defaultFantasyRatio, g_defaultUnusualRatio,
                    g_defaultBreastUnusualRatio, g_defaultAthleticRatio, g_defaultRaceCoherence, g_defaultFemaleBodies, g_defaultMaleBodies, g_defaultMaleBuild);
}

namespace {

constexpr const char* kExclusionDir  = R"(.\Data\SKSE\Plugins)";
constexpr const char* kMcmExclFile   = R"(.\Data\SKSE\Plugins\OBodyNGWeight_Exclusions_MCM.txt)";

std::string LowerStr(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// "Plugin.esp|0xLocalFormID" exclusion key, normalized to "pluginlower|0x<hex>". We compare origin plugin +
// LOCAL FormID (never the runtime FormID), so a hand-edited line and a live actor match regardless of load order.
std::string FormKey(std::string_view a_plugin, RE::FormID a_local) {
    char buf[12];
    auto res = std::to_chars(buf, buf + sizeof(buf), static_cast<std::uint32_t>(a_local), 16);
    return LowerStr(std::string(a_plugin)) + "|0x" + std::string(buf, res.ptr);
}

// Parse a hex local FormID (optional "0x"/surrounding spaces). 0 on failure.
RE::FormID ParseHexId(std::string_view s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    s = s.substr(b, e - b);
    if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s = s.substr(2);
    std::uint32_t id = 0;
    auto res = std::from_chars(s.data(), s.data() + s.size(), id, 16);
    return res.ec == std::errc{} ? static_cast<RE::FormID>(id) : 0;
}

// Each line is either a whole-plugin name, or "Plugin.esp|0xLocalFormID" to exclude a single NPC.
void ReadExclusionFile(const std::filesystem::path& a_path, std::unordered_set<std::string>& a_names,
                       std::unordered_set<std::string>& a_forms) {
    std::ifstream f(a_path);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        size_t b = 0, e = line.size();
        while (b < e && std::isspace(static_cast<unsigned char>(line[b]))) ++b;
        while (e > b && std::isspace(static_cast<unsigned char>(line[e - 1]))) --e;
        if (e <= b || line[b] == ';' || line[b] == '#') continue;
        const std::string s = line.substr(b, e - b);
        const size_t bar = s.find('|');
        if (bar != std::string::npos) {                          // "Plugin.esp|0xLocalFormID" -> one NPC
            std::string plugin = s.substr(0, bar);
            while (!plugin.empty() && std::isspace(static_cast<unsigned char>(plugin.back()))) plugin.pop_back();
            const RE::FormID id = ParseHexId(s.substr(bar + 1));
            if (!plugin.empty() && id != 0) {
                a_forms.insert(FormKey(plugin, id));
                // Users paste the FULL load-order FormID xEdit shows ("05123456" / "FE123ABC"), not the local
                // one this key uses — accept those too. A LOCAL id never has its top byte set (regular plugins
                // use 24-bit locals, ESLs 12-bit), so a nonzero top byte reliably means "full id": also insert
                // the masked local (ESL full ids start 0xFE -> low 12 bits; anything else -> low 24 bits).
                const std::uint32_t top = static_cast<std::uint32_t>(id) >> 24;
                if (top == 0xFEu)   a_forms.insert(FormKey(plugin, id & 0xFFFu));
                else if (top != 0u) a_forms.insert(FormKey(plugin, id & 0xFFFFFFu));
            }
        } else {
            a_names.insert(LowerStr(s));                          // whole plugin
        }
    }
}

void WriteMcmExclusionFile() {
    std::ofstream f(kMcmExclFile, std::ios::trunc);
    if (!f) return;
    f << "; OBodyNG Weight - MCM/runtime-managed exclusions (auto-generated).\n";
    f << "; A plain plugin name excludes the whole .esp/.esl/.esm; 'Plugin.esp|0xLocalFormID' excludes one NPC\n";
    f << "; (the full load-order FormID xEdit shows, e.g. 05123456 or FE123ABC, is accepted too).\n";
    for (const auto& n : g_mcmExcluded)      f << n << "\n";
    for (const auto& k : g_mcmExcludedForms) f << k << "\n";   // already normalized "pluginlower|0xid"
}

}  // namespace

void SetExcludeKey(int a_key) {
    g_excludeKey = a_key;
    WritePrivateProfileStringA("Defaults", "ExcludeKey", std::to_string(a_key).c_str(), kIniPath);
}

void SetExportKey(int a_key) {
    g_exportKey = a_key;
    WritePrivateProfileStringA("Defaults", "ExportKey", std::to_string(a_key).c_str(), kIniPath);
}

void LoadExclusions() {
    g_mcmExcluded.clear();
    g_fileExcluded.clear();
    g_mcmExcludedForms.clear();
    g_fileExcludedForms.clear();
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::directory_iterator it(fs::path(kExclusionDir), ec), end;
    for (; !ec && it != end; it.increment(ec)) {
        std::error_code ec2;
        if (!it->is_regular_file(ec2)) continue;
        // Match OBodyNGWeight_Exclusions*.txt via the WIDE filename (no codepage conversion, so a
        // non-ASCII file elsewhere in the folder can't throw).
        std::wstring fn = it->path().filename().native();
        for (auto& c : fn) c = static_cast<wchar_t>(std::towlower(c));
        if (fn.rfind(L"obodyngweight_exclusions", 0) != 0) continue;
        if (fn.size() < 4 || fn.compare(fn.size() - 4, 4, L".txt") != 0) continue;
        if (fn == L"obodyngweight_exclusions_mcm.txt") ReadExclusionFile(it->path(), g_mcmExcluded, g_mcmExcludedForms);
        else                                           ReadExclusionFile(it->path(), g_fileExcluded, g_fileExcludedForms);
    }
    SKSE::log::info("Config: exclusions - plugins {}+{} (MCM/files), NPCs {}+{} (MCM/files)",
                    g_mcmExcluded.size(), g_fileExcluded.size(), g_mcmExcludedForms.size(), g_fileExcludedForms.size());
}

bool IsActorExcluded(RE::Actor* a_actor) {
    if (!a_actor) return false;
    if (g_mcmExcluded.empty() && g_fileExcluded.empty() &&
        g_mcmExcludedForms.empty() && g_fileExcludedForms.empty()) return false;
    const auto nameExcluded = [](RE::TESForm* a_form) -> bool {     // whole-plugin exclusion
        if (!a_form) return false;
        for (std::int32_t idx : { 0, -1 }) {  // 0 = origin (added by), -1 = winning override
            if (auto* file = a_form->GetFile(idx)) {
                const std::string n = LowerStr(std::string(file->GetFilename()));
                if (g_mcmExcluded.count(n) || g_fileExcluded.count(n)) return true;
            }
        }
        return false;
    };
    const auto formExcluded = [](RE::TESForm* a_form) -> bool {     // single-NPC (FormID) exclusion
        if (!a_form) return false;
        for (std::int32_t idx : { 0, -1 }) {
            if (auto* file = a_form->GetFile(idx)) {
                const std::string k = FormKey(file->GetFilename(), a_form->GetLocalFormID());
                if (g_mcmExcludedForms.count(k) || g_fileExcludedForms.count(k)) return true;
            }
        }
        return false;
    };
    RE::TESForm* base = a_actor->GetActorBase();
    return nameExcluded(base) || nameExcluded(a_actor) || formExcluded(base) || formExcluded(a_actor);
}

bool IsPluginExcluded(const char* a_plugin) {
    return a_plugin && g_mcmExcluded.count(LowerStr(a_plugin)) > 0;
}

void SetPluginExcluded(const char* a_plugin, bool a_on) {
    if (!a_plugin) return;
    const std::string n = LowerStr(a_plugin);
    if (n.empty()) return;
    if (a_on) g_mcmExcluded.insert(n);
    else      g_mcmExcluded.erase(n);
    WriteMcmExclusionFile();
}

bool SetActorExcluded(RE::Actor* a_actor, bool a_on) {
    if (!a_actor) return false;
    RE::TESForm* f = a_actor->GetActorBase();   // identify the NPC by its base FormID (per-NPC, not per-placement)
    if (!f) f = a_actor;
    auto* file = f->GetFile(0);                 // origin plugin (the one xEdit shows the FormID under)
    if (!file) {
        // Dynamic base (a leveled-list spawn instances a runtime FF TESNPC with no source file). Fall back to
        // keying by the placed REFERENCE: refs placed by a plugin have an origin file, and IsActorExcluded
        // checks the reference's FormKey too, so the exclusion still matches this NPC on every load.
        f = a_actor;
        file = f->GetFile(0);
    }
    if (!file) return false;                    // fully runtime (spawned ref of a dynamic base) — no durable ID to key
    const std::string key = FormKey(file->GetFilename(), f->GetLocalFormID());
    if (a_on) g_mcmExcludedForms.insert(key);
    else      g_mcmExcludedForms.erase(key);
    WriteMcmExclusionFile();
    return true;
}

std::vector<std::string> GetNpcPlugins() {
    std::set<std::string> names;  // sorted, unique, display case
    if (auto* dh = RE::TESDataHandler::GetSingleton()) {
        for (auto* npc : dh->GetFormArray<RE::TESNPC>()) {
            if (!npc) continue;
            if (auto* file = npc->GetFile(0))  // origin plugin (the one that added the NPC)
                names.emplace(file->GetFilename());
        }
    }
    // Full list (the MCM Exclusions page paginates it ~120/page; GetNpcPluginsPage chunks this).
    return std::vector<std::string>(names.begin(), names.end());
}

}  // namespace OBW::Config
