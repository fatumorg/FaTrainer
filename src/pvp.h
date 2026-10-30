#pragma once
#include <windows.h>
#include <cstdint>
#include "cheats.h"
#include "game.h"

namespace pvp {

// Every game starts the spike death of a player through one call in PVPGrabController (rva d68a30 in 1.55):
//   mov r8d, edi / mov rdx, rbx / mov rcx, [player + SpikesHitController] / call SpikesHitController::Start
// The knock-back's spike search runs in the attacker's game; this start runs in every game, the victim's too.
const char* const SPIKE_START_CALL = "FF 50 20 44 8B C7 48 8B D3 48 8B 88 ? ? ? ? E8 ? ? ? ?";
const int SPIKE_CONTROLLER_AT = 12, SPIKE_CALL_AT = 16;
const uint8_t SPIKE_START_PROLOGUE[] = {0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x50};
const DWORD REFUSE_LOG_GAP = 5000;

using SpikeStartFn = bool(__fastcall*)(uintptr_t controller, uintptr_t spike, int kind);
inline uintptr_t spike_start_fn = 0;
inline int spike_controller = -1;
inline SpikeStartFn spike_start_original = nullptr;

inline void locate(uintptr_t base) {
    auto hits = game::find_code(base, SPIKE_START_CALL, 2);
    if (hits.size() != 1) return;
    spike_controller = game::rdv<int32_t>(hits[0] + SPIKE_CONTROLLER_AT);
    spike_start_fn = hits[0] + SPIKE_CALL_AT + 5 + game::rdv<int32_t>(hits[0] + SPIKE_CALL_AT + 1);
}

inline bool own_spike_controller(uintptr_t controller) {
    uintptr_t me = cheats::local_object();
    return me && spike_controller > 0 && controller == game::rdv<uintptr_t>(me + spike_controller);
}

inline bool __fastcall spike_start_hook(uintptr_t controller, uintptr_t spike, int kind) {
    if (cheats::is_on("unspikeable") && own_spike_controller(controller)) {
        static DWORD last = 0;
        if (GetTickCount() - last > REFUSE_LOG_GAP) last = GetTickCount(), cheats::logf_hook("pvp: refused a spike impale on you (kind %d)", kind);
        return false;
    }
    return spike_start_original(controller, spike, kind);
}

inline bool install() {
    if (!spike_start_original && spike_start_fn)
        spike_start_original = (SpikeStartFn)game::detour(spike_start_fn, SPIKE_START_PROLOGUE, sizeof SPIKE_START_PROLOGUE, (const void*)&spike_start_hook);
    return spike_start_original;
}

}
