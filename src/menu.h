#pragma once
#include <algorithm>
#include <cmath>
#include "app.h"
#include "imgui.h"

namespace menu {

inline ImFont *f_body, *f_small, *f_head, *f_big, *f_brand;

struct Theme { ImVec4 accent, accent_soft, bg, side, card, hover, line, text, dim, ok, bad; };
inline Theme T;

inline float S(float v) { return v * config::cfg.scale; }
inline ImU32 col(ImVec4 c, float alpha = -1) {
    if (alpha >= 0) c.w = alpha;
    return ImGui::GetColorU32(c);
}

inline void apply_style() {
    auto& a = config::cfg.accent;
    T.accent = {a[0], a[1], a[2], 1};
    T.accent_soft = {a[0], a[1], a[2], 0.14f};
    T.bg = {0.043f, 0.047f, 0.058f, 0.985f};
    T.side = {0.058f, 0.063f, 0.077f, 1};
    T.card = {0.080f, 0.087f, 0.104f, 1};
    T.hover = {0.102f, 0.110f, 0.130f, 1};
    T.line = {1, 1, 1, 0.06f};
    T.text = {0.93f, 0.94f, 0.96f, 1};
    T.dim = {0.54f, 0.58f, 0.65f, 1};
    T.ok = {0.35f, 0.82f, 0.55f, 1};
    T.bad = {0.93f, 0.37f, 0.37f, 1};
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = S(16), s.ChildRounding = S(12), s.FrameRounding = S(8), s.PopupRounding = S(12);
    s.GrabRounding = S(8), s.ScrollbarRounding = S(8), s.TabRounding = S(8);
    s.WindowBorderSize = 1, s.ChildBorderSize = 0, s.FrameBorderSize = 0, s.PopupBorderSize = 1;
    s.WindowPadding = {0, 0}, s.FramePadding = {S(12), S(8)}, s.ItemSpacing = {S(10), S(10)};
    s.ItemInnerSpacing = {S(8), S(6)}, s.CellPadding = {S(10), S(8)}, s.ScrollbarSize = S(8), s.GrabMinSize = S(12);
    ImVec4* c = s.Colors;
    ImVec4 accent_mid = {a[0], a[1], a[2], 0.55f};
    c[ImGuiCol_WindowBg] = T.bg;
    c[ImGuiCol_ChildBg] = {0, 0, 0, 0};
    c[ImGuiCol_PopupBg] = {0.06f, 0.065f, 0.08f, 0.99f};
    c[ImGuiCol_Border] = T.line;
    c[ImGuiCol_ModalWindowDimBg] = {0, 0, 0, 0.5f};
    c[ImGuiCol_Text] = T.text;
    c[ImGuiCol_TextDisabled] = T.dim;
    c[ImGuiCol_FrameBg] = {0.12f, 0.13f, 0.155f, 1};
    c[ImGuiCol_FrameBgHovered] = {0.15f, 0.16f, 0.19f, 1};
    c[ImGuiCol_FrameBgActive] = {0.17f, 0.18f, 0.21f, 1};
    c[ImGuiCol_Button] = {0.13f, 0.14f, 0.165f, 1};
    c[ImGuiCol_ButtonHovered] = {0.17f, 0.18f, 0.215f, 1};
    c[ImGuiCol_ButtonActive] = accent_mid;
    c[ImGuiCol_Header] = T.accent_soft;
    c[ImGuiCol_HeaderHovered] = {1, 1, 1, 0.05f};
    c[ImGuiCol_HeaderActive] = accent_mid;
    c[ImGuiCol_Separator] = T.line;
    c[ImGuiCol_ScrollbarBg] = {0, 0, 0, 0};
    c[ImGuiCol_ScrollbarGrab] = {1, 1, 1, 0.10f};
    c[ImGuiCol_ScrollbarGrabHovered] = {1, 1, 1, 0.18f};
    c[ImGuiCol_ScrollbarGrabActive] = accent_mid;
    c[ImGuiCol_CheckMark] = c[ImGuiCol_SliderGrab] = c[ImGuiCol_SliderGrabActive] = T.accent;
    c[ImGuiCol_TableRowBg] = {0, 0, 0, 0};
    c[ImGuiCol_TableRowBgAlt] = {1, 1, 1, 0.02f};
    c[ImGuiCol_TableBorderLight] = T.line;
    c[ImGuiCol_NavCursor] = T.accent;
    c[ImGuiCol_TextSelectedBg] = {a[0], a[1], a[2], 0.35f};
    c[ImGuiCol_ResizeGrip] = {0, 0, 0, 0};
    c[ImGuiCol_ResizeGripHovered] = accent_mid;
    c[ImGuiCol_ResizeGripActive] = T.accent;
    ImGui::GetIO().FontGlobalScale = config::cfg.scale;
}

inline std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

inline std::string thousands(long long v) {
    std::string s = std::to_string(v < 0 ? -v : v);
    for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert(i, ",");
    return (v < 0 ? "-" : "") + s;
}

inline float animate(ImGuiID id, float target, float speed = 14) {
    float& v = *ImGui::GetStateStorage()->GetFloatRef(id, target);
    v += (target - v) * std::min(1.0f, ImGui::GetIO().DeltaTime * speed);
    return v;
}

inline ImVec4 mix(ImVec4 a, ImVec4 b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t}; }

inline void label(const char* text) {
    ImGui::PushFont(f_small);
    ImGui::PushStyleColor(ImGuiCol_Text, T.dim);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

inline bool accent_button(const char* text, ImVec2 size = {0, 0}) {
    ImGui::PushStyleColor(ImGuiCol_Button, T.accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, mix(T.accent, {1, 1, 1, 1}, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, mix(T.accent, {0, 0, 0, 1}, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(15, 15, 18, 255));
    bool r = ImGui::Button(text, size);
    ImGui::PopStyleColor(4);
    return r;
}

inline void begin_card(const char* id, const char* title = nullptr) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, T.card);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(16), S(14)});
    ImGui::BeginChild(id, {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    if (title) {
        label(title);
        ImGui::Dummy({0, S(2)});
    }
}

inline void end_card() {
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::Dummy({0, S(4)});
}

inline bool switch_row(const char* text, const char* hint, bool& value) {
    ImGui::PushID(text);
    float w = ImGui::GetContentRegionAvail().x, h = S(hint && *hint ? 58 : 42);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton("row", {w, h});
    if (clicked) value = !value;
    bool hovered = ImGui::IsItemHovered();
    auto* dl = ImGui::GetWindowDrawList();
    float hover = animate(ImGui::GetID("hover"), hovered ? 1.0f : 0.0f);
    if (hover > 0.01f) dl->AddRectFilled({p.x - S(8), p.y}, {p.x + w + S(8), p.y + h}, col({1, 1, 1, 0.035f * hover}), S(10));
    float body = ImGui::GetFontSize(), small = f_small->FontSize * config::cfg.scale;
    float text_y = p.y + (hint && *hint ? S(9) : (h - body) / 2);
    dl->AddText(f_body, body, {p.x + S(4), text_y}, col(T.text), text);
    if (hint && *hint) dl->AddText(f_small, small, {p.x + S(4), text_y + body + S(3)}, col(T.dim), hint);
    float sw = S(44), sh = S(24), t = animate(ImGui::GetID("knob"), value ? 1.0f : 0.0f);
    ImVec2 s0 = {p.x + w - sw - S(4), p.y + (h - sh) / 2};
    dl->AddRectFilled(s0, {s0.x + sw, s0.y + sh}, col(mix({0.2f, 0.215f, 0.25f, 1}, T.accent, t)), sh / 2);
    dl->AddCircleFilled({s0.x + sh / 2 + t * (sw - sh), s0.y + sh / 2}, sh / 2 - S(3), col(mix({0.75f, 0.77f, 0.8f, 1}, {1, 1, 1, 1}, t)));
    ImGui::PopID();
    return clicked;
}

inline void cheat_switch(const char* key, const char* override_label = nullptr) {
    auto* c = cheats::find(key);
    if (!c) return;
    bool v = c->on;
    if (switch_row(override_label ? override_label : c->label, c->hint, v)) {
        c->on = v;
        save_config();
    }
}

inline void stat_tile(const char* id, const char* name, float value, float full, ImVec4 color, float width) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, T.card);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(16), S(14)});
    ImGui::BeginChild(id, {width, S(104)}, ImGuiChildFlags_AlwaysUseWindowPadding);
    label(name);
    ImGui::PushFont(f_head);
    if (std::isnan(value)) ImGui::TextDisabled("--");
    else ImGui::Text("%.0f", value);
    ImGui::PopFont();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x, frac = full > 0 && !std::isnan(value) ? std::clamp(value / full, 0.0f, 1.0f) : 0;
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled({p.x, p.y + S(4)}, {p.x + w, p.y + S(10)}, col({1, 1, 1, 0.07f}), S(3));
    dl->AddRectFilled({p.x, p.y + S(4)}, {p.x + w * animate(ImGui::GetID("bar"), frac, 6), p.y + S(10)}, col(color), S(3));
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

