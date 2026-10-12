#pragma once
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "color.h"
#include "config.h"
#include "game.h"
#include "pak.h"

namespace visuals {

using game::rdv;

using game::rd;

const int SLOT_VAR_FLOAT = 114, SLOT_VAR_VEC3 = 116, VAR_SLOTS = 1024;
using VarFloatFn = float (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
using VarVec3Fn = float* (*)(uintptr_t, float*, uintptr_t, uintptr_t, uintptr_t);
inline VarFloatFn original_var_float = nullptr;
inline VarVec3Fn original_var_vec3 = nullptr;
inline uintptr_t var_vtable[VAR_SLOTS + 1];

inline void tint(float* c, const float color[3], float scale) {
    float peak = std::max({color[0], color[1], color[2], 0.001f}), bright = std::max({c[0], c[1], c[2]}) * scale;
    for (int k = 0; k < 3; k++) c[k] = color[k] / peak * bright;
}

inline bool hunter_glow_var(const char* s) { return !strncmp(s, "v3_btz_zombie_v", 15) && !strstr(s, "not_drained"); }

struct SurvivorGlow { const char* key; const char* label; std::vector<const char*> vars; };
inline const SurvivorGlow SURVIVOR_GLOWS[] = {
    {"normal", "Survivors", {"v3_btz_human_glow_color_default"}},
    {"uv", "Hit by UV Suppressor", {"v3_btz_human_glow_color_ld"}},
    {"horde", "Marked by Horde Summoner", {"v3_btz_human_glow_color_cth"}},
    {"sense", "Hit by Sense Suppressor", {"v3_btz_human_glow_color_camo", "v3_btz_human_glow_color_camo_spit"}},
};

inline const config::HudColor* survivor_glow_for(const char* var) {
    for (auto& g : SURVIVOR_GLOWS)
        for (const char* name : g.vars)
            if (!strcmp(var, name)) {
                auto it = config::cfg.survivor_glow.find(g.key);
                return it != config::cfg.survivor_glow.end() && it->second.on ? &it->second : nullptr;
            }
    return nullptr;
}

inline bool survivor_glow_on() {
    for (auto& [key, g] : config::cfg.survivor_glow)
        if (g.on) return true;
    return false;
}

inline float* tinted_var_vec3(uintptr_t self, float* out, uintptr_t name, uintptr_t scope, uintptr_t extra) {
    float* v = original_var_vec3(self, out, name, scope, extra);
    auto& glow = config::cfg.hunter_glow;
    const char* s = name ? *(const char**)name : nullptr;
    if (!v || !s) return v;
    const config::HudColor* survivor = glow.on && hunter_glow_var(s) ? nullptr : survivor_glow_for(s);
    if (!survivor && !(glow.on && hunter_glow_var(s))) return v;
    float c[3] = {v[0], v[1], v[2]};
    tint(c, survivor ? survivor->color : glow.color, survivor ? 1.0f : glow.glow);
    memcpy(out, c, sizeof c);
    return out;
}

inline void install_var_hook(uintptr_t root, bool floats_wanted, VarFloatFn scaled_float) {
    uintptr_t object = rdv<uintptr_t>(root), current = rdv<uintptr_t>(object);
    uintptr_t ours = (uintptr_t)&var_vtable[1];
    if (!current || current == ours || !(floats_wanted || config::cfg.hunter_glow.on || survivor_glow_on())) return;
    for (size_t n = VAR_SLOTS + 1; n > SLOT_VAR_VEC3 + 1; n /= 2)
        if (rd(current - 8, var_vtable, n * 8)) {
            original_var_float = (VarFloatFn)var_vtable[1 + SLOT_VAR_FLOAT];
            original_var_vec3 = (VarVec3Fn)var_vtable[1 + SLOT_VAR_VEC3];
            if (scaled_float) var_vtable[1 + SLOT_VAR_FLOAT] = (uintptr_t)scaled_float;
            var_vtable[1 + SLOT_VAR_VEC3] = (uintptr_t)&tinted_var_vec3;
            game::wr<uintptr_t>(object, ours);
            return;
        }
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
        auto d = game::g.descs.find(ITEMS[i].id);
        if (d == game::g.descs.end() || uv_originals.count(d->second) || !uv_desc(d->second)) continue;
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

enum UiKind { UI_NONE, UI_VIEW, UI_TEXT };

struct Ui {
    int(__fastcall* children_count)(uintptr_t element) = nullptr;
    uintptr_t(__fastcall* child)(uintptr_t element, int index) = nullptr;
    float*(__fastcall* get_color)(uintptr_t view, float* out) = nullptr;
    void(__fastcall* set_color)(uintptr_t view, const float* rgb) = nullptr;
    float*(__fastcall* get_text_color)(uintptr_t text, float* out) = nullptr;
    void(__fastcall* set_text_color)(uintptr_t text, const float* rgba) = nullptr;
    std::string (*name_of)(uintptr_t element) = nullptr;
    int (*kind_of)(uintptr_t element, uintptr_t* at) = nullptr;
    uintptr_t (*vtable_of)(uintptr_t element) = nullptr;
    float (*text_alpha)(uintptr_t text) = nullptr;
    float(__fastcall* get_opacity)(uintptr_t element) = nullptr;
    void(__fastcall* set_opacity)(uintptr_t element, float opacity) = nullptr;
    bool(__fastcall* shown)(uintptr_t element) = nullptr;
};

inline Ui ui;

struct HudPart { const char* key; const char* label; const char* cluster; std::vector<const char*> groups; };

inline const HudPart HUD_PARTS[] = {
    {"health", "Health bar", "HEALTH AND STAMINA", {"HudHealthIndicator"}},
    {"level", "Legend level bar", "HEALTH AND STAMINA", {"HudLegendXp"}},
    {"boosters", "Boosters", "HEALTH AND STAMINA", {"HudBoosters"}},
    {"medkits", "Medkits", "HEALTH AND STAMINA", {"HudMedkit"}},
    {"stamina", "Stamina", "HEALTH AND STAMINA", {"HudStamina"}},
    {"crosshair", "Crosshair", "AIM AND WEAPONS", {"HudCrosshair"}},
    {"rope", "Grappling hook reticle", "AIM AND WEAPONS", {"HudRopeReticle"}},
    {"throw", "Throw targets", "AIM AND WEAPONS", {"HudThrowTargetIndicator"}},
    {"weapon", "Weapon", "AIM AND WEAPONS", {"HudPrimaryWeaponIndicator"}},
    {"equipment", "Throwable or tool", "AIM AND WEAPONS", {"HudSecondaryWeaponIndicator"}},
    {"grenade", "Grenade warning", "AIM AND WEAPONS", {"HudGrenadeIndicator"}},
    {"reload", "Reload hint", "AIM AND WEAPONS", {"HudReloadNotify"}},
    {"selection", "Weapon wheel", "AIM AND WEAPONS", {"HudWeaponSelection", "HudEquipmentSelection", "HudArrowsSelection", "HudBoltSelection"}},
    {"xp", "XP bars", "PROGRESS", {"HudXpBars"}},
    {"driver_xp", "Driver XP", "PROGRESS", {"HudDriverXp"}},
    {"xp_popups", "XP popups", "PROGRESS", {"HudFloatingConsole"}},
    {"night_reward", "Night bonus", "PROGRESS", {"HudNightReward"}},
    {"pickups", "Item pickups", "PROGRESS", {"HudItemPickup"}},
    {"radar", "Radar", "MAP AND OBJECTIVES", {"HudRadar"}},
    {"objectives", "Quest and objective list", "MAP AND OBJECTIVES", {"StackObjectives"}},
    {"marker", "Objective markers", "MAP AND OBJECTIVES", {"Hud3dObjective", "HudChallengeObjectiveIndicator"}},
    {"coop", "Co-op players", "MAP AND OBJECTIVES", {"HudCoopMarker", "HudPlayersList", "HudPlayerInfo"}},
    {"killfeed", "Killfeed", "MESSAGES", {"HudMpMessages"}},
    {"big_messages", "Big messages", "MESSAGES", {"HudBigMessages"}},
    {"prompts", "Button prompts", "MESSAGES", {"HudTrigger"}},
    {"tutorial", "Tutorial hints", "MESSAGES", {"HudTutorialDI", "HudInfoPanel"}},
    {"subtitles", "Subtitles", "MESSAGES", {"HudDialogue"}},
    {"enemy_hp", "Enemy health", "ENEMIES", {"HudEnemyHp"}},
    {"boss_hp", "Boss health", "ENEMIES", {"HudBossHpBar"}},
    {"aggro", "Danger and chase", "ENEMIES", {"HudAggroIndicator", "HudPursuitIndicator"}},
    {"hunter_info", "Night Hunter info", "BE THE ZOMBIE", {"HudHunterInfo"}},
    {"players", "Player markers", "BE THE ZOMBIE", {"HudPlayersIndicator"}},
    {"nests", "Nest counter", "BE THE ZOMBIE", {"HudZombieHiveCounter"}},
    {"invasion", "Invasion messages", "BE THE ZOMBIE", {"HudZombieInvasionMessaging"}},
    {"hit_direction", "Hit direction", "BE THE ZOMBIE", {"HudPvPDamageIndicator"}},
};
const int HUD_PART_COUNT = (int)(sizeof HUD_PARTS / sizeof HUD_PARTS[0]);
const int HUD_MAX_DEPTH = 32, HUD_MAX_ELEMENTS = 20000;

struct HudNode {
    uintptr_t element, vtable, at, engine, engine_vtable;
    int kind, part;
    float original[3], written[3];
    bool wrote, dead;
};

struct HudRoot {
    uintptr_t element, vtable;
    int part;
    float game_opacity;
    bool hid;
};

const int UI_ENGINE_OBJECT = 0x18, ENGINE_NAME = 0x8, NAME_LIMIT = 64, TEXT_COLOR = 0x384;
const DWORD HUD_REWALK_MS = 2000;
inline std::vector<HudNode> hud_nodes;
inline std::vector<HudRoot> hud_roots;
inline std::unordered_map<uintptr_t, size_t> hud_known, hud_engines;
inline std::unordered_map<uintptr_t, size_t> hud_known_roots;
inline uintptr_t hud_root = 0;
inline std::mutex hud_mx;
inline thread_local bool trainer_writing = false;

inline int part_named(const std::string& name) {
    for (int p = 0; p < HUD_PART_COUNT; p++)
        for (const char* g : HUD_PARTS[p].groups)
            if (name == g) return p;
    return -1;
}

inline bool node_alive(const HudNode& n) { return !n.dead && ui.vtable_of(n.element) == n.vtable && (!n.engine || rdv<uintptr_t>(n.engine) == n.engine_vtable); }

inline void walk_hud(uintptr_t element, int part, int depth) {
    if (depth > HUD_MAX_DEPTH || !ui.children_count) return;
    int n = ui.children_count(element);
    for (int i = 0; i < n && (int)hud_nodes.size() < HUD_MAX_ELEMENTS; i++) {
        uintptr_t c = ui.child(element, i);
        if (!c) continue;
        int named = part_named(ui.name_of(c)), p = named >= 0 ? named : part;
        if (named >= 0) {
            auto r = hud_known_roots.find(c);
            if (r == hud_known_roots.end() || hud_roots[r->second].vtable != ui.vtable_of(c)) {
                hud_known_roots[c] = hud_roots.size();
                hud_roots.push_back({c, ui.vtable_of(c), named, 1.0f, false});
            }
        }
        auto known = hud_known.find(c);
        if (known == hud_known.end() || !node_alive(hud_nodes[known->second])) {
            uintptr_t at = 0;
            int kind = ui.kind_of(c, &at);
            if (kind != UI_NONE) {
                uintptr_t engine = rdv<uintptr_t>(at + UI_ENGINE_OBJECT);
                HudNode node{c, ui.vtable_of(c), at, engine, engine ? rdv<uintptr_t>(engine) : 0, kind, p, {}, {}, false, false};
                (kind == UI_TEXT ? ui.get_text_color : ui.get_color)(at, node.original);
                if (known != hud_known.end()) hud_nodes[known->second].dead = true;
                hud_known[c] = hud_nodes.size();
                if (engine) hud_engines[engine] = hud_nodes.size();
                hud_nodes.push_back(node);
            }
        }
        walk_hud(c, p, depth + 1);
    }
}

inline void write_color(const HudNode& n, const float rgb[3]) {
    trainer_writing = true;
    if (n.kind != UI_TEXT) ui.set_color(n.at, rgb);
    else {
        float rgba[4] = {rgb[0], rgb[1], rgb[2], ui.text_alpha(n.at)};
        ui.set_text_color(n.at, rgba);
    }
    trainer_writing = false;
}

inline void forget_hud() {
    std::lock_guard<std::mutex> l(hud_mx);
    for (auto& n : hud_nodes)
        if (n.wrote && node_alive(n)) write_color(n, n.original);
    for (auto& r : hud_roots)
        if (r.hid && ui.set_opacity && ui.vtable_of(r.element) == r.vtable) ui.set_opacity(r.element, r.game_opacity);
    hud_nodes.clear(), hud_roots.clear(), hud_known.clear(), hud_engines.clear(), hud_known_roots.clear(), hud_root = 0;
}

inline void find_hud(uintptr_t root) {
    forget_hud();
    if (!root) return;
    std::lock_guard<std::mutex> l(hud_mx);
    hud_root = root;
    walk_hud(root, -1, 0);
}

inline bool hud_found() { return !hud_nodes.empty(); }
inline size_t hud_elements() { return hud_nodes.size(); }

inline const config::HudColor* part_color(int part) {
    if (part < 0) return nullptr;
    auto it = config::cfg.hud_parts.find(HUD_PARTS[part].key);
    return it != config::cfg.hud_parts.end() && it->second.on ? &it->second : nullptr;
}

inline bool part_hidden(int part) {
    auto it = config::cfg.hud_parts.find(HUD_PARTS[part].key);
    return it != config::cfg.hud_parts.end() && it->second.hidden;
}

inline int preview_part = -1;
inline DWORD preview_until = 0;

inline void hide_parts() {
    if (!ui.get_opacity || !ui.set_opacity) return;
    bool previewing = GetTickCount() < preview_until;
    for (auto& r : hud_roots) {
        if (ui.vtable_of(r.element) != r.vtable) continue;
        float now = ui.get_opacity(r.element);
        bool show = previewing && r.part == preview_part, hide = !show && part_hidden(r.part);
        if (show && now < 1) r.game_opacity = now, r.hid = true, ui.set_opacity(r.element, 1);
        else if (hide && now != 0) r.game_opacity = now, r.hid = true, ui.set_opacity(r.element, 0); else if (!hide && !show && r.hid) r.hid = false, ui.set_opacity(r.element, r.game_opacity);
    }
}

inline void wanted_color(const HudNode& n, const float from[3], float out[3]) {
    memcpy(out, from, 3 * sizeof(float));
    const config::HudColor* own = part_color(n.part);
    const auto& tint = config::cfg.hud_tint;
    if (own || tint.on) visuals::tint(out, own ? own->color : tint.color, 1.0f);
}

inline bool adopt_game_color(uintptr_t engine, float rgb[3]) {
    if (trainer_writing || !engine) return false;
    std::lock_guard<std::mutex> l(hud_mx);
    auto it = hud_engines.find(engine);
    if (it == hud_engines.end()) return false;
    HudNode& n = hud_nodes[it->second];
    if (n.engine != engine || !node_alive(n)) return false;
    memcpy(n.original, rgb, sizeof n.original);
    wanted_color(n, n.original, n.written);
    n.wrote = memcmp(n.written, n.original, sizeof n.written) != 0;
    memcpy(rgb, n.written, sizeof n.written);
    return n.wrote;
}

using ViewColorFn = void(__fastcall*)(uintptr_t engine, float r, float g, float b);
using ViewColor4Fn = void(__fastcall*)(uintptr_t view, const float* rgba);
using TextColorFn = void(__fastcall*)(uintptr_t engine, const float* rgba);
using TextColor3Fn = void(__fastcall*)(uintptr_t text, const float* rgb);
using TextColorPartsFn = void(__fastcall*)(uintptr_t text, float r, float g, float b, float a);
inline ViewColorFn view_color_original = nullptr;
inline ViewColor4Fn view_color4_original = nullptr;
inline TextColorFn text_color_original = nullptr;
inline TextColor3Fn text_color3_original = nullptr;
inline TextColorPartsFn text_color_parts_original = nullptr;

inline void __fastcall view_color_hook(uintptr_t engine, float r, float g, float b) {
    float c[3] = {r, g, b};
    adopt_game_color(engine, c);
    view_color_original(engine, c[0], c[1], c[2]);
}

inline void __fastcall view_color4_hook(uintptr_t view, const float* rgba) {
    float c[4] = {rgba[0], rgba[1], rgba[2], rgba[3]};
    view_color4_original(view, adopt_game_color(*(uintptr_t*)(view + UI_ENGINE_OBJECT), c) ? c : rgba);
}

inline void __fastcall text_color_hook(uintptr_t engine, const float* rgba) {
    float c[4] = {rgba[0], rgba[1], rgba[2], rgba[3]};
    if (!trainer_writing && !memcmp((const void*)(engine + TEXT_COLOR), c, sizeof c) && adopt_game_color(engine, c)) {
        memcpy((void*)(engine + TEXT_COLOR), c, 3 * sizeof(float));
        return text_color_original(engine, c);
    }
    text_color_original(engine, rgba);
}

inline void __fastcall text_color3_hook(uintptr_t text, const float* rgb) {
    float c[3] = {rgb[0], rgb[1], rgb[2]};
    text_color3_original(text, adopt_game_color(*(uintptr_t*)(text + UI_ENGINE_OBJECT), c) ? c : rgb);
}

inline void __fastcall text_color_parts_hook(uintptr_t text, float r, float g, float b, float a) {
    float c[3] = {r, g, b};
    adopt_game_color(*(uintptr_t*)(text + UI_ENGINE_OBJECT), c);
    text_color_parts_original(text, c[0], c[1], c[2], a);
}

inline const uint8_t VIEW_COLOR_PROLOGUE[] = {0x40, 0x53, 0x48, 0x83, 0xEC, 0x50, 0x0F, 0x29, 0x74, 0x24, 0x40, 0x48, 0x8D, 0x99, 0x50, 0x02, 0x00, 0x00};
inline const uint8_t TEXT_COLOR_PROLOGUE[] = {0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0x99, 0x78, 0x03, 0x00, 0x00};
const size_t EXPORT_SPAN = 0x40;

inline int install_color_hooks(HMODULE engine, HMODULE gamedll) {
    if (!engine || view_color_original) return 0;
    const char ENGINE_DLL[] = "engine_x64_rwdi.dll";
    const char VIEW3[] = "?SetColor@IUIElementView@@QEAAXAEBVvec3@@@Z", VIEW4[] = "?SetColor@IUIElementView@@QEAAXAEBVvec4@@@Z";
    const char TEXT4[] = "?SetTextColor@IUIText@@QEAAXAEBVvec4@@@Z", TEXT3[] = "?SetTextColor@IUIText@@QEAAXAEBVvec3@@@Z", TEXT_PARTS[] = "?SetTextColor@IUIText@@QEAAXMMMM@Z";
    int installed = 0;
    uintptr_t view_internal = game::tail_jump((uintptr_t)GetProcAddress(engine, VIEW3), EXPORT_SPAN);
    uintptr_t text_internal = game::tail_jump((uintptr_t)GetProcAddress(engine, TEXT4), EXPORT_SPAN);
    if (uintptr_t t = game::detour(view_internal, VIEW_COLOR_PROLOGUE, sizeof VIEW_COLOR_PROLOGUE, (void*)view_color_hook)) view_color_original = (ViewColorFn)t, installed++;
    if (uintptr_t t = game::detour(text_internal, TEXT_COLOR_PROLOGUE, sizeof TEXT_COLOR_PROLOGUE, (void*)text_color_hook)) text_color_original = (TextColorFn)t, installed++;
    if (auto f = game::hook_import(gamedll, ENGINE_DLL, VIEW4, (void*)view_color4_hook)) view_color4_original = (ViewColor4Fn)f, installed++;
    if (auto f = game::hook_import(gamedll, ENGINE_DLL, TEXT3, (void*)text_color3_hook)) text_color3_original = (TextColor3Fn)f, installed++;
    if (auto f = game::hook_import(gamedll, ENGINE_DLL, TEXT_PARTS, (void*)text_color_parts_hook)) text_color_parts_original = (TextColorPartsFn)f, installed++;
    return installed;
}

using SetFovFn = void(__fastcall*)(uintptr_t camera, float fov);
using GetFovFn = float(__fastcall*)(uintptr_t camera);
struct CameraFov { float game, written; };
inline SetFovFn set_fov_original = nullptr;
inline GetFovFn get_fov_original = nullptr;
inline std::mutex fov_mx;
inline std::unordered_map<uintptr_t, CameraFov> camera_fovs;
inline std::atomic<float> game_fov{NAN}, shown_fov{NAN};
const float FOV_LIMIT = 170.0f;

inline void __fastcall set_fov_hook(uintptr_t camera, float fov) {
    float factor = config::cfg.fov;
    float written = factor == 1.0f ? fov : std::clamp(fov * factor, 1.0f, FOV_LIMIT);
    {
        std::lock_guard<std::mutex> l(fov_mx);
        if (factor == 1.0f) camera_fovs.erase(camera);
        else camera_fovs[camera] = {fov, written};
    }
    game_fov = fov, shown_fov = written;
    set_fov_original(camera, written);
}

inline float __fastcall get_fov_hook(uintptr_t camera) {
    float now = get_fov_original(camera);
    std::lock_guard<std::mutex> l(fov_mx);
    auto it = camera_fovs.find(camera);
    return it != camera_fovs.end() && it->second.written == now ? it->second.game : now;
}

inline bool install_fov_hooks(HMODULE gamedll) {
    if (set_fov_original) return true;
    const char ENGINE_DLL[] = "engine_x64_rwdi.dll";
    auto get = game::hook_import(gamedll, ENGINE_DLL, "?GetFOV@IBaseCamera@@QEAAMXZ", (void*)get_fov_hook);
    if (get) get_fov_original = (GetFovFn)get;
    auto set = get ? game::hook_import(gamedll, ENGINE_DLL, "?SetFOV@IBaseCamera@@QEAAXM@Z", (void*)set_fov_hook) : nullptr;
    if (set) set_fov_original = (SetFovFn)set;
    return set != nullptr;
}

inline void apply_hud() {
    for (auto& n : hud_nodes) {
        if (n.dead) continue;
        if (!node_alive(n)) {
            n.dead = true;
            continue;
        }
        float now[3];
        (n.kind == UI_TEXT ? ui.get_text_color : ui.get_color)(n.at, now);
        if (memcmp(now, n.wrote ? n.written : n.original, sizeof now)) memcpy(n.original, now, sizeof now), n.wrote = false;
        float want[3];
        wanted_color(n, n.original, want);
        if (!memcmp(want, now, sizeof want)) continue;
        write_color(n, want);
        memcpy(n.written, want, sizeof want);
        n.wrote = memcmp(want, n.original, sizeof want) != 0;
    }
    hide_parts();
}

inline int hud_element_offset = -1;
inline uintptr_t hud_root_vtable = 0;
inline std::map<uintptr_t, std::pair<int, int>> kind_cache;

inline uintptr_t module_of(uintptr_t p) {
    MEMORY_BASIC_INFORMATION m{};
    return VirtualQuery((const void*)p, &m, sizeof m) ? (uintptr_t)m.AllocationBase : 0;
}

inline std::string engine_name_of(uintptr_t element) {
    uintptr_t text = rdv<uintptr_t>(rdv<uintptr_t>(rdv<uintptr_t>(element + UI_ENGINE_OBJECT) + ENGINE_NAME));
    char b[NAME_LIMIT + 1] = {};
    for (int i = 0; i < NAME_LIMIT && game::rd(text + i, b + i, 1) && b[i]; i++) {}
    return b;
}

inline int engine_kind_of(uintptr_t element, uintptr_t* at) {
    uintptr_t vt = rdv<uintptr_t>(element);
    auto it = kind_cache.find(vt);
    if (it == kind_cache.end()) {
        uintptr_t base = module_of(vt);
        int text = base ? game::base_offset(base, vt, "IUIText") : -1;
        int view = base && text < 0 ? game::base_offset(base, vt, "IUIElementView") : -1;
        it = kind_cache.emplace(vt, text >= 0 ? std::pair{(int)UI_TEXT, text} : view >= 0 ? std::pair{(int)UI_VIEW, view} : std::pair{(int)UI_NONE, 0}).first;
    }
    *at = element + it->second.second;
    return it->second.first;
}

inline uintptr_t engine_vtable_of(uintptr_t element) { return rdv<uintptr_t>(element); }

const int TEXT_ALPHA = 0x390;
inline float engine_text_alpha(uintptr_t text) { return rdv<float>(rdv<uintptr_t>(text + UI_ENGINE_OBJECT) + TEXT_ALPHA, 1.0f); }

inline bool locate_engine(HMODULE engine) {
    if (!engine) return false;
    auto get = [&](const char* name) { return (void*)GetProcAddress(engine, name); };
    ui.children_count = (decltype(ui.children_count))get("?GetChildrenCount@IUIElement@@QEAAHXZ");
    ui.child = (decltype(ui.child))get("?GetChildren@IUIElement@@QEAAPEAV1@H@Z");
    ui.get_color = (decltype(ui.get_color))get("?GetColor@IUIElementView@@QEBA?AVvec3@@XZ");
    ui.set_color = (decltype(ui.set_color))get("?SetColor@IUIElementView@@QEAAXAEBVvec3@@@Z");
    ui.get_text_color = (decltype(ui.get_text_color))get("?GetTextColor@IUIText@@QEAA?AVvec3@@XZ");
    ui.set_text_color = (decltype(ui.set_text_color))get("?SetTextColor@IUIText@@QEAAXAEBVvec4@@@Z");
    ui.get_opacity = (decltype(ui.get_opacity))get("?GetOpacity@IUIElement@@UEBAMXZ");
    ui.set_opacity = (decltype(ui.set_opacity))get("?SetOpacity@IUIElement@@UEAAXM@Z");
    ui.shown = (decltype(ui.shown))get("?IsActuallyVisible@IUIElement@@QEBA_NXZ");
    ui.name_of = engine_name_of, ui.kind_of = engine_kind_of, ui.vtable_of = engine_vtable_of, ui.text_alpha = engine_text_alpha;
    return ui.children_count && ui.child && ui.get_color && ui.set_color && ui.get_text_color && ui.set_text_color;
}

inline void locate(uintptr_t gamedll) {
    hud_element_offset = game::g.vt_hud ? game::base_offset(gamedll, game::g.vt_hud, "IUIElement") : -1;
}

inline bool hud_colors_on() {
    if (config::cfg.hud_tint.on) return true;
    for (auto& [key, h] : config::cfg.hud_parts)
        if (h.on || h.hidden) return true;
    return false;
}

inline bool game_crosshair_shown() {
    for (auto& r : hud_roots)
        if (!strcmp(HUD_PARTS[r.part].key, "crosshair") && ui.vtable_of(r.element) == r.vtable) return !ui.shown || ui.shown(r.element);
    return false;
}

inline void spin_rainbows(double seconds) {
    for (auto& [key, cycle] : config::cfg.rainbow)
        if (float* c = config::color_of(key)) color::rainbow(c, seconds / cycle);
}

inline bool looks_missing() {
    bool descs_missing;
    {
        std::lock_guard<std::mutex> l(game::mx);
        descs_missing = game::g.descs.empty();
    }
    return (hud_colors_on() && !hud_found()) || (config::cfg.uv.on && descs_missing);
}

inline uintptr_t live_hud_root() {
    std::vector<uintptr_t> screens;
    {
        std::lock_guard<std::mutex> l(game::mx);
        screens = game::g.huds;
    }
    uintptr_t best = 0;
    int most = 0;
    for (uintptr_t s : screens) {
        if (hud_element_offset < 0 || rdv<uintptr_t>(s) != game::g.vt_hud || !rdv<uintptr_t>(s + hud_element_offset + UI_ENGINE_OBJECT)) continue;
        int n = ui.children_count(s + hud_element_offset);
        if (n > most) most = n, best = s + hud_element_offset;
    }
    return best;
}

inline void keep_hud(void (*log)(const char*, ...)) {
    static bool was_on = false;
    static DWORD walked_at = 0;
    bool on = hud_colors_on();
    if ((!on && !was_on) || !ui.children_count) return;
    if (!hud_root || rdv<uintptr_t>(hud_root) != hud_root_vtable) {
        uintptr_t root = live_hud_root();
        if (root != hud_root || !hud_found()) {
            find_hud(root);
            walked_at = GetTickCount();
            hud_root_vtable = root ? rdv<uintptr_t>(root) : 0;
            if (root) log("hud: found %zu elements", hud_elements());
        }
    }
    std::lock_guard<std::mutex> l(hud_mx);
    if (hud_root && GetTickCount() - walked_at > HUD_REWALK_MS) walked_at = GetTickCount(), walk_hud(hud_root, -1, 0);
    apply_hud();
    was_on = on;
}

struct TextEntry { const char* key; const wchar_t* text; };
template <class T> struct TtlString { const T* text; uint32_t size; uint32_t capacity; };

struct TextApi {
    TextEntry*(__fastcall* find)(uintptr_t unused, const char* const* key) = nullptr;
    bool(__fastcall* add)(uintptr_t tools, const TtlString<char>* key, const TtlString<wchar_t>* text, bool overwrite) = nullptr;
    uintptr_t tools = 0, table = 0;
};

inline TextApi texts_api;
const int TEXT_BUCKETS = 256, TEXT_BUCKET_SIZE = 0x18, FIND_TABLE_AT = 0x51;
const DWORD TEXTS_SETTLE_MS = 3000;
inline uintptr_t tools_global = 0;
const int SLOT_ADD_TEXT = 69;
inline const char TEXT_FIND_CODE[] = "48 89 5C 24 08 57 48 83 EC 20 4C 8B 12 48 8B FA BA 01 00 10 00";
inline const char TOOLS_CODE[] = "48 8B 05 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 18 48 8B 01 FF 90 90 01 00 00 48 8B 0D ? ? ? ? 48 8D 55 ? 4C 8B C0 FF 93 90 03 00 00";

inline std::wstring wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n);
    return out;
}

inline std::string utf8(const wchar_t* text) {
    if (!text || !*text) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    std::string out(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), n, nullptr, nullptr);
    return out;
}

