#pragma once
#include "SKEE.h"
#include <atomic>
#include <SKSE/SKSE.h>

namespace OBW {

// SKEE (RaceMenu/NiOverride) BodyMorph interface, acquired from the "skee" plugin at kPostPostLoad.
// Lets the C++ side apply body morphs directly (no Papyrus per-slider round-trips, no 128-element
// array cap). Null disables morph application; never fall back to OBody's unchecked reset.
inline SKEE::IBodyMorphInterface* g_morph = nullptr;

inline bool CheckMorphInterface(const char* operation) {
    if (g_morph) return true;
    static std::atomic_bool reported{false};
    if (!reported.exchange(true)) {
        SKSE::log::error("{}: SKEE BodyMorph interface unavailable; skipping morph application. "
                        "OBW remains active. See interface initialization diagnostics above.", operation);
    }
    return false;
}

}  // namespace OBW
