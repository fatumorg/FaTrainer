#pragma once
#include <atomic>
#include <deque>
#include <random>
#include <cstdio>
#include <set>
#include "config.h"
#include "game.h"
#include "input.h"
#include "skills.h"

namespace cheats {

using game::g;
using game::rd;
using game::rdv;
using game::wr;

const int LOCAL_PLAYER = 0x780, PARAM_CONTAINER = 0xe58, PARAM_TABLE = 0xd0, PARAM_VALUE = 8;
const int HEALTH_OBJECT = 0x8f8, STAMINA_OBJECTS[] = {0x1340, 0x1348};
const int STAMINA_CURRENT = 0x10, STAMINA_FULL = 0x14;
const int ROPE_ENERGY = 0x40, ROPE_DEPLETED = 0x44;
const int REPL_OWNED = 0x28;
const int PARAM_SLOTS = 1100, CACHED_VALUE = 8, CACHED_FLAGS = 0x18, CACHED_VERSION = -8, PARAM_PROVIDER = 0x9c0;
const int SKILL_TREES = 0x40, TREE_RECORD = 0x20, TREE_XP = 8, TREE_LEVEL_START = 0xc, TREE_SPAN = 0x10, TREE_LEVEL = 0x14, TREE_MAX = 0x16;
const int MODULE_OWNER = 0x40, MODULE_HEALTH = 0x78;
const int SLOT_IS_IMMORTAL = 3, SLOT_SET_HEALTH = 4, HEALTH_VALUE = 0x964, SLOT_REFILL = 5, SLOT_HEALTH = 199, SLOT_MAX_HEALTH = 41, SLOT_MODULE_UPDATE = 245;
const int UV_CHARGE = 0x50, UV_EXHAUSTED = 0x55;
const int GAME_PROFILE = 0x540, PROFILE_RANK_HUMAN = 0x2d98, PROFILE_RANK_ZOMBIE = 0x2dd0, PLAYER_RANK_HUMAN = 0x74c, PLAYER_RANK_ZOMBIE = 0x750;
const int PRISON_START_TIME = 0x44, PRISON_END_TIME = 0x48, PRISON_REWARD_TIER = 0x4c, PRISON_STATE = 0x54;
const int SLOT_FLOAT_FIELD_EDITOR = 34, SLOT_PHYSICS_POSITION = 52, SLOT_KILL = 295;
const float KILL_RADIUS = 80.0f;
const int COPIED_SLOTS = 64, SLOT_VAR_FLOAT = 114, SLOT_VAR_VEC3 = 116, VAR_SLOTS = 1024;
const float ONE_HIT_HEALTH = 1.0f;
inline const uint8_t SPOT_DISTANCE_CLAMP[4] = {0xF3, 0x0F, 0x5F, 0xD3}, SPOT_DISTANCE_ZERO[4] = {0x0F, 0x57, 0xD2, 0x90};

struct Cheat {
    const char* key;
    const char* label;
    const char* hint;
    std::vector<std::pair<const char*, float>> numbers;
    std::vector<const char*> switches;
    std::atomic<bool> on{false};
};

inline const char* SPIT_REGEN[] = {"ZombieSpitControlTheHordeAmmoRegenTime", "ZombieSpitLightDisableAmmoRegenTime",
                                   "ZombieSpitDefensiveSmokeAmmoRegenTime"};

inline Cheat CHEATS[] = {
    {"god", "God mode", "You take no damage from anything.", {}, {}},
    {"stamina", "Infinite stamina", "Sprint, climb and fight without getting tired.", {}, {"InfiniteStamina"}},
    {"hook", "Infinite grappling hook", "The hook recharges instantly, so you can grapple again right away, also in the air.",
     {{"RopeEnergyRegenTime", 0.01f}}, {"CanUseHook", "ThrowHookCanUseInAir"}},
    {"uv", "Infinite UV flashlight", "The UV light never drains.", {{"FlashlightDrainMul", 0}, {"FlashlightRechargeSpeed", 1000}}, {}},
    {"one_hit", "One hit kill", "Zombies and humans drop to 1 health. Works when you are the host.", {}, {}},
    {"ammo", "Infinite ammo", "Magazines never empty and reserve ammo stays full.", {}, {}},
    {"supplies", "Infinite consumables", "Medkits, throwables and crafting materials never run out.", {}, {}},
    {"lockpick", "Instant lockpicking", "Every pick position opens the lock and lockpicks never break.", {}, {}},
    {"durability", "Unbreakable weapons", "Melee weapons stop losing durability when you hit things.",
     {{"BluntWpnDurabilityLoss", 0}, {"CutWpnDurabilityLoss", 0}}, {}},
    {"no_fall", "No fall damage", "Land safely from any height.", {{"FallDamageReduction", 1}, {"FallHeightMedium", 9999}, {"FallHeightHigh", 9999}},
     {"FallDampingEnabled", "AutomaticFallDamping"}},
    {"no_reload", "No reload", "999 round magazines and instant reloads. Best together with infinite ammo.",
     {{"FirearmsPistolReloadTimeMul", 0.05f}, {"FirearmsRevolverReloadTimeMul", 0.05f}, {"FirearmsRifleReloadTimeMul", 0.05f},
      {"FirearmsShotgunReloadTimeMul", 0.05f}, {"FirearmsHeavyReloadTimeMul", 0.05f}},
     {"FastRevolverReload"}},
    {"z_energy", "Infinite hunter energy", "Fitness and stamina never drain, UV light cannot exhaust you.", {},
     {"InfiniteFitness", "InfiniteStamina"}},
    {"z_cooldowns", "No ability cooldowns", "Tendril, camouflage and ground pound are always ready.",
     {{"TDCooldown", 0}, {"CamouflageCooldown", 0}, {"ZombieGroundPoundCooldown", 0}, {"ChargeLightCooldown", 0}},
     {}},
    {"z_spits", "Infinite spits", "Every spit type recharges instantly.", {}, {}},
    {"z_pound_hits", "Sure-hit ground pound", "Hits every survivor in range, even when a step or uneven ground would make it miss.",
     {}, {}},
    {"prison_pause", "Pause prison timers", "The run timer and the reward room countdown stand still. Works when you are the host.", {}, {}},
    {"z_camo", "Long camouflage", "Camouflage lasts ten minutes and you can run and attack while hidden.",
     {{"CamouflageDuration", 600}}, {"CamouflageEnabled", "CamouflageCanRun", "CamouflageCanAttack"}},
    {"dodge_spit", "Dodge spit",
     "Steps you aside from spit that would hit you, after a human reaction time.",
     {}, {}},
    {"one_hit_hunter", "One hit kill on the Night Hunter", "The Night Hunter drops to 1 health, so your next hit kills him. His own game decides his health in a real match.",
     {}, {}},
};

enum Group { G_GEAR, G_MOVEMENT, G_PROGRESS, G_ZOMBIE, G_HUMAN, G_SURVIVAL, G_HUNTER };

struct ScriptVar { const char* name; float at_max; };

struct Tweak {
    const char* key;
    const char* label;
    const char* hint;
    Group group;
    std::vector<const char*> params;
    std::vector<ScriptVar> vars = {};
    float max = 10.0f;
    std::atomic<float> factor{1.0f};
};

inline Tweak TWEAKS[] = {
    {"uv_slow", "Slow down UV flashlight", "The UV light uses its charge slower. x10 lasts ten times longer. Infinite UV flashlight overrides it.", G_GEAR, {}, {},
     20.0f},
    {"speed", "Movement speed", "Walk, sprint and wall run faster.", G_MOVEMENT,
     {"MoveSprintSpeed", "MoveForwardMaxSpeed", "MoveStrafeMaxSpeed", "MoveBackwardMaxSpeed", "WallrunSpeed"}, {}, 3.0f},
    {"jump", "Jump height", "Jump higher.", G_MOVEMENT, {"JumpMaxHeight", "JumpMinHeight"}, {}, 4.0f},
    {"xp", "XP gain", "Experience in every skill tree, Survivor and Night Hunter included. The game shows the boosted amount.", G_PROGRESS, {}},
    {"damage_taken", "Take less damage", "Damage you take is divided by this, as a survivor and as the Night Hunter. God mode overrides it.", G_SURVIVAL, {}},
    {"z_uv", "Less UV damage", "The UV flashlights and UV lamps hurt you slower. At Max they do not hurt you at all.", G_HUNTER, {},
     {{"f_btz_flashlight_health_damage_per_second_min", 0}, {"f_btz_flashlight_health_damage_per_second_max", 0},
      {"f_btz_zombie_world_light_damage_percent", 0}, {"f_btz_zombie_world_light_damage_percent_hub", 0}}},
    {"z_pounce", "Pounce", "At Max you pounce survivors 40 m away when you aim at them. Also grows the pounce slam blast.",
     G_ZOMBIE, {"ZombiePounceHighRageExplosionRange"},
     {{"f_btz_zombie_grab_range", 40}, {"f_btz_zombie_grab_range_velocity_factor", 1}}},
    {"z_pound", "Ground pound", "Reach of the ground pound and the aerial ground pound, and how far above or below you a survivor can stand and still get hit (normally 2 m).",
     G_ZOMBIE, {"ZombieGroundPoundRange", "GroundPoundRangeMul"}, {{"f_btz_zombie_groundpound_damage_height", 8}}},
    {"z_tackle", "Tackle", "How far away the charge tackle still connects. Normally 5 m.", G_ZOMBIE, {"ZombieChargeAttackRange"}},
    {"z_claws", "Claws", "Reach of your claw swipes. The game still aims at the survivor nearest to you.", G_ZOMBIE, {"RangeMeleeMul"}},
    {"z_spit", "Spit hit radius", "How far from a survivor your spit can land and still hit him (normally 5 m). The game that hosts the match decides spit hits, so this only works while you host, never when you invade, and it is off while you play a survivor.",
     G_ZOMBIE, {}, {}, 6.0f},
    {"h_dfa", "Death from above range", "How far away the hunter can be when you start it. At Max, 12 m. Also grows the landing shockwave.",
     G_HUMAN, {"JumpAttackRange", "JumpAttackShockwaveRadius"}, {{"f_btz_jump_attack_range", 12}, {"f_btz_jump_attack_range_velocity_factor", 0.5f}}},
    {"h_dfa_pull", "Death from above pull", "How far off target you can start it; the attack pulls you onto the hunter. At Max he can be beside or behind you.",
     G_HUMAN, {}, {{"f_btz_jump_attack_angle_max", 180}, {"f_btz_pvp_grab_above_angle_threshold", 90}, {"f_btz_pvp_grab_below_angle_threshold", -90}}},
    {"h_dfa_height", "Death from above height", "How fast you must already be falling to start it. Normally only after a long drop; at Max a normal jump is enough.",
     G_HUMAN, {}},
    {"h_dropkick", "Dropkick", "At Max you dropkick the hunter from 12 m away, even when he is not in front of you.", G_HUMAN,
     {"AirKickRangeMul"}, {{"f_btz_wrestling_kick_range", 12}, {"f_btz_wrestling_kick_range_velocity_factor", 0.5f}, {"f_btz_wrestling_kick_angle_max", 180}}},
    {"h_kicks", "Other kicks & ground pound", "Wrestling kick and ground pound reach.", G_HUMAN, {"WrestlingKickRangeMul", "GroundPoundRangeMul"}},
    {"h_melee", "Melee", "Reach of melee attacks, how far the game looks for a target and how far above or below your aim it still locks on.",
     G_HUMAN, {"RangeMeleeMul", "BestTargetMeleeRange", "MaxVerticalAngleForRangeMeleeCorrection"}},
};

inline Tweak* find_tweak(const std::string& key) {
    for (auto& t : TWEAKS)
        if (key == t.key) return &t;
    return nullptr;
}

struct Tree { int type; const char* name; };
inline const char* SKILL_POINT_PARAMS[8] = {"SkillPointsPrestigeLevel0", "SkillPointsRunner", "SkillPointsFighter", "SkillPointsStatus",
                                            "SkillPointsReputation", "SkillPointsLegend", "SkillPointsDriver", "SkillPointsHellraid"};
inline std::atomic<int> extra_skill_points[8] = {};
inline const Tree TREES[] = {{3, "Survivor"}, {1, "Agility"}, {2, "Power"}, {5, "Legend"}, {6, "Driver"}, {7, "Hellraid"}, {4, "Reputation"}, {0, "Night Hunter"}};

inline Cheat* find(const std::string& key) {
    for (auto& c : CHEATS)
        if (key == c.key) return &c;
    return nullptr;
}
inline bool is_on(const char* key) { return find(key) && find(key)->on; }

inline uintptr_t player = 0;
enum Side { SIDE_UNKNOWN, SIDE_SURVIVOR, SIDE_HUNTER };
inline std::atomic<int> side{SIDE_UNKNOWN};
inline uintptr_t dfa_fall_speed = 0;
inline float dfa_fall_original = NAN;
const float DFA_FALL_SPEED_AT_MAX = 0.5f;
inline uintptr_t local_player_root = 0, params_root = 0, unlimited_ammo_flag = 0, set_level_fn = 0, level_from_xp_fn = 0, cache_get_fn = 0, lockpick_patch = 0, forced_damage_jump = 0, pound_exposure_check = 0, profile_root = 0;
inline uintptr_t vt_param_float = 0, vt_player_float = 0, vt_param_bool = 0, var_root = 0, xp_award_site = 0;
inline uintptr_t skill_manager = 0, add_skill_fn = 0, remove_skill_fn = 0;
using VarFloatFn = float (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
using VarVec3Fn = float* (*)(uintptr_t, float*, uintptr_t, uintptr_t, uintptr_t);
inline VarFloatFn original_var_float = nullptr;
inline VarVec3Fn original_var_vec3 = nullptr;
inline uintptr_t var_vtable[VAR_SLOTS + 1];
inline std::map<std::string, int> param_ids;
inline uintptr_t immortal_vtable[COPIED_SLOTS + 1];
inline uintptr_t original_vtable = 0;
inline float best_stamina[2] = {};
inline std::mutex modules_mx;
inline std::vector<uintptr_t> enemy_modules;
inline std::map<uintptr_t, bool> nest_modules;
inline uintptr_t vt_nest_logic = 0;
inline const char* THROWABLE_CLASSES[] = {"ThrowableObject", "ThrowableLiquid"};
inline uintptr_t vt_throwable[2] = {};
inline int throwable_control[2] = {-1, -1};
const int AI_FIELDS = 0x2000;
inline std::map<uintptr_t, int> stack_floor;
inline std::map<uintptr_t, float> condition_floor;
const int ITEM_CONDITION = 0x44;
struct Saved { std::vector<uint8_t> original, written; };
inline std::map<uintptr_t, Saved> saved_bytes;
using CacheGetFn = uintptr_t (*)(uintptr_t, int);
inline CacheGetFn cache_get_original = nullptr;
struct CachedParam { std::atomic<uint8_t> kind{0}; std::atomic<bool> stale{false}; std::atomic<float> value{0}; std::atomic<uint32_t> reads{0}; };
enum { CACHED_NONE, CACHED_FLOAT, CACHED_SWITCH };
inline CachedParam cached[PARAM_SLOTS];
inline std::mutex cached_mx;
inline std::map<int, std::set<uintptr_t>> overridden_providers;
inline std::atomic<uintptr_t> player_provider{0};
inline std::vector<uintptr_t> saved_containers;

inline bool in_game_module(uintptr_t p) {
    for (auto& s : game::sections(g.base))
        if (p >= s.start && p < s.end) return true;
    return false;
}

inline uintptr_t slot(uintptr_t object, int index) { return rdv<uintptr_t>(rdv<uintptr_t>(object) + index * 8); }

inline int float_getter_offset(uintptr_t fn) {
    uint8_t b[9];
    if (!rd(fn, b, sizeof b) || b[0] != 0xF3 || b[1] != 0x0F || b[2] != 0x10 || b[3] != 0x81 || b[8] != 0xC3) return -1;
    int32_t off;
    memcpy(&off, b + 4, 4);
    return off;
}

inline void read_param_names() {
    auto secs = game::sections(g.base);
    auto* text = game::section_named(secs, ".text");
    auto* rdata = game::section_named(secs, ".rdata");
    if (!text || !rdata) return;
    const char needle[] = "\0MaxStamina";
    auto hit = std::search((const char*)rdata->start, (const char*)rdata->end, needle, needle + sizeof needle);
    if (hit == (const char*)rdata->end) return;
    uintptr_t name = (uintptr_t)hit + 1;
    auto jump_target = [&](uintptr_t c) -> uintptr_t {
        const uint8_t* b = (const uint8_t*)c;
        if (b[7] == 0xE9) return game::rip_target(c + 7, 1, 5);
        if (b[7] == 0xEB) return c + 9 + (int8_t)b[8];
        return 0;
    };
    auto is_case = [&](uintptr_t c, uintptr_t common) {
        const uint8_t* b = (const uint8_t*)c;
        return c >= text->start && c + 12 <= text->end && b[0] == 0x48 && b[1] == 0x8D && b[2] == 0x15 && jump_target(c) &&
               (!common || jump_target(c) == common);
    };
    uintptr_t case_at = 0, common = 0;
    for (uintptr_t p = text->start; p + 12 <= text->end && !case_at; p++)
        if (is_case(p, 0) && game::rip_target(p, 3, 7) == name) case_at = p, common = jump_target(p);
    if (!case_at) return;
    uint32_t rva = (uint32_t)(case_at - g.base);
    uintptr_t at = 0;
    for (uintptr_t p = text->start & ~(uintptr_t)3; p + 4 <= text->end && !at; p += 4)
        if (*(const uint32_t*)p == rva) at = p;
    if (!at) return;
    auto entry = [&](uintptr_t p) { return g.base + *(const uint32_t*)p; };
    auto in_table = [&](uintptr_t p) { return p >= text->start && p + 4 <= text->end && entry(p) + 0x10000 > common && entry(p) < common + 0x10000; };
    uintptr_t start = at;
    while (in_table(start - 4)) start -= 4;
    for (uintptr_t p = start; in_table(p); p += 4)
        if (is_case(entry(p), common)) param_ids[(const char*)game::rip_target(entry(p), 3, 7)] = (int)((p - start) / 4);
}

inline void locate(uintptr_t base) {
    if (auto hits = game::find_code(base, "48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 8B 88 80 07 00 00"); !hits.empty())
        local_player_root = game::rip_target(hits[0], 3, 7);
    if (auto hits = game::find_code(base, "48 8B 05 ? ? ? ? 74 07 48 8B ? 58 0E 00 00"); !hits.empty())
        params_root = game::rip_target(hits[0], 3, 7);
    if (auto hits = game::find_code(base, "E8 ? ? ? ? 40 38 B8 B1 15 00 00"); !hits.empty()) {
        uintptr_t getter = game::rip_target(hits[0], 1, 5);
        for (uintptr_t p = getter; p < getter + 0x60; p++)
            if (!memcmp((const void*)p, "\x48\x8D\x05", 3) && !memcmp((const void*)(p + 7), "\x48\x83\xC4\x28\xC3", 5)) {
                unlimited_ammo_flag = game::rip_target(p, 3, 7) + rdv<int32_t>(hits[0] + 8);
                break;
            }
    }
    if (auto hits = game::find_code(base, "48 89 5C 24 18 48 89 74 24 20 57 48 83 EC 20 49 63 F0 48 8B D9 48 8B 49 40 48 8B FE 48 C1 E7 05 44 0F BF C2");
        !hits.empty())
        set_level_fn = hits[0];
    if (auto hits = game::find_code(base, "40 55 56 41 57 48 83 EC 50 48 8B 41 40 48 8B F1 4C 63 FA 49 8B EF 48 C1 E5 05 66 83 7C 28 14 00");
        !hits.empty())
        level_from_xp_fn = hits[0];
    if (auto hits = game::find_code(base, "8B ? C0 09 00 00 BA ? ? ? ? E8"); !hits.empty()) cache_get_fn = game::rip_target(hits[0] + 11, 1, 5);
    if (auto hits = game::find_code(base, "45 85 FF 0F 8E ? ? ? ? 48 8B 9E E8 13 00 00 0F 29 7C 24 60"); hits.size() == 1) xp_award_site = hits[0];
    if (auto hits = game::find_code(base, "40 57 41 57 48 83 EC 28 48 89 6C 24 50 45 33 FF 48 89 74 24 58 0F B7 FA 4C 89 74 24 20 48 8B E9");
        hits.size() == 1)
        remove_skill_fn = hits[0];
    if (auto hits = game::find_code(base, "40 53 55 56 41 55 41 57 48 83 EC 50 33 F6 45 8B E8 0F B7 EA 4C 8B F9 40 38 B4 24 A8 00 00 00");
        hits.size() == 1)
        add_skill_fn = hits[0];
    const int MANAGER_CALL = 8;
    if (auto hits = game::find_code(base, "4C 89 A4 24 88 00 00 00 E8 ? ? ? ? 66 83 FD FF 0F 8E"); hits.size() == 1) {
        uintptr_t getter = game::rip_target(hits[0] + MANAGER_CALL, 1, 5);
        for (uintptr_t p = getter; p < getter + 0x60; p++)
            if (!memcmp((const void*)p, "\x48\x8D\x05", 3) && !memcmp((const void*)(p + 7), "\x48\x83\xC4\x28\xC3", 5)) {
                skill_manager = game::rip_target(p, 3, 7);
                break;
            }
    }
    const int CLAMP_AT = 0x2e;
    auto spot = game::find_code(base, "F3 0F 10 56 50 B1 01 F3 0F 5C 90 18 01 00 00 F3 0F 10 4E 54 F3 0F 59 0D ? ? ? ? 0F 54 15 ? ? ? ? 0F 54 0D ? ? ? ? F3 0F 5C D1");
    if (!spot.empty() && !memcmp((const void*)(spot[0] + CLAMP_AT), SPOT_DISTANCE_CLAMP, 4)) lockpick_patch = spot[0] + CLAMP_AT;
    if (auto hits = game::find_code(base, "4C 8B 35 ? ? ? ? 4D 85 F6 0F 84 ? ? ? ? 4D 8B B6 40 05 00 00 4D 85 F6 0F 84 ? ? ? ? F3 41 0F 10 86 68 2D 00 00");
        !hits.empty())
        profile_root = game::rip_target(hits[0], 3, 7);
    const int POUND_EXPOSURE_AT = 12;
    if (auto hits = game::find_code(base, "E8 ? ? ? ? 48 8B 0B 41 0F 2F C3 40 0F 97 C7 48 85 C9 74"); hits.size() == 1)
        pound_exposure_check = hits[0] + POUND_EXPOSURE_AT;
    const int FORCED_DAMAGE_AT = 16;
    if (auto hits = game::find_code(base, "0F 2F B3 64 09 00 00 73 0D 80 BB 2E 07 00 00 00 0F 84"); !hits.empty())
        forced_damage_jump = hits[0] + FORCED_DAMAGE_AT;
    if (auto hits = game::find_code(base, "48 8B 05 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 18 48 8B 01 FF 90 90 01 00 00 48 8B 0D ? ? ? ? 48 8D 55 ? 4C 8B C0 FF 93 90 03 00 00");
        !hits.empty())
        var_root = game::rip_target(hits[0], 3, 7);
    if (auto hits = game::find_code(base, "F3 0F 10 05 ? ? ? ? F3 44 0F 10 15 ? ? ? ? 41 0F 57 C2 0F 2F 40 04"); !hits.empty())
        dfa_fall_speed = game::rip_target(hits[0], 4, 8);
    vt_nest_logic = game::find_vtable(base, "HiveBroodLogicModule");
    for (int i = 0; i < 2; i++) {
        vt_throwable[i] = game::find_vtable(base, THROWABLE_CLASSES[i]);
        throwable_control[i] = vt_throwable[i] ? game::base_offset(base, vt_throwable[i], "IControlObject") : -1;
    }
    vt_param_float = game::find_vtable(base, "?$Param@M");
    vt_player_float = game::find_vtable(base, "FloatPlayerVariable");
    vt_param_bool = game::find_vtable(base, "?$Param@_N");
    read_param_names();
}

inline bool alive(uintptr_t p) {
    uintptr_t vt = p && g.vt_player ? rdv<uintptr_t>(p) : 0;
    return vt && (vt == g.vt_player || vt == g.vt_tutorial_player);
}

inline void no_log(const char*, ...) {}
inline void (*logf_hook)(const char*, ...) = no_log;

inline uintptr_t find_player() {
    uintptr_t local = local_player_root ? rdv<uintptr_t>(rdv<uintptr_t>(local_player_root) + LOCAL_PLAYER) : 0;
    if (alive(local)) return local;
    if (alive(player)) return player;
    for (uintptr_t p : g.players)
        if (alive(p) && in_game_module(rdv<uintptr_t>(p + HEALTH_OBJECT))) return p;
    return 0;
}

inline float health() {
    int off = float_getter_offset(slot(player, SLOT_HEALTH));
    return alive(player) && off > 0 ? rdv<float>(player + off, NAN) : NAN;
}

inline uintptr_t stamina_object(int i) { return alive(player) ? rdv<uintptr_t>(player + STAMINA_OBJECTS[i]) : 0; }
inline float stamina() { return rdv<float>(stamina_object(0) + STAMINA_CURRENT, NAN); }
inline float stamina_full() { return rdv<float>(stamina_object(0) + STAMINA_FULL, NAN); }

inline std::atomic<bool> immortal{false};
inline std::atomic<float> damage_divisor{1.0f};

inline bool __fastcall immortal_check(uintptr_t self) {
    return immortal || ((bool(__fastcall*)(uintptr_t))rdv<uintptr_t>(original_vtable + SLOT_IS_IMMORTAL * 8))(self);
}

inline uintptr_t __fastcall scaled_set_health(uintptr_t self, float value, bool flag) {
    float cur = rdv<float>(self + HEALTH_VALUE, NAN), divisor = damage_divisor;
    if (!immortal && divisor > 1.0f && value < cur) value = cur - (cur - value) / divisor;
    return ((uintptr_t(__fastcall*)(uintptr_t, float, bool))rdv<uintptr_t>(original_vtable + SLOT_SET_HEALTH * 8))(self, value, flag);
}

inline void hook_health(bool god, float divisor) {
    immortal = god;
    damage_divisor = divisor;
    bool enable = god || divisor > 1.0f;
    uintptr_t health_object = player + HEALTH_OBJECT;
    uintptr_t current = rdv<uintptr_t>(health_object);
    uintptr_t ours = (uintptr_t)&immortal_vtable[1];
    if (enable && current != ours && in_game_module(current)) {
        if (!rd(current - 8, immortal_vtable, sizeof immortal_vtable)) return;
        original_vtable = current;
        immortal_vtable[1 + SLOT_IS_IMMORTAL] = (uintptr_t)&immortal_check;
        immortal_vtable[1 + SLOT_SET_HEALTH] = (uintptr_t)&scaled_set_health;
        wr<uintptr_t>(health_object, ours);
    } else if (!enable && current == ours && original_vtable) {
        wr<uintptr_t>(health_object, original_vtable);
    }
}

inline float max_health() {
    auto fn = (float(__fastcall*)(uintptr_t, int))slot(player, SLOT_MAX_HEALTH);
    return alive(player) && in_game_module((uintptr_t)fn) ? fn(player, -1) : NAN;
}

inline void refill() {
    if (!alive(player)) return;
    float before = health();
    uintptr_t health_object = player + HEALTH_OBJECT;
    auto fn = (void(__fastcall*)(uintptr_t))slot(health_object, SLOT_REFILL);
    if (in_game_module((uintptr_t)fn)) fn(health_object);
    int off = float_getter_offset(slot(player, SLOT_HEALTH));
    float full = max_health();
    if (off > 0 && full > 0 && full < 100000 && !(health() >= full)) wr<float>(player + off, full);
    for (int i = 0; i < 2; i++) {
        uintptr_t s = rdv<uintptr_t>(player + STAMINA_OBJECTS[i]);
        float top = rdv<float>(s + STAMINA_FULL, NAN);
        if (s && top > 0 && top < 100000) wr<float>(s + STAMINA_CURRENT, top);
    }
    logf_hook("refill: health %.0f -> %.0f (max %.0f), stamina %.0f", before, health(), full, stamina());
}

inline void set_health(float v) {
    int off = float_getter_offset(slot(player, SLOT_HEALTH));
    float full = max_health();
    if (alive(player) && off > 0 && full > 0 && full < 100000) wr<float>(player + off, std::clamp(v, 1.0f, full));
}

inline void set_stamina(float v) {
    for (int i = 0; i < 2; i++) {
        uintptr_t s = stamina_object(i);
        float top = rdv<float>(s + STAMINA_FULL, NAN);
        if (s && top > 0 && top < 100000) wr<float>(s + STAMINA_CURRENT, std::clamp(v / stamina_full() * top, 0.0f, top));
    }
    best_stamina[0] = best_stamina[1] = 0;
}

inline void keep_stamina() {
    for (int i = 0; i < 2; i++) {
        uintptr_t s = rdv<uintptr_t>(player + STAMINA_OBJECTS[i]);
        float cur = rdv<float>(s + STAMINA_CURRENT, NAN), full = rdv<float>(s + STAMINA_FULL, NAN);
        if (!s || !(cur >= 0 && cur < 100000) || !(full >= 0 && full < 100000)) continue;
        best_stamina[i] = std::max({best_stamina[i], cur, full});
        if (cur < best_stamina[i]) wr<float>(s + STAMINA_CURRENT, best_stamina[i]);
    }
}

inline std::vector<uintptr_t> param_containers() {
    std::vector<uintptr_t> out;
    for (uintptr_t c : {alive(player) ? rdv<uintptr_t>(player + PARAM_CONTAINER) : 0, rdv<uintptr_t>(params_root)})
        if (c && std::find(out.begin(), out.end(), c) == out.end()) out.push_back(c);
    return out;
}

inline uintptr_t param_value(uintptr_t container, const std::string& name) {
    auto it = param_ids.find(name);
    if (it == param_ids.end()) return 0;
    uintptr_t table = rdv<uintptr_t>(rdv<uintptr_t>(container + PARAM_TABLE));
    uintptr_t param = rdv<uintptr_t>(table + (it->second + 1) * 8);
    return param ? param + PARAM_VALUE : 0;
}

inline void put_bytes(uintptr_t at, const void* v, size_t n, std::set<uintptr_t>& touched) {
    if (!at || IsBadWritePtr((void*)at, n)) return;
    auto it = saved_bytes.find(at);
    if (it == saved_bytes.end()) {
        std::vector<uint8_t> b(n);
        if (!rd(at, b.data(), n)) return;
        it = saved_bytes.emplace(at, Saved{b, {}}).first;
    }
    it->second.written.assign((const uint8_t*)v, (const uint8_t*)v + n);
    touched.insert(at);
    memcpy((void*)at, v, n);
}

inline float original_float(uintptr_t at) {
    auto it = saved_bytes.find(at);
    float f;
    if (it == saved_bytes.end() || it->second.original.size() < 4) return rdv<float>(at, NAN);
    memcpy(&f, it->second.original.data(), 4);
    return f;
}

inline void set_param(const std::string& name, float value, bool is_switch, const std::vector<uintptr_t>& containers,
                      std::set<uintptr_t>& touched) {
    uint8_t yes = value != 0;
    for (uintptr_t c : containers) {
        uintptr_t at = param_value(c, name);
        if (is_switch) put_bytes(at, &yes, 1, touched);
        else put_bytes(at, &value, 4, touched);
    }
}

inline void set_desc_stat(uintptr_t desc, int stat, float v, std::set<uintptr_t>& touched) {
    auto& f = g.stats[stat];
    if (f.off < 0 || !desc) return;
    int n = (int)std::lround(v);
    put_bytes(desc + f.off, f.is_float ? (const void*)&v : (const void*)&n, 4, touched);
}

inline void item_overrides(std::set<uintptr_t>& touched) {
    bool uv = is_on("uv"), no_reload = is_on("no_reload");
    if (!uv && !no_reload) return;
    for (int i = 0; i < ITEM_COUNT; i++) {
        const ItemInfo& info = ITEMS[i];
        auto d = g.descs.find(info.id);
        if (d == g.descs.end()) continue;
        if (uv && info.st[ST_DepletionTime] < 1e6f) set_desc_stat(d->second, ST_DepletionTime, 1e7f, touched);
        if (no_reload && info.st[ST_AmmoCount] > 0) {
            set_desc_stat(d->second, ST_AmmoCount, 999, touched);
            if (info.st[ST_ReloadTime] > 0) set_desc_stat(d->second, ST_ReloadTime, 0.05f, touched);
        }
    }
}

inline void apply_cached(const std::map<std::string, std::pair<float, bool>>& want) {
    uint8_t kinds[PARAM_SLOTS] = {};
    for (auto& [name, w] : want) {
        auto it = param_ids.find(name);
        int id = it == param_ids.end() ? -1 : it->second + 1;
        if (id < 0 || id >= PARAM_SLOTS) continue;
        cached[id].value = w.first;
        kinds[id] = w.second ? CACHED_SWITCH : CACHED_FLOAT;
    }
    std::lock_guard<std::mutex> l(cached_mx);
    for (int id = 0; id < PARAM_SLOTS; id++) {
        if (cached[id].kind == kinds[id]) continue;
        cached[id].kind = kinds[id];
        if (!kinds[id]) cached[id].stale = !overridden_providers[id].empty();
    }
}

inline uintptr_t local_provider() {
    uintptr_t local = local_player_root ? rdv<uintptr_t>(rdv<uintptr_t>(local_player_root) + LOCAL_PLAYER) : 0;
    return alive(local) ? rdv<uintptr_t>(local + PARAM_PROVIDER) : 0;
}

inline uintptr_t cache_get_hook(uintptr_t provider, int id) {
    uintptr_t param = cache_get_original(provider, id);
    if (id < 0 || id >= PARAM_SLOTS || !param) return param;
    auto& c = cached[id];
    uint8_t kind = c.kind;
    bool mine = provider == player_provider.load();
    if (mine) c.reads++;
    if (!kind && !c.stale) return param;
    if (kind && !mine) mine = provider == local_provider();
    if (!kind || !mine) {
        {
            std::lock_guard<std::mutex> l(cached_mx);
            auto& providers = overridden_providers[id];
            if (!providers.erase(provider)) return param;
            if (providers.empty()) c.stale = false;
        }
        *(uint64_t*)(param + CACHED_VERSION) = ~0ull;
        return cache_get_original(provider, id);
    }
    uint8_t& flags = *(uint8_t*)(param + CACHED_FLAGS);
    if (!(flags & 1) || !*(uintptr_t*)param) {
        uintptr_t vt = kind == CACHED_SWITCH ? vt_param_bool : vt_param_float;
        if (!vt) return param;
        *(uintptr_t*)param = vt;
        flags |= 1;
    }
    if (kind == CACHED_SWITCH) *(uint8_t*)(param + CACHED_VALUE) = c.value != 0;
    else *(float*)(param + CACHED_VALUE) = c.value;
    std::lock_guard<std::mutex> l(cached_mx);
    overridden_providers[id].insert(provider);
    return param;
}

inline void absolute_jump(uint8_t* at, uintptr_t target) {
    const uint8_t jmp[6] = {0xFF, 0x25, 0, 0, 0, 0};
    memcpy(at, jmp, 6);
    memcpy(at + 6, &target, 8);
}

inline bool install_cache_hook() {
    const uint8_t prologue[15] = {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18};
    if (cache_get_original) return true;
    if (!cache_get_fn || memcmp((const void*)cache_get_fn, prologue, sizeof prologue)) return false;
    auto* trampoline = (uint8_t*)VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline) return false;
    memcpy(trampoline, prologue, sizeof prologue);
    absolute_jump(trampoline + sizeof prologue, cache_get_fn + sizeof prologue);
    cache_get_original = (CacheGetFn)trampoline;
    uint8_t patch[sizeof prologue];
    absolute_jump(patch, (uintptr_t)&cache_get_hook);
    patch[14] = 0x90;
    DWORD old;
    VirtualProtect((void*)cache_get_fn, sizeof patch, PAGE_EXECUTE_READWRITE, &old);
    memcpy((void*)cache_get_fn, patch, sizeof patch);
    VirtualProtect((void*)cache_get_fn, sizeof patch, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void*)cache_get_fn, sizeof patch);
    return true;
}

