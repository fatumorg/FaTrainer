#pragma once
#include <cmath>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "stock.h"

namespace detect {

// "Probably cheating": a value another player reached that the stock game cannot, with room for lag.
enum Kind { K_DROPKICK, K_DFA, K_POUNCE, K_TACKLE, K_POUND, K_CLAW, K_KICK, K_MELEE, K_SPIT, K_SPEED, K_JUMP, K_WALLRUN, K_TELEPORT, K_NO_DAMAGE,
            K_MAX_HEALTH, K_HEAL, K_COOLDOWN, K_SPITS, K_CAMO, K_THROWABLE, K_IMPALE, K_DATA, K_COUNT };

struct Finding { Kind kind; float measured, normal; int count; };

const float ALLOW_FRACTION = 0.10f;
const float ALLOW_ABSOLUTE[K_COUNT] = {1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 0.5f, 1.5f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const float TELEPORT_JUMP = 30, NO_DAMAGE_HITS = 5;

// The stock value each range, speed and limit is measured against. Kinds without one count events only.
const char* const NORMAL_FROM[K_COUNT] = {"f_btz_wrestling_kick_range", "JumpAttackRange", "f_btz_zombie_grab_range", "ZombieChargeAttackRange",
                                          "ZombieGroundPoundRange", nullptr, nullptr, nullptr, nullptr, "MoveSprintSpeed", "JumpMaxHeight", nullptr, nullptr,
                                          nullptr, "MaxHealth", nullptr, "ZombieGroundPoundCooldown", nullptr, "CamouflageDurationEnhanced", nullptr, nullptr,
                                          nullptr};
inline float test_normals[K_COUNT] = {};

inline void test_normal(Kind k, float v) { test_normals[k] = v; }

inline float normal(Kind k) {
    if (test_normals[k] > 0) return test_normals[k];
    return NORMAL_FROM[k] ? stock::value(NORMAL_FROM[k]) : NAN;
}

// Cooldowns count in the player's favor when they are shorter; everything else when it is larger.
inline bool over(Kind k, float measured) {
    float n = normal(k);
    if (!std::isfinite(n) || !std::isfinite(measured)) return false;
    if (k == K_COOLDOWN) return measured < n * (1 - ALLOW_FRACTION) - ALLOW_ABSOLUTE[k];
    return measured > n * (1 + ALLOW_FRACTION) + ALLOW_ABSOLUTE[k];
}

class Tracker {
public:
    void sample(Kind k, float measured) {
        if (!over(k, measured)) return;
        Finding& f = found[k];
        bool better = k == K_COOLDOWN ? measured < f.measured : measured > f.measured;
        if (!f.count || better) f.measured = measured;
        f.kind = k, f.normal = normal(k), f.count++;
    }
    void event(Kind k, float measured = 0) {
        Finding& f = found[k];
        f.kind = k, f.measured = std::max(f.measured, measured), f.count++;
    }
    std::vector<Finding> findings() const {
        std::vector<Finding> out;
        for (auto& [k, f] : found)
            if (f.count) out.push_back(f);
        return out;
    }
private:
    std::map<int, Finding> found;
};

inline std::string number(float v) {
    char b[32];
    snprintf(b, sizeof b, "%.1f", v);
    std::string s = b;
    if (s.size() > 2 && s.compare(s.size() - 2, 2, ".0") == 0) s.resize(s.size() - 2);
    return s;
}

inline std::string describe(const Finding& f) {
    std::string v = number(f.measured), n = number(f.normal);
    std::string times = f.count == 1 ? ", once" : ", " + std::to_string(f.count) + " times";
    const char* attack = nullptr;
    switch (f.kind) {
    case K_DROPKICK: attack = "Dropkick"; break;
    case K_DFA: attack = "Death from above"; break;
    case K_POUNCE: attack = "Pounce"; break;
    case K_TACKLE: attack = "Tackle"; break;
    case K_POUND: attack = "Ground pound"; break;
    case K_CLAW: attack = "Claws"; break;
    case K_KICK: attack = "Kick"; break;
    case K_MELEE: attack = "Melee hit"; break;
    case K_SPIT: attack = "Spit hit"; break;
    default: break;
    }
    if (attack) return std::string(attack) + " from " + v + " m (normal up to " + n + " m)" + times;
    switch (f.kind) {
    case K_SPEED: return "Ran " + v + " m/s (normal up to " + n + " m/s)" + times;
    case K_JUMP: return "Jumped " + v + " m (normal up to " + n + " m)" + times;
    case K_WALLRUN: return "Wall-ran " + v + " m/s (normal up to " + n + " m/s)" + times;
    case K_TELEPORT: return "Moved " + v + " m at once" + times;
    case K_NO_DAMAGE: return "Took " + std::to_string(f.count) + " hits, health never dropped";
    case K_MAX_HEALTH: return "Max health " + v + " (normal " + n + ")" + times;
    case K_HEAL: return "Healed " + v + " without a medkit" + times;
    case K_COOLDOWN: return "Ability again after " + v + " s (normal " + n + " s)" + times;
    case K_SPITS: return v + " spits faster than their recharge" + times;
    case K_CAMO: return "Camouflage for " + v + " s (normal up to " + n + " s)" + times;
    case K_THROWABLE: return "Throwable back after " + v + " s (normal " + n + " s)" + times;
    case K_IMPALE: return "Spike death with no kick or spikes nearby" + times;
    default: return "Game data differs" + times;
    }
}

#if !FATRAINER_VISUALS
inline std::mutex players_mx;
inline std::map<uintptr_t, Tracker> players;
#endif

}
