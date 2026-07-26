#include "ShapeCore.hpp"

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

}  // namespace OBW