inline void write_code(uintptr_t at, const uint8_t* bytes, size_t n) {
    DWORD old;
    VirtualProtect((void*)at, n, PAGE_EXECUTE_READWRITE, &old);
    memcpy((void*)at, bytes, n);
    VirtualProtect((void*)at, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void*)at, n);
}

inline bool lockpick_patched() { return lockpick_patch && !memcmp((const void*)lockpick_patch, SPOT_DISTANCE_ZERO, 4); }

inline void patch_lockpick(bool on) {
    if (lockpick_patch && lockpick_patched() != on) write_code(lockpick_patch, on ? SPOT_DISTANCE_ZERO : SPOT_DISTANCE_CLAMP, 4);
}

inline const uint8_t POUND_EXPOSURE_TEST[4] = {0x40, 0x0F, 0x97, 0xC7}, POUND_ALWAYS_HITS[4] = {0x40, 0xB7, 0x01, 0x90};

inline bool pound_always_hits() { return pound_exposure_check && !memcmp((const void*)pound_exposure_check, POUND_ALWAYS_HITS, 4); }

inline void patch_pound(bool on) {
    if (pound_exposure_check && pound_always_hits() != on) write_code(pound_exposure_check, on ? POUND_ALWAYS_HITS : POUND_EXPOSURE_TEST, 4);
}