enum TextKind { TEXT_SPOKEN, TEXT_ITEMS, TEXT_SKILLS, TEXT_QUESTS, TEXT_TUTORIALS, TEXT_HUD, TEXT_MENUS, TEXT_KINDS };
struct TextKindInfo { const char* key; const char* label; const char* hint; };
inline const TextKindInfo TEXT_KIND_INFO[TEXT_KINDS] = {
    {"spoken", "Spoken lines", "Subtitles, radio calls and what people shout. The voices themselves never change."},
    {"items", "Weapons and items", "Names and descriptions of everything you can carry, blueprints too."},
    {"skills", "Skills", "Skill names and descriptions."},
    {"quests", "Quests and objectives", "Quest names, objectives, bounties and challenges."},
    {"tutorials", "Tutorials and hints", "Tutorial pop-ups, loading tips and hints."},
    {"hud", "HUD and messages", "Killfeed, big messages, prompts and notifications."},
    {"menus", "Menus and the rest", "Menus, settings, achievements and everything else."},
};

inline bool starts_with(const std::string& s, std::initializer_list<const char*> prefixes) {
    for (const char* p : prefixes)
        if (!s.compare(0, strlen(p), p)) return true;
    return false;
}

inline int text_kind(const std::string& key) {
    std::string k = key;
    for (auto& c : k) c = (char)tolower((unsigned char)c);
    auto has = [&](const char* part) { return k.find(part) != std::string::npos; };
    if (has("dlg") || has("_cht") || has("tlk") || starts_with(k, {"chatter", "random_", "player", "cin_", "enc", "dltb_"})) return TEXT_SPOKEN;
    size_t cut = key.rfind('_');
    if ((cut != std::string::npos && game::lookup(key.substr(0, cut).c_str())) || starts_with(k, {"craftplan", "craft_", "outfit", "collectable", "blueprint"}))
        return TEXT_ITEMS;
    if (starts_with(k, {"skill"})) return TEXT_SKILLS;
    if (has("_obj_") || starts_with(k, {"q_", "sq_", "rd_", "bounty", "challenge", "mission", "quest", "chapter", "ssa_", "ugc_"})) return TEXT_QUESTS;
    if (starts_with(k, {"tut", "btz_tut", "hint", "loading", "tip"})) return TEXT_TUTORIALS;
    if (starts_with(k, {"hud", "hvote", "msg", "btz", "trigger", "action", "notif"})) return TEXT_HUD;
    return TEXT_MENUS;
}

