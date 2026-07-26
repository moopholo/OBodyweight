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