inline bool forced_damage_blocked() { return forced_damage_jump && rdv<uint8_t>(forced_damage_jump) == 0x90; }

inline void block_forced_damage(bool on) {
    if (!forced_damage_jump || forced_damage_blocked() == on) return;
    const uint8_t conditional[2] = {0x0F, 0x84}, always[2] = {0x90, 0xE9};
    write_code(forced_damage_jump, on ? always : conditional, 2);
}

inline const char* const BLAST_SPITS[] = {"Throwable_Control_The_Horde", "Throwable_Control_The_Horde_Upgraded", "Throwable_Control_The_Horde_Spit_Pound",
                                           "Throwable_LightDisable_Spit", "Throwable_LightDisableUpgraded_Spit", "Throwable_LightDisable_Spit_Pound",
                                           "ZZZZZ_Throwable_Camo_Spit", "ZZZZZ_Throwable_Camo_Spit_Pound"};
inline const char* const TOXIC_SPITS[] = {"ZZZZZ_Throwable_Toxic_Spit", "ZZZZZ_Throwable_Toxic_Spit_Pound_Inner", "ZZZZZ_Throwable_Toxic_Spit_Pound_Outer"};
const int TOXIC_SPLASH_MIN = 0x468, TOXIC_SPLASH_MAX = 0x46c;
const float TOXIC_SPLASH_LIMIT = 20.0f;
inline std::map<uintptr_t, float> spit_radius_originals;

inline void remember_radius(uintptr_t at, float expected) {
    float v = rdv<float>(at, NAN);
    if (!spit_radius_originals.count(at) && v > 0 && (std::isnan(expected) ? v <= TOXIC_SPLASH_LIMIT : v == expected)) spit_radius_originals[at] = v;
}

inline void apply_spit_radius(float factor) {
    const auto& range = g.stats[ST_DamageRange];
    if (factor != 1.0f && range.off >= 0 && range.is_float)
        for (const char* id : BLAST_SPITS)
            if (auto d = g.descs.find(id); d != g.descs.end()) remember_radius(d->second + range.off, game::lookup(id) ? game::lookup(id)->st[ST_DamageRange] : NAN);
    if (factor != 1.0f)
        for (const char* id : TOXIC_SPITS)
            if (auto d = g.descs.find(id); d != g.descs.end()) {
                float lo = rdv<float>(d->second + TOXIC_SPLASH_MIN, NAN), hi = rdv<float>(d->second + TOXIC_SPLASH_MAX, NAN);
                if (!(lo > 0 && hi >= lo && hi <= TOXIC_SPLASH_LIMIT)) continue;
                remember_radius(d->second + TOXIC_SPLASH_MIN, NAN);
                remember_radius(d->second + TOXIC_SPLASH_MAX, NAN);
            }
    for (auto& [at, original] : spit_radius_originals) {
        float want = original * factor;
        if (rdv<float>(at, NAN) != want) wr<float>(at, want);
    }
}

inline bool tweak_applies(const Tweak& t) {
    bool hunter_side = t.group == G_ZOMBIE || t.group == G_HUNTER;
    if (hunter_side) return side != SIDE_SURVIVOR;
    return t.group != G_HUMAN || side != SIDE_HUNTER;
}

inline bool float_value(uintptr_t at) {
    uintptr_t vt = at ? rdv<uintptr_t>(at - PARAM_VALUE) : 0;
    return vt && (vt == vt_param_float || vt == vt_player_float);
}

inline void apply_overrides() {
    auto containers = param_containers();
    if (containers != saved_containers) saved_bytes.clear(), saved_containers = containers;
    std::map<std::string, std::pair<float, bool>> want;
    for (auto& c : CHEATS) {
        if (!c.on) continue;
        for (auto& [name, v] : c.numbers) want[name] = {v, false};
        for (const char* name : c.switches) want[name] = {1, true};
        if (!strcmp(c.key, "z_spits"))
            for (const char* base : SPIT_REGEN)
                for (int players = 1; players <= 4; players++) want[base + std::to_string(players) + "v1"] = {0.05f, false};
    }
    for (auto& t : TWEAKS) {
        float factor = t.factor;
        if (factor == 1.0f || containers.empty() || !tweak_applies(t)) continue;
        for (const char* name : t.params) {
            float base = original_float(param_value(containers[0], name));
            if (std::isnan(base)) continue;
            auto it = want.find(name);
            float v = base * factor;
            if (it == want.end() || v > it->second.first) want[name] = {v, false};
        }
    }
    for (int type = 0; type < 8 && !containers.empty(); type++) {
        int extra = extra_skill_points[type];
        uintptr_t at = param_value(containers[0], SKILL_POINT_PARAMS[type]);
        if (!extra || !float_value(at)) continue;
        float base = original_float(at);
        if (!std::isnan(base)) want[SKILL_POINT_PARAMS[type]] = {std::max(0.0f, base + extra), false};
    }
    std::set<uintptr_t> touched;
    for (auto& [name, w] : want) set_param(name, w.first, w.second, containers, touched);
    apply_cached(want);
    if (is_on("ammo") && unlimited_ammo_flag) {
        uint8_t yes = 1;
        put_bytes(unlimited_ammo_flag, &yes, 1, touched);
    }
    item_overrides(touched);
    for (auto it = saved_bytes.begin(); it != saved_bytes.end();) {
        if (touched.count(it->first)) { ++it; continue; }
        auto& saved = it->second;
        std::vector<uint8_t> now(saved.written.size());
        bool still_ours = rd(it->first, now.data(), now.size()) && now == saved.written;
        if (still_ours && !IsBadWritePtr((void*)it->first, saved.original.size())) memcpy((void*)it->first, saved.original.data(), saved.original.size());
        it = saved_bytes.erase(it);
    }
}