struct TtlText { char* text; uint32_t size, capacity; };
struct GameLanguage { const char* code; const char* name; bool voice; };
inline const GameLanguage GAME_LANGUAGES[] = {
    {"En", "English", true}, {"Pl", "Polish", true}, {"De", "German", true}, {"Fr", "French", true}, {"Es", "Spanish", true},
    {"It", "Italian", true}, {"Br", "Portuguese (Brazil)", true}, {"El", "Greek", true}, {"Nl", "Dutch", false}, {"Cs", "Czech", false},
    {"Tr", "Turkish", false}, {"Ru", "Russian", false}, {"Jp", "Japanese", false}, {"Ko", "Korean", false}, {"Cn", "Chinese (Simplified)", false},
    {"Tw", "Chinese (Traditional)", false}, {"Th", "Thai", false}};
inline const char FOLLOW_GAME[] = "game";

inline std::string language_code(std::string code) {
    if (code.size() == 2) code[0] = (char)toupper((unsigned char)code[0]), code[1] = (char)tolower((unsigned char)code[1]);
    return code;
}

inline const char* game_language_name(const std::string& code) {
    for (auto& l : GAME_LANGUAGES)
        if (language_code(code) == l.code) return l.name;
    return nullptr;
}

inline std::string forced_language, forced_voice, steam_language, started_language, started_voice, startup_game_setting, startup_voice_setting;
inline std::string command_line;
using ForceLangFn = TtlText*(__fastcall*)(TtlText* out, TtlText* fallback);
using SpeechFn = bool(__fastcall*)(uintptr_t game, const char* id, bool force);
using CommandLineFn = LPSTR(WINAPI*)();
inline ForceLangFn force_lang_original = nullptr;
inline SpeechFn speech_original = nullptr;
inline CommandLineFn command_line_original = nullptr;

