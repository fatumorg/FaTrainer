#pragma once
#include <windows.h>
#include <cmath>
#include <cstdint>
#include <vector>
#include "cheats.h"
#include "game.h"

namespace pvp {

// Every game starts the spike death of a player through one call in PVPGrabController (rva d68a30 in 1.55):
//   mov r8d, edi / mov rdx, rbx / mov rcx, [player + SpikesHitController] / call SpikesHitController::Start
// The knock-back's spike search runs in the attacker's game; this start runs in every game, the victim's too.
const char* const SPIKE_START_CALL = "FF 50 20 44 8B C7 48 8B D3 48 8B 88 ? ? ? ? E8 ? ? ? ?";
const int SPIKE_CONTROLLER_AT = 12, SPIKE_CALL_AT = 16;
const uint8_t SPIKE_START_PROLOGUE[] = {0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x50};
const DWORD REFUSE_LOG_GAP = 5000, SEARCH_LOG_GAP = 1000;

// The spike search (rva d860c0 in 1.55) runs in the attacker's game for every knock-back, ground pound and thrown
// player: (result, owner, from, velocity, time window, gravity, player), true when it found a spike trap. It looks for
// trap_spikes_a/b models within time window * speed + 2 m, ahead of the throw only, and flies the target onto the
// trap at the throw's own speed.
const char* const SPIKE_SEARCH_CODE = "48 8B C4 48 89 50 10 55 53 56 57 41 55 41 56 41 57 48 8D A8 D8 FE FF FF 48 81 EC F0 01 00 00 33 F6 C7 01 FF FF FF FF";
const uint8_t SPIKE_SEARCH_PROLOGUE[] = {0x48, 0x8B, 0xC4, 0x48, 0x89, 0x50, 0x10, 0x55, 0x53, 0x56, 0x57, 0x41, 0x55, 0x41, 0x56};

// The game's slow motion (ConcentrationModeManager) picks a mode per player through one function (rva bcc8b0 in 1.55).
// Mode 7 is a PvP grab (pounce, tackle, dropkick, death from above) still flying after its planned time: 10% speed
// until it lands. A normal grab lands in time; one from farther than normal overruns by a third of a second or more,
// which at 10% is seconds of slow motion.
const char* const SLOWMO_PICK_CODE = "40 53 48 83 EC 20 48 8B 81 B8 13 00 00 48 8B D9 80 78 5C 00";
const uint8_t SLOWMO_PICK_PROLOGUE[] = {0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0x81, 0xB8, 0x13, 0x00, 0x00, 0x48, 0x8B, 0xD9};
const int GRAB_CONTROLLER = 0xc88, GRAB_TIME = 0x2a0, GRAB_PLANNED = 0x2a4, SLOWMO_GRAB_OVERRUN = 7, SLOWMO_NONE = 0;
const float SLOWMO_OVERRUN_MAX = 0.03f;  // game seconds: about a third of a second at 10% speed

using SpikeStartFn = bool(__fastcall*)(uintptr_t controller, uintptr_t spike, int kind);
using SpikeSearchFn = bool(__fastcall*)(uintptr_t result, uintptr_t owner, const cheats::Vec3* from, const cheats::Vec3* velocity, float time,
                                        float gravity, uintptr_t player);
const float SPIKE_SPEED_MAX = 3, SPIKE_TURN_MAX_DEG = 180, SPIKE_TURN_STEP_DEG = 30, DEG = 3.14159265f / 180;
inline uintptr_t spike_start_fn = 0;
inline int spike_controller = -1;
inline SpikeStartFn spike_start_original = nullptr;
inline uintptr_t spike_search_fn = 0;
inline SpikeSearchFn spike_search_original = nullptr;
using SlowmoPickFn = int(__fastcall*)(uintptr_t player);
inline uintptr_t slowmo_pick_fn = 0;
inline SlowmoPickFn slowmo_pick_original = nullptr;

inline void locate(uintptr_t base) {
    auto hits = game::find_code(base, SPIKE_START_CALL, 2);
    if (hits.size() != 1) return;
    spike_controller = game::rdv<int32_t>(hits[0] + SPIKE_CONTROLLER_AT);
    spike_start_fn = hits[0] + SPIKE_CALL_AT + 5 + game::rdv<int32_t>(hits[0] + SPIKE_CALL_AT + 1);
    if (auto search = game::find_code(base, SPIKE_SEARCH_CODE, 2); search.size() == 1) spike_search_fn = search[0];
    if (auto pick = game::find_code(base, SLOWMO_PICK_CODE, 2); pick.size() == 1) slowmo_pick_fn = pick[0];
}

inline bool own_spike_controller(uintptr_t controller) {
    uintptr_t me = cheats::local_object();
    return me && spike_controller > 0 && controller == game::rdv<uintptr_t>(me + spike_controller);
}

inline bool __fastcall spike_start_hook(uintptr_t controller, uintptr_t spike, int kind) {
    bool mine = own_spike_controller(controller), refuse = mine && cheats::is_on("unspikeable");
    static DWORD last = 0;
    if (GetTickCount() - last > REFUSE_LOG_GAP)
        last = GetTickCount(), cheats::logf_hook("pvp: spike impale on %s (kind %d)%s", mine ? "you" : "another player", kind, refuse ? ", refused" : "");
    return !refuse && spike_start_original(controller, spike, kind);
}

// Spike range speeds the throw up (a faster throw searches farther and lands sooner), Spike angle retries the search
// with the throw turned in SPIKE_TURN_STEP_DEG steps both ways, nearest angle first. The game still checks every trap.
inline std::vector<cheats::Vec3> spike_throws(cheats::Vec3 v, float range, float turn) {
    float speed = 1 + (SPIKE_SPEED_MAX - 1) * range;
    std::vector<cheats::Vec3> out{{v.x * speed, v.y, v.z * speed}};
    for (float a = SPIKE_TURN_STEP_DEG; a <= SPIKE_TURN_MAX_DEG * turn + 0.01f; a += SPIKE_TURN_STEP_DEG)
        for (float side : {a, -a}) {
            if (a >= SPIKE_TURN_MAX_DEG && side < 0) continue;
            float c = std::cos(side * DEG), sn = std::sin(side * DEG);
            out.push_back({(v.x * c - v.z * sn) * speed, v.y, (v.x * sn + v.z * c) * speed});
        }
    return out;
}

inline bool __fastcall spike_search_hook(uintptr_t result, uintptr_t owner, const cheats::Vec3* from, const cheats::Vec3* velocity, float time,
                                         float gravity, uintptr_t player) {
    float range = cheats::tweak_strength("spike_range"), turn = cheats::tweak_strength("spike_angle");
    std::vector<cheats::Vec3> throws = velocity ? spike_throws(*velocity, range, turn) : std::vector<cheats::Vec3>{};
    size_t caught = 0;
    bool found = !velocity && spike_search_original(result, owner, from, velocity, time, gravity, player);
    for (; caught < throws.size() && !found; caught++) found = spike_search_original(result, owner, from, &throws[caught], time, gravity, player);
    static DWORD last = 0;
    if (GetTickCount() - last > SEARCH_LOG_GAP) {
        last = GetTickCount();
        float speed = velocity ? std::sqrt(velocity->x * velocity->x + velocity->z * velocity->z) : 0;
        cheats::logf_hook("pvp: spike search from gamedll+%llx, throw %.1f m/s for %.2f s, %s", (unsigned long long)((uintptr_t)__builtin_return_address(0) - game::g.base),
                          speed, time, found ? caught > 1 ? "a trap off the throw caught" : "a trap caught" : "no trap");
    }
    return found;
}

inline int __fastcall slowmo_pick_hook(uintptr_t player) {
    int mode = slowmo_pick_original(player);
    if (mode != SLOWMO_GRAB_OVERRUN) return mode;
    uintptr_t grab = game::rdv<uintptr_t>(player + GRAB_CONTROLLER);
    return game::rdv<float>(grab + GRAB_TIME) - game::rdv<float>(grab + GRAB_PLANNED) > SLOWMO_OVERRUN_MAX ? SLOWMO_NONE : mode;
}

inline bool install() {
    if (!spike_start_original && spike_start_fn)
        spike_start_original = (SpikeStartFn)game::detour(spike_start_fn, SPIKE_START_PROLOGUE, sizeof SPIKE_START_PROLOGUE, (const void*)&spike_start_hook);
    if (!spike_search_original && spike_search_fn)
        spike_search_original =
            (SpikeSearchFn)game::detour(spike_search_fn, SPIKE_SEARCH_PROLOGUE, sizeof SPIKE_SEARCH_PROLOGUE, (const void*)&spike_search_hook);
    if (!slowmo_pick_original && slowmo_pick_fn)
        slowmo_pick_original = (SlowmoPickFn)game::detour(slowmo_pick_fn, SLOWMO_PICK_PROLOGUE, sizeof SLOWMO_PICK_PROLOGUE, (const void*)&slowmo_pick_hook);
    return spike_start_original && spike_search_original && slowmo_pick_original;
}

}