inline float scaled_var_float(uintptr_t self, uintptr_t name, uintptr_t scope, uintptr_t extra) {
    float v = original_var_float(self, name, scope, extra);
    const char* s = name ? *(const char**)name : nullptr;
    if (!s || strncmp(s, "f_btz_", 6)) return v;
    float best = v;
    for (auto& t : TWEAKS)
        for (auto& var : t.vars)
            if (!strcmp(s, var.name) && t.factor != 1.0f && tweak_applies(t)) {
                float strength = std::clamp((t.factor - 1.0f) / (t.max - 1.0f), 0.0f, 1.0f);
                float moved = v + (var.at_max - v) * strength;
                if (std::fabs(moved) > std::fabs(best)) best = moved;
            }
    return best;
}

inline void tint(float* c, const float color[3], float scale) {
    float peak = std::max({color[0], color[1], color[2], 0.001f}), bright = std::max({c[0], c[1], c[2]}) * scale;
    for (int k = 0; k < 3; k++) c[k] = color[k] / peak * bright;
}

inline bool hunter_glow_var(const char* s) { return !strncmp(s, "v3_btz_zombie_v", 15) && !strstr(s, "not_drained"); }

inline float* tinted_var_vec3(uintptr_t self, float* out, uintptr_t name, uintptr_t scope, uintptr_t extra) {
    float* v = original_var_vec3(self, out, name, scope, extra);
    auto& glow = config::cfg.hunter_glow;
    const char* s = name ? *(const char**)name : nullptr;
    if (!v || !glow.on || !s || !hunter_glow_var(s)) return v;
    float c[3] = {v[0], v[1], v[2]};
    tint(c, glow.color, glow.glow);
    memcpy(out, c, sizeof c);
    return out;
}

inline bool var_hook_needed() {
    if (config::cfg.hunter_glow.on) return true;
    for (auto& t : TWEAKS)
        if (!t.vars.empty() && t.factor != 1.0f) return true;
    return false;
}

inline void install_var_hook() {
    uintptr_t object = rdv<uintptr_t>(var_root), current = rdv<uintptr_t>(object);
    uintptr_t ours = (uintptr_t)&var_vtable[1];
    if (!current || current == ours || !var_hook_needed()) return;
    for (size_t n = VAR_SLOTS + 1; n > SLOT_VAR_VEC3 + 1; n /= 2)
        if (rd(current - 8, var_vtable, n * 8)) {
            original_var_float = (VarFloatFn)var_vtable[1 + SLOT_VAR_FLOAT];
            original_var_vec3 = (VarVec3Fn)var_vtable[1 + SLOT_VAR_VEC3];
            var_vtable[1 + SLOT_VAR_FLOAT] = (uintptr_t)&scaled_var_float;
            var_vtable[1 + SLOT_VAR_VEC3] = (uintptr_t)&tinted_var_vec3;
            wr<uintptr_t>(object, ours);
            return;
        }
}

inline void all_off() {
    for (auto& c : CHEATS) c.on = false;
    for (auto& t : TWEAKS) t.factor = 1.0f;
}

inline config::Profile current_profile() {
    config::Profile p;
    for (auto& ch : CHEATS)
        if (ch.on) p.cheats.push_back(ch.key);
    for (auto& t : TWEAKS)
        if (t.factor != 1.0f) p.tweaks.push_back({t.key, t.factor});
    return p;
}

inline void apply_profile(const config::Profile& p) {
    all_off();
    for (auto& key : p.cheats)
        if (auto* c = find(key)) c->on = true;
    for (auto& [key, factor] : p.tweaks)
        if (auto* t = find_tweak(key)) t->factor = std::clamp(factor, 1.0f, t->max);
}

inline bool profile_active(const config::Profile& p) {
    for (auto& c : CHEATS)
        if (c.on != (std::find(p.cheats.begin(), p.cheats.end(), c.key) != p.cheats.end())) return false;
    for (auto& t : TWEAKS) {
        auto it = std::find_if(p.tweaks.begin(), p.tweaks.end(), [&](auto& kv) { return kv.first == t.key; });
        float want = it == p.tweaks.end() ? 1.0f : std::clamp(it->second, 1.0f, t.max);
        if (fabsf(t.factor - want) > 0.01f) return false;
    }
    return true;
}

enum PresetEsp { ESP_LEGIT, ESP_RAGE };

struct Preset {
    const char* key;
    const char* role;
    const char* style;
    const char* summary;
    std::vector<const char*> cheats;
    std::vector<std::pair<const char*, float>> tweaks;
    PresetEsp esp;
};

inline const Preset PRESETS[] = {
    {"survivor_legit", "Survivor", "Legit",
     "Looks like a sharp player. A third more reach on death from above and the dropkick, a little more melee reach, endless grappling hook, a human spit dodge, longer UV and a quiet ESP.",
     {"hook", "dodge_spit"},
     {{"uv_slow", 2.5f}, {"h_dfa", 3.5f}, {"h_dfa_pull", 3.5f}, {"h_dfa_height", 4.0f}, {"h_dropkick", 3.5f}, {"h_kicks", 1.5f}, {"h_melee", 1.3f}},
     ESP_LEGIT},
    {"survivor_rage", "Survivor", "Rage",
     "Nothing holds back. God mode, endless stamina, UV, ammo and supplies, one hit kills, spit dodge, double speed and jump, and every death from above, kick and melee range at Max.",
     {"god", "stamina", "hook", "uv", "no_fall", "ammo", "no_reload", "supplies", "durability", "one_hit", "dodge_spit"},
     {{"speed", 2.0f}, {"jump", 2.0f}, {"h_dfa", 10}, {"h_dfa_pull", 10}, {"h_dfa_height", 10}, {"h_dropkick", 10}, {"h_kicks", 10}, {"h_melee", 10}},
     ESP_RAGE},
    {"hunter_legit", "Night Hunter", "Legit",
     "Feels like a good hunter on a good day. Slightly longer pounce, tackle, claws and ground pound, a ground pound that does not miss, a wider spit hit and a quiet ESP. Energy and cooldowns stay normal.",
     {"z_pound_hits"},
     {{"z_pounce", 3.0f}, {"z_tackle", 2.0f}, {"z_claws", 1.4f}, {"z_pound", 1.5f}, {"z_spit", 1.8f}},
     ESP_LEGIT},
    {"hunter_rage", "Night Hunter", "Rage",
     "Unkillable and everywhere. Hunter god mode, endless energy and spits, no cooldowns, long camouflage, and pounce, tackle, claws, ground pound and spit hits at Max.",
     {"god", "z_energy", "z_cooldowns", "z_spits", "z_pound_hits", "z_camo"},
     {{"z_pounce", 10}, {"z_pound", 10}, {"z_tackle", 10}, {"z_claws", 10}, {"z_spit", 6}},
     ESP_RAGE},
};

inline config::Profile preset_profile(const Preset& p) {
    config::Profile out;
    for (const char* c : p.cheats) out.cheats.push_back(c);
    for (auto& [k, v] : p.tweaks) out.tweaks.push_back({k, v});
    return out;
}

inline bool preset_active(const Preset& p) {
    for (auto& c : CHEATS)
        if (c.on != (std::find_if(p.cheats.begin(), p.cheats.end(), [&](const char* k) { return !strcmp(k, c.key); }) != p.cheats.end())) return false;
    for (auto& t : TWEAKS) {
        auto it = std::find_if(p.tweaks.begin(), p.tweaks.end(), [&](auto& kv) { return !strcmp(kv.first, t.key); });
        float want = it == p.tweaks.end() ? 1.0f : std::clamp(it->second, 1.0f, t.max);
        if (fabsf(t.factor - want) > 0.01f) return false;
    }
    return true;
}

inline int active_count() {
    int n = 0;
    for (auto& c : CHEATS) n += c.on;
    for (auto& t : TWEAKS) n += t.factor != 1.0f;
    return n;
}

inline uintptr_t skill_container() {
    if (!alive(player)) return 0;
    uintptr_t own = rdv<uintptr_t>(player + PARAM_CONTAINER);
    return own ? own : rdv<uintptr_t>(params_root);
}

inline uintptr_t tree_record(int type) {
    uintptr_t trees = rdv<uintptr_t>(skill_container() + SKILL_TREES);
    return trees ? trees + type * TREE_RECORD : 0;
}
inline int tree_level(int type) { return tree_record(type) ? rdv<uint16_t>(tree_record(type) + TREE_LEVEL) : -1; }
inline int tree_max(int type) {
    int m = tree_record(type) ? rdv<uint16_t>(tree_record(type) + TREE_MAX) : 0;
    return m > 0 && m < 1000 ? m : 0;
}

inline std::map<int, uint32_t> xp_seen;
inline uintptr_t xp_seen_player = 0;

inline float skill_points(int type) {
    std::vector<uintptr_t> containers = param_containers();
    uintptr_t at = containers.empty() || type < 0 || type > 7 ? 0 : param_value(containers[0], SKILL_POINT_PARAMS[type]);
    return float_value(at) ? rdv<float>(at, NAN) : NAN;
}

inline uint32_t tree_xp(int type) { return tree_record(type) ? rdv<uint32_t>(tree_record(type) + TREE_XP) : 0; }
inline uint32_t tree_level_start(int type) { return tree_record(type) ? rdv<uint32_t>(tree_record(type) + TREE_LEVEL_START) : 0; }
inline uint32_t tree_span(int type) { return tree_record(type) ? rdv<uint32_t>(tree_record(type) + TREE_SPAN) : 0; }

inline void set_tree_progress(int type, float frac) {
    uintptr_t r = tree_record(type);
    if (!r || tree_level(type) >= tree_max(type) || !tree_span(type)) return;
    uint32_t span = tree_span(type);
    uint32_t xp = tree_level_start(type) + std::min(span - 1, (uint32_t)(std::clamp(frac, 0.0f, 1.0f) * span));
    logf_hook("skills: tree %d xp %u -> %u", type, rdv<uint32_t>(r + TREE_XP), xp);
    wr<uint32_t>(r + TREE_XP, xp);
    xp_seen.clear();
}

inline float tree_progress(int type) {
    uintptr_t r = tree_record(type);
    if (!r || tree_level(type) >= tree_max(type)) return 1;
    uint32_t start = rdv<uint32_t>(r + TREE_LEVEL_START), span = rdv<uint32_t>(r + TREE_SPAN), xp = rdv<uint32_t>(r + TREE_XP);
    return span ? std::clamp((float)((double)xp - start) / span, 0.0f, 1.0f) : 0;
}

inline uint8_t* xp_stub = nullptr;
const int XP_STUB_FACTOR = 0x100, XP_STUB_SCALE = 0x108, XP_STUB_COUNTS = 0x110, XP_SITE_BYTES = 16;
const int64_t XP_SCALE = 1000;
inline uint32_t xp_counts_seen[8] = {};

inline uint8_t* build_xp_stub(uintptr_t site) {
    auto* stub = (uint8_t*)VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!stub) return nullptr;
    std::vector<uint8_t> c;
    auto bytes = [&](std::initializer_list<uint8_t> b) { c.insert(c.end(), b); };
    auto rip_to = [&](int target) {
        int32_t d = target - (int)(c.size() + 4);
        for (int i = 0; i < 4; i++) c.push_back((uint8_t)(d >> (i * 8)));
    };
    auto absolute_jump = [&](uintptr_t to) {
        bytes({0xFF, 0x25, 0, 0, 0, 0});
        for (int i = 0; i < 8; i++) c.push_back((uint8_t)(to >> (i * 8)));
    };
    bytes({0x50, 0x52, 0x49, 0x63, 0xC7, 0x48, 0x0F, 0xAF, 0x05});
    rip_to(XP_STUB_FACTOR);
    bytes({0x48, 0x99, 0x48, 0xF7, 0x3D});
    rip_to(XP_STUB_SCALE);
    bytes({0x41, 0x89, 0xC7, 0x4C, 0x89, 0xF0, 0x83, 0xE0, 0x07, 0x48, 0x8D, 0x15});
    rip_to(XP_STUB_COUNTS);
    bytes({0xF0, 0xFF, 0x04, 0x82, 0x5A, 0x58, 0x45, 0x85, 0xFF, 0x7F, 0x0E});
    absolute_jump(site + 9 + rdv<int32_t>(site + 5));
    for (int i = 9; i < XP_SITE_BYTES; i++) c.push_back(rdv<uint8_t>(site + i));
    absolute_jump(site + XP_SITE_BYTES);
    memcpy(stub, c.data(), c.size());
    *(int64_t*)(stub + XP_STUB_FACTOR) = XP_SCALE;
    *(int64_t*)(stub + XP_STUB_SCALE) = XP_SCALE;
    return stub;
}