inline void note(const char* text) {
    ImGui::PushFont(f_small);
    ImGui::PushStyleColor(ImGuiCol_Text, T.dim);
    ImGui::PushTextWrapPos(0);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

inline void empty_state(const char* text) {
    ImGui::Dummy({0, S(24)});
    note(text);
}

struct Cat { const char* label; ImU32 color; };
enum { C_ALL, C_WEAPONS, C_THROWABLE, C_HEALING, C_CRAFTING, C_UPGRADES, C_BLUEPRINTS, C_OTHER, C_COUNT };
inline const char* CAT_NAMES[C_COUNT] = {"All", "Weapons", "Throwables", "Healing", "Crafting", "Upgrades", "Blueprints", "Other"};

inline int cat_of(const char* id) {
    std::string s = id;
    auto has = [&](const char* p) { return s.find(p) != std::string::npos; };
    if (!strncmp(id, "Craftplan_", 10)) return C_BLUEPRINTS;
    if (has("Upgrade") || !strncmp(id, "CWU_", 4)) return C_UPGRADES;
    if (has("Firearm") || has("Bow") || has("Ammo") || has("Melee")) return C_WEAPONS;
    if (has("Throwable")) return C_THROWABLE;
    if (has("Medkit") || has("Potion") || has("Booster")) return C_HEALING;
    if (has("Craft")) return C_CRAFTING;
    return C_OTHER;
}

inline Cat category(const char* id) {
    switch (cat_of(id)) {
        case C_WEAPONS: return {strstr(id, "Melee") ? "Melee" : "Firearm", strstr(id, "Melee") ? IM_COL32(243, 154, 30, 255) : IM_COL32(80, 160, 245, 255)};
        case C_THROWABLE: return {"Throwable", IM_COL32(170, 110, 245, 255)};
        case C_HEALING: return {"Healing", IM_COL32(235, 90, 90, 255)};
        case C_CRAFTING: return {"Crafting", IM_COL32(95, 200, 120, 255)};
        case C_UPGRADES: return {"Upgrade", IM_COL32(60, 205, 200, 255)};
        case C_BLUEPRINTS: return {"Blueprint", IM_COL32(235, 205, 90, 255)};
    }
    return {"Gear", IM_COL32(150, 155, 165, 255)};
}

inline bool is_test_item(const char* id) {
    for (const char* p : {"AAA", "TEST", "Test_", "_test", "zzz", "ZZZZ", "Dev", "DEBUG", "Debug"})
        if (strstr(id, p)) return true;
    return false;
}

inline void item_label(const char* name, const char* id) {
    Cat c = category(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float row = S(44);
    ImGui::GetWindowDrawList()->AddRectFilled({p.x, p.y + S(6)}, {p.x + S(3), p.y + row - S(2)}, c.color, S(2));
    ImGui::SetCursorScreenPos({p.x + S(14), p.y + S(3)});
    ImGui::TextUnformatted(name);
    ImGui::SetCursorScreenPos({p.x + S(14), p.y + S(26)});
    ImGui::PushFont(f_small);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(c.color), "%s", c.label);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", id);
    ImGui::PopFont();
}

inline uintptr_t g_edit_desc = 0, g_edit_item = 0;
inline std::string g_edit_name, g_edit_id;
inline bool g_edit_request = false;
inline const char* RARITY[] = {"Gray", "Green", "Blue", "Purple", "Orange", "Gold"};

inline bool editable(const game::Item& it) {
    if (!it.info) return false;
    int c = cat_of(it.info->id);
    return c == C_WEAPONS || c == C_THROWABLE;
}

inline void open_editor(const game::Item& it, const std::string& id) {
    g_edit_desc = game::item_desc(it), g_edit_item = it.addr, g_edit_name = it.name, g_edit_id = id;
    g_edit_request = true;
}

inline void edit_stat(int stat, float v) {
    logf("edit: %s %s %g -> %g", g_edit_id.c_str(), STAT_KEYS[stat], game::get_stat(g_edit_desc, stat), v);
    game::set_stat(g_edit_desc, stat, v);
}

inline void editor_popup() {
    if (g_edit_request) {
        ImGui::OpenPopup("Edit weapon");
        g_edit_request = false;
    }
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_Appearing, {0.5f, 0.5f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(24), S(20)});
    if (!ImGui::BeginPopupModal("Edit weapon", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PopStyleVar();
        return;
    }
    ImGui::PushFont(f_head);
    ImGui::TextUnformatted(g_edit_name.c_str());
    ImGui::PopFont();
    label((g_edit_id + "  -  stats apply to every item of this type while the trainer runs, rarity to this weapon").c_str());
    ImGui::Dummy({0, S(4)});
    struct Row { int stat; const char* name; const char* fmt; float mul; };
    const Row rows[] = {{ST_Damage, "Damage", "%.1f", 2},          {ST_Condition, "Durability", "%.0f", 10},
                        {ST_CriticalProb, "Critical chance", "%.2f", 2}, {ST_CriticalDamage, "Critical damage", "%.2f", 2},
                        {ST_Force, "Knockback force", "%.1f", 2},  {ST_StaminaUsage, "Stamina per swing", "%.3f", 0.5f},
                        {ST_DamageRange, "Reach", "%.2f", 1.5f},   {ST_UpgradeLevel, "Upgrade level", "%.0f", 0},
                        {ST_AllowedRepairs, "Repairs left", "%.0f", 0}, {ST_MaxStackCount, "Max stack", "%.0f", 0},
                        {ST_AmmoCount, "Magazine size", "%.0f", 2}, {ST_ReloadTime, "Reload time", "%.2f", 0.5f},
                        {ST_Price, "Price", "%.0f", 0}};
    int shown = 0;
    if (ImGui::BeginTable("stats", 3, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, S(190));
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, S(180));
        ImGui::TableSetupColumn("quick", ImGuiTableColumnFlags_WidthFixed, S(70));
        for (auto& r : rows) {
            if (!game::has_stat(r.stat)) continue;
            float v = game::get_stat(g_edit_desc, r.stat);
            if (std::isnan(v)) continue;
            shown++;
            ImGui::PushID(r.stat);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(r.name);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(S(170));
            float e = v;
            ImGui::InputFloat("##v", &e, 0, 0, r.fmt);
            if (ImGui::IsItemDeactivatedAfterEdit()) edit_stat(r.stat, e);
            ImGui::TableNextColumn();
            char b[16];
            snprintf(b, sizeof b, r.mul > 0 ? "x%g" : "+1", r.mul);
            if (ImGui::Button(b, {S(64), 0})) edit_stat(r.stat, r.mul > 0 ? v * r.mul : v + 1);
            ImGui::PopID();
        }
        {
            int cur = game::rarity(g_edit_item, g_edit_desc);
            if (cur >= 0 && cur < 6) {
                shown++;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Rarity");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(S(170));
                if (ImGui::Combo("##rarity", &cur, RARITY, 6)) {
                    logf("edit: %s rarity -> %s", g_edit_id.c_str(), RARITY[cur]);
                    game::set_rarity(g_edit_item, g_edit_desc, cur);
                }
            }
        }
        ImGui::EndTable();
    }
    if (!shown) note("No editable stats found for this item. Press Refresh after loading your save and check fatrainer.log.");
    ImGui::Dummy({0, S(6)});
    if (accent_button("Done", {S(120), 0})) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
    ImGui::PopStyleVar();
}

inline void search_box(const char* id, const char* hint, char* buf, size_t size) {
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint(id, hint, buf, size);
}

