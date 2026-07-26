#include "ShapeCore.hpp"

#include <cctype>

namespace OBW {

RaceClass ClassifyRaceStr(const std::string& s) {
    if (s.empty()) return RaceClass::kNeutral;
    auto has = [&](const char* k) { return s.find(k) != std::string::npos; };
    if (has("elder"))                                       return RaceClass::kElder;     // aged (before race/elf)
    if (has("khajiit") || has("rhajiit"))                   return RaceClass::kKhajiit;
    if (has("argonian") || has("saxhleel"))                 return RaceClass::kArgonian;
    if (has("highelf") || has("high elf") || has("altmer")) return RaceClass::kAltmer;
    if (has("woodelf") || has("wood elf") || has("bosmer")) return RaceClass::kBosmer;
    if (has("darkelf") || has("dark elf") || has("dunmer")) return RaceClass::kDunmer;
    if (has("orsimer") || has("orc"))                       return RaceClass::kOrc;
    if (has("redguard"))                                    return RaceClass::kRedguard;
    if (has("breton"))                                      return RaceClass::kBreton;
    if (has("imperial"))                                    return RaceClass::kImperial;
    if (has("nord"))                                        return RaceClass::kNord;
    if (has("elf") || has("mer"))                           return RaceClass::kDunmer;    // generic modded mer → lean elf
    return RaceClass::kNeutral;
}

std::string_view RaceClassName(RaceClass rc) {
    switch (rc) {
        case RaceClass::kNord:     return "nord";
        case RaceClass::kImperial: return "imperial";
        case RaceClass::kBreton:   return "breton";
        case RaceClass::kRedguard: return "redguard";
        case RaceClass::kOrc:      return "orc";
        case RaceClass::kAltmer:   return "altmer";
        case RaceClass::kBosmer:   return "bosmer";
        case RaceClass::kDunmer:   return "dunmer";
        case RaceClass::kKhajiit:  return "khajiit";
        case RaceClass::kArgonian: return "argonian";
        case RaceClass::kElder:    return "elder";
        case RaceClass::kNeutral:
        default:                   return "neutral";
    }
}

// ── string helpers (pure, ASCII) ─────────────────────────────────────────────────────────────────
namespace {

std::string ToLowerCopy(std::string_view s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string Trim(std::string_view s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return std::string(s.substr(b, e - b));
}

std::vector<std::string> SplitCommas(std::string_view s) {
    std::vector<std::string> out;
    std::size_t start = 0;
    for (std::size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == ',') {
            std::string tok = Trim(s.substr(start, i - start));
            if (!tok.empty()) out.push_back(std::move(tok));
            start = i + 1;
        }
    }
    return out;
}

bool IEquals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    return true;
}

// Case-insensitive prefix test; returns the remainder (original case) after the prefix, or empty view.
bool IStartsWith(std::string_view s, std::string_view prefix, std::string_view& rest) {
    if (s.size() < prefix.size()) return false;
    for (std::size_t i = 0; i < prefix.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(s[i])) != std::tolower(static_cast<unsigned char>(prefix[i])))
            return false;
    rest = s.substr(prefix.size());
    return true;
}

float ParseFloat(std::string_view s, float fallback) {
    try { return std::stof(std::string(s)); }
    catch (...) { return fallback; }
}

}  // namespace

// ── default rule tables ──────────────────────────────────────────────────────────────────────────
// The per-race archetype multipliers OBW has always shipped (calibrated to keep the full range
// possible — no zeroes — so an Orc CAN still be petite, just rarely). Moved here from WeightManager so
// they are testable and INI-overridable. NEW: default hard-exclude sets so aged NPCs never roll the
// overtly "sexy"/peak-athletic archetypes regardless of the coherence slider.