inline bool install_xp_hook() {
    if (xp_stub) return true;
    if (!xp_award_site || rdv<uint32_t>(xp_award_site) != 0x0FFF8545) return false;
    uint8_t* stub = build_xp_stub(xp_award_site);
    if (!stub) return false;
    uint8_t jump[XP_SITE_BYTES] = {0xFF, 0x25, 0, 0, 0, 0};
    memcpy(jump + 6, &stub, 8);
    jump[14] = jump[15] = 0x90;
    DWORD old;
    if (!VirtualProtect((void*)xp_award_site, XP_SITE_BYTES, PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy((void*)xp_award_site, jump, XP_SITE_BYTES);
    VirtualProtect((void*)xp_award_site, XP_SITE_BYTES, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void*)xp_award_site, XP_SITE_BYTES);
    xp_stub = stub;
    return true;
}

inline void set_xp_factor(float factor) {
    if (factor != 1.0f) install_xp_hook();
    if (xp_stub) *(volatile int64_t*)(xp_stub + XP_STUB_FACTOR) = (int64_t)llroundf(factor * XP_SCALE);
}

inline bool xp_awarded_by_hook(int type) {
    if (!xp_stub || type < 0 || type > 7) return false;
    uint32_t now = ((volatile uint32_t*)(xp_stub + XP_STUB_COUNTS))[type];
    bool changed = now != xp_counts_seen[type];
    xp_counts_seen[type] = now;
    return changed;
}

inline void level_up_with_xp(int type) {
    uintptr_t container = skill_container(), r = tree_record(type);
    if (!container || !r || !level_from_xp_fn || tree_level(type) >= tree_max(type)) return;
    uint32_t next = rdv<uint32_t>(r + TREE_LEVEL_START) + rdv<uint32_t>(r + TREE_SPAN);
    if (rdv<uint32_t>(r + TREE_XP) < next) wr<uint32_t>(r + TREE_XP, next);
    ((void(__fastcall*)(uintptr_t, int))level_from_xp_fn)(container, type);
    xp_seen.clear();
}

const int TREES_CHANGED_ON_LOAD = 3;

inline void boost_xp(float factor) {
    uintptr_t container = skill_container();
    if (factor <= 1.0f || !container || !level_from_xp_fn || player != xp_seen_player) xp_seen.clear();
    xp_seen_player = player;
    if (factor <= 1.0f || !container || !level_from_xp_fn) return;
    std::map<int, uint32_t> now;
    for (auto& t : TREES)
        if (tree_max(t.type)) now[t.type] = rdv<uint32_t>(tree_record(t.type) + TREE_XP);
    int gained = 0;
    std::set<int> hooked;
    for (auto& [type, xp] : now) {
        gained += xp_seen.count(type) && xp > xp_seen[type];
        if (xp_awarded_by_hook(type)) hooked.insert(type);
    }
    if (gained < TREES_CHANGED_ON_LOAD)
        for (auto& [type, xp] : now) {
            if (!xp_seen.count(type) || xp <= xp_seen[type] || hooked.count(type) || tree_level(type) >= tree_max(type)) continue;
            double boosted = xp_seen[type] + (double)(xp - xp_seen[type]) * factor;
            xp = (uint32_t)std::min(boosted, (double)INT32_MAX);
            wr<uint32_t>(tree_record(type) + TREE_XP, xp);
            ((void(__fastcall*)(uintptr_t, int))level_from_xp_fn)(container, type);
            xp = rdv<uint32_t>(tree_record(type) + TREE_XP);
        }
    xp_seen.swap(now);
}

inline void set_tree_level(int type, int level) {
    uintptr_t container = skill_container();
    if (!container || !set_level_fn || !tree_max(type)) return;
    level = std::clamp(level, 0, tree_max(type));
    logf_hook("skills: tree %d level %d -> %d", type, tree_level(type), level);
    ((void(__fastcall*)(uintptr_t, int16_t, int))set_level_fn)(container, (int16_t)level, type);
    xp_seen.clear();
}

const int SKILL_NAME = 0x08, SKILL_MAX_LEVEL = 0x1c, SKILL_LIST = 0x30, SKILL_COUNT = 0x38, LEARNED_SKILLS = 0xd8;
const int NODE_KEY = -0x10, NODE_LEVEL = -0xa, NODE_LEFT = 0, NODE_RIGHT = 0x10, MAX_GAME_SKILLS = 2048;
inline std::vector<int> skill_slots;
inline std::mutex skills_mx;
inline std::vector<int> skill_levels;

inline int game_skill_count() {
    int n = skill_manager ? rdv<int>(skill_manager + SKILL_COUNT) : 0;
    return n > 0 && n < MAX_GAME_SKILLS ? n : 0;
}

inline uintptr_t game_skill(int index) { return rdv<uintptr_t>(rdv<uintptr_t>(skill_manager + SKILL_LIST) + index * 8); }

inline void map_skills() {
    int n = game_skill_count();
    if (!n || skill_slots.size() == std::size(SKILLS)) return;
    std::map<std::string, int> by_name;
    for (int i = 0; i < n; i++) {
        char name[64] = {};
        if (rd(rdv<uintptr_t>(game_skill(i) + SKILL_NAME), name, sizeof name - 1)) by_name[name] = i;
    }
    std::vector<int> slots;
    for (auto& s : SKILLS) {
        auto it = by_name.find(s.id);
        slots.push_back(it == by_name.end() ? -1 : it->second);
    }
    skill_slots = slots;
}

inline std::map<int, int> learned_skills(uintptr_t container) {
    std::map<int, int> out;
    std::vector<uintptr_t> todo{rdv<uintptr_t>(container + LEARNED_SKILLS)};
    while (!todo.empty() && out.size() < MAX_GAME_SKILLS) {
        uintptr_t node = todo.back();
        todo.pop_back();
        if (node < 0x10000) continue;
        int key = rdv<int16_t>(node + NODE_KEY, -1), level = rdv<int16_t>(node + NODE_LEVEL);
        if (key < 0 || out.count(key)) continue;
        out[key] = level;
        todo.push_back(rdv<uintptr_t>(node + NODE_LEFT));
        todo.push_back(rdv<uintptr_t>(node + NODE_RIGHT));
    }
    return out;
}

inline void read_skill_levels() {
    map_skills();
    uintptr_t container = skill_container();
    std::vector<int> levels(std::size(SKILLS), -1);
    if (container && skill_slots.size() == std::size(SKILLS)) {
        auto learned = learned_skills(container);
        for (size_t i = 0; i < levels.size(); i++)
            if (skill_slots[i] >= 0) levels[i] = learned.count(skill_slots[i]) ? learned[skill_slots[i]] : 0;
    }
    std::lock_guard<std::mutex> l(skills_mx);
    skill_levels.swap(levels);
}

inline std::vector<int> skill_levels_now() {
    std::lock_guard<std::mutex> l(skills_mx);
    return skill_levels;
}

inline int skill_index(const char* id) {
    for (size_t i = 0; i < std::size(SKILLS); i++)
        if (!strcmp(SKILLS[i].id, id)) return (int)i;
    return -1;
}

inline int skill_level(int i) {
    uintptr_t container = skill_container();
    if (!container || i < 0 || (size_t)i >= skill_slots.size() || skill_slots[i] < 0) return -1;
    auto learned = learned_skills(container);
    auto it = learned.find(skill_slots[i]);
    return it == learned.end() ? 0 : it->second;
}

inline void set_skill_level(int i, int level) {
    uintptr_t container = skill_container();
    int now = skill_level(i);
    if (now < 0 || !add_skill_fn || !remove_skill_fn) return;
    uint16_t slot = (uint16_t)skill_slots[i];
    int max = rdv<uint16_t>(game_skill(slot) + SKILL_MAX_LEVEL);
    level = std::clamp(level, 0, std::max(1, max));
    if (level == now) return;
    logf_hook("skills: %s level %d -> %d", SKILLS[i].id, now, level);
    if (level < now) ((void(__fastcall*)(uintptr_t, uint16_t))remove_skill_fn)(container, slot), now = 0;
    using AddSkill = int(__fastcall*)(uintptr_t, uint16_t, int, bool, bool);
    for (; now < level; now++) ((AddSkill)add_skill_fn)(container, slot, 0, false, true);
}

inline void learn_skill(int i, int depth = 0) {
    if (i < 0 || depth > 16) return;
    for (const char* need : SKILLS[i].needs)
        if (need && skill_level(skill_index(need)) == 0) learn_skill(skill_index(need), depth + 1);
    if (skill_level(i) == 0) set_skill_level(i, 1);
}

inline void forget_skill(int i, int depth = 0) {
    if (i < 0 || depth > 16) return;
    for (size_t k = 0; k < std::size(SKILLS); k++)
        for (const char* need : SKILLS[k].needs)
            if (need && !strcmp(need, SKILLS[i].id) && skill_level((int)k) > 0) forget_skill((int)k, depth + 1);
    set_skill_level(i, 0);
}

inline int spent_points(int tree, const std::vector<int>& levels) {
    int spent = 0;
    for (size_t i = 0; i < std::size(SKILLS) && i < levels.size(); i++)
        if (SKILLS[i].tree == tree && levels[i] > 0) spent += SKILLS[i].cost * levels[i];
    return spent;
}

inline void set_whole_tree(int tree, bool learned) {
    for (size_t i = 0; i < std::size(SKILLS) && skill_slots.size() == std::size(SKILLS); i++)
        if (SKILLS[i].tree == tree && skill_slots[i] >= 0) {
            if (!learned) set_skill_level((int)i, 0);
            else set_skill_level((int)i, std::max(1, (int)rdv<uint16_t>(game_skill(skill_slots[i]) + SKILL_MAX_LEVEL)));
        }
}

inline bool is_health_module(uintptr_t m) {
    uintptr_t vt = rdv<uintptr_t>(m);
    return vt && (vt == g.vt_health[0] || vt == g.vt_health[1] || vt == g.vt_health[2]);
}

inline bool is_enemy_module(uintptr_t m) {
    uintptr_t owner = rdv<uintptr_t>(m + MODULE_OWNER);
    return is_health_module(m) && owner && owner != player && g.vt_human &&
           slot(owner, SLOT_HEALTH) == rdv<uintptr_t>(g.vt_human + SLOT_HEALTH * 8);
}

inline bool owned_by_nest(uintptr_t owner) {
    if (!vt_nest_logic) return false;
    for (int off = 0; off < AI_FIELDS; off += 8) {
        uintptr_t field = rdv<uintptr_t>(owner + off);
        if (field > 0x10000 && rdv<uintptr_t>(field) == vt_nest_logic) return true;
    }
    return false;
}

inline bool is_nest_module(uintptr_t m) {
    {
        std::lock_guard<std::mutex> l(modules_mx);
        auto it = nest_modules.find(m);
        if (it != nest_modules.end()) return it->second;
    }
    bool nest = owned_by_nest(rdv<uintptr_t>(m + MODULE_OWNER));
    std::lock_guard<std::mutex> l(modules_mx);
    nest_modules[m] = nest;
    return nest;
}

inline void keep_enemy_modules(const std::vector<std::vector<uintptr_t>>& lists) {
    std::vector<uintptr_t> found;
    std::map<uintptr_t, bool> nests;
    for (auto& list : lists)
        for (uintptr_t m : list)
            if (is_enemy_module(m)) {
                found.push_back(m);
                nests[m] = owned_by_nest(rdv<uintptr_t>(m + MODULE_OWNER));
            }
    std::lock_guard<std::mutex> l(modules_mx);
    enemy_modules.swap(found);
    nest_modules.swap(nests);
}

inline void scan_enemies() { keep_enemy_modules(game::scan({std::begin(g.vt_health), std::end(g.vt_health)})); }

inline void weaken_enemies() {
    std::lock_guard<std::mutex> l(modules_mx);
    for (uintptr_t m : enemy_modules) {
        float cur = rdv<float>(m + MODULE_HEALTH, NAN);
        if (is_enemy_module(m) && !nest_modules[m] && cur > ONE_HIT_HEALTH && cur < 1e7f) wr<float>(m + MODULE_HEALTH, ONE_HIT_HEALTH);
    }
}

inline bool is_ammo(const game::Item& it) { return it.info && strstr(it.info->id, "Ammo"); }

inline std::set<uintptr_t> items_present(uintptr_t inv) {
    uintptr_t arr = rdv<uintptr_t>(inv + 0x40);
    uint32_t n = rdv<uint32_t>(inv + 0x48);
    std::set<uintptr_t> present;
    for (uint32_t i = 0; i < n && i < 4000; i++) present.insert(rdv<uintptr_t>(arr + i * 8));
    return present;
}

inline void keep_stacks(bool ammo, bool supplies) {
    if (!ammo && !supplies) return stack_floor.clear();
    for (auto& inv : g.invs) {
        if (inv.kind == game::K_STASH) continue;
        auto present = items_present(inv.obj);
        for (auto& it : inv.items) {
            bool wanted = is_ammo(it) ? ammo : supplies;
            if (!wanted || !it.info || !(it.info->st[ST_MaxStackCount] > 1) || !present.count(it.addr)) continue;
            int cur = game::count(it);
            auto f = stack_floor.find(it.addr);
            if (f == stack_floor.end()) f = stack_floor.emplace(it.addr, std::max(cur, 2)).first;
            else if (cur > f->second) f->second = cur;
            if (cur < f->second) game::set_count(it, f->second);
        }
    }
}

inline void keep_durability(bool on) {
    if (!on) return condition_floor.clear();
    std::map<uintptr_t, float> kept;
    for (auto& inv : g.invs) {
        if (inv.kind != game::K_BACKPACK) continue;
        for (uintptr_t addr : items_present(inv.obj)) {
            const ItemInfo* info = game::resolve(addr, g.rule);
            if (!info || !(info->st[ST_Condition] > 0)) continue;
            float cur = rdv<float>(addr + ITEM_CONDITION, NAN);
            if (!(cur > 0)) continue;
            auto f = condition_floor.find(addr);
            float floor = f == condition_floor.end() ? cur : std::max(cur, f->second);
            if (cur < floor) wr<float>(addr + ITEM_CONDITION, floor);
            kept[addr] = floor;
        }
    }
    condition_floor.swap(kept);
}

extern "C" {
uintptr_t fatrainer_update_original[3] = {};
void fatrainer_update_hook0();
void fatrainer_update_hook1();
void fatrainer_update_hook2();
}

inline void install_update_hooks() {
    void (*hooks[3])() = {fatrainer_update_hook0, fatrainer_update_hook1, fatrainer_update_hook2};
    for (int i = 0; i < 3; i++) {
        if (!g.vt_health[i]) continue;
        auto at = (uintptr_t*)(g.vt_health[i] + SLOT_MODULE_UPDATE * 8);
        if (*at == (uintptr_t)hooks[i] || !in_game_module(*at)) continue;
        fatrainer_update_original[i] = *at;
        DWORD old;
        VirtualProtect(at, 8, PAGE_READWRITE, &old);
        *at = (uintptr_t)hooks[i];
        VirtualProtect(at, 8, old, &old);
    }
}

struct Vec3 { float x, y, z; };
using GetPositionFn = Vec3* (*)(uintptr_t, Vec3*);
using SetPositionFn = void (*)(uintptr_t, const Vec3*);
using FieldIntFn = uintptr_t (*)(uintptr_t, const char*, int*);
using FieldBoolFn = uintptr_t (*)(uintptr_t, const char*, bool*);
using RequestOwnershipFn = void (*)(uintptr_t);
inline RequestOwnershipFn request_ownership = nullptr;
inline GetPositionFn get_position = nullptr;
inline SetPositionFn set_position = nullptr;
inline FieldIntFn field_int = nullptr, field_enum = nullptr;
inline FieldBoolFn field_bool = nullptr;
inline uintptr_t float_field_editor = 0;

using EngineGetterFn = uintptr_t (*)(uintptr_t);
inline EngineGetterFn active_level = nullptr, view_camera = nullptr;
struct EngineVector { uintptr_t* data; uint32_t size, capacity; };
using FindInRadiusFn = bool (*)(uintptr_t, EngineVector*, const Vec3*, float, uintptr_t, bool, int*);
inline FindInRadiusFn find_in_radius = nullptr;

inline void locate_engine(HMODULE engine) {
    if (!engine) return;
    active_level = (EngineGetterFn)GetProcAddress(engine, "?GetActiveLevel@IGame@@QEAAPEAVILevel@@XZ");
    view_camera = (EngineGetterFn)GetProcAddress(engine, "?GetFirstActiveViewCamera@ILevel@@QEAAPEAVIBaseCamera@@XZ");
    find_in_radius = (FindInRadiusFn)GetProcAddress(
        engine, "?FindObjectsInRadius@ILevel@@QEAA_NPEAV?$vector@PEAVIControlObject@@@ttl@@AEBVvec3@@MPEBVCRTTI@@_NPEAH@Z");
    get_position = (GetPositionFn)GetProcAddress(engine, "?GetWorldPosition@IControlObject@@QEBA?AVvec3@@XZ");
    set_position = (SetPositionFn)GetProcAddress(engine, "?SetWorldPosition@IControlObject@@QEAAXAEBVvec3@@@Z");
    field_int = (FieldIntFn)GetProcAddress(engine, "?GetFieldInt@CRTTIObject@@QEBAPEBVCRTTIFieldInt@@PEBDAEAH@Z");
    field_enum = (FieldIntFn)GetProcAddress(engine, "?GetFieldEnum@CRTTIObject@@QEBAPEBVCRTTIFieldEnum@@PEBDAEAH@Z");
    field_bool = (FieldBoolFn)GetProcAddress(engine, "?GetFieldBool@CRTTIObject@@QEBAPEBVCRTTIFieldBool@@PEBDAEA_N@Z");
    request_ownership = (RequestOwnershipFn)GetProcAddress(engine, "?RequestOwnership@IGSObject@@QEAAXXZ");
    float_field_editor = (uintptr_t)GetProcAddress(engine, "?GetFieldFloatEditor@CRTTIObject@@UEBAPEBVCRTTIFieldFloat@@PEBDAEAM@Z");
}

const int CONTROL_ENTITY = 0x8, ENTITY_NODE = 0xe8, NODE_FLAGS = 0x28;
inline bool position_of(uintptr_t control, Vec3* out) {
    *out = {NAN, NAN, NAN};
    uintptr_t entity = rdv<uintptr_t>(control + CONTROL_ENTITY), node = rdv<uintptr_t>(entity + ENTITY_NODE);
    uint32_t flags;
    if (!get_position || !node || !rd(node + NODE_FLAGS, &flags, sizeof flags)) return false;
    get_position(control, out);
    return std::isfinite(out->x);
}

struct Section { uintptr_t sensor; int type; int stage; bool last; Vec3 pos; };
enum { SENSOR_START = 1, SENSOR_STAGE = 2, SENSOR_REWARD = 3, SENSOR_EVAC = 4 };
inline std::vector<Section> prison_sections;
inline Vec3 saved_position{};
inline bool has_saved_position = false;
inline DWORD last_prison_tick = 0;

inline uintptr_t follow_jump(uintptr_t fn) {
    uint8_t op[2];
    if (!rd(fn, op, 2) || op[0] != 0xFF || op[1] != 0x25) return fn;
    return rdv<uintptr_t>(fn + 6 + rdv<int32_t>(fn + 2));
}

inline bool reflected(uintptr_t object) { return float_field_editor && follow_jump(slot(object, SLOT_FLOAT_FIELD_EDITOR)) == float_field_editor; }

inline std::atomic<bool> sections_wanted{false};
inline int next_section = 0;

inline std::pair<int, int> run_order(const Section& s) {
    bool known = s.type >= SENSOR_START && s.type <= SENSOR_EVAC;
    return {known ? s.type : SENSOR_EVAC + 1, s.stage};
}

inline void read_sections() {
    if (!alive(player)) return;
    std::vector<Section> out;
    for (uintptr_t s : g.prison_sensors) {
        if (!get_position || rdv<uintptr_t>(s) != g.vt_prison_sensor) continue;
        Section sec{s, 0, -1, false, {NAN, NAN, NAN}};
        if (g.sensor_control < 0) continue;
        position_of(s + g.sensor_control, &sec.pos);
        uintptr_t r = g.sensor_rtti < 0 ? 0 : s + g.sensor_rtti;
        if (r && reflected(r) && field_int && field_bool && field_enum) {
            int v = 0;
            bool b = false;
            if (field_int(r, "m_Stage", &v)) sec.stage = v;
            if (field_bool(r, "m_IsLastStage", &b)) sec.last = b;
            if (field_enum(r, "m_SensorType", &v)) sec.type = v;
        }
        auto same = [&](const Section& o) {
            return o.type == sec.type && o.stage == sec.stage && std::fabs(o.pos.x - sec.pos.x) + std::fabs(o.pos.y - sec.pos.y) + std::fabs(o.pos.z - sec.pos.z) < 1;
        };
        if (std::isfinite(sec.pos.x) && std::none_of(out.begin(), out.end(), same)) out.push_back(sec);
    }
    std::sort(out.begin(), out.end(), [](auto& a, auto& b) { return run_order(a) < run_order(b); });
    prison_sections = out;
    for (auto& sec : out)
        logf_hook("section: type %d stage %d last %d at %.1f %.1f %.1f", sec.type, sec.stage, sec.last, sec.pos.x, sec.pos.y, sec.pos.z);
}

inline Vec3 player_position() {
    Vec3 p{NAN, NAN, NAN};
    if (alive(player) && g.player_control >= 0) position_of(player + g.player_control, &p);
    return p;
}

inline const uint8_t PHYSICS_POSITION_START[12] = {0x48, 0x89, 0x5C, 0x24, 0x10, 0x56, 0x48, 0x83, 0xEC, 0x40, 0x8B, 0x02};

inline bool move_body(Vec3 to) {
    uintptr_t fn = slot(player, SLOT_PHYSICS_POSITION);
    uint8_t start[sizeof PHYSICS_POSITION_START];
    if (!in_game_module(fn) || !rd(fn, start, sizeof start) || memcmp(start, PHYSICS_POSITION_START, sizeof start)) return false;
    ((void (*)(uintptr_t, const Vec3*))fn)(player, &to);
    return true;
}

inline void teleport(Vec3 to, bool quiet = false) {
    if (!alive(player) || !set_position || g.player_control < 0 || !std::isfinite(to.x)) return;
    Vec3 from = player_position();
    bool body = move_body(to);
    set_position(player + g.player_control, &to);
    Vec3 now = player_position();
    if (!quiet) logf_hook("teleport: from %.1f %.1f %.1f to %.1f %.1f %.1f, now %.1f %.1f %.1f, body %s", from.x, from.y, from.z, to.x, to.y, to.z, now.x, now.y,
              now.z, body ? "moved" : "missing");
}

struct RoutePoint { Vec3 pos; bool stop; };
enum RouteMode { ROUTE_IDLE, ROUTE_RECORDING, ROUTE_REPLAYING, ROUTE_PAUSED };
const float ROUTE_STEP = 2.0f;
const DWORD ROUTE_STOP_AFTER = 4000;
inline std::vector<RoutePoint> route;
inline std::atomic<int> route_mode{ROUTE_IDLE};
inline size_t route_at = 0;
inline DWORD route_still_since = 0;

inline float distance(Vec3 a, Vec3 b) { return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z)); }

