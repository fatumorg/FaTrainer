#pragma once
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <map>
#include <cstdint>
#include <vector>
#include "cheats.h"
#include "config.h"
#include "detect.h"
#include "game.h"

namespace pvp {

enum Event { EV_DROPKICK, EV_DFA, EV_TACKLE, EV_POUNCE, EV_POUND, EV_SPIT, EV_CLAW, EV_KICK, EV_EVADE, EV_IMPALE, EV_NEST_DAMAGE, EV_NEST_DEATH,
             EV_DAMAGE, EV_DEATH, EV_HEALTH, EV_EXPLOSION, EV_BURN, EV_COUNT };

struct Shield { bool unspikeable, undropkickable, no_dfa, lock_tackle, lock_pounce, lock_pound, lock_spit; };

inline bool refuse(Event e, const Shield& s) {
    switch (e) {
    case EV_IMPALE: return s.unspikeable;
    case EV_DROPKICK: return s.undropkickable;
    case EV_DFA: return s.no_dfa;
    case EV_TACKLE: return s.lock_tackle;
    case EV_POUNCE: return s.lock_pounce;
    case EV_POUND: return s.lock_pound;
    case EV_SPIT: return s.lock_spit;
    default: return false;
    }
}

inline bool evade_with(const char* cheat, int attack, int method) { return cheats::is_on(cheat) && config::cfg.pvp.evade[attack] == method; }

inline Shield shield_now() {
    return {cheats::is_on("unspikeable"), cheats::is_on("undropkickable"), cheats::is_on("no_dfa"),
            evade_with("evade_tackle", config::EVADE_TACKLE, config::EVADE_LOCK), evade_with("evade_pounce", config::EVADE_POUNCE, config::EVADE_LOCK),
            evade_with("evade_pound", config::EVADE_POUND, config::EVADE_LOCK), evade_with("evade_spit", config::EVADE_SPIT, config::EVADE_LOCK)};
}

inline const char* event_name(Event e) {
    switch (e) {
    case EV_DROPKICK: return "dropkick";
    case EV_DFA: return "death from above";
    case EV_TACKLE: return "tackle";
    case EV_POUNCE: return "pounce";
    case EV_POUND: return "ground pound";
    case EV_SPIT: return "spit";
    case EV_IMPALE: return "spike impale";
    default: return "attack";
    }
}

// Every PvP grab goes through PVPGrabController (player+0xc88) of the attacker: kind 1 and 2 are the hunter's pounce,
// 3 the ground pound's knock-back, 4 the survivor's death from above, 5 the dropkick (DefensiveController).
// The attacker's game decides the grab; every other game applies the copy it receives:
//   0xd6b530 (attacker's controller, message) for grabs, the target at message+0x28;
//   0xd6d8a0 (victim's controller, attacker, message) for knock-backs.
// Both messages carry the grab id at +0x40 and the kind at +0x48.
const char* const GRAB_APPLY_CODE = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 83 B9 88 02 00 00 00 48 8B F2 48 8B F9 7C 07";
const uint8_t GRAB_APPLY_PROLOGUE[] = {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57};
const char* const KNOCK_APPLY_CODE =
    "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 30 8B 81 88 02 00 00 33 F6 49 8B F8 4C 8B F2";
const uint8_t KNOCK_APPLY_PROLOGUE[] = {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18};
const int GRAB_ID = 0x288, MSG_ID = 0x40, MSG_KIND = 0x48, MSG_TARGET = 0x28;

// The attacker's own start of a grab (rva d67a90 in 1.55): (attacker's PVPGrabController, target player, kind). It
// builds the message every game applies, so a grab started here plays out in every game like a real one.
const char* const GRAB_START_CODE = "48 8B C4 55 48 8D 68 A8 48 81 EC 50 01 00 00 48 89 58 08 41 8B D8 48 89 70 10 48 8B F1 48 89 78 18 48 8D 4C 24 40 48 8B FA";
const int KIND_POUNCE = 2, KIND_DROPKICK = 5;
using GrabStartFn = void(__fastcall*)(uintptr_t controller, uintptr_t target, int kind);

inline Event grab_event(int kind) {
    switch (kind) {
    case 1: case 2: return EV_POUNCE;
    case 3: return EV_POUND;
    case 4: return EV_DFA;
    case 5: return EV_DROPKICK;
    default: return EV_COUNT;
    }
}

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
using GrabApplyFn = uintptr_t(__fastcall*)(uintptr_t controller, uintptr_t message);
using KnockApplyFn = uintptr_t(__fastcall*)(uintptr_t controller, uintptr_t attacker, uintptr_t message);
inline uintptr_t grab_apply_fn = 0, knock_apply_fn = 0, grab_start_fn = 0;
inline GrabApplyFn grab_apply_original = nullptr;
inline KnockApplyFn knock_apply_original = nullptr;

inline void locate(uintptr_t base) {
    auto hits = game::find_code(base, SPIKE_START_CALL, 2);
    if (hits.size() != 1) return;
    spike_controller = game::rdv<int32_t>(hits[0] + SPIKE_CONTROLLER_AT);
    spike_start_fn = hits[0] + SPIKE_CALL_AT + 5 + game::rdv<int32_t>(hits[0] + SPIKE_CALL_AT + 1);
    if (auto search = game::find_code(base, SPIKE_SEARCH_CODE, 2); search.size() == 1) spike_search_fn = search[0];
    if (auto pick = game::find_code(base, SLOWMO_PICK_CODE, 2); pick.size() == 1) slowmo_pick_fn = pick[0];
    if (auto grab = game::find_code(base, GRAB_APPLY_CODE, 2); grab.size() == 1) grab_apply_fn = grab[0];
    if (auto knock = game::find_code(base, KNOCK_APPLY_CODE, 2); knock.size() == 1) knock_apply_fn = knock[0];
    if (auto start = game::find_code(base, GRAB_START_CODE, 2); start.size() == 1) grab_start_fn = start[0];
}

inline bool own_spike_controller(uintptr_t controller) {
    uintptr_t me = cheats::local_object();
    return me && spike_controller > 0 && controller == game::rdv<uintptr_t>(me + spike_controller);
}

inline bool __fastcall spike_start_hook(uintptr_t controller, uintptr_t spike, int kind) {
    bool mine = own_spike_controller(controller), refuse = mine && pvp::refuse(EV_IMPALE, shield_now());
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

// Auto-Kill: a player we started a grab on gets spikes where it stands. The spike search fills its result (kept at
// PVPGrabController+0x2ec and sent in the grab message to every game) with a trap id (-1 none), the landing point, the
// trap's axis, the gravity (FLT_MAX none) and the flight time; SpikesHitController::Start (gamedll d86ce0) only checks
// those and impales at the landing point. An id no trap has makes the impale skip the trap's own model.
const double KILL_PENDING_SECONDS = 4;
const int32_t NO_SUCH_TRAP = 0x7ffffff0;
const float KILL_FLIGHT = 0.1f;
struct SpikeHit { int32_t trap; cheats::Vec3 at, axis; float gravity, time; };
static_assert(sizeof(SpikeHit) == 0x24, "spike search result");
inline std::mutex kill_mx;
inline std::map<uintptr_t, double> kill_pending;

inline bool kill_wanted(uintptr_t p) {
    std::lock_guard<std::mutex> l(kill_mx);
    auto it = kill_pending.find(p);
    return it != kill_pending.end() && cheats::now_seconds() < it->second;
}

inline bool spikes_here(uintptr_t result, uintptr_t owner, const cheats::Vec3* from, const cheats::Vec3* velocity, float gravity, uintptr_t player) {
    cheats::Vec3 axis{1, 0, 0};
    if (float h = velocity ? std::sqrt(velocity->x * velocity->x + velocity->z * velocity->z) : 0; h > 0.01f) axis = {velocity->x / h, 0, velocity->z / h};
    *(SpikeHit*)result = {NO_SUCH_TRAP, *from, axis, gravity, KILL_FLIGHT};
    cheats::logf_hook("pvp: auto-kill puts spikes under a player");
    std::lock_guard<std::mutex> l(kill_mx);
    kill_pending.erase(owner), kill_pending.erase(player);
    return true;
}

inline bool __fastcall spike_search_hook(uintptr_t result, uintptr_t owner, const cheats::Vec3* from, const cheats::Vec3* velocity, float time,
                                         float gravity, uintptr_t player) {
    if (from && (kill_wanted(owner) || kill_wanted(player))) return spikes_here(result, owner, from, velocity, gravity, player);
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

// A new grab on us (its id is not the one the controller already runs) is dropped before it is applied, so our game
// never moves us. A grab already running always goes through, so switching a shield on mid-grab cannot leave us stuck.
inline bool refuse_grab(uintptr_t controller, uintptr_t message, bool at_me, const char* how) {
    int id = game::rdv<int>(message + MSG_ID, -1), kind = game::rdv<int>(message + MSG_KIND, -1);
    if (!at_me || id < 0 || id == game::rdv<int>(controller + GRAB_ID, -1)) return false;
    Event e = grab_event(kind);
    bool refused = pvp::refuse(e, shield_now());
    static DWORD last = 0;
    if (GetTickCount() - last > REFUSE_LOG_GAP)
        last = GetTickCount(), cheats::logf_hook("pvp: %s on you (%s, kind %d)%s", event_name(e), how, kind, refused ? ", refused" : "");
    return refused;
}

const int MSG_OWNER = 0x08;
inline std::atomic<uintptr_t> pouncer{0};
inline std::atomic<double> pounced_at{-1};

inline detect::Kind detect_kind(Event e) {
    switch (e) {
    case EV_DROPKICK: return detect::K_DROPKICK;
    case EV_DFA: return detect::K_DFA;
    case EV_POUNCE: return detect::K_POUNCE;
    case EV_POUND: return detect::K_POUND;
    default: return detect::K_COUNT;
    }
}

// Detection: how far another player was from his target when his new grab started.
inline void measure_grab(uintptr_t controller, uintptr_t message, uintptr_t me) {
    uintptr_t owner = game::rdv<uintptr_t>(message + MSG_OWNER), target = game::rdv<uintptr_t>(message + MSG_TARGET);
    detect::Kind k = detect_kind(grab_event(game::rdv<int>(message + MSG_KIND)));
    cheats::Vec3 a, b;
    if (k == detect::K_COUNT || !owner || !target || owner == me || game::rdv<int>(message + MSG_ID, -1) < 0 ||
        game::rdv<int>(message + MSG_ID, -1) == game::rdv<int>(controller + GRAB_ID, -1) || game::g.player_control < 0 ||
        !cheats::position_of(owner + game::g.player_control, &a) || !cheats::position_of(target + game::g.player_control, &b))
        return;
    float d = cheats::distance(a, b);
    std::lock_guard<std::mutex> l(detect::players_mx);
    detect::players[owner].sample(k, d);
}

inline uintptr_t __fastcall grab_apply_hook(uintptr_t controller, uintptr_t message) {
    uintptr_t me = cheats::local_object();
    bool at_me = me && game::rdv<uintptr_t>(message + MSG_TARGET) == me;
    measure_grab(controller, message, me);
    if (refuse_grab(controller, message, at_me, "grab")) return 0;
    if (at_me && grab_event(game::rdv<int>(message + MSG_KIND)) == EV_POUNCE && game::rdv<int>(message + MSG_ID, -1) != game::rdv<int>(controller + GRAB_ID, -1))
        pouncer = game::rdv<uintptr_t>(message + MSG_OWNER), pounced_at = cheats::now_seconds();
    return grab_apply_original(controller, message);
}

inline uintptr_t __fastcall knock_apply_hook(uintptr_t controller, uintptr_t attacker, uintptr_t message) {
    uintptr_t me = cheats::local_object();
    bool at_me = me && attacker != me && controller == game::rdv<uintptr_t>(me + GRAB_CONTROLLER);
    return refuse_grab(controller, message, at_me, "knock-back") ? 0 : knock_apply_original(controller, attacker, message);
}

inline bool install() {
    if (!grab_apply_original && grab_apply_fn)
        grab_apply_original = (GrabApplyFn)game::detour(grab_apply_fn, GRAB_APPLY_PROLOGUE, sizeof GRAB_APPLY_PROLOGUE, (const void*)&grab_apply_hook);
    if (!knock_apply_original && knock_apply_fn)
        knock_apply_original = (KnockApplyFn)game::detour(knock_apply_fn, KNOCK_APPLY_PROLOGUE, sizeof KNOCK_APPLY_PROLOGUE, (const void*)&knock_apply_hook);
    if (!spike_start_original && spike_start_fn)
        spike_start_original = (SpikeStartFn)game::detour(spike_start_fn, SPIKE_START_PROLOGUE, sizeof SPIKE_START_PROLOGUE, (const void*)&spike_start_hook);
    if (!spike_search_original && spike_search_fn)
        spike_search_original =
            (SpikeSearchFn)game::detour(spike_search_fn, SPIKE_SEARCH_PROLOGUE, sizeof SPIKE_SEARCH_PROLOGUE, (const void*)&spike_search_hook);
    if (!slowmo_pick_original && slowmo_pick_fn)
        slowmo_pick_original = (SlowmoPickFn)game::detour(slowmo_pick_fn, SLOWMO_PICK_PROLOGUE, sizeof SLOWMO_PICK_PROLOGUE, (const void*)&slowmo_pick_hook);
    return spike_start_original && spike_search_original && slowmo_pick_original && grab_apply_original && knock_apply_original;
}

// Auto-Evade. Yaw 0 looks along +Z and grows toward +X; pitch grows upward. Radians.
inline void aim_angles(cheats::Vec3 eye, cheats::Vec3 target, float& yaw, float& pitch) {
    float dx = target.x - eye.x, dy = target.y - eye.y, dz = target.z - eye.z;
    yaw = std::atan2(dx, dz), pitch = std::atan2(dy, std::sqrt(dx * dx + dz * dz));
}

inline double reaction(bool instant) { return cheats::reaction_time(instant); }

// A tackle is a sprint straight at you: closing faster than TACKLE_SPEED inside TACKLE_REACH. A ground pound comes
// down on you from above: the hunter falls faster than POUND_FALL within POUND_REACH. Each starts one dodge step.
const float TACKLE_REACH = 9, TACKLE_SPEED = 11, TACKLE_HEIGHT = 2.5f, POUND_REACH = 9, POUND_FALL = 8, POUND_ABOVE = 1.5f;
const double AIM_SECONDS = 1.0, TRACK_GAP = 0.25;
const DWORD STEP_MS = 280, FLASHLIGHT_PRESS_MS = 80;
// A pounce is the hunter flying at you: closing faster than POUNCE_SPEED inside POUNCE_REACH, off the ground.
const float POUNCE_REACH = 30, POUNCE_SPEED = 14, POUNCE_RISE = 0.5f;
// The UV light is the flashlight (T by default, DIK_T) in Be the Zombie. Its on/off state is not read from the game:
// every press of the key toggles it, and a UV Suppressor turns it off.
const BYTE FLASHLIGHT_KEY = 0x14;
inline bool uv_on = false, flashlight_held = false;

inline void track_flashlight() {
    bool held = input::held(FLASHLIGHT_KEY);
    if (held && !flashlight_held) uv_on = !uv_on;
    flashlight_held = held;
    uintptr_t me = cheats::local_player();
    if (!me || cheats::light_disabled(me)) uv_on = false;
}

inline void flash_uv() {
    if (uv_on) return;
    input::press(FLASHLIGHT_KEY, FLASHLIGHT_PRESS_MS);
    uv_on = true;
}
const float AIM_GAIN = 0.5f, AIM_COUNTS_PER_RADIAN = 440, AIM_MAX_COUNTS = 120;
struct Track { cheats::Vec3 at{NAN, NAN, NAN}, v{}; double when = -1; };
inline std::map<uintptr_t, Track> tracks;
inline double aim_until = -1, uv_at = -1;
inline uintptr_t aim_at = 0;

inline cheats::Vec3 forward_of(const float m[16]) {
    float f = std::sqrt(m[12] * m[12] + m[13] * m[13] + m[14] * m[14]);
    return f > 1e-6f ? cheats::Vec3{m[12] / f, m[13] / f, m[14] / f} : cheats::Vec3{0, 0, 1};
}

inline void schedule_step(const float m[16], cheats::Vec3 away, cheats::Vec3 flight, double now, const char* what, float distance) {
    using namespace cheats;
    if (pending_dodge.key || now < dodge_free_at) return;
    BYTE key = dodge_key(m, away, flight);
    double r = reaction(config::cfg.pvp.instant);
    logf_hook("dodge: %s from %.1f m, %s", what, distance, key ? key_name(key) : "no free direction");
    if (key) pending_dodge = {key, now + r, STEP_MS};
}

inline void steer(const float m[16], cheats::Vec3 eye, cheats::Vec3 target) {
    float want_yaw, want_pitch, yaw, pitch;
    aim_angles(eye, target, want_yaw, want_pitch);
    cheats::Vec3 f = forward_of(m);
    aim_angles({0, 0, 0}, f, yaw, pitch);
    float dyaw = std::remainder(want_yaw - yaw, 2 * 3.14159265f), dpitch = want_pitch - pitch;
    long dx = std::lround(std::clamp(dyaw * AIM_COUNTS_PER_RADIAN * AIM_GAIN, -AIM_MAX_COUNTS, AIM_MAX_COUNTS));
    long dy = std::lround(std::clamp(-dpitch * AIM_COUNTS_PER_RADIAN * AIM_GAIN, -AIM_MAX_COUNTS, AIM_MAX_COUNTS));
    input::look_x += dx, input::look_y += dy;
}

inline void auto_evade() {
    using namespace cheats;
    bool tackle = evade_with("evade_tackle", config::EVADE_TACKLE, config::EVADE_DODGE), pound = evade_with("evade_pound", config::EVADE_POUND, config::EVADE_DODGE);
    bool uv = evade_with("evade_pounce", config::EVADE_POUNCE, config::EVADE_AIM_UV);
    if (uv) track_flashlight();
    if (!tackle && !pound && !uv) return;
    double now = now_seconds();
    float m[16];
    Vec3 eye;
    if (side != SIDE_SURVIVOR || !view_matrix(m, &eye) || !std::isfinite(eye.x)) return;
    run_pending_dodge(now);
    std::vector<EspTarget> players;
    {
        std::lock_guard<std::mutex> l(esp_mx);
        players = seen_players;
    }
    Vec3 feet{eye.x, eye.y - BODY_HEIGHT, eye.z};
    std::map<uintptr_t, Track> next;
    for (auto& t : players) {
        if (!t.hunter) continue;
        Track tr = tracks.count(t.p) ? tracks[t.p] : Track{};
        if (tr.when >= 0 && now - tr.when > 1e-3 && now - tr.when < TRACK_GAP) {
            float k = (float)(1 / (now - tr.when));
            tr.v = {(t.feet.x - tr.at.x) * k, (t.feet.y - tr.at.y) * k, (t.feet.z - tr.at.z) * k};
        } else if (now - tr.when >= TRACK_GAP) tr.v = {};
        tr.at = t.feet, tr.when = now;
        next[t.p] = tr;
        float dx = feet.x - t.feet.x, dz = feet.z - t.feet.z, flat = std::sqrt(dx * dx + dz * dz), dy = t.feet.y - feet.y;
        if (flat < 1e-3f) continue;
        float closing = (tr.v.x * dx + tr.v.z * dz) / flat;
        if (tackle && flat < TACKLE_REACH && std::fabs(dy) < TACKLE_HEIGHT && closing > TACKLE_SPEED) {
            float along = (dx * tr.v.x + dz * tr.v.z) / std::max(1e-3f, tr.v.x * tr.v.x + tr.v.z * tr.v.z);
            schedule_step(m, {dx - tr.v.x * along, 0, dz - tr.v.z * along}, tr.v, now, "tackle", flat);
        }
        if (pound && flat < POUND_REACH && dy > POUND_ABOVE && tr.v.y < -POUND_FALL) schedule_step(m, {dx, 0, dz}, tr.v, now, "ground pound", flat);
        if (uv && now >= aim_until && flat < POUNCE_REACH && closing > POUNCE_SPEED && tr.v.y > -POUND_FALL && dy > -POUNCE_RISE) {
            aim_at = t.p, aim_until = now + AIM_SECONDS, uv_at = now + reaction(config::cfg.pvp.instant);
            logf_hook("dodge: hunter %.1f m away flies at you at %.1f m/s, turning to the hunter and turning UV on", flat, closing);
        }
    }
    tracks.swap(next);
    double at = pounced_at.exchange(-1);
    if (uv && at >= 0 && now >= aim_until) {
        aim_at = pouncer, aim_until = now + AIM_SECONDS, uv_at = now + reaction(config::cfg.pvp.instant);
        logf_hook("dodge: pounce on you, turning to the hunter and turning UV on");
    }
    if (now < aim_until) {
        auto t = std::find_if(players.begin(), players.end(), [&](const EspTarget& e) { return e.p == aim_at; });
        if (t == players.end()) t = std::min_element(players.begin(), players.end(), [](const EspTarget& a, const EspTarget& b) { return a.hunter > b.hunter || (a.hunter == b.hunter && a.distance < b.distance); });
        if (t != players.end() && (t->hunter || t->p == aim_at)) steer(m, eye, {t->feet.x, t->feet.y + BODY_HEIGHT * 0.6f, t->feet.z});
    }
    if (uv_at >= 0 && now >= uv_at) uv_at = -1, flash_uv();
}

struct Loop { double due = -1; bool was_alive = false; };

// Arms when the target is first seen alive or comes back alive, fires once delay seconds later.
inline bool loop_fire(Loop& l, bool alive, double now, float delay) {
    if (!alive) return l = Loop{}, false;
    if (!l.was_alive) l.was_alive = true, l.due = now + delay;
    if (l.due < 0 || now < l.due) return false;
    l.due = -1;
    return true;
}

inline std::atomic<uintptr_t> picked{0};
inline std::atomic<bool> kill_requested{false};
inline std::map<uintptr_t, Loop> loops;

inline std::vector<uintptr_t> kill_targets(int mode) {
    using namespace cheats;
    std::vector<EspTarget> players;
    {
        std::lock_guard<std::mutex> l(esp_mx);
        players = seen_players;
    }
    bool hunter = side == SIDE_HUNTER;
    std::vector<EspTarget> foes;
    for (auto& t : players)
        if (t.hunter != hunter && t.health > 0) foes.push_back(t);
    std::sort(foes.begin(), foes.end(), [](const EspTarget& a, const EspTarget& b) { return a.distance < b.distance; });
    std::vector<uintptr_t> out;
    for (auto& t : foes)
        if (!hunter || mode == config::TARGET_ALL || (mode == config::TARGET_NEAREST && out.empty()) || (mode == config::TARGET_PICKED && t.p == picked)) out.push_back(t.p);
    return out;
}

inline bool impale(uintptr_t target) {
    uintptr_t me = cheats::local_player();
    uintptr_t controller = me ? game::rdv<uintptr_t>(me + GRAB_CONTROLLER) : 0;
    if (!grab_start_fn || !controller || !target || target == me || !cheats::alive(target)) return false;
    {
        std::lock_guard<std::mutex> l(kill_mx);
        kill_pending[target] = cheats::now_seconds() + KILL_PENDING_SECONDS;
    }
    ((GrabStartFn)grab_start_fn)(controller, target, cheats::side == cheats::SIDE_HUNTER ? KIND_POUNCE : KIND_DROPKICK);
    cheats::logf_hook("pvp: auto-kill started a %s on a player %.0f m away", cheats::side == cheats::SIDE_HUNTER ? "pounce" : "dropkick",
                      cheats::distance(cheats::player_position(), [&] { cheats::Vec3 p{}; cheats::position_of(target + game::g.player_control, &p); return p; }()));
    return true;
}

inline int kill_once() {
    int started = 0;
    for (uintptr_t t : kill_targets(cheats::side == cheats::SIDE_HUNTER ? config::cfg.pvp.kill_target : config::TARGET_NEAREST)) started += impale(t);
    return started;
}

// Called every frame: Kill once requests and the Auto-Kill loop, one Loop per target player.
inline void auto_kill() {
    using namespace cheats;
    if (side != SIDE_SURVIVOR && side != SIDE_HUNTER) return;
    if (kill_requested.exchange(false)) logf_hook("pvp: kill once, %d grabs started", kill_once());
    if (!is_on("auto_kill")) return loops.clear();
    double now = now_seconds();
    auto targets = kill_targets(side == SIDE_HUNTER ? config::cfg.pvp.kill_target : config::TARGET_NEAREST);
    std::map<uintptr_t, Loop> next;
    for (uintptr_t t : targets) {
        Loop l = loops.count(t) ? loops[t] : Loop{};
        if (loop_fire(l, health_of(t) > 0, now, config::cfg.pvp.kill_delay)) impale(t);
        next[t] = l;
    }
    loops.swap(next);
}

}