inline std::string styled_like(const std::string& code, const std::string& example) {
    std::string out = language_code(code);
    for (size_t i = 0; i < out.size() && i < example.size(); i++)
        out[i] = isupper((unsigned char)example[i]) ? (char)toupper((unsigned char)out[i]) : (char)tolower((unsigned char)out[i]);
    return out;
}

inline LPSTR WINAPI command_line_hook() {
    command_line = std::string(command_line_original()) + " forcelang=" + forced_language;
    return (LPSTR)command_line.c_str();
}

inline TtlText* __fastcall force_lang_hook(TtlText* out, TtlText* fallback) {
    std::string given = fallback && fallback->text ? std::string(fallback->text, fallback->size) : "";
    if (steam_language.empty()) steam_language = language_code(given);
    if (!forced_language.empty()) forced_language = styled_like(forced_language, given);
    TtlText* r = force_lang_original(out, fallback);
    started_language = r && r->text ? language_code(std::string(r->text, r->size)) : steam_language;
    return r;
}

inline std::string wanted_voice(const std::string& setting) {
    if (setting == FOLLOW_GAME) return "";
    if (setting.empty() || setting == config::STEAM_LANGUAGE) return steam_language;
    return setting;
}

inline bool __fastcall speech_hook(uintptr_t game, const char* id, bool force) {
    std::string voice = wanted_voice(forced_voice);
    std::string use = voice.size() == 2 && id ? styled_like(voice, id) : id ? id : "";
    started_voice = language_code(use);
    return speech_original(game, use.c_str(), force);
}