inline void record_route(Vec3 here, DWORD now) {
    if (!std::isfinite(here.x)) return;
    if (route.empty() || distance(route.back().pos, here) >= ROUTE_STEP) {
        route.push_back({here, false});
        route_still_since = now;
    } else if (now - route_still_since >= ROUTE_STOP_AFTER) {
        route.back().stop = true;
    }
}

inline void replay_route() {
    if (route_at >= route.size()) {
        route_mode = ROUTE_IDLE;
        route_at = 0;
        return;
    }
    teleport(route[route_at].pos, true);
    if (route[route_at++].stop) route_mode = ROUTE_PAUSED;
}

inline uintptr_t game_profile() { return profile_root ? rdv<uintptr_t>(rdv<uintptr_t>(profile_root) + GAME_PROFILE) : 0; }

const int RANK_TITLES = 11, RANK_WINS_PER_TITLE = 3;
const float RANK_TITLE_GROWTH = 1.3f;

inline int rank_start(int title) {
    int edge = 0;
    for (int i = 0; i < title && i < RANK_TITLES; i++) edge = (int)(powf(RANK_TITLE_GROWTH, (float)i) * (RANK_WINS_PER_TITLE * 100) + (float)edge);
    return edge;
}

inline int rank_title(int points) {
    for (int i = 0; i < RANK_TITLES; i++)
        if (points < rank_start(i + 1)) return i;
    return RANK_TITLES;
}

inline int pvp_rank(bool zombie) {
    uintptr_t p = game_profile();
    return p ? rdv<int>(p + (zombie ? PROFILE_RANK_ZOMBIE : PROFILE_RANK_HUMAN), -1) : -1;
}

inline void set_pvp_rank(bool zombie, int rank) {
    uintptr_t p = game_profile();
    if (!p) return;
    rank = std::max(rank, 0);
    int old = pvp_rank(zombie), mirrored = 0;
    wr<int>(p + (zombie ? PROFILE_RANK_ZOMBIE : PROFILE_RANK_HUMAN), rank);
    int field = zombie ? PLAYER_RANK_ZOMBIE : PLAYER_RANK_HUMAN;
    for (uintptr_t lp : g.logical_players) {
        if (rdv<uintptr_t>(lp) != g.vt_logical_player || rdv<int>(lp + field, INT32_MIN) != old) continue;
        if (!rdv<uint8_t>(lp + REPL_OWNED) && request_ownership) request_ownership(lp);
        wr<int>(lp + field, rank);
        mirrored++;
    }
    logf_hook("pvp rank %s: %d -> %d (%d player copies)", zombie ? "night hunter" : "survivor", old, rank, mirrored);
}

inline void set_pvp_title(bool zombie, int title) { set_pvp_rank(zombie, rank_start(std::clamp(title, 0, RANK_TITLES))); }

inline int kill_enemies(float radius) {
    Vec3 me = player_position();
    if (!std::isfinite(me.x) || !get_position || g.human_control < 0) return 0;
    int killed = 0;
    std::lock_guard<std::mutex> l(modules_mx);
    for (uintptr_t m : enemy_modules) {
        if (!is_enemy_module(m) || !(rdv<float>(m + MODULE_HEALTH, NAN) > 0)) continue;
        uintptr_t owner = rdv<uintptr_t>(m + MODULE_OWNER), kill = slot(owner, SLOT_KILL);
        Vec3 at;
        if (!in_game_module(kill) || !position_of(owner + g.human_control, &at) || !(distance(me, at) <= radius)) continue;
        ((void (*)(uintptr_t))kill)(owner);
        killed++;
    }
    return killed;
}

inline void route_tick() {
    if (!alive(player)) return;
    if (route_mode == ROUTE_RECORDING) record_route(player_position(), GetTickCount());
    else if (route_mode == ROUTE_REPLAYING) replay_route();
}

inline bool save_route(const std::string& path) {
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return false;
    for (auto& p : route) fprintf(f, "%.2f %.2f %.2f %d\n", p.pos.x, p.pos.y, p.pos.z, (int)p.stop);
    fclose(f);
    return true;
}

inline bool load_route(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return false;
    std::vector<RoutePoint> out;
    RoutePoint p;
    int stop;
    while (fscanf(f, "%f %f %f %d", &p.pos.x, &p.pos.y, &p.pos.z, &stop) == 4) {
        p.stop = stop != 0;
        out.push_back(p);
    }
    fclose(f);
    route = out;
    return true;
}

inline int pause_prison() {
    DWORD now = GetTickCount();
    float dt = last_prison_tick ? (now - last_prison_tick) / 1000.0f : 0;
    last_prison_tick = is_on("prison_pause") ? now : 0;
    int live = 0;
    for (uintptr_t d : g.prison_data) {
        if (rdv<uintptr_t>(d) != g.vt_prison_data) continue;
        live++;
        if (!last_prison_tick || dt <= 0 || dt > 1) continue;
        if (!rdv<uint8_t>(d + REPL_OWNED) && request_ownership) request_ownership(d);
        float start = rdv<float>(d + PRISON_START_TIME, NAN), end = rdv<float>(d + PRISON_END_TIME, NAN);
        if (start > 0) wr<float>(d + PRISON_START_TIME, start + dt);
        if (end > 0) wr<float>(d + PRISON_END_TIME, end + dt);
    }
    return live;
}

inline std::map<uintptr_t, float> uv_last_charge;

inline int slow_uv_drain(float factor) {
    int live = 0;
    for (uintptr_t e : g.equipment) {
        if (rdv<uintptr_t>(e) != g.vt_equipment) continue;
        live++;
        float charge = rdv<float>(e + UV_CHARGE, NAN);
        if (!(charge >= 0 && charge <= 1)) continue;
        auto last = uv_last_charge.find(e);
        if (last != uv_last_charge.end() && charge < last->second) {
            charge = last->second - (last->second - charge) / factor;
            wr<float>(e + UV_CHARGE, charge);
        }
        uv_last_charge[e] = charge;
    }
    return live;
}