inline void inventory_page(game::Kind kind) {
    static char filter[64] = "";
    search_box("##f", "Search items...", filter, sizeof filter);
    std::string f = lower(filter);
    int shown_inv = 0;
    ImGui::BeginChild("list", {0, 0});
    for (size_t i = 0; i < game::g.invs.size(); i++) {
        auto& inv = game::g.invs[i];
        if (inv.kind != kind || (kind == game::K_STASH && shown_inv)) continue;
        shown_inv++;
        std::string title = std::to_string(inv.items.size()) + " ITEMS";
        if (inv.capacity > 0) title += "   /   " + std::to_string(inv.capacity) + " SLOTS";
        ImGui::Dummy({0, S(2)});
        label(title.c_str());
        if (!ImGui::BeginTable(("t" + std::to_string(i)).c_str(), 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) continue;
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Amount", ImGuiTableColumnFlags_WidthFixed, S(250));
        for (size_t j = 0; j < inv.items.size(); j++) {
            auto& it = inv.items[j];
            const char* id = it.info ? it.info->id : "";
            if (!f.empty() && lower(it.name + " " + id).find(f) == std::string::npos) continue;
            ImGui::PushID((int)(i * 10000 + j));
            ImGui::TableNextRow(0, S(50));
            ImGui::TableNextColumn();
            item_label(it.name.c_str(), id);
            ImGui::TableNextColumn();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + S(6));
            int v = game::count(it);
            float bw = ImGui::GetFrameHeight();
            if (ImGui::Button("-", {bw, 0})) game::set_count(it, std::max(0, v - 1));
            ImGui::SameLine(0, S(4));
            ImGui::SetNextItemWidth(S(70));
            int e = v;
            ImGui::InputInt("##c", &e, 0, 0);
            if (ImGui::IsItemDeactivatedAfterEdit()) game::set_count(it, e);
            ImGui::SameLine(0, S(4));
            if (ImGui::Button("+", {bw, 0})) game::set_count(it, v + 1);
            ImGui::SameLine(0, S(8));
            static bool auto_edit = getenv("DLT_EDIT") != nullptr;
            if (auto_edit && editable(it) && strstr(id, "Machete")) {
                open_editor(it, id);
                auto_edit = false;
            }
            if (editable(it)) {
                if (ImGui::Button("Edit", {S(64), 0})) open_editor(it, id);
            } else if (ImGui::Button("Set 99", {S(64), 0})) {
                game::set_count(it, 99);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (!shown_inv) empty_state("Nothing here yet. Load into your save, then press Refresh.");
    editor_popup();
    ImGui::EndChild();
}

inline void give_item(const ItemInfo* info, int amount, int target) {
    auto d = game::g.descs.find(info->id);
    uintptr_t desc = d == game::g.descs.end() ? 0 : d->second;
    std::string name = game::display_name(info);
    if (!desc) return toast("Not available in this game session: " + name);
    game::Kind k = target < 0 ? game::default_target(info->id) : (game::Kind)target;
    on_game_thread([=] {
        int into = game::give_anywhere(k, desc, amount);
        logf("give %s x%d -> %s", info->id, amount, into < 0 ? "refused by every inventory" : game::KIND_NAMES[into]);
        toast(into < 0 ? "The game refused " + name + ". Load your save, press Refresh and try again."
                       : "Added " + std::to_string(amount) + " x " + name + " to " + game::KIND_NAMES[into]);
        request_refresh();
    });
}

inline bool chip(const char* text, bool active) {
    ImGui::PushStyleColor(ImGuiCol_Button, active ? T.accent_soft : ImVec4{1, 1, 1, 0.04f});
    ImGui::PushStyleColor(ImGuiCol_Text, active ? T.accent : T.text);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(20));
    bool r = ImGui::Button(text);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    return r;
}

inline void give_page() {
    static char filter[64] = "";
    static int cat = C_ALL, amount = 1, target = -1;
    static bool show_test = false;
    note("Choose an amount and press Give. Each item goes where the game keeps it (backpack, materials, ammo...), so it shows up in your inventory right away.");
    ImGui::Dummy({0, S(2)});
    search_box("##gf", "Search every item in the game...", filter, sizeof filter);
    for (int c = 0; c < C_COUNT; c++) {
        if (chip(CAT_NAMES[c], cat == c)) cat = c;
        ImGui::SameLine(0, S(6));
    }
    ImGui::NewLine();
    ImGui::SetNextItemWidth(S(130));
    ImGui::InputInt("Amount", &amount, 1, 10);
    amount = std::clamp(amount, 1, 9999);
    ImGui::SameLine(0, S(24));
    const char* targets[] = {"Auto", "Backpack", "Stash", "Materials"};
    int ti = target < 0 ? 0 : target == game::K_BACKPACK ? 1 : target == game::K_STASH ? 2 : 3;
    ImGui::SetNextItemWidth(S(150));
    if (ImGui::Combo("Put in", &ti, targets, 4)) target = ti == 0 ? -1 : ti == 1 ? game::K_BACKPACK : ti == 2 ? game::K_STASH : game::K_MATERIALS;
    ImGui::SameLine(0, S(24));
    ImGui::Checkbox("Show unused items", &show_test);
    std::string f = lower(filter);
    std::vector<const ItemInfo*> list;
    for (int i = 0; i < ITEM_COUNT; i++) {
        const ItemInfo* info = &ITEMS[i];
        if (!show_test && (!info->name[0] || is_test_item(info->id))) continue;
        if (cat != C_ALL && cat_of(info->id) != cat) continue;
        if (!f.empty() && lower(game::display_name(info) + " " + info->id).find(f) == std::string::npos) continue;
        list.push_back(info);
    }
    if (cat == C_BLUEPRINTS && !list.empty() && accent_button(("Give all " + std::to_string(list.size()) + " blueprints").c_str()))
        for (auto* info : list) give_item(info, 1, target);
    ImGui::BeginChild("give", {0, 0});
    if (ImGui::BeginTable("g", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Give", ImGuiTableColumnFlags_WidthFixed, S(90));
        ImGuiListClipper clip;
        clip.Begin((int)list.size(), S(50) + ImGui::GetStyle().CellPadding.y * 2);
        while (clip.Step())
            for (int i = clip.DisplayStart; i < clip.DisplayEnd; i++) {
                auto* info = list[i];
                ImGui::PushID(i);
                ImGui::TableNextRow(0, S(50));
                ImGui::TableNextColumn();
                item_label(game::display_name(info).c_str(), info->id);
                ImGui::TableNextColumn();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + S(6));
                if (accent_button("Give", {S(80), 0})) give_item(info, amount, target);
                ImGui::PopID();
            }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

inline int cash() {
    int best = -1;
    for (uintptr_t w : game::g.wallets) best = std::max(best, game::money(w));
    return best;
}

inline void set_cash(int amount) {
    amount = std::clamp(amount, 0, 999999999);
    for (uintptr_t w : game::g.wallets) game::set_money(w, amount);
}

inline void cash_page() {
    if (game::g.wallets.empty()) return empty_state("No money found yet. Load into your save, then press Refresh.");
    static int custom = 100000;
    int now = cash();
    begin_card("wallet", "CASH");
    ImGui::PushFont(f_big);
    ImGui::TextUnformatted(("$" + thousands(now)).c_str());
    ImGui::PopFont();
    ImGui::Dummy({0, S(2)});
    for (int a : {1000, 10000, 100000}) {
        if (ImGui::Button(("+" + thousands(a)).c_str())) set_cash(now + a);
        ImGui::SameLine();
    }
    if (ImGui::Button("Max")) set_cash(9999999);
    ImGui::SameLine(0, S(24));
    ImGui::SetNextItemWidth(S(150));
    ImGui::InputInt("##custom", &custom, 0, 0);
    ImGui::SameLine();
    if (accent_button("Set")) set_cash(custom);
    end_card();
    note("Type an amount and press Set, or use the quick buttons. The new amount shows in the game's inventory screen.");
}

inline void tweak_slider(cheats::Tweak& t) {
    ImGui::PushID(t.key);
    float v = t.factor;
    ImGui::TextUnformatted(t.label);
    if (v != 1.0f) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset")) v = 1.0f, t.factor = v, save_config();
    }
    note(t.hint);
    ImGui::SetNextItemWidth(-1);
    char text[32];
    if (v == 1.0f) snprintf(text, sizeof text, "Normal");
    else if (v >= t.max) snprintf(text, sizeof text, "Max");
    else if (!t.vars.empty()) snprintf(text, sizeof text, "%.0f%%%%", (v - 1.0f) / (t.max - 1.0f) * 100);
    else snprintf(text, sizeof text, "x%.1f", v);
    if (ImGui::SliderFloat("##f", &v, 1.0f, t.max, text, ImGuiSliderFlags_NoInput)) t.factor = v;
    if (ImGui::IsItemDeactivatedAfterEdit()) save_config();
    ImGui::Dummy({0, S(6)});
    ImGui::PopID();
}

inline void tweak_sliders(cheats::Group group) {
    for (auto& t : cheats::TWEAKS)
        if (t.group == group) tweak_slider(t);
}

inline void player_page() {
    if (!cheats::player) {
        begin_card("missing");
        note("Your character was not found yet. Load into your save, then press Refresh.");
        end_card();
    } else {
        static float best_health = 0;
        if (cheats::health() > best_health) best_health = cheats::health();
        float tile = (ImGui::GetContentRegionAvail().x - S(12)) / 2;
        stat_tile("health", "HEALTH", cheats::health(), best_health, T.bad, tile);
        ImGui::SameLine(0, S(12));
        stat_tile("stamina", "STAMINA", cheats::stamina(), cheats::stamina_full(), T.ok, tile);
        ImGui::Dummy({0, S(2)});
        if (accent_button("Refill health & stamina", {ImGui::GetContentRegionAvail().x, S(38)})) on_game_thread([] {
            std::lock_guard<std::mutex> l(game::mx);
            cheats::refill();
        });
        ImGui::Dummy({0, S(6)});
    }
    begin_card("survival", "SURVIVAL");
    cheat_switch("god");
    cheat_switch("stamina");
    end_card();
    begin_card("gear", "GEAR");
    cheat_switch("hook");
    cheat_switch("uv");
    tweak_sliders(cheats::G_GEAR);
    cheat_switch("lockpick");
    end_card();
    begin_card("movement", "MOVEMENT");
    cheat_switch("no_fall");
    ImGui::Dummy({0, S(4)});
    tweak_sliders(cheats::G_MOVEMENT);
    end_card();
}

inline void kill_all_button() {
    if (!accent_button("Kill all enemies nearby", {ImGui::GetContentRegionAvail().x, S(38)})) return;
    std::thread([] {
        cheats::scan_enemies();
        on_game_thread([] {
            std::lock_guard<std::mutex> l(game::mx);
            int n = cheats::kill_enemies(cheats::KILL_RADIUS);
            toast(n ? "Killed " + std::to_string(n) + (n == 1 ? " enemy" : " enemies") : std::string("No living enemies within 80 m"));
        });
    }).detach();
}

inline void combat_page() {
    begin_card("enemies", "ENEMIES");
    cheat_switch("one_hit");
    ImGui::Dummy({0, S(4)});
    kill_all_button();
    note("Kills every zombie and human within 80 m the way the game's own scripts do, so quest objectives count them. Friendly NPCs nearby die too.");
    end_card();
    begin_card("supplies", "AMMO & SUPPLIES");
    cheat_switch("ammo");
    cheat_switch("no_reload");
    cheat_switch("supplies");
    end_card();
    begin_card("weapons", "WEAPONS");
    cheat_switch("durability");
    end_card();
    note("To change damage, durability, rarity and more of one weapon, press Edit next to it on the Backpack or Stash page. Weapon stat edits from the Backpack and Stash pages stay applied while the trainer runs.");
}

inline std::string key_name(int vk) {
    LONG code = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC) << 16;
    if ((vk >= VK_PRIOR && vk <= VK_DOWN) || vk == VK_INSERT || vk == VK_DELETE) code |= 1 << 24;
    char b[64] = "";
    if (!GetKeyNameTextA(code, b, sizeof b)) snprintf(b, sizeof b, "Key %d", vk);
    return b;
}

inline bool pressed_key(int& vk) {
    for (int k = 8; k < 255; k++) {
        if (k == VK_LBUTTON || k == VK_RBUTTON || k == VK_MBUTTON || k == VK_XBUTTON1 || k == VK_XBUTTON2) continue;
        if (k == VK_SHIFT || k == VK_CONTROL || k == VK_MENU) continue;
        if (GetAsyncKeyState(k) & 0x8000) {
            vk = k;
            return true;
        }
    }
    return false;
}

inline const char* SPIT_NAMES[config::SPIT_KEYS] = {"Horde Summoner spit", "UV Suppressor spit", "Sense Suppressor spit", "Toxic spit"};
inline const char* SPIT_GAME_KEY_NAMES[config::SPIT_KEYS] = {"1", "2", "3", "4"};

inline void spit_keys_card() {
    static int capturing = -1;
    static bool waiting_release = false;
    begin_card("spit_keys", "SPIT KEYS");
    note("Pick your own key for each spit type. The trainer presses the game's key for it (1 to 4) when you press yours while the menu is closed.");
    if (capturing >= 0) {
        int vk = 0;
        bool any = pressed_key(vk);
        if (waiting_release) waiting_release = any;
        else if (any) {
            if (vk == VK_ESCAPE) capturing = -1;
            else {
                config::cfg.spit_keys[capturing] = vk == VK_BACK || vk == VK_DELETE ? 0 : vk;
                capturing = -1;
                save_config();
            }
        }
    }
    for (int i = 0; i < config::SPIT_KEYS; i++) {
        ImGui::PushID(i);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(SPIT_NAMES[i]);
        ImGui::SameLine(S(220));
        int vk = config::cfg.spit_keys[i];
        std::string text = capturing == i ? "Press a key..." : vk ? key_name(vk) : "Not set";
        if (ImGui::Button(text.c_str(), {S(170), 0})) capturing = i, waiting_release = true;
        ImGui::SameLine(0, S(8));
        ImGui::BeginDisabled(!vk);
        if (ImGui::Button("Clear", {S(70), 0})) config::cfg.spit_keys[i] = 0, save_config();
        ImGui::EndDisabled();
        ImGui::SameLine(0, S(12));
        label((std::string("game key ") + SPIT_GAME_KEY_NAMES[i]).c_str());
        ImGui::PopID();
    }
    if (capturing >= 0) note("Esc cancels, Backspace or Delete removes the key.");
    end_card();
}

inline bool multiplayer_locked(const char* id) {
    begin_card(id, "NOT IN THE NEXUS VERSION");
    note("Multiplayer features are cut from the Nexus Version, so they are grayed out, and every cheat turns off while you play the Night Hunter. "
         "This is an older version. The newest FaTrainer, with every feature, is on fatrainer.fatum.cc and GitHub (github.com/ifatum/DyingLightFaTrainer).");
    end_card();
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.3f);
    ImGui::BeginDisabled();
    ImGui::PopStyleVar();
    return true;
}

inline void zombie_page() {
    multiplayer_locked("nexus_zombie");
    begin_card("about");
    note("For Be The Zombie matches, where you play the Night Hunter and invade another player's game. Turn these on before or during a match.");
    end_card();
    begin_card("hunter", "HUNTER");
    cheat_switch("god", "Hunter god mode");
    cheat_switch("z_energy");
    end_card();
    begin_card("abilities", "ABILITIES");
    cheat_switch("z_cooldowns");
    cheat_switch("z_spits");
    cheat_switch("z_camo");
    end_card();
    spit_keys_card();
    note("Attack ranges and aim angles are on the PvP page.");
    ImGui::EndDisabled();
}

inline const char* SURVIVOR_RANKS[] = {"Prey", "Casualty", "Endangered", "Underdog", "Runner", "Contender",
                                       "Challenger", "Fighter", "Dominant", "Ruthless", "Indomitable", "Ultimate Survivor"};
inline const char* HUNTER_RANKS[] = {"Walker", "Runner", "Biter", "Bolter", "Stalker", "Beast",
                                     "Mauler", "Juggernaut", "Widow Maker", "Carnivore", "Hunter", "Apex Predator"};

inline void rank_row(const char* label, bool zombie, const char* const* names) {
    int rank = cheats::pvp_rank(zombie);
    if (rank < 0) return;
    ImGui::PushID(label);
    ImGui::TableNextRow(0, S(44));
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    if (rank < 12) ImGui::Text("%s", names[rank]);
    else ImGui::Text("%d", rank);
    ImGui::TableNextColumn();
    struct Step { const char* text; int delta; };
    for (Step st : {Step{"-10", -10}, Step{"-1", -1}, Step{"+1", 1}, Step{"+10", 10}}) {
        if (ImGui::Button(st.text, {S(56), 0})) {
            int target = rank + st.delta;
            on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_pvp_rank(zombie, target); });
        }
        ImGui::SameLine(0, S(6));
    }
    ImGui::NewLine();
    ImGui::PopID();
}

