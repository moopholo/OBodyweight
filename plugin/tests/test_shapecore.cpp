#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ShapeCore.hpp"

using OBW::ClassifyRaceStr;
using OBW::RaceClass;

TEST_CASE("ClassifyRaceStr: vanilla editorIDs") {
    CHECK(ClassifyRaceStr("nordrace") == RaceClass::kNord);
    CHECK(ClassifyRaceStr("imperialrace") == RaceClass::kImperial);
    CHECK(ClassifyRaceStr("bretonrace") == RaceClass::kBreton);
    CHECK(ClassifyRaceStr("redguardrace") == RaceClass::kRedguard);
    CHECK(ClassifyRaceStr("orcrace") == RaceClass::kOrc);
    CHECK(ClassifyRaceStr("highelfrace") == RaceClass::kAltmer);
    CHECK(ClassifyRaceStr("woodelfrace") == RaceClass::kBosmer);
    CHECK(ClassifyRaceStr("darkelfrace") == RaceClass::kDunmer);
    CHECK(ClassifyRaceStr("khajiitrace") == RaceClass::kKhajiit);
    CHECK(ClassifyRaceStr("argonianrace") == RaceClass::kArgonian);
}

TEST_CASE("ClassifyRaceStr: FULL-name spellings + mer synonyms") {
    CHECK(ClassifyRaceStr("high elf") == RaceClass::kAltmer);
    CHECK(ClassifyRaceStr("wood elf") == RaceClass::kBosmer);
    CHECK(ClassifyRaceStr("dark elf") == RaceClass::kDunmer);
    CHECK(ClassifyRaceStr("altmer") == RaceClass::kAltmer);
    CHECK(ClassifyRaceStr("bosmer") == RaceClass::kBosmer);
    CHECK(ClassifyRaceStr("dunmer") == RaceClass::kDunmer);
    CHECK(ClassifyRaceStr("orsimer") == RaceClass::kOrc);
    CHECK(ClassifyRaceStr("saxhleel") == RaceClass::kArgonian);
}

TEST_CASE("ClassifyRaceStr: Elder wins over the underlying race (tested first)") {
    CHECK(ClassifyRaceStr("elderrace") == RaceClass::kElder);
    CHECK(ClassifyRaceStr("nordraceelder") == RaceClass::kElder);       // nord + elder → Elder
    CHECK(ClassifyRaceStr("elderracevampire") == RaceClass::kElder);
    CHECK(ClassifyRaceStr("imperialraceelderchild") == RaceClass::kElder);
}

TEST_CASE("ClassifyRaceStr: specific beats generic (highelf is not generic 'elf')") {
    CHECK(ClassifyRaceStr("highelf") == RaceClass::kAltmer);            // not kDunmer via 'elf'
    CHECK(ClassifyRaceStr("woodelf") == RaceClass::kBosmer);
    CHECK(ClassifyRaceStr("somemoddedmer") == RaceClass::kDunmer);      // generic mer → lean elf
    CHECK(ClassifyRaceStr("customelfrace") == RaceClass::kDunmer);
}

TEST_CASE("ClassifyRaceStr: vampire + modded variants match by substring") {
    CHECK(ClassifyRaceStr("nordracevampire") == RaceClass::kNord);
    CHECK(ClassifyRaceStr("0_nordrace_wor") == RaceClass::kNord);       // CotR/HG converted race
    CHECK(ClassifyRaceStr("cotr_highelfracevampire") == RaceClass::kAltmer);
    CHECK(ClassifyRaceStr("dlc1nordracevampire") == RaceClass::kNord);
}

TEST_CASE("ClassifyRaceStr: unknown / empty → Neutral (legacy uniform, no regression)") {
    CHECK(ClassifyRaceStr("") == RaceClass::kNeutral);
    CHECK(ClassifyRaceStr("draugr") == RaceClass::kNeutral);
    CHECK(ClassifyRaceStr("customhumanrace") == RaceClass::kNeutral);
    CHECK(ClassifyRaceStr("spiderrace") == RaceClass::kNeutral);
}

// ── Race / group body rules ──────────────────────────────────────────────────
using OBW::RaceRules;
using OBW::DefaultRaceRules;
using OBW::ResolveGroup;
using OBW::GroupArchMult;
using OBW::GroupFrameBias;
using OBW::ApplyRulesSection;
using OBW::RaceClassName;

TEST_CASE("RaceClassName: canonical lowercase names match INI sections") {
    CHECK(RaceClassName(RaceClass::kElder) == "elder");
    CHECK(RaceClassName(RaceClass::kOrc) == "orc");
    CHECK(RaceClassName(RaceClass::kNeutral) == "neutral");
    CHECK(RaceClassName(RaceClass::kAltmer) == "altmer");
}

TEST_CASE("DefaultRaceRules: Elder ships hard-excludes for the sexy archetypes (both sexes)") {
    RaceRules f = DefaultRaceRules(false);
    CHECK(f.groups.at("elder").exclude.count("Amazon") == 1);
    CHECK(f.groups.at("elder").exclude.count("Slim Thick") == 1);
    CHECK(f.groups.at("elder").exclude.count("Athletic") == 1);
    CHECK(f.groups.at("elder").exclude.count("BBW") == 0);   // soft/mature shapes still allowed
    CHECK(f.groups.at("orc").mult.at("Amazon") == doctest::Approx(3.0f));

    RaceRules m = DefaultRaceRules(true);
    CHECK(m.groups.at("elder").exclude.count("Bodybuilder") == 1);
    CHECK(m.groups.at("elder").exclude.count("Powerlifter") == 1);
    CHECK(m.groups.at("orc").mult.at("Powerlifter") == doctest::Approx(3.0f));
}