const int LOGICAL_PLAYER = 0x9d8, PLAYER_TEAM = 0x6dc, PLAYER_ROLE = 0x6e0, ROLE_HUNTER = 2, PLAYER_RAGE = 0x738;
const int RANK_SURVIVOR = 0x74c, RANK_HUNTER = 0x750, CAMERA_ENGINE = 8, CAMERA_COMBINED = 0xb0;
const float PLAYER_HEIGHT = 1.8f;

struct EspTarget { Vec3 feet; float health, max_health, distance, rage; bool hunter, ally; int rank; uintptr_t p; double seen; };
inline std::mutex esp_mx;
inline std::vector<uintptr_t> esp_players;

inline void keep_players(const std::vector<uintptr_t>& list) {
    std::vector<uintptr_t> found;
    for (uintptr_t p : list)
        if (alive(p) && in_game_module(rdv<uintptr_t>(p + HEALTH_OBJECT))) found.push_back(p);
    std::lock_guard<std::mutex> l(esp_mx);
    esp_players.swap(found);
}

inline void scan_targets(bool enemies, bool players) {
    if (!enemies && !players) return;
    std::vector<uintptr_t> vts(std::begin(g.vt_health), std::end(g.vt_health));
    if (!enemies) std::fill(vts.begin(), vts.end(), 0);
    vts.push_back(players ? g.vt_player : 0);
    vts.push_back(players ? g.vt_tutorial_player : 0);
    auto found = game::scan(vts);
    if (enemies) keep_enemy_modules({found[0], found[1], found[2]});
    if (players) {
        found[3].insert(found[3].end(), found[4].begin(), found[4].end());
        keep_players(found[3]);
    }
}

inline uintptr_t logical_player(uintptr_t p) {
    uintptr_t lp = rdv<uintptr_t>(p + LOGICAL_PLAYER);
    return lp && g.vt_logical_player && rdv<uintptr_t>(lp) == g.vt_logical_player ? lp : 0;
}

const int CAMERA_EYE[3] = {0x4c, 0x5c, 0x6c};

inline bool view_matrix(float m[16], Vec3* eye = nullptr) {
    uintptr_t game_object = profile_root ? rdv<uintptr_t>(profile_root) : 0;
    if (!game_object || !active_level || !view_camera) return false;
    uintptr_t level = active_level(game_object);
    uintptr_t camera = level ? view_camera(level) : 0;
    uintptr_t engine_camera = camera ? rdv<uintptr_t>(camera + CAMERA_ENGINE) : 0;
    if (!engine_camera || !rd(engine_camera + CAMERA_COMBINED, m, 16 * sizeof(float))) return false;
    if (eye) *eye = {rdv<float>(engine_camera + CAMERA_EYE[0], NAN), rdv<float>(engine_camera + CAMERA_EYE[1], NAN),
                     rdv<float>(engine_camera + CAMERA_EYE[2], NAN)};
    return true;
}

inline bool to_screen(const float m[16], Vec3 p, float width, float height, float& sx, float& sy) {
    float w = p.x * m[12] + p.y * m[13] + p.z * m[14] + m[15];
    if (!(w > 0.01f)) return false;
    float x = (p.x * m[0] + p.y * m[1] + p.z * m[2] + m[3]) / w;
    float y = (p.x * m[4] + p.y * m[5] + p.z * m[6] + m[7]) / w;
    sx = (x + 1.0f) * width * 0.5f;
    sy = (y - 1.0f) * height * -0.5f;
    return std::isfinite(sx) && std::isfinite(sy);
}

inline float health_of(uintptr_t p) {
    int off = float_getter_offset(slot(p, SLOT_HEALTH));
    return off > 0 ? rdv<float>(p + off, NAN) : NAN;
}

inline float max_health_of(uintptr_t p) {
    auto fn = (float(__fastcall*)(uintptr_t, int))slot(p, SLOT_MAX_HEALTH);
    return in_game_module((uintptr_t)fn) ? fn(p, -1) : NAN;
}

inline bool playing_hunter() {
    uintptr_t lp = alive(player) ? logical_player(player) : 0;
    return lp && rdv<int>(lp + PLAYER_ROLE) == ROLE_HUNTER;
}

inline bool hunter_is_someone_else() {
    std::vector<uintptr_t> players = g.players;
    {
        std::lock_guard<std::mutex> l(esp_mx);
        players.insert(players.end(), esp_players.begin(), esp_players.end());
    }
    for (uintptr_t p : players) {
        uintptr_t lp = p != player && alive(p) ? logical_player(p) : 0;
        if (lp && rdv<int>(lp + PLAYER_ROLE) == ROLE_HUNTER) return true;
    }
    return false;
}

inline void weaken_hunter() {
    std::lock_guard<std::mutex> l(esp_mx);
    for (uintptr_t p : esp_players) {
        uintptr_t lp = p != player && alive(p) ? logical_player(p) : 0;
        int off = float_getter_offset(slot(p, SLOT_HEALTH));
        float cur = off > 0 ? rdv<float>(p + off, NAN) : NAN;
        if (lp && rdv<int>(lp + PLAYER_ROLE) == ROLE_HUNTER && cur > ONE_HIT_HEALTH && cur < 1e7f) wr<float>(p + off, ONE_HIT_HEALTH);
    }
}

inline double now_seconds() {
    static LARGE_INTEGER frequency = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)frequency.QuadPart;
}

inline uintptr_t local_player() {
    uintptr_t local = local_player_root ? rdv<uintptr_t>(rdv<uintptr_t>(local_player_root) + LOCAL_PLAYER) : 0;
    return alive(local) ? local : 0;
}

const double HOLD_SECONDS = 0.5;
const float OWN_BODY_RADIUS = 1.2f, OWN_BODY_BELOW = 2.6f, OWN_BODY_ABOVE = 0.5f;

inline bool own_body(Vec3 eye, Vec3 feet) {
    float dx = feet.x - eye.x, dz = feet.z - eye.z, dy = feet.y - eye.y;
    return dx * dx + dz * dz < OWN_BODY_RADIUS * OWN_BODY_RADIUS && dy > -OWN_BODY_BELOW && dy < OWN_BODY_ABOVE;
}

struct View { float m[16]; Vec3 eye; double at = -1; };
inline View view;
inline std::vector<EspTarget> seen_players;
inline std::string esp_state;

inline void note_esp_state(const char* state) {
    const DWORD QUIET_MS = 10000;
    static DWORD logged_at = 0;
    if (esp_state == state || GetTickCount() - logged_at < QUIET_MS) return;
    esp_state = state;
    logged_at = GetTickCount();
    logf_hook("esp: %s", state);
}

inline void track_players() {
    double now = now_seconds();
    View v;
    if (!view_matrix(v.m, &v.eye) || !std::isfinite(v.eye.x)) return note_esp_state("no camera");
    v.at = now;
    uintptr_t me = local_player();
    uintptr_t my_lp = logical_player(me);
    int my_team = my_lp ? rdv<int>(my_lp + PLAYER_TEAM, -1) : -1;
    std::vector<uintptr_t> players;
    std::vector<EspTarget> before;
    {
        std::lock_guard<std::mutex> l(esp_mx);
        players = esp_players;
        before = seen_players;
    }
    std::vector<EspTarget> now_seen;
    for (uintptr_t p : players) {
        if (p == me || !alive(p) || g.player_control < 0) continue;
        auto last = std::find_if(before.begin(), before.end(), [&](const EspTarget& t) { return t.p == p; });
        EspTarget t{};
        if (!position_of(p + g.player_control, &t.feet)) {
            if (last != before.end() && now - last->seen < HOLD_SECONDS) now_seen.push_back(*last);
            continue;
        }
        if (!me && own_body(v.eye, t.feet)) continue;
        t.p = p;
        t.seen = now;
        t.distance = distance(v.eye, t.feet);
        t.health = health_of(p);
        t.max_health = max_health_of(p);
        uintptr_t lp = logical_player(p);
        t.hunter = lp && rdv<int>(lp + PLAYER_ROLE) == ROLE_HUNTER;
        t.ally = lp && my_team >= 0 && rdv<int>(lp + PLAYER_TEAM, -2) == my_team;
        t.rank = lp ? rdv<int>(lp + (t.hunter ? RANK_HUNTER : RANK_SURVIVOR), -1) : -1;
        t.rage = lp ? rdv<float>(lp + PLAYER_RAGE, NAN) : NAN;
        now_seen.push_back(t);
    }
    note_esp_state(players.empty() ? "no players found yet" : now_seen.empty() ? "players found, none positioned" : me ? "tracking" : "tracking without local player");
    std::lock_guard<std::mutex> l(esp_mx);
    view = v;
    seen_players.swap(now_seen);
}

const float DODGE_SCAN_RADIUS = 40, DODGE_MIN_SPEED = 8, DODGE_HIT_RADIUS = 1.2f, DODGE_HORIZON = 1.2f, DODGE_STEP = 0.01f;
const float GRAVITY = 9.81f, BODY_HEIGHT = 1.7f, MAX_OWN_SPEED = 30, MIN_DODGE_SCORE = 0.3f;
const double VELOCITY_WINDOW = 0.12, MIN_VELOCITY_SPAN = 0.03, DODGE_GAP = 0.25, STALE_DODGE = 0.3;
const double REACTION_MIN = 0.12, REACTION_SPREAD = 0.12, LEAST_MOVE_TIME = 0.08;
const DWORD HOLD_MIN_MS = 220, HOLD_SPREAD_MS = 160;
const BYTE KEY_FORWARD = DIK_W, KEY_BACK = DIK_S, KEY_LEFT = DIK_A, KEY_RIGHT = DIK_D;

struct Flying { std::deque<std::pair<double, Vec3>> samples; bool handled = false; };
struct Approach { float miss = INFINITY, when = 0; Vec3 away{}; };
struct PendingDodge { BYTE key = 0; double at = 0; DWORD hold = 0; };
inline std::map<uintptr_t, Flying> flying;
inline PendingDodge pending_dodge;
inline double dodge_free_at = 0;
inline Vec3 my_last{NAN, NAN, NAN}, my_velocity{};
inline double my_last_at = 0;
inline EngineVector nearby{};
inline std::mt19937 dodge_rng{GetTickCount()};

inline Approach closest_approach(Vec3 p, Vec3 v, Vec3 eye, Vec3 mine) {
    Approach a;
    for (float t = 0; t <= DODGE_HORIZON; t += DODGE_STEP) {
        Vec3 s{p.x + v.x * t, p.y + v.y * t - 0.5f * GRAVITY * t * t, p.z + v.z * t};
        Vec3 me{eye.x + mine.x * t, eye.y + mine.y * t, eye.z + mine.z * t};
        float gap = s.y > me.y ? s.y - me.y : s.y < me.y - BODY_HEIGHT ? me.y - BODY_HEIGHT - s.y : 0;
        float dx = me.x - s.x, dz = me.z - s.z, d = std::sqrt(dx * dx + dz * dz + gap * gap);
        if (d < a.miss) a = {d, t, {dx, 0, dz}};
    }
    return a;
}

inline bool flat_unit(Vec3& v) {
    float n = std::sqrt(v.x * v.x + v.z * v.z);
    if (n < 1e-4f) return false;
    v = {v.x / n, 0, v.z / n};
    return true;
}

inline BYTE dodge_key(const float m[16], Vec3 away, Vec3 flight) {
    Vec3 forward{m[12], 0, m[14]}, right{m[0], 0, m[2]};
    if (!flat_unit(forward) || !flat_unit(right)) return 0;
    if (!flat_unit(away)) {
        away = {-flight.z, 0, flight.x};
        if (!flat_unit(away)) return 0;
        if (dodge_rng() & 1) away = {-away.x, 0, -away.z};
    }
    struct Option { BYTE key, opposite; Vec3 dir; };
    const Option options[] = {{KEY_RIGHT, KEY_LEFT, right}, {KEY_LEFT, KEY_RIGHT, {-right.x, 0, -right.z}},
                              {KEY_FORWARD, KEY_BACK, forward}, {KEY_BACK, KEY_FORWARD, {-forward.x, 0, -forward.z}}};
    BYTE best = 0;
    float best_score = MIN_DODGE_SCORE;
    for (auto& o : options) {
        float score = o.dir.x * away.x + o.dir.z * away.z;
        if (score <= best_score || input::held(o.opposite)) continue;
        best = o.key, best_score = score;
    }
    return best && !input::held(best) ? best : 0;
}

inline const char* key_name(BYTE key) { return key == KEY_LEFT ? "left" : key == KEY_RIGHT ? "right" : key == KEY_FORWARD ? "forward" : "back"; }