inline void ranks_card() {
    if (cheats::pvp_rank(false) < 0 && cheats::pvp_rank(true) < 0) return;
    multiplayer_locked("nexus_ranks");
    begin_card("ranks", "BE THE ZOMBIE RANKS");
    note("Your PvP rank as a survivor and as the Night Hunter. The game may adjust it again after your next Be The Zombie match.");
    ImGui::Dummy({0, S(4)});
    if (ImGui::BeginTable("ranks", 3, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, S(130));
        ImGui::TableSetupColumn("rank", ImGuiTableColumnFlags_WidthFixed, S(210));
        ImGui::TableSetupColumn("buttons", ImGuiTableColumnFlags_WidthStretch);
        rank_row("Survivor", false, SURVIVOR_RANKS);
        rank_row("Night Hunter", true, HUNTER_RANKS);
        ImGui::EndTable();
    }
    end_card();
    ImGui::EndDisabled();
}

inline void skills_page() {
    begin_card("xp", "EXPERIENCE");
    tweak_sliders(cheats::G_PROGRESS);
    end_card();
    if (!cheats::player) return empty_state("Your character was not found yet. Load into your save, then press Refresh.");
    if (!cheats::set_level_fn) return empty_state("The game's level function was not found. Check fatrainer.log.");
    begin_card("trees", "SKILL TREES");
    note("Level up gives you exactly the XP for the next level, like playing would. The other buttons set the level directly. Skill points appear in the skill menu right away.");
    ImGui::Dummy({0, S(4)});
    if (ImGui::BeginTable("trees", 4, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, S(130));
        ImGui::TableSetupColumn("level", ImGuiTableColumnFlags_WidthFixed, S(90));
        ImGui::TableSetupColumn("xp", ImGuiTableColumnFlags_WidthFixed, S(120));
        ImGui::TableSetupColumn("buttons", ImGuiTableColumnFlags_WidthStretch);
        for (auto& t : cheats::TREES) {
            int max = cheats::tree_max(t.type), level = cheats::tree_level(t.type);
            if (!max || level < 0) continue;
            ImGui::PushID(t.type);
            ImGui::TableNextRow(0, S(44));
            ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.3f);
            ImGui::BeginDisabled(t.type == 0);
            ImGui::PopStyleVar();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(t.name);
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", level);
            ImGui::SameLine(0, S(4));
            ImGui::TextDisabled("/ %d", max);
            ImGui::TableNextColumn();
            ImGui::BeginDisabled(!cheats::level_from_xp_fn || level >= max);
            if (accent_button("Level up", {S(110), 0})) {
                int type = t.type;
                on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::level_up_with_xp(type); });
            }
            ImGui::EndDisabled();
            ImGui::TableNextColumn();
            struct Step { const char* text; int delta; };
            for (Step st : {Step{"-10", -10}, Step{"-1", -1}, Step{"+1", 1}, Step{"+10", 10}}) {
                if (ImGui::Button(st.text, {S(56), 0})) {
                    int type = t.type, target = level + st.delta;
                    on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_tree_level(type, target); });
                }
                ImGui::SameLine(0, S(6));
            }
            if (ImGui::Button("Max", {S(64), 0})) {
                int type = t.type;
                on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_tree_level(type, cheats::tree_max(type)); });
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    end_card();
    ranks_card();
}