RaceRules DefaultRaceRules(bool male) {
    RaceRules r;
    if (!male) {
        r.groups["nord"].mult     = { {"Stocky",2.0f},{"Athletic",1.5f},{"Rectangle",1.3f},{"Amazon",1.3f},{"Strongwoman",1.2f},{"Balanced",1.1f},{"Petite",0.5f},{"Lollipop",0.7f},{"Diamond",0.7f} };
        r.groups["imperial"].mult = { {"Balanced",1.2f},{"Hourglass",1.1f},{"MILF",1.1f},{"Amazon",0.7f},{"Strongwoman",0.6f} };
        r.groups["breton"].mult   = { {"Petite",1.7f},{"Slim",1.4f},{"Balanced",1.2f},{"Lollipop",1.2f},{"Amazon",0.4f},{"Strongwoman",0.4f},{"Stocky",0.6f},{"BBW",0.7f} };
        r.groups["redguard"].mult = { {"Athletic",2.2f},{"AthleticCurvy",2.0f},{"Slim Thick",1.6f},{"Amazon",1.3f},{"Strongwoman",1.2f},{"Obese",0.4f},{"BBW",0.6f},{"Petite",0.6f} };
        r.groups["orc"].mult      = { {"Amazon",3.0f},{"Strongwoman",3.0f},{"Stocky",2.2f},{"BBW",1.4f},{"Athletic",1.3f},{"Rectangle",1.2f},{"Petite",0.15f},{"Slim",0.4f},{"Lollipop",0.3f},{"Top Hourglass",0.5f},{"Diamond",0.6f} };
        r.groups["altmer"].mult   = { {"Slim",2.2f},{"Top Hourglass",1.3f},{"Lollipop",1.3f},{"Rectangle",1.2f},{"Inverted Triangle",1.2f},{"Petite",0.5f},{"Stocky",0.2f},{"Obese",0.25f},{"BBW",0.4f},{"Strongwoman",0.3f},{"Amazon",0.4f} };
        r.groups["bosmer"].mult   = { {"Petite",3.0f},{"Slim",2.2f},{"Rectangle",1.3f},{"Athletic",1.2f},{"BBW",0.25f},{"Obese",0.2f},{"Amazon",0.2f},{"Strongwoman",0.3f},{"Voluptuous",0.5f},{"MILF",0.6f} };
        r.groups["dunmer"].mult   = { {"Slim",1.8f},{"Rectangle",1.4f},{"Athletic",1.3f},{"Slim Thick",1.2f},{"Inverted Triangle",1.1f},{"BBW",0.4f},{"Obese",0.4f},{"Amazon",0.6f} };
        r.groups["khajiit"].mult  = { {"Slim",1.6f},{"Athletic",1.5f},{"Petite",1.3f},{"Slim Thick",1.3f},{"AthleticCurvy",1.2f},{"Rectangle",1.1f},{"Obese",0.3f},{"BBW",0.4f} };
        r.groups["argonian"].mult = { {"Rectangle",1.6f},{"Slim",1.4f},{"Athletic",1.3f},{"Inverted Triangle",1.2f},{"TopHeavy",0.4f},{"Lollipop",0.2f},{"Voluptuous",0.5f},{"BBW",0.5f},{"Obese",0.5f} };
        r.groups["elder"].mult    = { {"AppleSoft",2.2f},{"Diamond",1.4f},{"MILF",1.3f},{"Voluptuous",1.3f},{"Rectangle",1.2f},{"BBW",1.1f},{"Hourglass",0.6f},{"Petite",0.7f} };
        // Aged NPCs never roll the overtly "sexy" / peak-athletic archetypes — an absolute rail (holds
        // even with coherence off), not just the old 0.1 multipliers.
        r.groups["elder"].exclude = { "Athletic","AthleticCurvy","Slim Thick","Amazon","Strongwoman",
                                      "Lollipop","Top Hourglass","Bottom Hourglass","Spoon" };
        // Frame (body-VOLUME) bias: Orcs read bigger, Bosmer tiny, Altmer slender-despite-tall.
        r.groups["nord"].frameBias     =  4.0f;
        r.groups["breton"].frameBias   = -5.0f;
        r.groups["redguard"].frameBias =  2.0f;
        r.groups["orc"].frameBias      =  8.0f;
        r.groups["altmer"].frameBias   = -3.0f;
        r.groups["bosmer"].frameBias   = -8.0f;
        r.groups["dunmer"].frameBias   = -4.0f;
        r.groups["khajiit"].frameBias  = -4.0f;
        r.groups["argonian"].frameBias = -3.0f;
        r.groups["elder"].frameBias    =  3.0f;
    } else {
        r.groups["nord"].mult     = { {"Soldier",2.0f},{"Stocky",1.8f},{"Powerlifter",1.5f},{"Bodybuilder",1.2f},{"Fit",1.2f},{"Twink",0.4f},{"Lanky",0.7f} };
        r.groups["imperial"].mult = { {"Average",1.3f},{"Soldier",1.2f},{"Fit",1.1f},{"Bodybuilder",0.7f},{"Powerlifter",0.7f} };
        r.groups["breton"].mult   = { {"Lean",1.5f},{"Lanky",1.3f},{"Average",1.2f},{"Twink",1.2f},{"Swimmer",1.1f},{"Powerlifter",0.4f},{"Bodybuilder",0.5f},{"Heavyset",0.7f} };
        r.groups["redguard"].mult = { {"Fit",2.0f},{"Soldier",1.8f},{"Swimmer",1.5f},{"Bodybuilder",1.2f},{"Heavyset",0.5f},{"Dadbod",0.7f},{"Twink",0.5f} };
        r.groups["orc"].mult      = { {"Powerlifter",3.0f},{"Stocky",2.5f},{"Bodybuilder",2.2f},{"Soldier",1.6f},{"Twink",0.15f},{"Lanky",0.4f},{"Lean",0.5f} };
        r.groups["altmer"].mult   = { {"Lean",1.8f},{"Lanky",1.6f},{"Swimmer",1.4f},{"Twink",1.2f},{"Fit",1.1f},{"Powerlifter",0.3f},{"Heavyset",0.4f},{"Stocky",0.4f},{"Bodybuilder",0.6f} };
        r.groups["bosmer"].mult   = { {"Lanky",2.2f},{"Twink",2.0f},{"Lean",1.6f},{"Powerlifter",0.2f},{"Bodybuilder",0.3f},{"Heavyset",0.3f},{"Stocky",0.4f} };
        r.groups["dunmer"].mult   = { {"Lean",1.7f},{"Fit",1.4f},{"Swimmer",1.3f},{"Lanky",1.3f},{"Heavyset",0.4f},{"Powerlifter",0.6f} };
        r.groups["khajiit"].mult  = { {"Fit",1.6f},{"Lean",1.5f},{"Swimmer",1.4f},{"Lanky",1.2f},{"Heavyset",0.3f},{"Powerlifter",0.5f},{"Dadbod",0.6f} };
        r.groups["argonian"].mult = { {"Lean",1.5f},{"Fit",1.4f},{"Swimmer",1.3f},{"Lanky",1.3f},{"Heavyset",0.5f},{"Powerlifter",0.6f} };
        r.groups["elder"].mult    = { {"Dadbod",1.8f},{"Heavyset",1.6f},{"Average",1.2f},{"Lanky",1.2f},{"Twink",0.5f} };
        // Aged men never roll the peak-athletic builds.
        r.groups["elder"].exclude = { "Bodybuilder","Powerlifter","Fit","Swimmer" };
        r.groups["nord"].frameBias     =  4.0f;
        r.groups["breton"].frameBias   = -5.0f;
        r.groups["redguard"].frameBias =  2.0f;
        r.groups["orc"].frameBias      =  8.0f;
        r.groups["altmer"].frameBias   = -3.0f;
        r.groups["bosmer"].frameBias   = -8.0f;
        r.groups["dunmer"].frameBias   = -4.0f;
        r.groups["khajiit"].frameBias  = -4.0f;
        r.groups["argonian"].frameBias = -3.0f;
        r.groups["elder"].frameBias    =  3.0f;
    }
    return r;
}

