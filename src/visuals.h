#pragma once
#include <windows.h>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "config.h"
#include "game.h"

namespace visuals {

using game::rdv;

enum UiKind { UI_NONE, UI_VIEW, UI_TEXT };

struct Ui {
    int(__fastcall* children_count)(uintptr_t element) = nullptr;
    uintptr_t(__fastcall* child)(uintptr_t element, int index) = nullptr;
    float*(__fastcall* get_color)(uintptr_t view, float* out) = nullptr;
    void(__fastcall* set_color)(uintptr_t view, const float* rgb) = nullptr;
    float*(__fastcall* get_text_color)(uintptr_t text, float* out) = nullptr;
    void(__fastcall* set_text_color)(uintptr_t text, const float* rgb) = nullptr;
    std::string (*name_of)(uintptr_t element) = nullptr;
    int (*kind_of)(uintptr_t element, uintptr_t* at) = nullptr;
    uintptr_t (*vtable_of)(uintptr_t element) = nullptr;
};

inline Ui ui;

struct HudPart { const char* key; const char* label; std::vector<const char*> groups; };

inline const HudPart HUD_PARTS[] = {
    {"health", "Health", {"health_wrap", "HudMedkit"}},
    {"stamina", "Stamina", {"HudStamina"}},
    {"crosshair", "Crosshair", {"HudCrosshair", "HudRopeReticle", "HudThrowTargetIndicator"}},
    {"weapons", "Weapons and ammo", {"HudPrimaryWeaponIndicator", "HudSecondaryWeaponIndicator", "HudGrenadeIndicator", "HudReloadNotify"}},
    {"xp", "XP", {"HudXpBars", "HudDriverXp", "HudNightReward"}},
    {"messages", "Killfeed and messages", {"HudMpMessages", "HudBigMessages"}},
    {"map", "Radar and objectives", {"HudRadar", "StackObjectives", "Hud3dObjective"}},
    {"enemies", "Enemy health", {"HudEnemyHp", "HudBossHpBar"}},
    {"hunter", "Night Hunter", {"HudHunterInfo", "HudPlayersIndicator", "HudZombieHiveCounter"}},
};
const int HUD_PART_COUNT = (int)(sizeof HUD_PARTS / sizeof HUD_PARTS[0]);
const int HUD_MAX_DEPTH = 32, HUD_MAX_ELEMENTS = 20000;

struct HudNode {
    uintptr_t element, vtable, at;
    int kind, part;
    float original[3];
    bool wrote, dead;
};

inline std::vector<HudNode> hud_nodes;
inline uintptr_t hud_root = 0;

inline int part_of(const std::string& name, int inherited) {
    for (int p = 0; p < HUD_PART_COUNT; p++)
        for (const char* g : HUD_PARTS[p].groups)
            if (name == g) return p;
    return inherited;
}

inline void walk_hud(uintptr_t element, int part, int depth) {
    if (depth > HUD_MAX_DEPTH || !ui.children_count) return;
    int n = ui.children_count(element);
    for (int i = 0; i < n && (int)hud_nodes.size() < HUD_MAX_ELEMENTS; i++) {
        uintptr_t c = ui.child(element, i);
        if (!c) continue;
        int p = part_of(ui.name_of(c), part);
        uintptr_t at = 0;
        int kind = ui.kind_of(c, &at);
        if (kind != UI_NONE) {
            HudNode node{c, ui.vtable_of(c), at, kind, p, {}, false, false};
            (kind == UI_TEXT ? ui.get_text_color : ui.get_color)(at, node.original);
            hud_nodes.push_back(node);
        }
        walk_hud(c, p, depth + 1);
    }
}

inline void forget_hud() { hud_nodes.clear(), hud_root = 0; }

inline void find_hud(uintptr_t root) {
    forget_hud();
    if (!root) return;
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

inline void apply_hud() {
    const auto& tint = config::cfg.hud_tint;
    for (auto& n : hud_nodes) {
        if (n.dead) continue;
        if (ui.vtable_of(n.element) != n.vtable) {
            n.dead = true;
            continue;
        }
        float want[3];
        const config::HudColor* own = part_color(n.part);
        for (int c = 0; c < 3; c++) want[c] = own ? own->color[c] : tint.on ? n.original[c] * tint.color[c] : n.original[c];
        bool is_original = !memcmp(want, n.original, sizeof want);
        if (!n.wrote && is_original) continue;
        float now[3];
        (n.kind == UI_TEXT ? ui.get_text_color : ui.get_color)(n.at, now);
        if (memcmp(want, now, sizeof want)) (n.kind == UI_TEXT ? ui.set_text_color : ui.set_color)(n.at, want);
        n.wrote = !is_original;
    }
}

const int UI_ENGINE_OBJECT = 0x18, ENGINE_NAME = 0x8, NAME_LIMIT = 64;
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

inline bool locate_engine(HMODULE engine) {
    if (!engine) return false;
    auto get = [&](const char* name) { return (void*)GetProcAddress(engine, name); };
    ui.children_count = (decltype(ui.children_count))get("?GetChildrenCount@IUIElement@@QEAAHXZ");
    ui.child = (decltype(ui.child))get("?GetChildren@IUIElement@@QEAAPEAV1@H@Z");
    ui.get_color = (decltype(ui.get_color))get("?GetColor@IUIElementView@@QEBA?AVvec3@@XZ");
    ui.set_color = (decltype(ui.set_color))get("?SetColor@IUIElementView@@QEAAXAEBVvec3@@@Z");
    ui.get_text_color = (decltype(ui.get_text_color))get("?GetTextColor@IUIText@@QEAA?AVvec3@@XZ");
    ui.set_text_color = (decltype(ui.set_text_color))get("?SetTextColor@IUIText@@QEAAXAEBVvec3@@@Z");
    ui.name_of = engine_name_of, ui.kind_of = engine_kind_of, ui.vtable_of = engine_vtable_of;
    return ui.children_count && ui.child && ui.get_color && ui.set_color && ui.get_text_color && ui.set_text_color;
}

inline void locate(uintptr_t gamedll) {
    hud_element_offset = game::g.vt_hud ? game::base_offset(gamedll, game::g.vt_hud, "IUIElement") : -1;
}

inline bool hud_colors_on() {
    if (config::cfg.hud_tint.on) return true;
    for (auto& [key, h] : config::cfg.hud_parts)
        if (h.on) return true;
    return false;
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
    bool on = hud_colors_on();
    if ((!on && !was_on) || !ui.children_count) return;
    if (!hud_root || rdv<uintptr_t>(hud_root) != hud_root_vtable) {
        uintptr_t root = live_hud_root();
        if (root != hud_root || !hud_found()) {
            find_hud(root);
            hud_root_vtable = root ? rdv<uintptr_t>(root) : 0;
            if (root) log("hud: found %zu elements", hud_elements());
        }
    }
    apply_hud();
    was_on = on;
}

}