inline void install_language_hooks(const std::string& game_language, const std::string& voice_language) {
    startup_game_setting = game_language, startup_voice_setting = voice_language;
    HMODULE exe = GetModuleHandleA(nullptr), engine = GetModuleHandleA("engine_x64_rwdi.dll");
    if (!engine || force_lang_original) return;
    const char ENGINE_DLL[] = "engine_x64_rwdi.dll";
    forced_language = game_language.size() == 2 ? language_code(game_language) : "";
    forced_voice = voice_language;
    force_lang_original = (ForceLangFn)game::hook_import(exe, ENGINE_DLL, "?CheckForceLangParameter@IGame@@SA?AV?$string_base@D@ttl@@AEAV23@@Z", (void*)force_lang_hook);
    if (!force_lang_original) return;
    if (!forced_language.empty()) command_line_original = (CommandLineFn)game::hook_import(engine, "KERNEL32.dll", "GetCommandLineA", (void*)command_line_hook);
    if (!forced_language.empty() || (!voice_language.empty() && voice_language != config::STEAM_LANGUAGE))
        speech_original = (SpeechFn)game::hook_import(exe, ENGINE_DLL, "?SetSpeechID@IGame@@QEBA_NPEBD_N@Z", (void*)speech_hook);
}

inline std::string language_report() {
    return "steam " + (steam_language.empty() ? std::string("unknown") : steam_language) + ", game " + (started_language.empty() ? "unknown" : started_language) +
           (forced_language.empty() ? "" : " (forced " + forced_language + (command_line_original ? "" : ", command line not hooked") + ")") + ", voices " +
           (started_voice.empty() ? (speech_original ? "not set yet" : "game default") : started_voice);
}