inline void pvp_page() {
    multiplayer_locked("nexus_pvp");
    begin_card("about");
    note("Slide right to reach further. At Max the pounce, dropkick and death from above also hit targets that are not in front of you. Each player's game decides its own attacks.");
    end_card();
    begin_card("zombie", "AS THE NIGHT HUNTER");
    tweak_sliders(cheats::G_ZOMBIE);
    end_card();
    begin_card("human", "AS A SURVIVOR");
    tweak_sliders(cheats::G_HUMAN);
    end_card();
    ImGui::EndDisabled();
}

inline ImU32 rgb(const float c[3], float alpha = 1.0f) { return IM_COL32((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), (int)(alpha * 255)); }

inline void outlined_text(ImDrawList* dl, ImVec2 at, ImU32 color, const char* text) {
    for (auto d : {ImVec2{-1, 0}, ImVec2{1, 0}, ImVec2{0, -1}, ImVec2{0, 1}}) dl->AddText({at.x + d.x, at.y + d.y}, IM_COL32(0, 0, 0, 200), text);
    dl->AddText(at, color, text);
}

inline void draw_esp() {
    auto& e = config::cfg.esp;
    static std::vector<cheats::EspTarget> targets;
    float m[16];
    static const bool sample = getenv("DLT_ESP") != nullptr;
    if (sample) {
        const float perspective[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
        memcpy(m, perspective, sizeof m);
        targets = {{{-1.2f, -1.0f, 6}, 180, 250, 23, 40, true, false, 7}, {{1.5f, -1.0f, 9}, 60, 175, 41, NAN, false, true, 3}};
    } else if (!cheats::esp_snapshot(m, targets, e.max_distance)) {
        return;
    }
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    const float BOX_WIDTH = 0.45f, BAR = 4.0f;
    for (auto& t : targets) {
        if (t.ally && !e.allies) continue;
        cheats::Vec3 head{t.feet.x, t.feet.y + cheats::PLAYER_HEIGHT, t.feet.z};
        float fx, fy, hx, hy;
        if (!cheats::to_screen(m, t.feet, screen.x, screen.y, fx, fy) || !cheats::to_screen(m, head, screen.x, screen.y, hx, hy)) continue;
        float h = fy - hy;
        if (!(h > 4)) continue;
        float w = h * BOX_WIDTH, left = fx - w / 2, right = fx + w / 2;
        ImU32 color = rgb(t.hunter ? e.hunter : e.survivor);
        if (e.snaplines) dl->AddLine({screen.x / 2, screen.y}, {fx, fy}, rgb(t.hunter ? e.hunter : e.survivor, 0.6f), 1.5f);
        if (e.box) {
            dl->AddRect({left - 1, hy - 1}, {right + 1, fy + 1}, IM_COL32(0, 0, 0, 160), 0, 0, 3.0f);
            dl->AddRect({left, hy}, {right, fy}, color, 0, 0, 1.5f);
        }
        float fraction = t.max_health > 0 ? std::clamp(t.health / t.max_health, 0.0f, 1.0f) : NAN;
        if (e.health_bar && std::isfinite(fraction)) {
            float x = left - BAR - 3;
            dl->AddRectFilled({x - 1, hy - 1}, {x + BAR + 1, fy + 1}, IM_COL32(0, 0, 0, 180));
            ImU32 fill = IM_COL32((int)(255 * (1 - fraction)), (int)(220 * fraction), 60, 255);
            dl->AddRectFilled({x, fy - h * fraction}, {x + BAR, fy}, fill);
        }
        if (e.role) {
            std::string top = t.hunter ? "Night Hunter" : "Survivor";
            if (t.ally) top += " (ally)";
            ImVec2 size = ImGui::CalcTextSize(top.c_str());
            outlined_text(dl, {fx - size.x / 2, hy - size.y - 2}, color, top.c_str());
        }
        std::vector<std::string> lines;
        char b[64];
        if (e.distance) snprintf(b, sizeof b, "%.0f m", t.distance), lines.push_back(b);
        if (e.health_text && std::isfinite(t.health)) snprintf(b, sizeof b, "%.0f / %.0f HP", t.health, t.max_health), lines.push_back(b);
        if (e.rank && t.rank >= 0) snprintf(b, sizeof b, "Rank %d", t.rank), lines.push_back(b);
        if (e.rage && t.hunter && std::isfinite(t.rage)) snprintf(b, sizeof b, "Rage %.0f", t.rage), lines.push_back(b);
        float y = fy + 2;
        for (auto& line : lines) {
            ImVec2 size = ImGui::CalcTextSize(line.c_str());
            outlined_text(dl, {fx - size.x / 2, y}, IM_COL32(235, 235, 235, 255), line.c_str());
            y += size.y;
        }
    }
}

inline void visuals_page() {
    auto& e = config::cfg.esp;
    bool changed = false;
    multiplayer_locked("nexus_esp");
    begin_card("esp", "PLAYER ESP");
    changed |= switch_row("Show players", "Draws every other player in your game through walls: survivors and the Night Hunter.", e.on);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##range", &e.max_distance, 25, 1000, "Up to %.0f m")) changed = true;
    ImGui::Dummy({0, S(4)});
    struct Option { const char* name; bool* value; };
    Option options[] = {{"Box", &e.box}, {"Role", &e.role}, {"Health bar", &e.health_bar}, {"Health number", &e.health_text},
                        {"Distance", &e.distance}, {"PvP rank", &e.rank}, {"Hunter rage", &e.rage}, {"Line from screen bottom", &e.snaplines},
                        {"Show allies", &e.allies}};
    if (ImGui::BeginTable("esp_options", 3)) {
        for (auto& o : options) {
            ImGui::TableNextColumn();
            changed |= ImGui::Checkbox(o.name, o.value);
        }
        ImGui::EndTable();
    }
    ImGui::Dummy({0, S(4)});
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Night Hunter");
    ImGui::SameLine(S(150));
    changed |= ImGui::ColorEdit3("##hunter", e.hunter, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine(0, S(30));
    ImGui::TextUnformatted("Survivors");
    ImGui::SameLine();
    changed |= ImGui::ColorEdit3("##survivor", e.survivor, ImGuiColorEditFlags_NoInputs);
    note("Allies and enemies come from the game's teams. The ESP keeps drawing when the menu is closed.");
    end_card();
    ImGui::EndDisabled();

    auto& u = config::cfg.uv;
    begin_card("uv", "UV LIGHT");
    changed |= switch_row("Custom UV light color", "Your UV flashlight shines in your own color.", u.on);
    ImGui::BeginDisabled(!u.on);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Color");
    ImGui::SameLine(S(150));
    changed |= ImGui::ColorEdit3("##uvcolor", u.color, ImGuiColorEditFlags_NoInputs);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Glow");
    ImGui::SameLine(S(150));
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##uvglow", &u.glow, 0.2f, 4.0f, "x%.1f")) changed = true;
    ImGui::EndDisabled();
    note("Glow scales the light's brightness and its visible beam. If the color does not change right away, switch the UV light off and on.");
    end_card();
    if (changed) save_config();
}

inline std::string section_name(size_t index) {
    auto& sec = cheats::prison_sections[index];
    char b[64];
    switch (sec.type) {
        case cheats::SENSOR_START: return "Start";
        case cheats::SENSOR_REWARD: return "Reward room";
        case cheats::SENSOR_EVAC: return "Evacuation";
        case cheats::SENSOR_STAGE: {
            int split = 1;
            for (size_t i = 0; i < index; i++) split += cheats::prison_sections[i].type == cheats::SENSOR_STAGE;
            snprintf(b, sizeof b, "Split %d%s", split, sec.last ? " (last)" : "");
            return b;
        }
    }
    snprintf(b, sizeof b, "Section %zu", index + 1);
    return b;
}

inline void go_to(cheats::Vec3 to) {
    on_game_thread([=] {
        std::lock_guard<std::mutex> l(game::mx);
        cheats::teleport(to);
    });
}

inline void route_action(std::function<void()> f) {
    on_game_thread([f] {
        std::lock_guard<std::mutex> l(game::mx);
        f();
    });
}

inline void route_card() {
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        route_action([] { cheats::load_route(route_path()); });
    }
    begin_card("route", "ROUTE");
    note("Record your route once: press Record, play the prison normally, press Stop at the end. Replay then teleports you along it in small "
         "steps, so every quest trigger fires in order. It pauses where you stood still while recording (fights, doors): clear the wave (Kill all enemies nearby), "
         "then press Continue.");
    ImGui::Dummy({0, S(2)});
    int mode = cheats::route_mode;
    size_t stops = 0;
    for (auto& p : cheats::route) stops += p.stop;
    char status[160];
    if (mode == cheats::ROUTE_RECORDING) snprintf(status, sizeof status, "Recording: %zu points, %zu stops", cheats::route.size(), stops);
    else if (mode == cheats::ROUTE_REPLAYING) snprintf(status, sizeof status, "Replaying: point %zu of %zu", cheats::route_at, cheats::route.size());
    else if (mode == cheats::ROUTE_PAUSED) snprintf(status, sizeof status, "Paused at a stop (point %zu of %zu). Clear the fight, then Continue.", cheats::route_at, cheats::route.size());
    else if (cheats::route.empty()) snprintf(status, sizeof status, "No route recorded yet.");
    else snprintf(status, sizeof status, "Saved route: %zu points, %zu stops.", cheats::route.size(), stops);
    label(status);
    ImGui::Dummy({0, S(2)});
    if (mode == cheats::ROUTE_RECORDING) {
        if (accent_button("Stop recording")) route_action([] {
            cheats::route_mode = cheats::ROUTE_IDLE;
            toast(cheats::save_route(route_path()) ? "Route saved" : "Route could not be saved");
        });
    } else if (mode == cheats::ROUTE_IDLE) {
        if (ImGui::Button("Record")) route_action([] {
            cheats::route.clear();
            cheats::route_at = 0;
            cheats::route_mode = cheats::ROUTE_RECORDING;
        });
        ImGui::SameLine();
        ImGui::BeginDisabled(cheats::route.empty());
        if (accent_button("Replay")) route_action([] {
            cheats::route_at = 0;
            cheats::route_mode = cheats::ROUTE_REPLAYING;
        });
        ImGui::EndDisabled();
    } else {
        if (mode == cheats::ROUTE_PAUSED && accent_button("Continue")) route_action([] { cheats::route_mode = cheats::ROUTE_REPLAYING; });
        if (mode == cheats::ROUTE_PAUSED) ImGui::SameLine();
        if (ImGui::Button("Stop replay")) route_action([] {
            cheats::route_mode = cheats::ROUTE_IDLE;
            cheats::route_at = 0;
        });
    }
    end_card();
}

inline void prison_page() {
    begin_card("about");
    note("For the Harran Prison mode. Load into the prison first. Timers and teleports are decided by the host, so use them in your own game.");
    end_card();
    begin_card("timers", "TIMERS");
    cheat_switch("prison_pause");
    end_card();
    begin_card("fights", "FIGHTS");
    note("Objectives like killing the bandits or a final wave need those enemies dead before the quest moves on.");
    ImGui::Dummy({0, S(2)});
    kill_all_button();
    end_card();
    begin_card("sections", "TELEPORT TO A SECTION");
    auto& sections = cheats::prison_sections;
    if (sections.empty()) note("No prison sections found yet. Load into Harran Prison, then press Find sections.");
    else {
        note("In run order: the start, every split checkpoint, the reward room and the evacuation. The prison is scripted: when an objective asks you to kill infected or a final wave, finish it where you are (Kill all enemies nearby above) and wait for the objective to change before you press Next section. Skipping a fight leaves the objective stuck.");
        cheats::next_section = std::clamp(cheats::next_section, 0, (int)sections.size() - 1);
        std::string next = "Next section: " + section_name(cheats::next_section);
        if (accent_button(next.c_str(), {ImGui::GetContentRegionAvail().x, S(38)})) {
            go_to(sections[cheats::next_section].pos);
            cheats::next_section = (cheats::next_section + 1) % (int)sections.size();
        }
        ImGui::Dummy({0, S(4)});
    }
    for (size_t i = 0; i < sections.size(); i++) {
        ImGui::PushID((int)i);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(section_name(i).c_str());
        ImGui::SameLine(S(220));
        if (accent_button("Teleport", {S(110), 0})) {
            go_to(sections[i].pos);
            cheats::next_section = (int)(i + 1) % (int)sections.size();
        }
        ImGui::PopID();
    }
    ImGui::Dummy({0, S(2)});
    ImGui::BeginDisabled(game::g.scanning);
    if (ImGui::Button("Find sections")) {
        cheats::sections_wanted = true;
        request_refresh();
    }
    ImGui::EndDisabled();
    end_card();
    route_card();
    begin_card("position", "YOUR POSITION");
    note("Save where you stand and jump back to it later. Works everywhere, not only in the prison.");
    ImGui::Dummy({0, S(2)});
    if (ImGui::Button("Save position")) on_game_thread([] {
        std::lock_guard<std::mutex> l(game::mx);
        cheats::saved_position = cheats::player_position();
        cheats::has_saved_position = std::isfinite(cheats::saved_position.x);
        toast(cheats::has_saved_position ? "Position saved" : "Your position could not be read");
    });
    ImGui::SameLine();
    ImGui::BeginDisabled(!cheats::has_saved_position);
    if (accent_button("Go to saved position")) go_to(cheats::saved_position);
    ImGui::EndDisabled();
    end_card();
}

struct Key { int vk; const char* name; };
inline const Key KEYS[] = {{VK_INSERT, "Insert"}, {VK_F8, "F8"},   {VK_HOME, "Home"}, {VK_END, "End"},   {VK_DELETE, "Delete"},
                           {VK_PRIOR, "Page Up"}, {VK_NEXT, "Page Down"}, {VK_F1, "F1"}, {VK_F2, "F2"}, {VK_F3, "F3"},
                           {VK_F4, "F4"},         {VK_F5, "F5"},   {VK_F6, "F6"},     {VK_F7, "F7"},     {VK_F9, "F9"},
                           {VK_F10, "F10"},       {VK_F11, "F11"}, {VK_F12, "F12"}};

struct Page {
    const char* id;
    const char* title;
    const char* subtitle;
    void (*draw)();
    bool (*shown)();
    const char* section;
    std::vector<const char*> keys;
};
inline bool always() { return true; }
inline void settings_page();
inline bool has_tools() { return game::find_inventory(game::K_TOOLS) != nullptr; }
inline const Page PAGES[] = {
    {"player", "Player", "Health, stamina, gear and movement", player_page, always, "CHEATS", {"god", "stamina", "hook", "uv", "uv_slow", "lockpick", "no_fall", "speed", "jump"}},
    {"combat", "Combat", "Enemies, ammo, supplies and weapons", combat_page, always, "CHEATS", {"one_hit", "ammo", "no_reload", "supplies", "durability"}},
    {"skills", "Skills", "Experience and skill tree levels", skills_page, always, "CHEATS", {"xp"}},
    {"zombie", "Night Hunter", "Be The Zombie abilities", zombie_page, always, "MODES", {"z_energy", "z_cooldowns", "z_spits", "z_camo"}},
    {"pvp", "PvP", "How far your attacks reach in Be The Zombie", pvp_page, always, "MODES",
     {"z_pounce", "z_pound", "z_tackle", "z_claws", "z_spit", "h_dfa", "h_dfa_pull", "h_dfa_height", "h_dropkick", "h_kicks", "h_melee"}},
    {"visuals", "Visuals", "Player ESP and UV light color", visuals_page, always, "MODES", {}},
    {"prison", "Prison", "Harran Prison timers and teleports", prison_page, always, "MODES", {"prison_pause"}},
    {"cash", "Cash", "Your money", cash_page, always, "ITEMS", {}},
    {"backpack", "Backpack", "Items you carry. Press Edit to change a weapon", [] { inventory_page(game::K_BACKPACK); }, always, "ITEMS", {}},
    {"stash", "Stash", "Items stored in your stash. Press Edit to change a weapon", [] { inventory_page(game::K_STASH); }, always, "ITEMS", {}},
    {"materials", "Materials", "Crafting parts and consumables", [] { inventory_page(game::K_MATERIALS); }, always, "ITEMS", {}},
    {"tools", "Tools", "Special items", [] { inventory_page(game::K_TOOLS); }, has_tools, "ITEMS", {}},
    {"give", "Give items", "Spawn any item in the game", give_page, always, "ITEMS", {}},
    {"settings", "Settings", "Look, controls and sidebar", settings_page, always, "", {}},
};

inline int active_on(const Page& p) {
    int n = 0;
    for (const char* k : p.keys) {
        if (auto* c = cheats::find(k)) n += c->on;
        if (auto* t = cheats::find_tweak(k)) n += t->factor != 1.0f;
    }
    return n;
}

inline const Page* page_by_id(const std::string& id) {
    for (auto& p : PAGES)
        if (id == p.id) return &p;
    return nullptr;
}

inline std::vector<std::string> page_ids() {
    std::vector<std::string> out;
    for (auto& p : PAGES) out.push_back(p.id);
    return out;
}

inline void profiles_card() {
    static std::vector<std::string> names = config::list_profiles();
    static char name[48] = "";
    static std::string confirm_delete;
    begin_card("profiles", "CONFIGS");
    note("Save the cheats and sliders that are on right now under a name, and load them again with one click. Loading turns everything else off.");
    ImGui::Dummy({0, S(2)});
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - S(130));
    ImGui::InputTextWithHint("##name", "Config name, e.g. PvP hunter", name, sizeof name);
    ImGui::SameLine(0, S(8));
    std::string clean = config::profile_name(name);
    ImGui::BeginDisabled(clean.empty());
    if (accent_button("Save", {-1, 0})) {
        bool replaced = std::find(names.begin(), names.end(), clean) != names.end();
        bool ok = config::save_profile(clean, cheats::current_profile());
        toast(ok ? (replaced ? "Updated config " : "Saved config ") + clean : std::string("Could not save the config"));
        names = config::list_profiles();
        name[0] = 0;
    }
    ImGui::EndDisabled();
    if (names.empty()) note("No configs yet.");
    for (auto& n : names) {
        ImGui::PushID(n.c_str());
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(n.c_str());
        float right = ImGui::GetWindowContentRegionMax().x;
        ImGui::SameLine(right - S(150));
        if (ImGui::Button("Load", {S(70), 0})) {
            config::Profile p;
            if (config::load_profile(n, p)) {
                cheats::apply_profile(p);
                save_config();
                toast("Loaded config " + n);
            } else {
                toast("Could not read config " + n);
            }
        }
        ImGui::SameLine(0, S(8));
        bool sure = confirm_delete == n;
        if (ImGui::Button(sure ? "Sure?" : "Delete", {S(72), 0})) {
            if (sure) {
                config::delete_profile(n);
                confirm_delete.clear();
                toast("Deleted config " + n);
                ImGui::PopID();
                names = config::list_profiles();
                break;
            }
            confirm_delete = n;
        }
        ImGui::PopID();
    }
    end_card();
}