inline void dodge_spits() {
    double now = now_seconds();
    float m[16];
    Vec3 eye;
    uintptr_t game_object = profile_root ? rdv<uintptr_t>(profile_root) : 0;
    uintptr_t level = game_object && active_level ? active_level(game_object) : 0;
    if (!level || !find_in_radius || !view_matrix(m, &eye) || !std::isfinite(eye.x)) return;
    if (std::isfinite(my_last.x) && now - my_last_at > 0.001) {
        float k = (float)(1.0 / (now - my_last_at));
        my_velocity = {(eye.x - my_last.x) * k, (eye.y - my_last.y) * k, (eye.z - my_last.z) * k};
        if (distance(my_velocity, {}) > MAX_OWN_SPEED) my_velocity = {};
    }
    my_last = eye, my_last_at = now;

    if (pending_dodge.key && now >= pending_dodge.at) {
        BYTE key = pending_dodge.key;
        pending_dodge.key = 0;
        if (now - pending_dodge.at < STALE_DODGE && !input::held(key)) {
            input::press(key, pending_dodge.hold);
            dodge_free_at = now + pending_dodge.hold / 1000.0 + DODGE_GAP;
            logf_hook("dodge: stepping %s for %lu ms", key_name(key), (unsigned long)pending_dodge.hold);
        }
    }

    nearby.size = 0;
    find_in_radius(level, &nearby, &eye, DODGE_SCAN_RADIUS, 0, false, nullptr);
    std::set<uintptr_t> present;
    for (uint32_t i = 0; i < nearby.size && nearby.data; i++) {
        uintptr_t control = nearby.data[i];
        bool spit = false;
        for (int c = 0; c < 2; c++) spit |= vt_throwable[c] && throwable_control[c] >= 0 && rdv<uintptr_t>(control - throwable_control[c]) == vt_throwable[c];
        Vec3 p;
        if (!spit || !position_of(control, &p)) continue;
        present.insert(control);
        auto& f = flying[control];
        if (f.samples.empty() || distance(f.samples.back().second, p) > 0) f.samples.push_back({now, p});
        while (f.samples.size() > 2 && now - f.samples[1].first > VELOCITY_WINDOW) f.samples.pop_front();
        double span = f.samples.back().first - f.samples.front().first;
        if (f.handled || span < MIN_VELOCITY_SPAN) continue;
        Vec3 a = f.samples.front().second, b = f.samples.back().second;
        Vec3 v{(float)((b.x - a.x) / span), (float)((b.y - a.y) / span - GRAVITY * span / 2), (float)((b.z - a.z) / span)};
        Vec3 rel{v.x - my_velocity.x, v.y - my_velocity.y, v.z - my_velocity.z};
        Vec3 to_me{eye.x - p.x, eye.y - BODY_HEIGHT / 2 - p.y, eye.z - p.z};
        if (distance(v, {}) < DODGE_MIN_SPEED || rel.x * to_me.x + rel.y * to_me.y + rel.z * to_me.z <= 0) continue;
        Approach hit = closest_approach(p, v, eye, my_velocity);
        if (hit.miss > DODGE_HIT_RADIUS) continue;
        f.handled = true;
        double reaction = REACTION_MIN + std::uniform_real_distribution<double>(0, REACTION_SPREAD)(dodge_rng);
        const char* skipped = hit.when < reaction + LEAST_MOVE_TIME ? "too close to react"
                              : pending_dodge.key || now < dodge_free_at ? "already dodging" : nullptr;
        BYTE key = skipped ? 0 : dodge_key(m, hit.away, v);
        if (!skipped && !key) skipped = "no free direction";
        logf_hook("dodge: throwable %.1f m away at %.1f m/s passes %.2f m from you in %.2f s, %s", distance(eye, p), distance(v, {}), hit.miss,
                  hit.when, skipped ? skipped : key_name(key));
        if (key) pending_dodge = {key, now + reaction, HOLD_MIN_MS + (DWORD)(dodge_rng() % HOLD_SPREAD_MS)};
    }
    for (auto it = flying.begin(); it != flying.end();) it = present.count(it->first) ? std::next(it) : flying.erase(it);
}

inline bool esp_snapshot(float m[16], std::vector<EspTarget>& out, float max_distance) {
    std::lock_guard<std::mutex> l(esp_mx);
    if (view.at < 0 || now_seconds() - view.at > HOLD_SECONDS) return false;
    memcpy(m, view.m, sizeof view.m);
    out.clear();
    for (auto& t : seen_players)
        if (t.distance <= max_distance) out.push_back(t);
    return true;
}

const int FLASH_COLORS[] = {0x180, 0x18c, 0x198, 0x1a4, 0x1b0}, FLASH_BEAMS[] = {0x198, 0x1b0};
const int FLASH_INTENSITIES[] = {0x1bc, 0x1c0};
const int FLASH_PART_BYTES = 0x1c4 - 0x180;
struct UvOriginal { uint8_t bytes[FLASH_PART_BYTES]; };
inline std::map<uintptr_t, UvOriginal> uv_originals;
inline bool uv_applied = false;

inline bool uv_desc(uintptr_t desc) {
    float c[3];
    return rd(desc + FLASH_COLORS[0], c, sizeof c) && c[2] >= 200 && c[2] <= 260 && c[1] < 50 && c[0] < 120;
}

inline void apply_uv_light(bool on, const float color[3], float glow) {
    if (!on) {
        if (uv_applied)
            for (auto& [desc, o] : uv_originals) game::wr_bytes(desc + FLASH_COLORS[0], o.bytes, sizeof o.bytes);
        uv_applied = false;
        return;
    }
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (std::isnan(ITEMS[i].st[ST_DepletionTime])) continue;
        auto d = g.descs.find(ITEMS[i].id);
        if (d == g.descs.end() || uv_originals.count(d->second) || !uv_desc(d->second)) continue;
        UvOriginal o;
        if (rd(d->second + FLASH_COLORS[0], o.bytes, sizeof o.bytes)) uv_originals[d->second] = o;
    }
    for (auto& [desc, o] : uv_originals) {
        uint8_t now[FLASH_PART_BYTES];
        memcpy(now, o.bytes, sizeof now);
        for (int at : FLASH_COLORS) tint((float*)(now + at - FLASH_COLORS[0]), color, 1.0f);
        for (int at : FLASH_BEAMS)
            for (int k = 0; k < 3; k++) ((float*)(now + at - FLASH_COLORS[0]))[k] *= glow;
        for (int at : FLASH_INTENSITIES) *(float*)(now + at - FLASH_COLORS[0]) *= glow;
        uint8_t current[FLASH_PART_BYTES];
        if (rd(desc + FLASH_COLORS[0], current, sizeof current) && memcmp(current, now, sizeof now))
            game::wr_bytes(desc + FLASH_COLORS[0], now, sizeof now);
    }
    uv_applied = true;
}

inline void apply_dfa_fall_speed() {
    float cur = rdv<float>(dfa_fall_speed, NAN);
    if (std::isnan(dfa_fall_original)) {
        if (!(cur > 0)) return;
        dfa_fall_original = cur;
    }
    Tweak* t = find_tweak("h_dfa_height");
    float strength = std::clamp((t->factor - 1.0f) / (t->max - 1.0f), 0.0f, 1.0f);
    float want = dfa_fall_original + (DFA_FALL_SPEED_AT_MAX - dfa_fall_original) * strength;
    if (cur != want) wr<float>(dfa_fall_speed, want);
}

inline int keep_uv_charge() {
    int kept = 0;
    for (uintptr_t e : g.equipment) {
        if (rdv<uintptr_t>(e) != g.vt_equipment) continue;
        kept++;
        float charge = rdv<float>(e + UV_CHARGE, NAN);
        if (!(charge >= 0 && charge <= 1)) continue;
        if (charge < 1) wr<float>(e + UV_CHARGE, 1.0f);
        if (rdv<uint8_t>(e + UV_EXHAUSTED)) wr<uint8_t>(e + UV_EXHAUSTED, 0);
    }
    return kept;
}

inline std::atomic<bool> objects_missing{false}, respawned{false};

inline int keep_rope_energy() {
    int live = 0;
    auto containers = param_containers();
    float full = containers.empty() ? NAN : rdv<float>(param_value(containers[0], "RopeMaxEnergy"), NAN);
    for (uintptr_t r : g.ropes) {
        if (rdv<uintptr_t>(r) != g.vt_rope) continue;
        live++;
        float energy = rdv<float>(r + ROPE_ENERGY, NAN);
        if (!(full > 0 && full < 1000) || !(energy >= 0 && energy <= full)) continue;
        if (energy < full) wr<float>(r + ROPE_ENERGY, full);
        if (rdv<uint8_t>(r + ROPE_DEPLETED)) wr<uint8_t>(r + ROPE_DEPLETED, 0);
    }
    return live;
}

inline void tick() {
    std::lock_guard<std::mutex> l(game::mx);
    bool prison_missing = !pause_prison() && is_on("prison_pause");
    bool uv_missing = is_on("uv") && !keep_uv_charge();
    float uv_slow = find_tweak("uv_slow")->factor;
    bool uv_slow_missing = false;
    if (!is_on("uv") && uv_slow > 1.0f) uv_slow_missing = !slow_uv_drain(uv_slow);
    else uv_last_charge.clear();
    bool rope_missing = is_on("hook") && !keep_rope_energy();
    patch_lockpick(is_on("lockpick"));
    patch_pound(is_on("z_pound_hits"));
    block_forced_damage(is_on("god"));
    apply_spit_radius(playing_hunter() ? std::clamp(find_tweak("z_spit")->factor.load(), 1.0f, find_tweak("z_spit")->max) : 1.0f);
    route_tick();
    objects_missing = prison_missing || uv_missing || uv_slow_missing || rope_missing;
    game::reapply_stats();
    player = find_player();
    if (alive(player)) {
        keep_stacks(is_on("ammo"), is_on("supplies"));
        keep_durability(is_on("durability"));
        if (is_on("one_hit")) weaken_enemies();
        if (is_on("one_hit_hunter")) weaken_hunter();
    }
    uintptr_t lp = alive(player) ? logical_player(player) : 0;
    side = !lp ? SIDE_UNKNOWN : rdv<int>(lp + PLAYER_ROLE) == ROLE_HUNTER ? SIDE_HUNTER : SIDE_SURVIVOR;
    uintptr_t provider = alive(player) ? rdv<uintptr_t>(player + PARAM_PROVIDER) : 0;
    uintptr_t before = player_provider.exchange(provider);
    if (provider && before && provider != before) {
        logf_hook("player changed (respawn or level load), finding game objects again");
        respawned = true;
    }
    install_var_hook();
    apply_dfa_fall_speed();
    apply_uv_light(config::cfg.uv.on, config::cfg.uv.color, config::cfg.uv.glow);
    apply_overrides();
    if (!player) return;
    read_skill_levels();
    set_xp_factor(find_tweak("xp")->factor);
    boost_xp(find_tweak("xp")->factor);
    hook_health(is_on("god"), std::clamp(find_tweak("damage_taken")->factor.load(), 1.0f, find_tweak("damage_taken")->max));
    if (is_on("stamina") || is_on("z_energy")) keep_stamina();
    else best_stamina[0] = best_stamina[1] = 0;
}

inline uint32_t reads_of(const char* name) {
    auto it = param_ids.find(name);
    return it == param_ids.end() || it->second + 1 >= PARAM_SLOTS ? 0 : cached[it->second + 1].reads.load();
}

inline std::string tree_report() {
    uintptr_t container = skill_container();
    char head[64];
    snprintf(head, sizeof head, "container %llx trees %llx ", (unsigned long long)container, (unsigned long long)rdv<uintptr_t>(container + SKILL_TREES));
    std::string out = head;
    for (auto& t : TREES) out += std::to_string(t.type) + ":" + std::to_string(tree_level(t.type)) + "/" + std::to_string(tree_max(t.type)) + " ";
    return out;
}

inline std::string local_class() {
    uintptr_t local = local_player_root ? rdv<uintptr_t>(rdv<uintptr_t>(local_player_root) + LOCAL_PLAYER) : 0;
    return local ? game::class_name(local) : "none";
}

inline std::string describe() {
    char b[1600];
    snprintf(b, sizeof b,
             "player %s (%zu scanned, local root %s, local %s), health %.0f, stamina %.0f, params %zu, cache hook %s (hook reads %u, uv reads %u), "
             "ammo flag %s, set level %s, xp level %s, script vars %s, lockpick %s, god patch %s, pound patch %s, spit radii %zu, pvp rank survivor %d hunter %d, trees %s, enemies %zu, overrides %zu, equipment %zu, ropes %zu, "
             "prison data %zu (start %.1f end %.1f), prison sensors %zu, positions %s (player +%x, sensor +%x/+%x)",
             player ? "ok" : "missing", g.players.size(), local_player_root ? "ok" : "missing", local_class().c_str(), health(), stamina(),
             param_ids.size(), cache_get_original ? "ok" : cache_get_fn ? "not installed" : "missing", reads_of("GrapplingHookCooldown"),
             reads_of("FlashlightDrainMul"), unlimited_ammo_flag ? "ok" : "missing", set_level_fn ? "ok" : "missing",
             level_from_xp_fn ? "ok" : "missing", var_root ? "ok" : "missing", !lockpick_patch ? "missing" : lockpick_patched() ? "on" : "ready",
             !forced_damage_jump ? "missing" : forced_damage_blocked() ? "on" : "ready", !pound_exposure_check ? "missing" : pound_always_hits() ? "on" : "ready", spit_radius_originals.size(), pvp_rank(false), pvp_rank(true), tree_report().c_str(),
             enemy_modules.size(), saved_bytes.size(),
             g.equipment.size(), g.ropes.size(), g.prison_data.size(), g.prison_data.empty() ? NAN : rdv<float>(g.prison_data[0] + PRISON_START_TIME, NAN),
             g.prison_data.empty() ? NAN : rdv<float>(g.prison_data[0] + PRISON_END_TIME, NAN), g.prison_sensors.size(),
             get_position && set_position ? "ok" : "missing", game::g.player_control, game::g.sensor_control, game::g.sensor_rtti);
    return b;
}

}

#define FATRAINER_UPDATE_HOOK(n)                                                                                    \
    ".globl fatrainer_update_hook" #n "\n"                                                                          \
    "fatrainer_update_hook" #n ":\n"                                                                                \
    "push %rcx\npush %rdx\npush %r8\npush %r9\nsub $0x68, %rsp\n"                                                  \
    "movdqu %xmm0, 0x20(%rsp)\nmovdqu %xmm1, 0x30(%rsp)\nmovdqu %xmm2, 0x40(%rsp)\nmovdqu %xmm3, 0x50(%rsp)\n"     \
    "call fatrainer_module_update\n"                                                                                \
    "movdqu 0x20(%rsp), %xmm0\nmovdqu 0x30(%rsp), %xmm1\nmovdqu 0x40(%rsp), %xmm2\nmovdqu 0x50(%rsp), %xmm3\n"     \
    "add $0x68, %rsp\npop %r9\npop %r8\npop %rdx\npop %rcx\n"                                                       \
    "jmp *fatrainer_update_original+" #n "*8(%rip)\n"

asm(".text\n" FATRAINER_UPDATE_HOOK(0) FATRAINER_UPDATE_HOOK(1) FATRAINER_UPDATE_HOOK(2));

extern "C" void fatrainer_module_update(uintptr_t module) {
    static cheats::Cheat* one_hit = cheats::find("one_hit");
    if (!one_hit->on || !cheats::is_enemy_module(module) || cheats::is_nest_module(module)) return;
    float cur = cheats::rdv<float>(module + cheats::MODULE_HEALTH, NAN);
    if (cur > cheats::ONE_HIT_HEALTH && cur < 1e7f) cheats::wr<float>(module + cheats::MODULE_HEALTH, cheats::ONE_HIT_HEALTH);
}