TEST_CASE("ResolveGroup: vanilla races map to their RaceClass name") {
    RaceRules f = DefaultRaceRules(false);
    CHECK(ResolveGroup("nordrace", f) == "nord");
    CHECK(ResolveGroup("elderrace", f) == "elder");
    CHECK(ResolveGroup("nordraceelder", f) == "elder");   // elder wins
    CHECK(ResolveGroup("draugr", f) == "neutral");
}

TEST_CASE("GroupArchMult: hard exclude is ABSOLUTE — 0 at every coherence strength") {
    RaceRules f = DefaultRaceRules(false);
    CHECK(GroupArchMult("elder", "Amazon", f, 0.0f) == doctest::Approx(0.0f));  // even coherence OFF
    CHECK(GroupArchMult("elder", "Amazon", f, 1.0f) == doctest::Approx(0.0f));
    CHECK(GroupArchMult("elder", "Slim Thick", f, 0.5f) == doctest::Approx(0.0f));
    // a non-excluded elder archetype is unaffected at strength 0
    CHECK(GroupArchMult("elder", "MILF", f, 0.0f) == doctest::Approx(1.0f));
}

TEST_CASE("GroupArchMult: multiplier lerps by coherence strength; unknown → 1") {
    RaceRules f = DefaultRaceRules(false);
    CHECK(GroupArchMult("orc", "Amazon", f, 1.0f) == doctest::Approx(3.0f));    // full
    CHECK(GroupArchMult("orc", "Amazon", f, 0.5f) == doctest::Approx(2.0f));    // 1 + (3-1)*0.5
    CHECK(GroupArchMult("orc", "Amazon", f, 0.0f) == doctest::Approx(1.0f));    // off → neutral
    CHECK(GroupArchMult("orc", "Balanced", f, 1.0f) == doctest::Approx(1.0f));  // not listed → 1
    CHECK(GroupArchMult("neutral", "Amazon", f, 1.0f) == doctest::Approx(1.0f));// unknown group
}

TEST_CASE("GroupFrameBias: lerps by strength, 0 when off") {
    RaceRules f = DefaultRaceRules(false);
    CHECK(GroupFrameBias("orc", f, 1.0f) == doctest::Approx(8.0f));
    CHECK(GroupFrameBias("orc", f, 0.5f) == doctest::Approx(4.0f));
    CHECK(GroupFrameBias("orc", f, 0.0f) == doctest::Approx(0.0f));
    CHECK(GroupFrameBias("nosuch", f, 1.0f) == doctest::Approx(0.0f));
}

TEST_CASE("ApplyRulesSection: Mult / Exclude / un-exclude / FrameBias overlay defaults") {
    RaceRules f = DefaultRaceRules(false), m = DefaultRaceRules(true);
    // override a multiplier
    ApplyRulesSection(f, m, "nord", {{"Mult.Balanced", "3.0"}});
    CHECK(GroupArchMult("nord", "Balanced", f, 1.0f) == doctest::Approx(3.0f));
    // add a hard exclude to a race that had none
    ApplyRulesSection(f, m, "breton", {{"Exclude", "BBW, Obese"}});
    CHECK(GroupArchMult("breton", "BBW", f, 0.0f) == doctest::Approx(0.0f));
    // REMOVE a default elder exclude with a leading '-'
    CHECK(GroupArchMult("elder", "Amazon", f, 1.0f) == doctest::Approx(0.0f));
    ApplyRulesSection(f, m, "elder", {{"Exclude", "-Amazon"}});
    CHECK(GroupArchMult("elder", "Amazon", f, 1.0f) != doctest::Approx(0.0f));
    // frame bias
    ApplyRulesSection(f, m, "khajiit", {{"FrameBias", "10"}});
    CHECK(GroupFrameBias("khajiit", f, 1.0f) == doctest::Approx(10.0f));
}

TEST_CASE("ApplyRulesSection: .male suffix targets the male rule set only") {
    RaceRules f = DefaultRaceRules(false), m = DefaultRaceRules(true);
    ApplyRulesSection(f, m, "nord.male", {{"Mult.Average", "5.0"}});
    CHECK(GroupArchMult("nord", "Average", m, 1.0f) == doctest::Approx(5.0f));
    CHECK(GroupArchMult("nord", "Average", f, 1.0f) == doctest::Approx(1.0f));  // female untouched
}

TEST_CASE("ApplyRulesSection: custom Group with Match classifies modded races (both sexes)") {
    RaceRules f = DefaultRaceRules(false), m = DefaultRaceRules(true);
    ApplyRulesSection(f, m, "Group:SnowElf",
                      {{"Match", "snowelf, customsnow"}, {"Mult.Slim", "4.0"}, {"Exclude", "BBW"}});
    // a modded race string containing the substring resolves to the custom group, for both sexes
    CHECK(ResolveGroup("dlc1_customsnowelfrace", f) == "snowelf");
    CHECK(ResolveGroup("dlc1_customsnowelfrace", m) == "snowelf");
    CHECK(GroupArchMult("snowelf", "Slim", f, 1.0f) == doctest::Approx(4.0f));
    CHECK(GroupArchMult("snowelf", "BBW", f, 0.0f) == doctest::Approx(0.0f));   // hard exclude
    // male Mult.Slim was NOT set (Mult is sex-scoped to the female section) → neutral
    CHECK(GroupArchMult("snowelf", "Slim", m, 1.0f) == doctest::Approx(1.0f));
}