inline void settings_page() {
    auto& c = config::cfg;
    bool changed = false;
    begin_card("look", "APPEARANCE");
    const float presets[][3] = {{0.953f, 0.604f, 0.118f}, {0.93f, 0.33f, 0.33f}, {0.35f, 0.82f, 0.55f},
                                {0.33f, 0.62f, 0.98f}, {0.66f, 0.45f, 0.98f}, {0.95f, 0.45f, 0.70f}, {0.85f, 0.87f, 0.90f}};
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Accent");
    ImGui::SameLine(S(170));
    for (int i = 0; i < 7; i++) {
        ImGui::PushID(i);
        ImVec2 p = ImGui::GetCursorScreenPos();
        float d = ImGui::GetFrameHeight();
        if (ImGui::InvisibleButton("sw", {d, d})) memcpy(c.accent, presets[i], sizeof c.accent), changed = true;
        bool sel = !memcmp(c.accent, presets[i], sizeof c.accent);
        auto* dl = ImGui::GetWindowDrawList();
        dl->AddCircleFilled({p.x + d / 2, p.y + d / 2}, d / 2 - S(3), col({presets[i][0], presets[i][1], presets[i][2], 1}));
        if (sel || ImGui::IsItemHovered()) dl->AddCircle({p.x + d / 2, p.y + d / 2}, d / 2, col(T.text, sel ? 1.0f : 0.4f), 0, S(2));
        ImGui::SameLine(0, S(6));
        ImGui::PopID();
    }
    if (ImGui::ColorEdit3("##custom", c.accent, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) changed = true;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Interface size");
    ImGui::SameLine(S(170));
    static float pending_scale = c.scale;
    ImGui::SetNextItemWidth(S(200));
    ImGui::SliderFloat("##scale", &pending_scale, 0.75f, 1.5f, "%.2fx", ImGuiSliderFlags_NoInput);
    ImGui::SameLine();
    ImGui::BeginDisabled(pending_scale == c.scale);
    if (accent_button("Apply")) c.scale = pending_scale, changed = true;
    ImGui::EndDisabled();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Background dim");
    ImGui::SameLine(S(170));
    ImGui::SetNextItemWidth(S(260));
    float pct = c.dim * 100;
    if (ImGui::SliderFloat("##dim", &pct, 0, 95, "%.0f%%")) c.dim = pct / 100, changed = true;
    end_card();

    begin_card("controls", "CONTROLS");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Menu key");
    ImGui::SameLine(S(170));
    ImGui::SetNextItemWidth(S(180));
    const char* current = "Insert";
    for (auto& k : KEYS)
        if (k.vk == c.menu_key) current = k.name;
    if (ImGui::BeginCombo("##key", current)) {
        for (auto& k : KEYS)
            if (ImGui::Selectable(k.name, k.vk == c.menu_key)) c.menu_key = k.vk, changed = true;
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    note("F8 always works too.");
    bool remember = c.remember_cheats;
    if (switch_row("Remember cheats", "Turn your cheats back on the next time the game starts.", remember)) c.remember_cheats = remember, changed = true;
    end_card();

    begin_card("sidebar", "SIDEBAR");
    note("Choose which pages show up and in what order.");
    ImGui::Dummy({0, S(2)});
    for (size_t i = 0; i < c.pages.size(); i++) {
        auto* p = page_by_id(c.pages[i].id);
        if (!p) continue;
        ImGui::PushID((int)i);
        float right = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
        bool vis = c.pages[i].visible;
        ImGui::BeginDisabled(c.pages[i].id == "settings");
        if (ImGui::Checkbox("##vis", &vis)) c.pages[i].visible = vis, changed = true;
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(p->title);
        float bw = ImGui::GetFrameHeight();
        ImGui::SameLine(right - bw * 2 - S(6));
        ImGui::BeginDisabled(i == 0);
        if (ImGui::ArrowButton("up", ImGuiDir_Up)) std::swap(c.pages[i], c.pages[i - 1]), changed = true;
        ImGui::EndDisabled();
        ImGui::SameLine(0, S(6));
        ImGui::BeginDisabled(i + 1 == c.pages.size());
        if (ImGui::ArrowButton("down", ImGuiDir_Down)) std::swap(c.pages[i], c.pages[i + 1]), changed = true;
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    end_card();

    profiles_card();

    begin_card("reset", "SETTINGS FILE");
    note(("Saved automatically to " + config::default_path()).c_str());
    ImGui::Dummy({0, S(2)});
    if (ImGui::Button("Reset to defaults")) {
        config::cfg = config::Config();
        pending_scale = config::cfg.scale;
        config::normalize(page_ids());
        changed = true;
    }
    end_card();

    if (changed) {
        config::normalize(page_ids());
        apply_style();
        save_config();
    }
}

inline std::string g_page = "player";

inline bool nav_item(const Page& p, bool active, bool marked) {
    ImGui::PushID(p.id);
    float w = ImGui::GetContentRegionAvail().x, h = S(31);
    ImVec2 at = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton("nav", {w, h});
    float hover = animate(ImGui::GetID("h"), ImGui::IsItemHovered() ? 1.0f : 0.0f);
    float on = animate(ImGui::GetID("a"), active ? 1.0f : 0.0f);
    auto* dl = ImGui::GetWindowDrawList();
    if (hover > 0.01f) dl->AddRectFilled(at, {at.x + w, at.y + h}, col({1, 1, 1, 0.035f * hover}), S(10));
    if (on > 0.01f) {
        dl->AddRectFilled(at, {at.x + w, at.y + h}, col(T.accent, 0.13f * on), S(10));
        dl->AddRectFilled({at.x, at.y + h * 0.28f}, {at.x + S(3), at.y + h * 0.72f}, col(T.accent, on), S(2));
    }
    float fs = ImGui::GetFontSize();
    dl->AddText(f_body, fs, {at.x + S(18), at.y + (h - fs) / 2}, col(mix(T.dim, T.text, std::max(on, hover * 0.6f))), p.title);
    if (marked) dl->AddCircleFilled({at.x + w - S(14), at.y + h / 2}, S(4), col(T.accent));
    ImGui::PopID();
    return clicked;
}

inline void sidebar() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, T.side);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(14), S(20)});
    ImGui::BeginChild("side", {S(228), 0}, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPosX(S(18));
    ImGui::PushFont(f_brand);
    ImGui::TextColored(T.accent, "FaTrainer");
    ImGui::PopFont();
    ImGui::SetCursorPosX(S(19));
    label(("DYING LIGHT   v" + std::string(VERSION)).c_str());
    ImGui::Dummy({0, S(10)});
    int active = cheats::active_count();
    float footer = ImGui::GetFrameHeight() * (active ? 2 : 1) + S(active ? 64 : 52);
    ImGui::BeginChild("nav", {0, ImGui::GetContentRegionAvail().y - footer}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {S(10), S(2)});
    std::string section;
    for (auto& e : config::cfg.pages) {
        auto* p = page_by_id(e.id);
        if (!p || !e.visible || !p->shown()) continue;
        if (*p->section && p->section != section) {
            ImGui::Dummy({0, section.empty() ? 0 : S(6)});
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(18));
            label(p->section);
        }
        section = p->section;
        if (nav_item(*p, g_page == p->id, active_on(*p) > 0)) g_page = p->id;
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    if (active) {
        ImGui::Dummy({0, S(2)});
        if (ImGui::Button(("Turn all off (" + std::to_string(active) + ")").c_str(), {-1, 0})) cheats::all_off(), save_config();
    }
    ImGui::Dummy({0, S(4)});
    const char* st = game::g.scanning ? "Scanning..." : game::g.vt_money ? (cheats::player ? "Connected" : "Waiting for save") : "Game not ready";
    ImVec4 sc = game::g.scanning ? T.accent : game::g.vt_money ? (cheats::player ? T.ok : T.accent) : T.bad;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddCircleFilled({p.x + S(10), p.y + S(9)}, S(4), col(sc));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(22));
    ImGui::PushFont(f_small);
    ImGui::TextColored(sc, "%s", st);
    ImGui::PopFont();
    ImGui::BeginDisabled(game::g.scanning);
    if (ImGui::Button("Refresh", {-1, 0})) request_refresh();
    ImGui::EndDisabled();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