// ── resolution + lookups ─────────────────────────────────────────────────────────────────────────

std::string ResolveGroup(const std::string& raceLower, const RaceRules& rules) {
    for (const auto& [substr, group] : rules.customMatch)
        if (!substr.empty() && raceLower.find(substr) != std::string::npos)
            return group;
    return std::string(RaceClassName(ClassifyRaceStr(raceLower)));
}

float GroupArchMult(const std::string& group, const std::string& archetype,
                    const RaceRules& rules, float strength) {
    auto git = rules.groups.find(group);
    if (git == rules.groups.end()) return 1.0f;              // unknown group → uniform
    if (git->second.exclude.count(archetype)) return 0.0f;   // hard exclude: absolute, ignores strength
    if (strength <= 0.0f) return 1.0f;                       // coherence off → no biasing (legacy)
    auto mit = git->second.mult.find(archetype);
    const float raw = (mit != git->second.mult.end()) ? mit->second : 1.0f;
    return 1.0f + (raw - 1.0f) * strength;
}

float GroupFrameBias(const std::string& group, const RaceRules& rules, float strength) {
    if (strength <= 0.0f) return 0.0f;
    auto git = rules.groups.find(group);
    return (git != rules.groups.end()) ? git->second.frameBias * strength : 0.0f;
}

