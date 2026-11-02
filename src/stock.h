#pragma once
#include <cmath>
#include <cstring>
#include <utility>

namespace stock {

inline const char SOURCE[] = "Data0.pak sha256 0d74a71474395f018e36f0e9e605f6f6f5021119e2bcbfee2047ccdba762197c";
inline const std::pair<const char*, float> VALUES[] = {
    {"f_btz_wrestling_kick_range", 3.1f},
    {"JumpAttackRange", 4.0f},
    {"f_btz_zombie_grab_range", 10.0f},
    {"ZombieChargeAttackRange", 5.0f},
    {"ZombieGroundPoundRange", 5.0f},
    {"MoveSprintSpeed", 7.0f},
    {"JumpMaxHeight", 3.4f},
    {"MaxHealth", 292.325f},
    {"ZombieGroundPoundCooldown", 1.0f},
    {"CamouflageDurationEnhanced", 60.0f}
};
inline const char* const NAMES_USED[] = {"f_btz_wrestling_kick_range", "JumpAttackRange", "f_btz_zombie_grab_range", "ZombieChargeAttackRange", "ZombieGroundPoundRange", "MoveSprintSpeed", "JumpMaxHeight", "MaxHealth", "ZombieGroundPoundCooldown", "CamouflageDurationEnhanced"};

inline float value(const char* name) {
    for (auto& [n, v] : VALUES)
        if (name && !strcmp(n, name)) return v;
    return NAN;
}

}