inline void draw() {
    ImGuiIO& io = ImGui::GetIO();
    if (config::cfg.dim > 0) ImGui::GetBackgroundDrawList()->AddRectFilled({0, 0}, io.DisplaySize, IM_COL32(0, 0, 0, (int)(config::cfg.dim * 255)));
    static bool first = true;
    if (first) {
        first = false;
        if (const char* p = getenv("DLT_PAGE")) g_page = p;
        if (getenv("DLT_SECTIONS"))
            for (int type : {1, 2, 2, 2, 2, 2, 3, 4}) cheats::prison_sections.push_back({0, type, (int)cheats::prison_sections.size(), false, {1, 2, 3}});
    }
    ImGui::SetNextWindowSize({S(1040), S(700)}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_FirstUseEver, {0.5f, 0.5f});
    ImGui::SetNextWindowSizeConstraints({S(760), S(480)}, {FLT_MAX, FLT_MAX});
    ImGui::Begin(TITLE, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        std::lock_guard<std::mutex> l(game::mx);
        const Page* page = page_by_id(g_page);
        if (!page || !page->shown()) page = &PAGES[0], g_page = page->id;
        sidebar();
        ImGui::SameLine(0, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(30), S(24)});
        ImGui::BeginChild("content", {0, 0}, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();
        ImGui::PushFont(f_head);
        ImGui::TextUnformatted(page->title);
        ImGui::PopFont();
        label(page->subtitle);
        ImGui::Dummy({0, S(8)});
        ImGui::BeginChild("page", {0, 0});
        static const char* preview_scroll = getenv("DLT_SCROLL");
        if (preview_scroll && ImGui::GetFrameCount() > 5 && ImGui::GetFrameCount() < 30) ImGui::SetScrollY((float)atof(preview_scroll));
        page->draw();
        ImGui::EndChild();
        ImGui::EndChild();
    }
    ImGui::End();

    std::string msg;
    {
        std::lock_guard<std::mutex> q(g_qmx);
        if (GetTickCount() - g_toast_at < 3500) msg = g_toast;
    }
    if (!msg.empty()) {
        ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y - S(60)}, ImGuiCond_Always, {0.5f, 1});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(18), S(12)});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, S(24));
        ImGui::Begin("##toast", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
        ImGui::TextUnformatted(msg.c_str());
        ImGui::End();
        ImGui::PopStyleVar(2);
    }
}

inline void load_fonts(const void* ttf, int size) {
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH = 3;
    f_body = io.Fonts->AddFontFromMemoryTTF((void*)ttf, size, 20.0f, &cfg);
    f_small = io.Fonts->AddFontFromMemoryTTF((void*)ttf, size, 15.0f, &cfg);
    f_head = io.Fonts->AddFontFromMemoryTTF((void*)ttf, size, 30.0f, &cfg);
    f_brand = io.Fonts->AddFontFromMemoryTTF((void*)ttf, size, 26.0f, &cfg);
    f_big = io.Fonts->AddFontFromMemoryTTF((void*)ttf, size, 54.0f, &cfg);
}

inline void startup() {
    config::load(config::default_path());
    config::normalize(page_ids());
    config::Profile p;
    if (config::cfg.remember_cheats) p.cheats = config::cfg.cheats_on;
    p.tweaks = config::cfg.tweaks;
    cheats::apply_profile(p);
    if (getenv("DLT_ESP")) config::cfg.esp.on = config::cfg.esp.health_text = config::cfg.esp.rank = config::cfg.esp.rage = true;
}

}