// ── INI section overlay ──────────────────────────────────────────────────────────────────────────

namespace {

// Apply the non-Match keys of a section onto one group's rule.
void ApplyKeysToGroup(GroupRule& g, const std::vector<std::pair<std::string, std::string>>& kv) {
    for (const auto& [rawKey, rawVal] : kv) {
        std::string_view rest;
        if (IStartsWith(rawKey, "mult.", rest)) {
            std::string arch = Trim(rest);
            if (!arch.empty()) g.mult[arch] = ParseFloat(rawVal, g.mult.count(arch) ? g.mult[arch] : 1.0f);
        } else if (IEquals(rawKey, "exclude")) {
            for (std::string tok : SplitCommas(rawVal)) {
                if (tok[0] == '-') { std::string name = Trim(std::string_view(tok).substr(1)); g.exclude.erase(name); }
                else g.exclude.insert(std::move(tok));
            }
        } else if (IEquals(rawKey, "framebias")) {
            g.frameBias = ParseFloat(rawVal, g.frameBias);
        }
        // "match" is handled by the caller (it touches customMatch, not a GroupRule); unknown keys ignored.
    }
}

}  // namespace

bool ApplyRulesSection(RaceRules& female, RaceRules& male,
                       const std::string& section,
                       const std::vector<std::pair<std::string, std::string>>& kv) {
    // Split a trailing ".male" (case-insensitive) → target sex.
    std::string name = section;
    bool isMale = false;
    if (name.size() >= 5 && IEquals(std::string_view(name).substr(name.size() - 5), ".male")) {
        isMale = true;
        name.resize(name.size() - 5);
    }
    RaceRules& target = isMale ? male : female;

    // Custom group?  "Group:<name>"
    std::string_view rest;
    std::string groupName;
    bool custom = false;
    if (IStartsWith(name, "group:", rest)) {
        custom = true;
        groupName = ToLowerCopy(Trim(rest));
    } else {
        groupName = ToLowerCopy(Trim(name));
    }
    if (groupName.empty()) return false;

    // Custom "Match" substrings define which races fall into this group — applied to BOTH sexes so a
    // modded race is classified the same regardless of the NPC's sex.
    if (custom) {
        for (const auto& [rawKey, rawVal] : kv) {
            if (IEquals(rawKey, "match")) {
                for (std::string sub : SplitCommas(rawVal)) {
                    std::string low = ToLowerCopy(sub);
                    female.customMatch.emplace_back(low, groupName);
                    male.customMatch.emplace_back(low, groupName);
                }
            }
        }
    }

    ApplyKeysToGroup(target.groups[groupName], kv);
    return true;
}

}  // namespace OBW
