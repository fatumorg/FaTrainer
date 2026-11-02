#pragma once
#include "cheats.h"

namespace dlc {

enum Mode { MODE_NONE, MODE_PRISON, MODE_BOZAK };

// A Bozak Horde trial is a Quests::BozakTimerPhase: +0x90 is the time limit for the current co-op size (set from
// +0xc0..+0xcc when a wave starts, gamedll e2e547), checked every frame against the running time (e2e3d0).
const int BOZAK_LIMIT = 0x90;
const float BOZAK_LIMIT_MAX = 3600, WAVE_RADIUS = 100000;

inline bool live(uintptr_t object, uintptr_t vtable) { return vtable && game::rdv<uintptr_t>(object) == vtable; }

inline Mode mode() {
    for (uintptr_t d : game::g.prison_data)
        if (live(d, game::g.vt_prison_data)) return MODE_PRISON;
    for (uintptr_t t : game::g.bozak_timers)
        if (live(t, game::g.vt_bozak_timer)) return MODE_BOZAK;
    return MODE_NONE;
}

// Moves every running Bozak trial limit forward by the time since the last tick, so the time left stands still.
inline int pause_bozak(bool on, float dt) {
    int found = 0;
    for (uintptr_t t : game::g.bozak_timers) {
        if (!live(t, game::g.vt_bozak_timer)) continue;
        found++;
        float limit = game::rdv<float>(t + BOZAK_LIMIT, NAN);
        if (on && dt > 0 && dt <= 1 && limit > 0 && limit < BOZAK_LIMIT_MAX) game::wr<float>(t + BOZAK_LIMIT, limit + dt);
    }
    return found;
}

// Prison timers pause in cheats::tick (pause_prison); this is the Bozak part, called every frame from the window thread.
inline int pause_timers() {
    static DWORD last = 0;
    DWORD now = GetTickCount();
    float dt = last ? (now - last) / 1000.0f : 0;
    last = cheats::is_on("bozak_pause") ? now : 0;
    return pause_bozak(last != 0, dt);
}

inline int kill_wave() {
    std::lock_guard<std::mutex> l(game::mx);
    return cheats::kill_enemies(WAVE_RADIUS);
}

}