struct GameText { std::string key; std::wstring text; int kind; };
struct TextFile { std::string name; std::vector<GameText> texts; };
using TextLibrary = std::vector<TextFile>;

inline std::vector<GameText> parse_texts(const std::string& bin) {
    std::vector<GameText> out;
    size_t p = 8;
    for (uint32_t i = 0, n = pak::u32(bin, 4); i < n && p + 2 <= bin.size(); i++) {
        uint16_t key_size = pak::u16(bin, p);
        if (p + 4 + key_size > bin.size()) break;
        std::string key = bin.substr(p + 2, key_size);
        p += 2 + key_size;
        uint16_t text_size = pak::u16(bin, p);
        if (p + 2 + 2 * (size_t)text_size > bin.size()) break;
        std::wstring text(text_size, L'\0');
        memcpy(text.data(), bin.data() + p + 2, 2 * (size_t)text_size);
        p += 2 + 2 * (size_t)text_size;
        out.push_back({key, text, text_kind(key)});
    }
    return out;
}

struct Language { const char* code; const char* name; };
inline const Language LANGUAGES[] = {{"En", "English"}, {"Pl", "Polish"}, {"De", "German"}, {"Fr", "French"}, {"Es", "Spanish"},
                                     {"It", "Italian"}, {"Br", "Portuguese (Brazil)"}, {"Cs", "Czech"}, {"Nl", "Dutch"}, {"Tr", "Turkish"}};

inline bool console_text_file(const std::string& name) {
    size_t slash = name.find_last_of("/\\");
    std::string base = slash == std::string::npos ? name : name.substr(slash + 1);
    for (auto& c : base) c = (char)tolower((unsigned char)c);
    return base.size() < 4 || base.compare(base.size() - 4, 4, ".bin") || base.find("texts") == std::string::npos || !base.compare(0, 8, "testmap_") ||
           base.find("_xbo") != std::string::npos || base.find("_ps4") != std::string::npos;
}

inline std::string game_folder() {
    char p[MAX_PATH] = "";
    HMODULE gamedll = GetModuleHandleA("gamedll_x64_rwdi.dll");
    if (!gamedll || !GetModuleFileNameA(gamedll, p, MAX_PATH)) return "";
    std::string s = p;
    return s.substr(0, s.find_last_of("\\/") + 1);
}

inline TextLibrary load_language(const std::string& folder, const std::string& code) {
    TextLibrary files;
    std::string zip, bin;
    if (pak::read_file(folder + "DW\\Data" + code + ".pak", zip))
        for (auto& e : pak::entries(zip))
            if (!console_text_file(e.name) && pak::extract(zip, e, bin)) files.push_back({e.name, parse_texts(bin)});
    WIN32_FIND_DATAA dlc, file;
    HANDLE dlcs = FindFirstFileA((folder + "DW_DLC*").c_str(), &dlc);
    for (bool more = dlcs != INVALID_HANDLE_VALUE; more; more = FindNextFileA(dlcs, &dlc)) {
        std::string maps = folder + dlc.cFileName + "\\Data" + code + "\\Data\\Maps\\";
        HANDLE found = FindFirstFileA((maps + "*.bin").c_str(), &file);
        for (bool next = found != INVALID_HANDLE_VALUE; next; next = FindNextFileA(found, &file))
            if (!console_text_file(file.cFileName) && pak::read_file(maps + file.cFileName, bin)) files.push_back({file.cFileName, parse_texts(bin)});
        if (found != INVALID_HANDLE_VALUE) FindClose(found);
    }
    if (dlcs != INVALID_HANDLE_VALUE) FindClose(dlcs);
    return files;
}

inline std::mutex libraries_mx;
inline std::map<std::string, std::shared_ptr<const TextLibrary>> libraries;
inline std::set<std::string> libraries_loading;

inline std::shared_ptr<const TextLibrary> library(const std::string& code) {
    std::lock_guard<std::mutex> l(libraries_mx);
    if (auto it = libraries.find(code); it != libraries.end()) return it->second;
    if (libraries_loading.insert(code).second)
        std::thread([code] {
            auto loaded = std::make_shared<const TextLibrary>(load_language(game_folder(), code));
            std::lock_guard<std::mutex> l(libraries_mx);
            libraries[code] = loaded, libraries_loading.erase(code);
        }).detach();
    return nullptr;
}

struct WrittenText { const wchar_t* at; std::string from; };
inline std::map<std::string, std::wstring> text_originals;
inline std::unordered_map<std::string, WrittenText> text_written;
inline std::set<std::string> user_written;
inline std::shared_ptr<const TextLibrary> applied_library;
inline std::string applied_language;
inline std::vector<const wchar_t*> sentinels;
inline std::atomic<unsigned> texts_generation{0};

inline TextEntry* find_text(const std::string& key) {
    const char* k = key.c_str();
    TextEntry* e = texts_api.find(0, &k);
    return e && e->text ? e : nullptr;
}

inline void put_text(const std::string& key, const std::wstring& text) {
    TtlString<char> k{key.c_str(), (uint32_t)key.size(), (uint32_t)key.size()};
    TtlString<wchar_t> t{text.c_str(), (uint32_t)text.size(), (uint32_t)text.size()};
    texts_api.add(texts_api.tools, &k, &t, true);
}

inline void locate_texts(uintptr_t gamedll, HMODULE engine) {
    if (auto hits = game::find_code(gamedll, TOOLS_CODE); !hits.empty()) tools_global = game::rip_target(hits[0], 3, 7);
    if (auto hits = engine ? game::find_code((uintptr_t)engine, TEXT_FIND_CODE, 2) : std::vector<uintptr_t>{}; hits.size() == 1) {
        texts_api.find = (decltype(texts_api.find))hits[0];
        uintptr_t lea = hits[0] + FIND_TABLE_AT;
        if (rdv<uint8_t>(lea) == 0x48 && rdv<uint8_t>(lea + 1) == 0x8D && rdv<uint8_t>(lea + 2) == 0x05) texts_api.table = game::rip_target(lea, 3, 7);
    }
}

inline bool texts_ready() {
    if (tools_global) {
        texts_api.tools = rdv<uintptr_t>(tools_global);
        texts_api.add = texts_api.tools ? (decltype(texts_api.add))rdv<uintptr_t>(rdv<uintptr_t>(texts_api.tools) + SLOT_ADD_TEXT * 8) : nullptr;
    }
    return texts_api.find && texts_api.add && texts_api.tools;
}

inline std::map<char, int> format_specifiers(const std::string& text, bool* broken) {
    std::map<char, int> out;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] != '%') continue;
        if (i + 1 == text.size()) {
            *broken = true;
            break;
        }
        if (text[++i] != '%') out[text[i]]++;
    }
    return out;
}

inline bool safe_format(const std::string& replacement, const std::string& original) {
    bool broken = false, ignored = false;
    auto wanted = format_specifiers(replacement, &broken), allowed = format_specifiers(original, &ignored);
    if (broken) return false;
    for (auto& [spec, count] : wanted)
        if (!allowed.count(spec) || count > allowed[spec]) return false;
    return true;
}

inline std::string narrow(const wchar_t* text) {
    std::string out;
    for (; text && *text; text++) out += *text < 128 ? (char)*text : '?';
    return out;
}

inline bool ensure_text(const std::string& key, const std::wstring& text, const std::string& from) {
    TextEntry* e = find_text(key);
    if (!e) return false;
    auto w = text_written.find(key);
    bool ours = w != text_written.end() && w->second.at == e->text;
    if (ours && w->second.from == from) return false;
    if (!ours) text_originals[key] = e->text;
    put_text(key, text);
    e = find_text(key);
    text_written[key] = {e ? e->text : nullptr, from};
    texts_generation++;
    return true;
}

inline std::string subtitle_code() {
    const std::string& setting = config::cfg.subtitle_language;
    std::string code = setting == config::STEAM_LANGUAGE ? steam_language : setting;
    return code.size() == 2 && language_code(code) != started_language ? language_code(code) : "";
}

inline uint64_t texts_fingerprint() {
    uint64_t h = 0;
    for (int i = 0; texts_api.table && i < TEXT_BUCKETS; i++)
        for (int part = 0; part < 3; part++) h = h * 1000003 + rdv<uint64_t>(texts_api.table + i * TEXT_BUCKET_SIZE + part * 8);
    return h;
}

inline uint64_t texts_seen = 0;
inline DWORD texts_since = 0;

inline bool texts_settled(DWORD now_ms = GetTickCount()) {
    uint64_t now = texts_fingerprint();
    if (now != texts_seen) texts_seen = now, texts_since = now_ms;
    return now_ms - texts_since >= TEXTS_SETTLE_MS;
}

inline void texts_written() { texts_seen = texts_fingerprint(); }

inline const wchar_t* shown_text(const std::string& key) {
    TextEntry* e = find_text(key);
    return e ? e->text : nullptr;
}

inline void keep_texts() {
    if (!texts_ready() || !texts_settled()) return;
    const auto& c = config::cfg;
    std::string code = subtitle_code();
    auto lib = code.empty() ? nullptr : library(code);
    bool full = code != applied_language || lib != applied_library;
    for (size_t i = 0; lib && !full && i < lib->size() && i < sentinels.size(); i++)
        if (!(*lib)[i].texts.empty() && shown_text((*lib)[i].texts[0].key) != sentinels[i]) full = true;
    bool wrote = false;
    for (auto it = user_written.begin(); it != user_written.end();)
        if (c.texts.count(*it)) ++it;
        else it = user_written.erase(it), full = true;
    for (auto& [key, replacement] : c.texts) {
        if (!find_text(key)) continue;
        auto kept = text_originals.find(key);
        if (!safe_format(replacement, kept != text_originals.end() ? narrow(kept->second.c_str()) : narrow(shown_text(key)))) continue;
        wrote |= ensure_text(key, wide(replacement), "user " + replacement);
        user_written.insert(key);
    }
    if (full) {
        std::set<std::string> wanted;
        if (lib)
            for (auto& file : *lib)
                for (auto& t : file.texts)
                    if (!c.texts.count(t.key) && t.kind == TEXT_SPOKEN && wanted.insert(t.key).second) wrote |= ensure_text(t.key, t.text, "language " + code);
        for (auto it = text_originals.begin(); it != text_originals.end();) {
            if (c.texts.count(it->first) || wanted.count(it->first)) {
                ++it;
                continue;
            }
            if (find_text(it->first)) put_text(it->first, it->second), wrote = true;
            text_written.erase(it->first);
            it = text_originals.erase(it);
            texts_generation++;
        }
        sentinels.clear();
        for (size_t i = 0; lib && i < lib->size(); i++) sentinels.push_back((*lib)[i].texts.empty() ? nullptr : shown_text((*lib)[i].texts[0].key));
        applied_language = code, applied_library = lib;
    }
    if (wrote) texts_written();
}

struct Callout { const char* key; const char* before; const char* label; const char* after; const char* tone; };
inline const char POUNCE_TONE[] = "#FF4D6D", FAILED_POUNCE_TONE[] = "#38E1A0", TACKLE_TONE[] = "#FFB000", FAILED_TACKLE_TONE[] = "#5AC8FA", KILL_TONE[] = "#F2EDE4";
inline const Callout CALLOUTS[] = {
    {"HUDMessage_BTZDeath_Killer_Hunter_Highrage", "", "Pounced:", " %s", POUNCE_TONE},
    {"HUDMessage_BTZDeath_Observer_Hunter_Highrage", "%s ", "Pounced:", " %s", POUNCE_TONE},
    {"HUDMessage_BTZDeath_Victim_Hunter_Highrage", "%s ", "Pounced:", " You", POUNCE_TONE},
    {"BtzZombie_PounceHighRageKill_AwardReason", "", "Pounced", "", POUNCE_TONE},
    {"HUDMessage_BTZZombiePounceInterrupt_Attacker_SelfInterrupt", "", "Failed Pounce:", " %s", FAILED_POUNCE_TONE},
    {"HUDMessage_BTZZombiePounceInterrupt_Attacker_AdditionalInterrupt", "%s ", "Failed Pounce:", " %s", FAILED_POUNCE_TONE},
    {"HUDMessage_BTZZombiePounceInterrupt_Interrupter_AdditionalInterrupt", "%s ", "Failed Pounce:", " %s", FAILED_POUNCE_TONE},
    {"HUDMessage_BTZZombiePounceInterrupt_Observer_AdditionalInterrupt", "%s | %s ", "Failed Pounce:", " %s", FAILED_POUNCE_TONE},
    {"HUDMessage_BTZZombiePounceInterrupt_Observer_SelfInterrupt", "%s | %s ", "Failed Pounce", "", FAILED_POUNCE_TONE},
    {"HUDMessage_BTZZombiePounceInterrupt_Vicitm_AdditionalInterrupt", "%s | %s ", "Failed Pounce:", " You", FAILED_POUNCE_TONE},
    {"HUDMessage_BTZZombiePounceInterrupt_Victim_SelfInterrupt", "%s ", "Failed Pounce:", " You", FAILED_POUNCE_TONE},
    {"btz_pounce_interrupted", "", "Failed Pounce", "", FAILED_POUNCE_TONE},
    {"BtzHuman_PounceInterrupted_AwardReason", "", "Failed Pounce", "", FAILED_POUNCE_TONE},
    {"BtzZombie_PounceHighRageInterrupt_AwardReason", "", "Failed Pounce", "", FAILED_POUNCE_TONE},
    {"HUDMessage_BTZDeath_Killer_Hunter_Charge", "", "Tackled:", " %s", TACKLE_TONE},
    {"HUDMessage_BTZDeath_Observer_Hunter_Charge", "%s ", "Tackled:", " %s", TACKLE_TONE},
    {"HUDMessage_BTZDeath_Victim_Hunter_Charge", "%s ", "Tackled:", " You", TACKLE_TONE},
    {"BtzZombie_Charge_AwardReason", "", "Tackled", "", TACKLE_TONE},
    {"HUDMessage_BTZZombieChargeEvade_Attacker", "", "Failed Tackle:", " %s", FAILED_TACKLE_TONE},
    {"HUDMessage_BTZZombieChargeEvade_Observer", "%s | %s ", "Failed Tackle", "", FAILED_TACKLE_TONE},
    {"HUDMessage_BTZZombieChargeEvade_Victim", "%s ", "Failed Tackle:", " You", FAILED_TACKLE_TONE},
    {"BtzZombie_ChargeEvade_AwardReason", "", "Failed Tackle", "", FAILED_TACKLE_TONE},
    {"HUDMessage_BTZDeath_Killer_Hunter_Groundpound", "", "Ground Pound:", " %s", TACKLE_TONE},
    {"HUDMessage_BTZDeath_Observer_Hunter_Groundpound", "%s ", "Ground Pound:", " %s", TACKLE_TONE},
    {"HUDMessage_BTZDeath_Victim_Hunter_Groundpound", "%s ", "Ground Pound:", " You", TACKLE_TONE},
    {"HUDMessage_BTZDeath_Killer_Kick", "", "Kicked:", " %s", FAILED_TACKLE_TONE},
    {"HUDMessage_BTZDeath_Observer_Kick", "%s ", "Kicked:", " %s", FAILED_TACKLE_TONE},
    {"HUDMessage_BTZDeath_Victim_Kick", "%s ", "Kicked:", " You", FAILED_TACKLE_TONE},
    {"HUDMessage_BTZDeath_Killer_Pvp_Human_Jump_Attack", "", "Dropped:", " %s", FAILED_TACKLE_TONE},
    {"HUDMessage_BTZDeath_Observer_Pvp_Human_Jump_Attack", "%s ", "Dropped:", " %s", FAILED_TACKLE_TONE},
    {"HUDMessage_BTZDeath_Victim_Pvp_Human_Jump_Attack", "%s ", "Dropped:", " You", FAILED_TACKLE_TONE},
    {"HUDMessage_BTZDeath_Killer", "", "Killed:", " %s", KILL_TONE},
    {"HUDMessage_BTZDeath_Observer", "%s ", "Killed:", " %s", KILL_TONE},
    {"HUDMessage_BTZDeath_Victim", "%s ", "Killed:", " You", KILL_TONE},
};

inline std::string colored(const std::string& text, const char* tone) { return std::string("<font color=\"") + tone + "\">" + text + "</font>"; }

inline std::string callout_text(const Callout& c, bool tinted) { return std::string(c.before) + (tinted ? colored(c.label, c.tone) : std::string(c.label)) + c.after; }

inline std::string without_colors(std::string text) {
    for (size_t at; (at = text.find("<font")) != std::string::npos;) {
        size_t end = text.find('>', at);
        text.erase(at, end == std::string::npos ? std::string::npos : end - at + 1);
    }
    for (size_t at; (at = text.find("</font>")) != std::string::npos;) text.erase(at, 7);
    return text;
}

inline int callouts_used(const std::map<std::string, std::string>& texts) {
    int n = 0;
    for (auto& c : CALLOUTS) {
        auto it = texts.find(c.key);
        n += it != texts.end() && (it->second == callout_text(c, false) || it->second == callout_text(c, true));
    }
    return n;
}

struct TextRow { std::string key, shown, english, haystack; int kind; };
struct TextRows { std::vector<TextRow> rows; std::unordered_map<std::string, size_t> index; };
inline std::mutex rows_mx;
inline std::shared_ptr<const TextRows> text_rows;
inline unsigned rows_generation = ~0u;
inline DWORD rows_at = 0;
inline std::atomic<DWORD> rows_wanted_at{0};
const DWORD ROWS_REFRESH_MS = 10000, ROWS_WANTED_MS = 2000;

inline std::string lowered(std::string s) {
    for (auto& ch : s) ch = (char)tolower((unsigned char)ch);
    return s;
}

inline std::shared_ptr<const TextRows> rows() {
    rows_wanted_at = GetTickCount();
    std::lock_guard<std::mutex> l(rows_mx);
    return text_rows;
}

inline void keep_text_rows() {
    if (GetTickCount() - rows_wanted_at > ROWS_WANTED_MS) return;
    auto english = library("En");
    bool live = texts_ready();
    if (!english || (rows_generation == texts_generation && GetTickCount() - rows_at < ROWS_REFRESH_MS)) return;
    auto built = std::make_shared<TextRows>();
    for (auto& file : *english)
        for (auto& t : file.texts) {
            if (!built->index.emplace(t.key, built->rows.size()).second) continue;
            const wchar_t* shown = live ? shown_text(t.key) : nullptr;
            std::string english_text = utf8(t.text.c_str()), shown_now = shown ? utf8(shown) : english_text;
            built->rows.push_back({t.key, shown_now, english_text, lowered(t.key + "\n" + shown_now + "\n" + english_text), t.kind});
        }
    std::lock_guard<std::mutex> l(rows_mx);
    text_rows = built;
    rows_generation = texts_generation, rows_at = GetTickCount();
}

}
