#pragma once
#include <algorithm>
#include <cmath>
#include "app.h"
#include "brand.h"
#include "color.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace menu {

inline ImFont *f_body, *f_strong, *f_small, *f_label, *f_head, *f_tile, *f_big, *f_brand, *f_mono;

struct Theme { ImU32 accent, accent_ink, ground, side, raised, raised_hi, frame, line, line_soft, text, soft, muted, ok, bad; };
inline Theme T;

inline float S(float v) { return v * config::cfg.scale; }
inline float R(float v) { return S(v) * config::cfg.look.roundness; }
inline ImU32 C(ImU32 c, float alpha = 1) { return brand::C(c, alpha); }
inline ImVec4 V(ImU32 c, float alpha = 1) {
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
    v.w *= alpha;
    return v;
}
inline float now() { return (float)ImGui::GetTime(); }
inline float motion() {
    int m = config::cfg.look.motion;
    return m == config::MOTION_OFF ? 0.0f : m == config::MOTION_SUBTLE ? 0.5f : 1.0f;
}
inline float scene_time() { return motion() > 0 ? now() : 0.0f; }

inline void apply_style() {
    auto& a = config::cfg.accent;
    T.accent = IM_COL32((int)(a[0] * 255), (int)(a[1] * 255), (int)(a[2] * 255), 255);
    float luminance = 0.299f * a[0] + 0.587f * a[1] + 0.114f * a[2];
    T.accent_ink = luminance > 0.52f ? brand::ACCENT_INK : IM_COL32(250, 246, 240, 255);
    T.ground = IM_COL32(12, 11, 10, 255), T.side = IM_COL32(15, 14, 13, 255), T.raised = IM_COL32(20, 19, 17, 255), T.raised_hi = IM_COL32(29, 27, 25, 255);
    T.frame = IM_COL32(25, 24, 22, 255), T.line = IM_COL32(40, 38, 35, 255), T.line_soft = IM_COL32(29, 27, 25, 255);
    T.text = brand::TEXT, T.soft = brand::SOFT, T.muted = brand::MUTED, T.ok = brand::GOOD, T.bad = brand::BAD;
    float opacity = config::cfg.look.opacity;
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = R(16), s.ChildRounding = R(12), s.FrameRounding = R(8), s.PopupRounding = R(12);
    s.GrabRounding = R(6), s.ScrollbarRounding = R(6), s.TabRounding = R(8);
    s.WindowBorderSize = 1, s.ChildBorderSize = 0, s.FrameBorderSize = 0, s.PopupBorderSize = 1;
    s.WindowPadding = {0, 0}, s.FramePadding = {S(12), S(8)}, s.ItemSpacing = {S(10), S(10)};
    s.ItemInnerSpacing = {S(8), S(6)}, s.CellPadding = {S(10), S(8)}, s.ScrollbarSize = S(6), s.GrabMinSize = S(12);
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = V(T.ground, opacity);
    c[ImGuiCol_ChildBg] = {0, 0, 0, 0};
    c[ImGuiCol_PopupBg] = V(T.raised, 0.99f);
    c[ImGuiCol_Border] = V(T.line_soft);
    c[ImGuiCol_ModalWindowDimBg] = {0, 0, 0, 0.55f};
    c[ImGuiCol_Text] = V(T.text);
    c[ImGuiCol_TextDisabled] = V(T.muted);
    c[ImGuiCol_FrameBg] = V(T.frame);
    c[ImGuiCol_FrameBgHovered] = V(IM_COL32(35, 33, 30, 255));
    c[ImGuiCol_FrameBgActive] = V(IM_COL32(40, 38, 35, 255));
    c[ImGuiCol_Button] = V(T.frame);
    c[ImGuiCol_ButtonHovered] = V(IM_COL32(39, 37, 34, 255));
    c[ImGuiCol_ButtonActive] = V(T.accent, 0.5f);
    c[ImGuiCol_Header] = V(T.accent, 0.14f);
    c[ImGuiCol_HeaderHovered] = {1, 1, 1, 0.05f};
    c[ImGuiCol_HeaderActive] = V(T.accent, 0.5f);
    c[ImGuiCol_Separator] = V(T.line_soft);
    c[ImGuiCol_ScrollbarBg] = {0, 0, 0, 0};
    c[ImGuiCol_ScrollbarGrab] = V(T.line);
    c[ImGuiCol_ScrollbarGrabHovered] = V(T.muted);
    c[ImGuiCol_ScrollbarGrabActive] = V(T.accent);
    c[ImGuiCol_CheckMark] = c[ImGuiCol_SliderGrab] = c[ImGuiCol_SliderGrabActive] = V(T.accent);
    c[ImGuiCol_TableRowBg] = {0, 0, 0, 0};
    c[ImGuiCol_TableRowBgAlt] = {1, 1, 1, 0.018f};
    c[ImGuiCol_TableBorderLight] = V(T.line_soft);
    c[ImGuiCol_NavCursor] = V(T.accent);
    c[ImGuiCol_TextSelectedBg] = V(T.accent, 0.35f);
    c[ImGuiCol_ResizeGrip] = {0, 0, 0, 0};
    c[ImGuiCol_ResizeGripHovered] = V(T.accent, 0.5f);
    c[ImGuiCol_ResizeGripActive] = V(T.accent);
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
    if (motion() == 0) return v = target;
    return brand::follow(v, target, speed);
}

inline float font_size(ImFont* f) { return f->FontSize * config::cfg.scale; }

inline void label(const char* text) {
    ImGui::PushFont(f_small);
    ImGui::PushStyleColor(ImGuiCol_Text, V(T.muted));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

inline void caps(const char* text, ImU32 color = 0) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float size = font_size(f_label);
    brand::spaced_caps(ImGui::GetWindowDrawList(), f_label, size, p, text, C(color ? color : T.muted), S(1.4f));
    ImGui::Dummy({0, size});
}

inline bool chip(const char* text, bool active) {
    ImGui::PushStyleColor(ImGuiCol_Button, active ? V(T.accent, 0.16f) : ImVec4{1, 1, 1, 0.04f});
    ImGui::PushStyleColor(ImGuiCol_Text, active ? V(T.accent) : V(T.soft));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(20));
    bool r = ImGui::Button(text);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    return r;
}

inline bool flowing_chip(const char* text, bool active, bool first) {
    float w = ImGui::CalcTextSize(text).x + ImGui::GetStyle().FramePadding.x * 2;
    if (!first && ImGui::GetItemRectMax().x + S(6) + w <= ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x) ImGui::SameLine(0, S(6));
    return chip(text, active);
}

inline const char* shown_end(const char* text) {
    const char* hidden = strstr(text, "##");
    return hidden ? hidden : text + strlen(text);
}

inline bool accent_button(const char* text, ImVec2 size = {0, 0}) {
    ImGuiStyle& st = ImGui::GetStyle();
    const char* end = shown_end(text);
    float fs = ImGui::GetFontSize();
    float text_w = f_strong->CalcTextSizeA(fs, FLT_MAX, 0, text, end).x;
    if (size.x == 0) size.x = text_w + st.FramePadding.x * 2.6f;
    else if (size.x < 0) size.x = std::max(S(40), ImGui::GetContentRegionAvail().x + size.x + 1);
    if (size.y == 0) size.y = ImGui::GetFrameHeight();
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton(text, size);
    bool hovered = ImGui::IsItemHovered(), held = ImGui::IsItemActive();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    ImGuiID id = ImGui::GetItemID();
    float h = animate(id + 1, hovered ? 1.0f : 0.0f, 16), d = animate(id + 2, held ? 1.0f : 0.0f, 30);
    float lift = (h - d) * S(1.5f) * motion();
    ImVec2 a{p.x, p.y - lift}, b{p.x + size.x, p.y + size.y - lift};
    float r = R(8);
    auto* dl = ImGui::GetWindowDrawList();
    for (int i = 1; i <= 3 && h > 0.01f; i++) dl->AddRect({a.x - i * S(2), a.y - i * S(2)}, {b.x + i * S(2), b.y + i * S(2)}, C(T.accent, h * 0.1f / i), r + i * S(2), 0, S(2));
    ImU32 fill = brand::mix(brand::mix(T.accent, IM_COL32(255, 255, 255, 255), h * 0.14f), IM_COL32(0, 0, 0, 255), d * 0.12f);
    dl->AddRectFilled(a, b, C(fill), r);
    dl->AddRectFilledMultiColor({a.x + r, a.y + S(1)}, {b.x - r, a.y + size.y * 0.5f}, C(IM_COL32(255, 255, 255, 255), 0.07f), C(IM_COL32(255, 255, 255, 255), 0.07f),
                                C(IM_COL32(255, 255, 255, 255), 0), C(IM_COL32(255, 255, 255, 255), 0));
    dl->AddText(f_strong, fs, {(a.x + b.x - text_w) / 2, (a.y + b.y - fs) / 2}, C(T.accent_ink), text, end);
    return pressed;
}

inline int g_card = 0;
inline float g_page_at = -10;
inline std::vector<float> g_card_offsets;

inline float card_reveal(int index) {
    float m = motion();
    if (m == 0) return 1;
    float delay = m >= 1 ? std::min(index, 8) * 0.055f : 0;
    return brand::ease_out((now() - g_page_at - delay) / (m >= 1 ? 0.5f : 0.25f));
}

const ImGuiWindowFlags CARD_FLAGS = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

inline void begin_card(const char* id, const char* title = nullptr) {
    float a = card_reveal(g_card++);
    float offset = (1 - a) * S(18) * motion();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * a);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
    g_card_offsets.push_back(offset);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, V(T.raised, std::max(0.75f, config::cfg.look.opacity)));
    ImGui::PushStyleColor(ImGuiCol_Border, V(T.line_soft));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(18), S(16)});
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::BeginChild(id, {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders, CARD_FLAGS);
    if (title) {
        caps(title);
        ImGui::Dummy({0, S(2)});
    }
}

inline void end_card() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    float offset = g_card_offsets.empty() ? 0 : g_card_offsets.back();
    if (!g_card_offsets.empty()) g_card_offsets.pop_back();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - offset);
    ImGui::PopStyleVar();
    ImGui::Dummy({0, S(4)});
}

inline bool switch_row(const char* text, const char* hint, bool& value) {
    ImGui::PushID(text);
    float w = ImGui::GetContentRegionAvail().x, h = S(hint && *hint ? 60 : 44);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton("row", {w, h});
    if (clicked) value = !value;
    bool hovered = ImGui::IsItemHovered();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    auto* dl = ImGui::GetWindowDrawList();
    float hover = animate(ImGui::GetID("hover"), hovered ? 1.0f : 0.0f);
    if (hover > 0.01f) dl->AddRectFilled({p.x - S(8), p.y}, {p.x + w + S(8), p.y + h}, C(IM_COL32(255, 255, 255, 255), 0.03f * hover), R(10));
    float body = ImGui::GetFontSize(), small = font_size(f_small);
    float text_y = p.y + (hint && *hint ? S(10) : (h - body) / 2);
    dl->AddText(f_body, body, {p.x + S(4), text_y}, C(T.text), text);
    if (hint && *hint) dl->AddText(f_small, small, {p.x + S(4), text_y + body + S(3)}, C(T.muted), hint);
    float sw = S(44), sh = S(24), t = animate(ImGui::GetID("knob"), value ? 1.0f : 0.0f, 16);
    ImVec2 s0 = {p.x + w - sw - S(4), p.y + (h - sh) / 2};
    dl->AddRectFilled(s0, {s0.x + sw, s0.y + sh}, C(brand::mix(IM_COL32(44, 41, 38, 255), T.accent, t)), sh / 2);
    ImVec2 knob{s0.x + sh / 2 + t * (sw - sh), s0.y + sh / 2};
    float* flipped_at = ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("flip"), -10);
    if (clicked) *flipped_at = now();
    float pulse = motion() > 0 ? brand::clamp01((now() - *flipped_at) / 0.45f) : 1;
    if (pulse < 1) dl->AddCircle(knob, sh / 2 + S(10) * brand::ease_out(pulse), C(T.accent, 0.5f * (1 - pulse)), 0, S(2));
    if (t > 0.01f) dl->AddCircleFilled(knob, sh / 2 + S(3), C(T.accent, 0.18f * t));
    dl->AddCircleFilled(knob, sh / 2 - S(3), C(brand::mix(T.soft, IM_COL32(255, 252, 246, 255), t)));
    ImGui::PopID();
    return clicked;
}

#if !FATRAINER_VISUALS
inline void cheat_switch(const char* key, const char* override_label = nullptr) {
    auto* c = cheats::find(key);
    if (!c) return;
    bool v = c->on;
    if (switch_row(override_label ? override_label : c->label, c->hint, v)) {
        c->on = v;
        save_config();
    }
}

#endif

inline bool drag_bar(const char* id, float frac, ImU32 color, float& picked) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x, h = S(18), y = p.y + h / 2;
    ImGui::InvisibleButton(id, {w, h});
    bool active = ImGui::IsItemActive(), hovered = ImGui::IsItemHovered();
    if (hovered || active) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    ImGuiID bar = ImGui::GetItemID();
    ImGuiStorage* st = ImGui::GetStateStorage();
    float* held = st->GetFloatRef(bar + 1, -1);
    float* released_at = st->GetFloatRef(bar + 2, -10);
    if (active) *held = brand::clamp01((ImGui::GetIO().MousePos.x - p.x) / w);
    bool released = ImGui::IsItemDeactivated() && *held >= 0;
    if (released) picked = *held, *released_at = now();
    if (!active && !released && now() - *released_at > 0.8f) *held = -1;
    float target = *held >= 0 ? *held : brand::clamp01(frac);
    float shown = active ? target : animate(bar + 3, target, 10);
    float lift = animate(bar + 4, hovered || active ? 1.0f : 0.0f, 16), thick = S(2.5f) + lift * S(1.5f);
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled({p.x, y - thick}, {p.x + w, y + thick}, C(IM_COL32(255, 255, 255, 255), 0.06f + lift * 0.04f), S(3));
    dl->AddRectFilled({p.x, y - thick}, {p.x + w * shown, y + thick}, C(color), S(3));
    if (shown > 0.02f || lift > 0.01f) dl->AddCircleFilled({p.x + w * shown, y}, S(6) + lift * S(2), C(color, 0.25f + lift * 0.2f));
    if (lift > 0.01f) dl->AddCircleFilled({p.x + w * shown, y}, S(4) * lift, C(IM_COL32(255, 252, 246, 255), lift));
    return released;
}

inline void stat_tile(const char* id, const char* name, float value, float full, ImU32 color, float width,
                      const std::function<void(float)>& set = nullptr) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, V(T.raised, std::max(0.75f, config::cfg.look.opacity)));
    ImGui::PushStyleColor(ImGuiCol_Border, V(T.line_soft));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(18), S(16)});
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::BeginChild(id, {width, S(112)}, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders, CARD_FLAGS);
    caps(name);
    ImGui::Dummy({0, S(2)});
    ImGui::PushFont(f_tile);
    if (std::isnan(value)) ImGui::TextDisabled("--");
    else ImGui::Text("%.0f", value);
    ImGui::PopFont();
    float frac = full > 0 && !std::isnan(value) ? std::clamp(value / full, 0.0f, 1.0f) : 0, picked = 0;
    if (drag_bar("bar", frac, color, picked) && set && full > 0) set(picked * full);
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

inline void note(const char* text) {
    ImGui::PushFont(f_small);
    ImGui::PushStyleColor(ImGuiCol_Text, V(T.soft));
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

inline bool segmented(const char* id, const char* const* options, int count, int& value, float width) {
    ImGui::PushID(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = ImGui::GetFrameHeight(), cell = width / count;
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, {p.x + width, p.y + h}, C(T.frame), R(8));
    float* slide = ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("slide"), (float)value);
    if (motion() > 0) brand::follow(*slide, (float)value, 18);
    else *slide = (float)value;
    ImVec2 a{p.x + *slide * cell + S(3), p.y + S(3)};
    dl->AddRectFilled(a, {a.x + cell - S(6), p.y + h - S(3)}, C(T.accent), R(6));
    bool changed = false;
    for (int i = 0; i < count; i++) {
        ImGui::SetCursorScreenPos({p.x + i * cell, p.y});
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("seg", {cell, h}) && value != i) value = i, changed = true;
        ImGui::PopID();
        float on = brand::clamp01(1 - fabsf(*slide - i));
        float fs = ImGui::GetFontSize(), tw = f_strong->CalcTextSizeA(fs, FLT_MAX, 0, options[i]).x;
        dl->AddText(f_strong, fs, {p.x + i * cell + (cell - tw) / 2, p.y + (h - fs) / 2}, C(brand::mix(T.soft, T.accent_ink, on)), options[i]);
    }
    ImGui::SetCursorScreenPos({p.x, p.y + h});
    ImGui::Dummy({width, 0});
    ImGui::PopID();
    return changed;
}

inline void pill(ImDrawList* dl, ImVec2 at, const char* text, ImU32 dot, float pulse) {
    float fs = font_size(f_small), h = fs + S(10), w = f_small->CalcTextSizeA(fs, FLT_MAX, 0, text).x + S(30);
    dl->AddRectFilled(at, {at.x + w, at.y + h}, C(T.raised), h / 2);
    dl->AddRect(at, {at.x + w, at.y + h}, C(T.line), h / 2);
    ImVec2 c{at.x + S(12), at.y + h / 2};
    dl->AddCircleFilled(c, S(3.5f), C(dot));
    if (pulse >= 0) dl->AddCircle(c, S(3.5f) + S(6) * pulse, C(dot, 0.5f * (1 - pulse)), 0, S(1.2f));
    dl->AddText(f_small, fs, {at.x + S(22), at.y + (h - fs) / 2}, C(T.soft), text);
}

#if !FATRAINER_VISUALS
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
inline const ItemInfo* g_edit_info = nullptr;
inline bool g_edit_request = false;
inline const char* RARITY[] = {"Gray", "Green", "Blue", "Purple", "Orange", "Gold"};
inline const ImU32 RARITY_COLORS[] = {IM_COL32(170, 168, 162, 255), IM_COL32(110, 200, 95, 255), IM_COL32(80, 150, 245, 255),
                                      IM_COL32(175, 110, 245, 255), IM_COL32(243, 154, 30, 255), IM_COL32(235, 205, 90, 255)};
const float NO_RELOAD_MAGAZINE = 999, NO_RELOAD_TIME = 0.05f;

inline bool editable(const game::Item& it) {
    if (!it.info) return false;
    int c = cat_of(it.info->id);
    return c == C_WEAPONS || c == C_THROWABLE || !strncmp(it.info->id, "Flashlight_", 11) || !strncmp(it.info->id, "Special_Hook", 12);
}

inline void open_editor(const game::Item& it, const std::string& id) {
    g_edit_desc = game::item_desc(it), g_edit_item = it.addr, g_edit_name = it.name, g_edit_id = id, g_edit_info = it.info;
    g_edit_request = true;
}

inline void remember_edits() {
    cheats::set_weapon_edits(config::cfg.weapon_edits);
    save_config();
}

inline void edit_stat(int stat, float v) {
    logf("edit: %s %s %g -> %g", g_edit_id.c_str(), STAT_KEYS[stat], game::get_stat(g_edit_desc, stat), v);
    uintptr_t desc = g_edit_desc;
    on_game_thread([=] {
        std::lock_guard<std::mutex> l(game::mx);
        game::set_stat(desc, stat, v);
    });
    config::cfg.weapon_edits[g_edit_id][STAT_KEYS[stat]] = v;
    remember_edits();
}

inline float game_value(int stat) { return g_edit_info ? g_edit_info->st[stat] : NAN; }

inline void reset_stat(int stat) {
    float original = game_value(stat);
    auto it = config::cfg.weapon_edits.find(g_edit_id);
    if (it != config::cfg.weapon_edits.end() && it->second.erase(STAT_KEYS[stat]) && it->second.empty()) config::cfg.weapon_edits.erase(it);
    remember_edits();
    if (std::isnan(original)) return;
    uintptr_t desc = g_edit_desc;
    on_game_thread([=] {
        std::lock_guard<std::mutex> l(game::mx);
        game::write_stat(desc, stat, original);
        game::g.stat_overrides.erase({desc, stat});
    });
}

inline bool edited(int stat) {
    auto it = config::cfg.weapon_edits.find(g_edit_id);
    return it != config::cfg.weapon_edits.end() && it->second.count(STAT_KEYS[stat]);
}

struct EditRow { int stat; const char* name; const char* fmt; float mul; };
inline const EditRow EDIT_ROWS[] = {{ST_Damage, "Damage", "%.1f", 2},           {ST_Condition, "Durability", "%.0f", 10},
                                    {ST_CriticalProb, "Critical chance", "%.2f", 2}, {ST_CriticalDamage, "Critical damage", "%.2f", 2},
                                    {ST_Force, "Knockback", "%.1f", 2},           {ST_StaminaUsage, "Stamina per swing", "%.3f", 0.5f},
                                    {ST_DamageRange, "Reach", "%.2f", 1.5f},      {ST_AmmoCount, "Magazine", "%.0f", 2},
                                    {ST_ReloadTime, "Reload time", "%.2f", 0.5f}, {ST_AllowedRepairs, "Repairs left", "%.0f", 0},
                                    {ST_UpgradeLevel, "Upgrade level", "%.0f", 0}, {ST_MaxStackCount, "Max stack", "%.0f", 0},
                                    {ST_DepletionTime, "Battery", "%.0f", 10},    {ST_Price, "Price", "%.0f", 0}};

inline void stat_cell(const EditRow& r, float v, float width) {
    ImGui::PushID(r.stat);
    bool changed = edited(r.stat);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, V(T.frame, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_Border, changed ? V(T.accent, 0.55f) : V(T.line_soft));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(14), S(12)});
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::BeginChild("cell", {width, 0}, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, CARD_FLAGS);
    float top = ImGui::GetCursorPosY();
    caps(r.name, changed ? T.accent : 0);
    float original = game_value(r.stat);
    if (changed && !std::isnan(original)) {
        char was[48];
        snprintf(was, sizeof was, "Game %g", original);
        float w = f_small->CalcTextSizeA(font_size(f_small), FLT_MAX, 0, was).x;
        ImGui::SameLine(ImGui::GetContentRegionMax().x - w);
        ImGui::SetCursorPosY(top);
        label(was);
    }
    ImGui::Dummy({0, S(2)});
    float bw = S(58), gap = S(6);
    ImGui::PushFont(f_tile);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{0, 0, 0, 0});
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - bw * 2 - gap * 2);
    float e = v;
    ImGui::InputFloat("##v", &e, 0, 0, r.fmt);
    bool done = ImGui::IsItemDeactivatedAfterEdit() && e != v;
    float h = ImGui::GetItemRectSize().y;
    ImGui::PopStyleColor();
    ImGui::PopFont();
    if (done) edit_stat(r.stat, e);
    char quick[16];
    snprintf(quick, sizeof quick, r.mul > 0 ? "x%g" : "+1", r.mul);
    ImGui::SameLine(0, gap);
    if (ImGui::Button(quick, {bw, h})) edit_stat(r.stat, r.mul > 0 ? v * r.mul : v + 1);
    ImGui::SameLine(0, gap);
    ImGui::BeginDisabled(!changed);
    if (ImGui::Button("Undo", {bw, h})) reset_stat(r.stat);
    ImGui::EndDisabled();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::PopID();
}

inline void rarity_picker() {
    int cur = game::rarity(g_edit_item, g_edit_desc);
    if (cur < 0 || cur >= 6) return;
    caps("RARITY OF THIS ONE");
    ImGui::Dummy({0, S(2)});
    for (int i = 0; i < 6; i++) {
        if (i) ImGui::SameLine(0, S(6));
        bool on = i == cur;
        ImGui::PushStyleColor(ImGuiCol_Button, on ? V(RARITY_COLORS[i], 0.28f) : ImVec4{1, 1, 1, 0.04f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, V(RARITY_COLORS[i], 0.2f));
        ImGui::PushStyleColor(ImGuiCol_Text, on ? V(RARITY_COLORS[i]) : V(T.soft));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(20));
        if (ImGui::Button(RARITY[i]) && !on) {
            logf("edit: %s rarity -> %s", g_edit_id.c_str(), RARITY[i]);
            uintptr_t item = g_edit_item, desc = g_edit_desc;
            on_game_thread([=] {
                std::lock_guard<std::mutex> l(game::mx);
                game::set_rarity(item, desc, i);
            });
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }
    ImGui::Dummy({0, S(10)});
}

inline bool has_own_stat(int stat);

inline void editor_extras() {
    const char* id = g_edit_id.c_str();
    bool gun = has_own_stat(ST_AmmoCount) && game::firearm_desc(g_edit_desc);
    bool bow = strstr(id, "Bow") != nullptr, melee = g_edit_info && g_edit_info->melee;
    bool flashlight = !strncmp(id, "Flashlight_", 11), hook = !strncmp(id, "Special_Hook", 12);
    if (!gun && !bow && !melee && !flashlight && !hook) return;
    caps("EXTRAS");
    if (gun) {
        bool no_reload = game::get_stat(g_edit_desc, ST_AmmoCount) >= NO_RELOAD_MAGAZINE;
        if (switch_row("No reload for this weapon", "A 999 round magazine and an instant reload, for every one of this type.", no_reload)) {
            if (no_reload) {
                edit_stat(ST_AmmoCount, NO_RELOAD_MAGAZINE);
                if (!std::isnan(game::get_stat(g_edit_desc, ST_ReloadTime))) edit_stat(ST_ReloadTime, NO_RELOAD_TIME);
            } else {
                reset_stat(ST_AmmoCount), reset_stat(ST_ReloadTime);
            }
        }
    }
    if (gun || bow) cheat_switch("ammo", "Infinite ammo, every weapon");
    if (melee) cheat_switch("durability", "Unbreakable, every melee weapon");
    if (flashlight) cheat_switch("uv", "Infinite UV light");
    if (hook) cheat_switch("hook", "Infinite grappling hook");
    ImGui::Dummy({0, S(6)});
}

inline bool has_own_stat(int stat) {
    if (!game::has_stat(stat) || std::isnan(game::get_stat(g_edit_desc, stat))) return false;
    return !g_edit_info || !std::isnan(game_value(stat)) || edited(stat);
}

inline void editor_popup() {
    if (g_edit_request) {
        ImGui::OpenPopup("Edit weapon");
        g_edit_request = false;
    }
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 size{std::min(S(700), io.DisplaySize.x * 0.92f), std::min(S(780), io.DisplaySize.y * 0.88f)};
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_Appearing, {0.5f, 0.5f});
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(26), S(22)});
    if (!ImGui::BeginPopupModal("Edit weapon", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PopStyleVar();
        return;
    }
    Cat cat = category(g_edit_id.c_str());
    ImGui::PushFont(f_head);
    ImGui::TextUnformatted(g_edit_name.empty() ? g_edit_id.c_str() : g_edit_name.c_str());
    ImGui::PopFont();
    caps(cat.label, cat.color);
    label(g_edit_id.c_str());
    ImGui::Dummy({0, S(8)});
    float footer = ImGui::GetFrameHeight() + S(14);
    ImGui::BeginChild("body", {0, -footer}, ImGuiChildFlags_None);
    std::vector<const EditRow*> rows;
    for (auto& r : EDIT_ROWS)
        if (has_own_stat(r.stat)) rows.push_back(&r);
    if (!rows.empty()) {
        caps("STATS, EVERY ONE OF THIS TYPE");
        ImGui::Dummy({0, S(2)});
        float gap = S(10), cell = (ImGui::GetContentRegionAvail().x - gap) / 2;
        for (size_t i = 0; i < rows.size(); i++) {
            if (i % 2) ImGui::SameLine(0, gap);
            stat_cell(*rows[i], game::get_stat(g_edit_desc, rows[i]->stat), cell);
        }
        ImGui::Dummy({0, S(10)});
    }
    rarity_picker();
    editor_extras();
    if (rows.empty()) note("No editable stats found for this item. Press Refresh after loading your save and check fatrainer.log.");
    else note("Stat edits change every item of this type and come back each time you start the game. Rarity changes only this one.");
    ImGui::EndChild();
    ImGui::Dummy({0, S(4)});
    bool any = config::cfg.weapon_edits.count(g_edit_id) > 0;
    ImGui::BeginDisabled(!any);
    if (ImGui::Button("Undo every edit", {0, ImGui::GetFrameHeight()}))
        for (auto& r : EDIT_ROWS)
            if (edited(r.stat)) reset_stat(r.stat);
    ImGui::EndDisabled();
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - S(120));
    if (accent_button("Done", {S(120), 0})) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
    ImGui::PopStyleVar();
}

#endif

inline void search_box(const char* id, const char* hint, char* buf, size_t size) {
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint(id, hint, buf, size);
}

#if !FATRAINER_VISUALS
inline void delete_item(const game::Inventory& inv, const game::Item& it) {
    uintptr_t obj = inv.obj, item = it.addr;
    game::Kind kind = inv.kind;
    std::string name = it.name;
    on_game_thread([=] {
        std::lock_guard<std::mutex> l(game::mx);
        bool gone = game::remove_item(obj, item, kind);
        if (gone)
            for (auto& i : game::g.invs)
                if (i.obj == obj) i.items.erase(std::remove_if(i.items.begin(), i.items.end(), [&](const game::Item& x) { return x.addr == item; }), i.items.end());
        logf("delete %s from %s: %s", name.c_str(), game::KIND_NAMES[kind], gone ? "done" : "refused");
        toast(gone ? "Deleted " + name : "The game kept " + name + ". Press Refresh and try again.");
    });
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
        caps(title.c_str());
        if (!ImGui::BeginTable(("t" + std::to_string(i)).c_str(), 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) continue;
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Amount", ImGuiTableColumnFlags_WidthFixed, S(322));
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
            if (ImGui::Button("-", {bw, 0})) {
                if (v > 1) game::set_count(it, v - 1);
                else delete_item(inv, it);
            }
            ImGui::SameLine(0, S(4));
            ImGui::SetNextItemWidth(S(70));
            int e = v;
            ImGui::InputInt("##c", &e, 0, 0);
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                if (e > 0) game::set_count(it, e);
                else delete_item(inv, it);
            }
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
            ImGui::SameLine(0, S(4));
            static uintptr_t confirm_delete_item = 0;
            bool sure = confirm_delete_item == it.addr;
            if (ImGui::Button(sure ? "Sure?" : "Delete", {S(64), 0})) {
                if (sure) delete_item(inv, it), confirm_delete_item = 0;
                else confirm_delete_item = it.addr;
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
    ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    float fill = animate(ImGui::GetID("fill"), (t.factor - 1.0f) / (t.max - 1.0f), 18);
    if (fill > 0.002f) ImGui::GetWindowDrawList()->AddRectFilled(a, {a.x + (b.x - a.x) * fill, b.y}, C(T.accent, 0.16f), R(8));
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
        stat_tile("health", "HEALTH", cheats::health(), best_health, T.bad, tile, [](float v) {
            on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_health(std::max(v, 1.0f)); });
        });
        ImGui::SameLine(0, S(12));
        stat_tile("stamina", "STAMINA", cheats::stamina(), cheats::stamina_full(), T.ok, tile, [](float v) {
            on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_stamina(v); });
        });
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
    ImGui::Dummy({0, S(4)});
    tweak_sliders(cheats::G_SURVIVAL);
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

inline void kill_nearby() {
    std::thread([] {
        cheats::scan_enemies();
        on_game_thread([] {
            std::lock_guard<std::mutex> l(game::mx);
            int n = cheats::kill_enemies(cheats::KILL_RADIUS);
            toast(n ? "Killed " + std::to_string(n) + (n == 1 ? " enemy" : " enemies") : std::string("No living enemies within 80 m"));
        });
    }).detach();
}

inline void kill_all_button() {
    if (accent_button("Kill all enemies nearby", {ImGui::GetContentRegionAvail().x, S(38)})) kill_nearby();
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
    cheat_switch("last_wish");
    cheat_switch("supplies");
    end_card();
    begin_card("weapons", "WEAPONS");
    cheat_switch("durability");
    cheat_switch("item_dupe");
    ImGui::Dummy({0, S(4)});
    tweak_sliders(cheats::G_WEAPONS);
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

inline ImGuiID g_capturing_key = 0;

inline void take_key_from_others(int vk, const std::string& keeper) {
    for (auto it = config::cfg.cheat_keys.begin(); it != config::cfg.cheat_keys.end();)
        it = it->second == vk && it->first != keeper ? config::cfg.cheat_keys.erase(it) : std::next(it);
    for (int& spit : config::cfg.spit_keys)
        if (spit == vk && keeper.rfind("spit", 0) != 0) spit = 0;
}

inline bool key_button(const char* id, int& vk) {
    static bool waiting_release = false;
    ImGuiID me = ImGui::GetID(id);
    bool changed = false;
    if (g_capturing_key == me) {
        int k = 0;
        bool any = pressed_key(k);
        if (waiting_release) waiting_release = any;
        else if (any) {
            bool reserved = k == VK_ESCAPE || k == config::cfg.menu_key || k == VK_F8;
            if (!reserved) vk = k == VK_BACK || k == VK_DELETE ? 0 : k, changed = true;
            g_capturing_key = 0;
        }
    }
    std::string text = g_capturing_key == me ? "Press a key..." : vk ? key_name(vk) : "Not set";
    if (ImGui::Button((text + "###" + id).c_str(), {S(170), 0})) g_capturing_key = me, waiting_release = true;
    ImGui::SameLine(0, S(8));
    ImGui::BeginDisabled(!vk);
    if (ImGui::Button((std::string("Clear##") + id).c_str(), {S(70), 0})) vk = 0, changed = true;
    ImGui::EndDisabled();
    return changed;
}

inline void capture_note() {
    if (g_capturing_key) note("Esc cancels, Backspace or Delete removes the key. The menu key and F8 cannot be used.");
}

inline void spit_keys_card() {
    begin_card("spit_keys", "SPIT KEYS");
    note("Pick your own key for each spit type. The trainer presses the game's key for it (1 to 4) when you press yours while the menu is closed.");
    for (int i = 0; i < config::SPIT_KEYS; i++) {
        ImGui::PushID(i);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(SPIT_NAMES[i]);
        ImGui::SameLine(S(220));
        int& vk = config::cfg.spit_keys[i];
        if (key_button("spit", vk)) {
            if (vk) take_key_from_others(vk, "spit");
            save_config();
        }
        ImGui::SameLine(0, S(12));
        label((std::string("game key ") + SPIT_GAME_KEY_NAMES[i]).c_str());
        ImGui::PopID();
    }
    capture_note();
    end_card();
}

inline void zombie_page() {
    begin_card("about");
    note("For Be The Zombie matches, where you play the Night Hunter and invade another player's game. Turn these on before or during a match. "
         "Built-in Legit and Rage configs are on the PvP page and in Settings.");
    end_card();
    begin_card("hunter", "HUNTER");
    cheat_switch("god", "Hunter god mode");
    cheat_switch("z_energy");
    ImGui::Dummy({0, S(4)});
    tweak_slider(*cheats::find_tweak("damage_taken"));
    tweak_sliders(cheats::G_HUNTER);
    end_card();
    begin_card("abilities", "ABILITIES");
    cheat_switch("z_cooldowns");
    cheat_switch("z_spits");
    cheat_switch("z_pound_hits");
    cheat_switch("z_camo");
    end_card();
    spit_keys_card();
    note("Attack ranges and aim angles are on the PvP page.");
}

inline const char* SURVIVOR_RANKS[] = {"Prey", "Casualty", "Endangered", "Underdog", "Runner", "Contender",
                                       "Challenger", "Fighter", "Dominant", "Ruthless", "Indomitable", "Ultimate Survivor"};
inline const char* HUNTER_RANKS[] = {"Walker", "Runner", "Biter", "Bolter", "Stalker", "Beast",
                                     "Mauler", "Juggernaut", "Widow Maker", "Carnivore", "Hunter", "Apex Predator"};

inline void rank_row(const char* label, bool zombie, const char* const* names) {
    int points = cheats::pvp_rank(zombie);
    if (points < 0) return;
    int title = cheats::rank_title(points);
    ImGui::PushID(label);
    ImGui::TableNextRow(0, S(44));
    ImGui::BeginDisabled(zombie ? cheats::hunter_is_someone_else() : cheats::playing_hunter());
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(names[title]);
    ImGui::SameLine(0, S(6));
    ImGui::TextDisabled("%s pts", thousands(points).c_str());
    ImGui::TableNextColumn();
    struct Step { const char* text; int target; };
    for (Step st : {Step{"Lowest", 0}, Step{"-1", title - 1}, Step{"+1", title + 1}, Step{"Highest", cheats::RANK_TITLES}}) {
        if (ImGui::Button(st.text, {S(st.text[0] == '-' || st.text[0] == '+' ? 56 : 84), 0})) {
            int target = st.target;
            on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_pvp_title(zombie, target); });
        }
        ImGui::SameLine(0, S(6));
    }
    ImGui::NewLine();
    ImGui::EndDisabled();
    ImGui::PopID();
}

inline void ranks_card() {
    if (cheats::pvp_rank(false) < 0 && cheats::pvp_rank(true) < 0) return;
    begin_card("ranks", "BE THE ZOMBIE RANKS");
    note("Your PvP rank as a survivor and as the Night Hunter. In a Be The Zombie match the side you are not playing is grayed out. The game may adjust it again after your next Be The Zombie match.");
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
}

struct TreeView { const char* name; int type, level, max; float progress, points; uint32_t xp, span; };

inline std::vector<TreeView> tree_views() {
    static const bool sample = getenv("DLT_SKILLS") != nullptr;
    std::vector<TreeView> out;
    if (sample) {
        int levels[] = {12, 18, 24, 37, 9, 0, 0, 3}, maxes[] = {25, 24, 24, 250, 25, 0, 0, 0};
        float progress[] = {0.62f, 0.35f, 1, 0.14f, 0.5f, 0, 0, 0}, points[] = {3, 0, 2, 5, NAN, 0, 0, 0};
        for (size_t i = 0; i < std::size(cheats::TREES); i++)
            if (maxes[i])
                out.push_back({cheats::TREES[i].name, cheats::TREES[i].type, levels[i], maxes[i], progress[i], points[i],
                               (uint32_t)(progress[i] * 48000), 48000});
        return out;
    }
    std::vector<int> learned = cheats::skill_levels_now();
    for (auto& t : cheats::TREES) {
        int max = cheats::tree_max(t.type), level = cheats::tree_level(t.type);
        if (max && level >= 0)
            out.push_back({t.name, t.type, level, max, cheats::tree_progress(t.type), cheats::skill_points(t.type) - cheats::spent_points(t.type, learned),
                           cheats::tree_xp(t.type) - cheats::tree_level_start(t.type), cheats::tree_span(t.type)});
    }
    return out;
}

inline void tree_tile(const TreeView& t, float width) {
    ImGui::PushID(t.type);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, V(T.raised, std::max(0.75f, config::cfg.look.opacity)));
    ImGui::PushStyleColor(ImGuiCol_Border, V(T.line_soft));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(18), S(16)});
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    bool has_points = !std::isnan(t.points);
    ImGui::BeginChild("tree", {width, S(has_points ? 262 : 214)}, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    bool maxed = t.level >= t.max;
    std::string caption = t.name;
    for (auto& ch : caption) ch = (char)toupper((unsigned char)ch);
    caps(caption.c_str(), maxed ? T.accent : 0);
    ImGui::Dummy({0, S(2)});
    ImGui::PushFont(f_tile);
    ImGui::Text("%d", t.level);
    ImGui::PopFont();
    ImGui::SameLine(0, S(6));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + S(10));
    ImGui::TextDisabled(maxed ? "/ %d  max" : "/ %d", t.max);
    int type = t.type, level = t.level;
    float picked = 0;
    ImGui::BeginDisabled(maxed);
    if (drag_bar("xp", t.progress, T.accent, picked))
        on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_tree_progress(type, picked); });
    ImGui::EndDisabled();
    ImGui::PushFont(f_small);
    if (maxed) ImGui::TextDisabled("Highest level reached");
    else ImGui::TextDisabled("%s / %s XP to the next level", thousands(t.xp).c_str(), thousands(t.span).c_str());
    ImGui::PopFont();
    ImGui::Dummy({0, S(4)});
    float w = ImGui::GetContentRegionAvail().x, gap = S(5), small = S(44);
    float add = std::max(S(80), w - small * 4 - gap * 4);
    auto step = [&](const char* text, int target) {
        if (ImGui::Button(text, {small, 0})) on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_tree_level(type, target); });
        ImGui::SameLine(0, gap);
    };
    ImGui::BeginDisabled(level <= 0);
    step("-10", level - 10);
    step("-1", level - 1);
    ImGui::EndDisabled();
    ImGui::BeginDisabled(maxed);
    if (accent_button("+1 point", {add, 0})) on_game_thread([=] {
        std::lock_guard<std::mutex> l(game::mx);
        if (cheats::level_from_xp_fn) cheats::level_up_with_xp(type);
        else cheats::set_tree_level(type, level + 1);
    });
    ImGui::SameLine(0, gap);
    step("+10", level + 10);
    if (ImGui::Button("Max", {small, 0})) on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_tree_level(type, cheats::tree_max(type)); });
    ImGui::EndDisabled();
    if (has_points) {
        ImGui::Dummy({0, S(4)});
        ImVec2 line = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(line, {line.x + w, line.y}, C(T.line_soft));
        ImGui::Dummy({0, S(8)});
        float right = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Free skill points");
        ImGui::SameLine(0, S(8));
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(V(T.accent), "%.0f", t.points);
        int extra = cheats::extra_skill_points[type];
        if (extra) {
            ImGui::SameLine(0, S(6));
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("(%+d from FaTrainer)", extra);
        }
        ImGui::SameLine(right - small * 2 - gap);
        if (ImGui::Button("-##points", {small, 0})) cheats::extra_skill_points[type]--;
        ImGui::SameLine(0, gap);
        if (ImGui::Button("+##points", {small, 0})) cheats::extra_skill_points[type]++;
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::PopID();
}

struct SkillTab { int tree; const char* name; };
inline const SkillTab SKILL_TABS[] = {{3, "Survivor"}, {1, "Agility"}, {2, "Power"}, {6, "Driver"}, {5, "Legend"}, {0, "Night Hunter"}};
inline const char* SKILL_BANDS[] = {"NOVICE", "ADEPT", "EXPERT"};
inline const float SKILL_BAND_EDGES[] = {0, 179.0f / 520, 340.0f / 520, 1};

inline float skill_aspect(int tree) {
    for (auto& c : SKILL_CANVASES)
        if (c.tree == tree) return c.aspect;
    return 4.0f / 3;
}

inline std::string skill_initials(const char* name) {
    std::string first, out;
    bool start = true;
    for (const char* p = name; *p; p++) {
        bool word = isalnum((unsigned char)*p);
        if (word && start) out += (char)toupper((unsigned char)*p);
        if (word && out.size() == 1) first += *p;
        start = !word;
    }
    if (out.size() == 1 && first.size() > 1) out += (char)tolower((unsigned char)first[1]);
    return out.substr(0, 2);
}

inline void skill_shape(ImDrawList* dl, ImVec2 c, float r, bool hexagon, ImU32 fill, ImU32 border, float thickness) {
    if (!hexagon) {
        float h = r * 0.86f;
        dl->AddRectFilled({c.x - h, c.y - h}, {c.x + h, c.y + h}, fill, R(6));
        dl->AddRect({c.x - h, c.y - h}, {c.x + h, c.y + h}, border, R(6), 0, thickness);
        return;
    }
    ImVec2 pts[6];
    for (int k = 0; k < 6; k++) {
        float a = (k * 60.0f - 90.0f) * 3.14159265f / 180.0f;
        pts[k] = {c.x + cosf(a) * r * 1.08f, c.y + sinf(a) * r * 1.08f};
    }
    dl->AddConvexPolyFilled(pts, 6, fill);
    dl->AddPolyline(pts, 6, border, ImDrawFlags_Closed, thickness);
}

inline std::vector<int> sample_skill_levels() {
    std::vector<int> out(std::size(SKILLS));
    for (size_t i = 0; i < out.size(); i++) out[i] = SKILLS[i].max_level > 1 ? (int)(i * 7 % 26) : (i % 3 != 2 && SKILLS[i].level_req < 12);
    return out;
}

inline void skill_tree_card() {
    static const bool sample = getenv("DLT_SKILLS") != nullptr;
    std::vector<int> levels = sample ? sample_skill_levels() : cheats::skill_levels_now();
    if (levels.size() != std::size(SKILLS)) return;
    std::vector<const SkillTab*> tabs;
    for (auto& t : SKILL_TABS) {
        bool shown = sample || (cheats::tree_max(t.tree) > 0 && (t.tree != 0 || cheats::side != cheats::SIDE_SURVIVOR));
        for (size_t i = 0; shown && i < std::size(SKILLS); i++)
            if (SKILLS[i].tree == t.tree && levels[i] >= 0) {
                tabs.push_back(&t);
                break;
            }
    }
    if (tabs.empty() || !cheats::add_skill_fn && !sample) return;
    begin_card("skilltree", "SKILL TREE");
    note("Click a skill to learn it, click it again to remove it. Learning a skill also learns the skills it needs; removing one also removes the skills that need it. "
         "Skills with levels: click adds a level, right click takes one away. Changes go into your save like skills you buy in the game.");
    ImGui::Dummy({0, S(6)});
    static int tab = 0;
    tab = std::clamp(tab, 0, (int)tabs.size() - 1);
    std::vector<const char*> names;
    for (auto* t : tabs) names.push_back(t->name);
    float width = ImGui::GetContentRegionAvail().x;
    segmented("skilltabs", names.data(), (int)names.size(), tab, width);
    int tree = tabs[tab]->tree;
    ImGui::Dummy({0, S(10)});
    float aspect = skill_aspect(tree);
    float h = std::min(width / aspect, S(560)), w = h * aspect;
    ImVec2 origin = ImGui::GetCursorScreenPos();
    origin.x += (width - w) / 2;
    auto* dl = ImGui::GetWindowDrawList();
    bool banded = tree == 1 || tree == 2 || tree == 3 || tree == 6;
    if (banded)
        for (int b = 0; b < 3; b++) {
            ImVec2 a{origin.x, origin.y + SKILL_BAND_EDGES[b] * h}, z{origin.x + w, origin.y + SKILL_BAND_EDGES[b + 1] * h - S(2)};
            dl->AddRectFilled(a, z, C(b % 2 ? T.frame : T.raised_hi, 0.55f), R(8));
            brand::spaced_caps(dl, f_label, font_size(f_label), {a.x + S(10), a.y + S(8)}, SKILL_BANDS[b], C(T.muted), S(1.4f));
        }
    else
        dl->AddRectFilled(origin, {origin.x + w, origin.y + h}, C(T.raised_hi, 0.45f), R(8));
    float r = std::clamp(w / 692.0f * 23.0f, S(14), S(26));
    auto center = [&](const SkillInfo& s) { return ImVec2{origin.x + s.x * w, origin.y + s.y * h}; };
    for (size_t i = 0; i < std::size(SKILLS); i++) {
        const SkillInfo& s = SKILLS[i];
        if (s.tree != tree) continue;
        for (const char* need : s.needs) {
            int k = need ? cheats::skill_index(need) : -1;
            if (k < 0 || SKILLS[k].tree != tree) continue;
            bool lit = levels[i] > 0 && levels[k] > 0;
            dl->AddLine(center(SKILLS[k]), center(s), C(lit ? T.accent : T.line, lit ? 0.9f : 0.8f), S(lit ? 3.0f : 2.0f));
        }
    }
    static int focus = -1;
    int hovered = -1;
    for (size_t i = 0; i < std::size(SKILLS); i++) {
        const SkillInfo& s = SKILLS[i];
        if (s.tree != tree) continue;
        ImVec2 c = center(s);
        ImGui::SetCursorScreenPos({c.x - r, c.y - r});
        ImGui::PushID((int)i);
        ImGui::InvisibleButton("skill", {r * 2, r * 2}, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        bool over = ImGui::IsItemHovered();
        bool left = ImGui::IsItemClicked(ImGuiMouseButton_Left), right = ImGui::IsItemClicked(ImGuiMouseButton_Right);
        ImGui::PopID();
        if (over) hovered = (int)i, ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        int level = levels[i];
        bool learned = level > 0, ready = true;
        for (const char* need : s.needs) {
            int k = need ? cheats::skill_index(need) : -1;
            if (k >= 0 && levels[k] <= 0) ready = false;
        }
        float hover = animate(ImGui::GetID((int)i + 9000), over ? 1.0f : 0.0f, 18);
        float lit = animate(ImGui::GetID((int)i + 19000), learned ? 1.0f : 0.0f, 12);
        ImU32 fill = brand::mix(brand::mix(ready || learned ? T.frame : T.ground, T.raised_hi, hover), T.accent, lit);
        ImU32 border = learned ? T.accent : ready ? brand::mix(T.line, T.accent, 0.35f + 0.65f * hover) : brand::mix(T.line, T.soft, 0.25f + 0.5f * hover);
        float grow = r * (1 + 0.08f * hover * motion());
        if (lit > 0.01f) skill_shape(dl, c, grow + S(5), s.node, C(T.accent, 0.10f * lit), C(T.accent, 0.0f), 1);
        skill_shape(dl, c, grow, s.node, C(fill), C(border), S(learned ? 2.0f : 1.4f));
        std::string mark = skill_initials(s.name);
        float fs = font_size(f_strong) * std::clamp(r / S(22), 0.7f, 1.1f);
        ImVec2 tsz = f_strong->CalcTextSizeA(fs, FLT_MAX, 0, mark.c_str());
        ImU32 ink = learned ? T.accent_ink : ready ? T.text : T.muted;
        dl->AddText(f_strong, fs, {c.x - tsz.x / 2, c.y - tsz.y / 2}, C(ink), mark.c_str());
        if (s.max_level > 1) {
            std::string count = std::to_string(std::max(0, level)) + "/" + std::to_string(s.max_level);
            float ls = font_size(f_label);
            ImVec2 csz = f_label->CalcTextSizeA(ls, FLT_MAX, 0, count.c_str());
            dl->AddText(f_label, ls, {c.x - csz.x / 2, c.y + r + S(5)}, C(learned ? T.accent : T.soft), count.c_str());
        }
        if ((left || right) && !sample && level >= 0) {
            int idx = (int)i, target = s.max_level > 1 ? level + (left ? 1 : -1) : learned ? 0 : 1;
            bool multi = s.max_level > 1;
            on_game_thread([idx, target, multi, learned] {
                std::lock_guard<std::mutex> l(game::mx);
                if (multi) cheats::set_skill_level(idx, target);
                else if (learned) cheats::forget_skill(idx);
                else cheats::learn_skill(idx);
                cheats::read_skill_levels();
            });
        }
    }
    if (hovered >= 0) focus = hovered;
    if (focus >= 0 && SKILLS[focus].tree != tree) focus = -1;
    ImGui::SetCursorScreenPos({origin.x - (width - w) / 2, origin.y + h + S(14)});
    ImGui::Dummy({width, 0});
    int count = 0, have = 0;
    for (size_t i = 0; i < std::size(SKILLS); i++)
        if (SKILLS[i].tree == tree) count++, have += levels[i] > 0;
    if (focus >= 0) {
        const SkillInfo& s = SKILLS[focus];
        ImGui::PushFont(f_strong);
        ImGui::TextUnformatted(s.name);
        ImGui::PopFont();
        ImGui::SameLine(0, S(10));
        ImGui::AlignTextToFramePadding();
        int level = levels[focus];
        if (s.max_level > 1) ImGui::TextColored(V(level > 0 ? T.accent : T.muted), "Level %d of %d", std::max(0, level), s.max_level);
        else ImGui::TextColored(V(level > 0 ? T.accent : T.muted), level > 0 ? "Learned" : "Not learned");
        note(s.desc);
        std::string needs;
        for (const char* need : s.needs) {
            int k = need ? cheats::skill_index(need) : -1;
            if (k >= 0) needs += std::string(needs.empty() ? "" : ", ") + SKILLS[k].name;
        }
        std::string info;
        if (!needs.empty()) info += "Needs " + needs + ".  ";
        if (s.level_req > 0) info += std::string(tabs[tab]->name) + " level " + std::to_string(s.level_req) + " in the game.  ";
        if (s.prestige_req > 0) info += "Prestige " + std::to_string(s.prestige_req) + " in the game.  ";
        if (s.cost > 0) info += std::to_string(s.cost) + (s.cost == 1 ? " skill point" : " skill points") + (s.max_level > 1 ? " per level." : ".");
        ImGui::PushFont(f_small);
        if (!info.empty()) ImGui::TextDisabled("%s", info.c_str());
        ImGui::PopFont();
    } else {
        ImGui::PushFont(f_small);
        ImGui::TextDisabled("Point at a skill to see what it does.");
        ImGui::PopFont();
    }
    ImGui::Dummy({0, S(6)});
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%d of %d learned", have, count);
    float points = sample ? 3.0f : cheats::skill_points(tree) - cheats::spent_points(tree, levels);
    if (!std::isnan(points)) {
        ImGui::SameLine(0, S(16));
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(V(points > 0 ? T.accent : T.muted), "%.0f free skill points", points);
    }
    float bw = S(118), gap = S(8);
    ImGui::SameLine(ImGui::GetContentRegionMax().x - bw * 2 - gap);
    ImGui::BeginDisabled(have == 0 || sample);
    if (ImGui::Button("Remove all", {bw, 0}))
        on_game_thread([tree] { std::lock_guard<std::mutex> l(game::mx); cheats::set_whole_tree(tree, false); cheats::read_skill_levels(); });
    ImGui::EndDisabled();
    ImGui::SameLine(0, gap);
    ImGui::BeginDisabled(have == count || sample);
    if (accent_button("Learn all", {bw, 0}))
        on_game_thread([tree] { std::lock_guard<std::mutex> l(game::mx); cheats::set_whole_tree(tree, true); cheats::read_skill_levels(); });
    ImGui::EndDisabled();
    end_card();
}

inline void skills_page() {
    begin_card("xp", "EXPERIENCE");
    tweak_sliders(cheats::G_PROGRESS);
    end_card();
    static const bool sample = getenv("DLT_SKILLS") != nullptr;
    if (!cheats::player && !sample) return empty_state("Your character was not found yet. Load into your save, then press Refresh.");
    if (!cheats::set_level_fn && !sample) return empty_state("The game's level function was not found. Check fatrainer.log.");
    auto trees = tree_views();
    if (trees.empty()) return empty_state("Your skill trees could not be read yet. Wait until your save has fully loaded, then press Refresh.");
    begin_card("trees", "SKILL POINTS");
    note("Each level is one skill point to spend in the game's skill menu. +1 point gives you exactly the XP for the next level, like playing would. "
         "Drag an XP bar to set the XP within a level. Free skill points - and + add points without changing the level; they last until you close the game.");
    end_card();
    float tile = (ImGui::GetContentRegionAvail().x - S(12)) / 2;
    for (size_t i = 0; i < trees.size(); i++) {
        if (i % 2) ImGui::SameLine(0, S(12));
        tree_tile(trees[i], tile);
        if (i % 2 || i + 1 == trees.size()) ImGui::Dummy({0, S(4)});
    }
    skill_tree_card();
    ranks_card();
}

inline void apply_preset(const cheats::Preset& p) {
    cheats::apply_profile(cheats::preset_profile(p));
    auto& e = config::cfg.esp;
    bool rage = p.esp == cheats::ESP_RAGE;
    e.on = e.role = e.health_bar = e.distance = true;
    e.box = e.health_text = e.rank = e.rage = e.snaplines = e.allies = rage;
    e.max_distance = rage ? 1000.0f : 300.0f;
    save_config();
    toast(std::string("Loaded config ") + p.role + " " + p.style);
}

inline void preset_tile(const cheats::Preset& p, float w, float h) {
    ImGui::PushID(p.key);
    ImGui::BeginGroup();
    ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
    bool on = cheats::preset_active(p);
    bool rage = p.esp == cheats::ESP_RAGE;
    float hover = animate(ImGui::GetID("hover"), ImGui::IsMouseHoveringRect(a, b) ? 1.0f : 0.0f);
    float lit = animate(ImGui::GetID("on"), on ? 1.0f : 0.0f);
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, b, C(brand::mix(T.frame, IM_COL32(34, 32, 29, 255), hover)), R(10));
    if (lit > 0.01f) dl->AddRectFilled(a, b, C(T.accent, 0.08f * lit), R(10));
    dl->AddRect(a, b, C(brand::mix(brand::mix(T.line_soft, T.line, hover), T.accent, lit)), R(10), 0, 1.0f + lit);
    float pad = S(16), fs = font_size(f_label);
    std::string role = p.role;
    std::transform(role.begin(), role.end(), role.begin(), ::toupper);
    brand::spaced_caps(dl, f_label, fs, {a.x + pad, a.y + pad}, role.c_str(), C(T.muted), S(1.4f));
    float big = font_size(f_tile);
    dl->AddText(f_tile, big, {a.x + pad, a.y + pad + fs + S(8)}, C(rage ? T.accent : T.text), p.style);
    float small = font_size(f_small);
    dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + big + S(14)}, C(T.soft), p.summary, nullptr, w - pad * 2);
    float bh = ImGui::GetFrameHeight();
    ImGui::Dummy({w, h});
    ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
    if (on) {
        if (ImGui::Button("Turn off", {S(110), 0})) cheats::all_off(), save_config();
        ImGui::SameLine(0, S(12));
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(V(T.accent), "Active");
    } else if (accent_button("Load", {S(96), 0})) {
        apply_preset(p);
    }
    static std::string confirm_hide;
    bool sure = confirm_hide == p.key;
    ImGui::SameLine(0, 0);
    ImGui::SetCursorScreenPos({b.x - pad - S(62), b.y - pad - bh});
    if (ImGui::Button(sure ? "Sure?" : "Delete", {S(62), 0})) {
        if (sure) {
            config::cfg.hidden_configs.push_back(p.key);
            confirm_hide.clear();
            save_config();
            toast(std::string("Deleted config ") + p.role + " " + p.style);
        } else {
            confirm_hide = p.key;
        }
    }
    ImGui::SetCursorScreenPos({a.x, b.y});
    ImGui::EndGroup();
    ImGui::PopID();
}

struct OwnConfigs { std::vector<std::string> names; std::map<std::string, config::Profile> data; bool loaded = false; };

inline OwnConfigs& own_configs(bool reload = false) {
    static OwnConfigs c;
    if (c.loaded && !reload) return c;
    c.names = config::list_profiles();
    c.data.clear();
    for (auto& n : c.names) config::load_profile(n, c.data[n]);
    c.loaded = true;
    return c;
}

inline std::string profile_summary(const config::Profile& p) {
    std::vector<std::string> parts;
    for (auto& k : p.cheats)
        if (auto* c = cheats::find(k)) parts.push_back(c->label);
    for (auto& [k, v] : p.tweaks)
        if (auto* t = cheats::find_tweak(k)) {
            char b[96];
            snprintf(b, sizeof b, "%s x%.1f", t->label, v);
            parts.push_back(b);
        }
    if (parts.empty()) return "Everything off.";
    const size_t SHOWN = 4;
    std::string out;
    for (size_t i = 0; i < parts.size() && i < SHOWN; i++) out += (i ? ", " : "") + parts[i];
    if (parts.size() > SHOWN) out += " and " + std::to_string(parts.size() - SHOWN) + " more";
    return out + ".";
}

inline void tile_frame(ImVec2 a, ImVec2 b, bool on, bool dashed = false) {
    float hover = animate(ImGui::GetID("hover"), ImGui::IsMouseHoveringRect(a, b) ? 1.0f : 0.0f);
    float lit = animate(ImGui::GetID("on"), on ? 1.0f : 0.0f);
    auto* dl = ImGui::GetWindowDrawList();
    ImU32 line = brand::mix(brand::mix(T.line_soft, T.line, hover), T.accent, lit);
    if (dashed) {
        line = brand::mix(T.line, T.accent, hover * 0.7f);
        dl->AddRectFilled(a, b, C(T.frame, 0.4f + 0.6f * hover), R(10));
        for (float x = a.x + S(12); x < b.x - S(12); x += S(10)) {
            dl->AddLine({x, a.y}, {std::min(x + S(5), b.x - S(12)), a.y}, C(line));
            dl->AddLine({x, b.y}, {std::min(x + S(5), b.x - S(12)), b.y}, C(line));
        }
        for (float y = a.y + S(12); y < b.y - S(12); y += S(10)) {
            dl->AddLine({a.x, y}, {a.x, std::min(y + S(5), b.y - S(12))}, C(line));
            dl->AddLine({b.x, y}, {b.x, std::min(y + S(5), b.y - S(12))}, C(line));
        }
        return;
    }
    dl->AddRectFilled(a, b, C(brand::mix(T.frame, IM_COL32(34, 32, 29, 255), hover)), R(10));
    if (lit > 0.01f) dl->AddRectFilled(a, b, C(T.accent, 0.08f * lit), R(10));
    dl->AddRect(a, b, C(line), R(10), 0, 1.0f + lit);
}

inline void own_config_tile(const std::string& name, const config::Profile& p, float w, float h) {
    static std::string renaming, confirm_delete;
    static char rename_to[48] = "";
    ImGui::PushID(name.c_str());
    ImGui::BeginGroup();
    ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
    bool on = cheats::profile_active(p);
    tile_frame(a, b, on);
    auto* dl = ImGui::GetWindowDrawList();
    float pad = S(16), fs = font_size(f_label), big = font_size(f_tile), small = font_size(f_small), bh = ImGui::GetFrameHeight();
    brand::spaced_caps(dl, f_label, fs, {a.x + pad, a.y + pad}, "YOUR CONFIG", C(T.muted), S(1.4f));
    ImGui::Dummy({w, h});
    if (renaming == name) {
        ImGui::SetCursorScreenPos({a.x + pad, a.y + pad + fs + S(8)});
        ImGui::SetNextItemWidth(w - pad * 2);
        if (ImGui::IsWindowAppearing() || !ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere();
        bool enter = ImGui::InputText("##rename", rename_to, sizeof rename_to, ImGuiInputTextFlags_EnterReturnsTrue);
        std::string clean = config::profile_name(rename_to);
        bool taken = clean != name && own_configs().data.count(clean);
        dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + bh + S(16)}, C(taken ? T.bad : T.soft),
                    taken ? "A config with this name already exists." : "Letters, numbers, spaces, - and _.", nullptr, w - pad * 2);
        ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
        ImGui::BeginDisabled(clean.empty() || taken);
        if (accent_button("Rename", {S(110), 0}) || (enter && !clean.empty() && !taken)) {
            if (clean == name || config::rename_profile(name, clean)) toast("Renamed config to " + clean), own_configs(true);
            else toast("Could not rename the config");
            renaming.clear();
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, S(8));
        if (ImGui::Button("Cancel", {S(90), 0})) renaming.clear();
    } else {
        dl->AddText(f_tile, big, {a.x + pad, a.y + pad + fs + S(8)}, C(on ? T.accent : T.text), name.c_str());
        dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + big + S(14)}, C(T.soft), profile_summary(p).c_str(), nullptr, w - pad * 2);
        ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
        if (on) {
            if (ImGui::Button("Turn off", {S(96), 0})) cheats::all_off(), save_config();
        } else if (accent_button("Load", {S(96), 0})) {
            cheats::apply_profile(p);
            save_config();
            toast("Loaded config " + name);
        }
        float gap = S(6), right = b.x - pad;
        bool sure = confirm_delete == name;
        ImGui::SameLine(0, 0);
        ImGui::SetCursorScreenPos({right - S(62) - S(72) - S(56) - gap * 2, b.y - pad - bh});
        if (ImGui::Button("Save", {S(56), 0})) {
            bool ok = config::save_profile(name, cheats::current_profile());
            toast(ok ? "Saved what is on now into " + name : std::string("Could not save the config"));
            own_configs(true);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Replace this config with the cheats and sliders that are on now");
        ImGui::SameLine(0, gap);
        if (ImGui::Button("Rename", {S(72), 0})) renaming = name, snprintf(rename_to, sizeof rename_to, "%s", name.c_str()), confirm_delete.clear();
        ImGui::SameLine(0, gap);
        if (ImGui::Button(sure ? "Sure?" : "Delete", {S(62), 0})) {
            if (sure) {
                config::delete_profile(name);
                confirm_delete.clear();
                toast("Deleted config " + name);
                own_configs(true);
            } else {
                confirm_delete = name;
            }
        }
    }
    ImGui::SetCursorScreenPos({a.x, b.y});
    ImGui::EndGroup();
    ImGui::PopID();
}

inline void new_config_tile(float w, float h) {
    static bool naming = false;
    static char name[48] = "";
    ImGui::PushID("new_config");
    ImGui::BeginGroup();
    ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
    tile_frame(a, b, naming, !naming);
    auto* dl = ImGui::GetWindowDrawList();
    float pad = S(16), fs = font_size(f_label), big = font_size(f_tile), small = font_size(f_small), bh = ImGui::GetFrameHeight();
    if (!naming) {
        if (ImGui::InvisibleButton("start", {w, h})) naming = true, name[0] = 0;
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        float tw = f_tile->CalcTextSizeA(big, FLT_MAX, 0, "+ New config").x;
        dl->AddText(f_tile, big, {(a.x + b.x - tw) / 2, (a.y + b.y) / 2 - big}, C(T.accent), "+ New config");
        const char* hint = "Saves the cheats and sliders that are on now.";
        float hw = f_small->CalcTextSizeA(small, FLT_MAX, 0, hint).x;
        dl->AddText(f_small, small, {(a.x + b.x - hw) / 2, (a.y + b.y) / 2 + S(8)}, C(T.soft), hint);
    } else {
        ImGui::Dummy({w, h});
        brand::spaced_caps(dl, f_label, fs, {a.x + pad, a.y + pad}, "NEW CONFIG", C(T.muted), S(1.4f));
        ImGui::SetCursorScreenPos({a.x + pad, a.y + pad + fs + S(8)});
        ImGui::SetNextItemWidth(w - pad * 2);
        if (!ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere();
        bool enter = ImGui::InputTextWithHint("##name", "Name, e.g. Hunter sweaty", name, sizeof name, ImGuiInputTextFlags_EnterReturnsTrue);
        std::string clean = config::profile_name(name);
        bool taken = own_configs().data.count(clean) > 0;
        std::string what = taken ? "A config with this name already exists." : profile_summary(cheats::current_profile());
        dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + bh + S(16)}, C(taken ? T.bad : T.soft), what.c_str(), nullptr, w - pad * 2);
        ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
        ImGui::BeginDisabled(clean.empty() || taken);
        if (accent_button("Save config", {S(130), 0}) || (enter && !clean.empty() && !taken)) {
            bool ok = config::save_profile(clean, cheats::current_profile());
            toast(ok ? "Saved config " + clean : std::string("Could not save the config"));
            own_configs(true);
            naming = false;
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, S(8));
        if (ImGui::Button("Cancel", {S(90), 0})) naming = false;
    }
    ImGui::SetCursorScreenPos({a.x, b.y});
    ImGui::EndGroup();
    ImGui::PopID();
}

inline void configs_card() {
    begin_card("configs", "CONFIGS");
    note("Built-in configs for Be The Zombie and your own. Legit stays believable to the other players, Rage holds nothing back. Loading a config turns everything else off; "
         "the built-in ones also set the player ESP. Your configs are saved in fatrainer_configs next to the game.");
    ImGui::Dummy({0, S(4)});
    float gap = S(12), w = (ImGui::GetContentRegionAvail().x - gap) / 2, h = S(206);
    int i = 0;
    auto place = [&] {
        if (i % 2) ImGui::SameLine(0, gap);
        else if (i) ImGui::Dummy({0, S(2)});
        i++;
    };
    auto& hidden = config::cfg.hidden_configs;
    for (auto& p : cheats::PRESETS)
        if (std::find(hidden.begin(), hidden.end(), p.key) == hidden.end()) place(), preset_tile(p, w, h);
    OwnConfigs& own = own_configs();
    std::vector<std::string> names = own.names;
    for (auto& n : names)
        if (own_configs().data.count(n)) place(), own_config_tile(n, own_configs().data[n], w, h);
    place(), new_config_tile(w, h);
    if (!hidden.empty()) {
        ImGui::Dummy({0, S(6)});
        if (ImGui::Button(("Restore built-in configs (" + std::to_string(hidden.size()) + ")").c_str())) hidden.clear(), save_config();
    }
    end_card();
}

inline void pvp_page() {
    configs_card();
    begin_card("about");
    note("Slide right to reach further. At Max the pounce, dropkick and death from above also hit targets that are not in front of you. Each player's game decides its own attacks.");
    end_card();
    begin_card("zombie", "AS THE NIGHT HUNTER");
    tweak_sliders(cheats::G_ZOMBIE);
    end_card();
    begin_card("human", "AS A SURVIVOR");
    cheat_switch("dodge_spit");
    cheat_switch("one_hit_hunter");
    tweak_sliders(cheats::G_HUMAN);
    end_card();
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

#endif

const float CHANNELS_WIDTH = 314;

inline bool color_field(const char* key, float rgb[3]) {
    bool changed = false;
    auto& cycles = config::cfg.rainbow;
    auto cycle = cycles.find(key);
    ImGui::PushID(key);
    if (ImGui::ColorButton("##swatch", {rgb[0], rgb[1], rgb[2], 1}, ImGuiColorEditFlags_NoTooltip, {S(34), ImGui::GetFrameHeight()})) ImGui::OpenPopup("picker");
    if (ImGui::BeginPopup("picker")) {
        changed |= ImGui::ColorPicker3("##pick", rgb, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs);
        ImGui::EndPopup();
    }
    if (cycle != cycles.end()) {
        ImGui::SameLine(0, S(10));
        ImGui::SetNextItemWidth(S(CHANNELS_WIDTH));
        changed |= ImGui::SliderFloat("##cycle", &cycle->second, config::RAINBOW_FASTEST, config::RAINBOW_SLOWEST, "One cycle in %.1f s",
                                      ImGuiSliderFlags_Logarithmic);
    } else {
        const char* channels[3] = {"R", "G", "B"};
        for (int i = 0; i < 3; i++) {
            ImGui::SameLine(0, S(10));
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%s", channels[i]);
            ImGui::SameLine(0, S(4));
            int v = color::to_byte(rgb[i]);
            ImGui::SetNextItemWidth(S(52));
            ImGui::PushID(i);
            if (ImGui::InputInt("##c", &v, 0, 0)) rgb[i] = std::clamp(v, 0, 255) / 255.0f, changed = true;
            ImGui::PopID();
        }
        ImGui::SameLine(0, S(10));
        char hex[16];
        snprintf(hex, sizeof hex, "%s", color::to_hex(rgb).c_str());
        ImGui::SetNextItemWidth(S(86));
        if (ImGui::InputText("##hex", hex, sizeof hex, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsUppercase)) changed |= color::from_hex(hex, rgb);
    }
    ImGui::SameLine(0, S(10));
    if (chip("Rainbow", cycle != cycles.end())) {
        if (cycle == cycles.end()) cycles[key] = config::RAINBOW_DEFAULT;
        else cycles.erase(cycle);
        changed = true;
    }
    ImGui::PopID();
    return changed;
}

inline bool tint_card(const char* id, const char* title, const char* label, const char* hint, config::UvLight& u, const char* footnote) {
    bool changed = false;
    begin_card(id, title);
    ImGui::PushID(id);
    changed |= switch_row(label, hint, u.on);
    ImGui::BeginDisabled(!u.on);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Color");
    ImGui::SameLine(S(150));
    changed |= color_field(id, u.color);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Glow");
    ImGui::SameLine(S(150));
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##glow", &u.glow, 0.2f, 4.0f, "x%.1f")) changed = true;
    ImGui::EndDisabled();
    note(footnote);
    ImGui::PopID();
    end_card();
    return changed;
}

struct Preview { std::string text; DWORD until = 0; };
inline Preview preview;
const DWORD PREVIEW_MS = 3500;
inline const char* SAMPLE_NAMES[] = {"Jade", "Rahim", "Brecken"};

inline bool previewing() { return GetTickCount() < preview.until; }

inline std::string with_names(const std::string& text);

inline void preview_lines(const std::vector<std::string>& lines) {
    preview.until = GetTickCount() + PREVIEW_MS;
    if (!visuals::game_messages()) {
        preview.text = lines.empty() ? "" : lines[0];
        return;
    }
    preview.text.clear();
    std::vector<std::string> named;
    for (auto& l : lines) named.push_back(with_names(l));
    on_game_thread([named] {
        for (auto& l : named) visuals::post_message(l);
    });
}

inline void preview_text(const std::string& text) { preview_lines({text}); }

inline bool part_row(const char* label, const std::string& key, std::map<std::string, config::HudColor>& colors, bool can_hide) {
    auto& c = config::cfg;
    bool changed = false;
    ImGui::PushID(key.c_str());
    auto it = colors.find(key);
    bool on = it != colors.end() && it->second.on;
    if (ImGui::Checkbox(label, &on)) {
        auto& h = colors[key];
        if (it == colors.end()) memcpy(h.color, c.accent, sizeof h.color);
        h.on = on, changed = true;
        it = colors.find(key);
    }
    ImGui::SameLine(S(250));
    ImGui::BeginDisabled(!on);
    float shown[3] = {1, 1, 1};
    if (it != colors.end()) memcpy(shown, it->second.color, sizeof shown);
    std::string color_key = (&colors == &c.hud_parts ? "hud." : "glow.") + key;
    if (color_field(color_key.c_str(), shown)) memcpy(colors[key].color, shown, sizeof shown), changed = true;
    ImGui::EndDisabled();
    bool hidden = it != colors.end() && it->second.hidden;
    if (can_hide) ImGui::SameLine(0, S(10));
    if (can_hide && chip(hidden ? "Hidden##hide" : "Hide##hide", hidden)) {
        auto& h = colors[key];
        if (it == colors.end()) memcpy(h.color, c.accent, sizeof h.color);
        h.hidden = !hidden, changed = true;
    }
    ImGui::PopID();
    return changed;
}

inline bool hud_colors_card() {
    auto& c = config::cfg;
    bool changed = false;
    begin_card("hud_colors", "HUD COLORS");
    changed |= switch_row("Tint the whole HUD", "Every bar, icon and text of the HUD takes on this color, keeping its own brightness.", c.hud_tint.on);
    ImGui::BeginDisabled(!c.hud_tint.on);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Tint");
    ImGui::SameLine(S(150));
    changed |= color_field("hud_tint", c.hud_tint.color);
    ImGui::EndDisabled();
    const char* cluster = nullptr;
    for (auto& part : visuals::HUD_PARTS) {
        if (!cluster || strcmp(cluster, part.cluster)) {
            cluster = part.cluster;
            ImGui::Dummy({0, S(10)});
            caps(cluster);
            ImGui::Dummy({0, S(2)});
        }
        changed |= part_row(part.label, part.key, c.hud_parts, true);
    }
    note(visuals::hud_found() ? "Only you see these colors. A piece's own color wins over the whole HUD tint. Hide makes that piece invisible until you show it again; the game's own animations still play on the pieces you color."
                              : "Load into the game to see the colors; the trainer finds the HUD once it is on screen and after a Refresh.");
    end_card();
    return changed;
}

inline bool survivor_glow_card() {
    bool changed = false;
    begin_card("survivor_glow", "SURVIVOR GLOW");
    note("As the Night Hunter you see survivors glow through walls with your sense. Pick your own color for each kind of glow; it keeps the glow's brightness.");
    ImGui::Dummy({0, S(4)});
    for (auto& g : visuals::SURVIVOR_GLOWS) changed |= part_row(g.label, g.key, config::cfg.survivor_glow, false);
    end_card();
    return changed;
}

inline void draw_crosshair(ImDrawList* dl, ImVec2 c, const config::Crosshair& x, float alpha) {
    int a = (int)std::clamp(255 * x.opacity * alpha, 0.0f, 255.0f);
    ImU32 col = IM_COL32((int)(x.color[0] * 255), (int)(x.color[1] * 255), (int)(x.color[2] * 255), a), edge = IM_COL32(0, 0, 0, a * 3 / 4);
    float t = x.thickness, half = t / 2, gap = x.gap, len = x.size;
    auto bar = [&](ImVec2 from, ImVec2 to) {
        if (x.outline) dl->AddRectFilled({from.x - 1, from.y - 1}, {to.x + 1, to.y + 1}, edge);
        dl->AddRectFilled(from, to, col);
    };
    if (x.style == config::CROSS || x.style == config::CROSS_DOT) {
        bar({c.x + gap, c.y - half}, {c.x + gap + len, c.y + half});
        bar({c.x - gap - len, c.y - half}, {c.x - gap, c.y + half});
        bar({c.x - half, c.y + gap}, {c.x + half, c.y + gap + len});
        bar({c.x - half, c.y - gap - len}, {c.x + half, c.y - gap});
    }
    if (x.style == config::DOT || x.style == config::CROSS_DOT) bar({c.x - half, c.y - half}, {c.x + half, c.y + half});
    if (x.style == config::CIRCLE) {
        if (x.outline) dl->AddCircle(c, len, edge, 48, t + 2);
        dl->AddCircle(c, len, col, 48, t);
    }
}

inline void draw_crosshair_overlay() {
    ImVec2 size = ImGui::GetIO().DisplaySize;
    draw_crosshair(ImGui::GetBackgroundDrawList(), {std::floor(size.x / 2), std::floor(size.y / 2)}, config::cfg.crosshair, 1.0f);
}

inline bool crosshair_card() {
    auto& x = config::cfg.crosshair;
    bool changed = false;
    begin_card("crosshair", "CROSSHAIR");
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x, h = S(120);
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, {p.x + w, p.y + h}, C(T.ground), R(10));
    draw_crosshair(dl, {std::floor(p.x + w / 2), std::floor(p.y + h / 2)}, x, x.on ? 1.0f : 0.45f);
    ImGui::Dummy({w, h + S(6)});
    changed |= switch_row("Custom crosshair", "Draws your own crosshair in the middle of the screen whenever the game shows its crosshair.", x.on);
    ImGui::BeginDisabled(!x.on);
    static const char* STYLES[config::CROSSHAIR_STYLES] = {"Cross", "Dot", "Circle", "Cross and dot"};
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Style");
    ImGui::SameLine(S(150));
    changed |= segmented("style", STYLES, config::CROSSHAIR_STYLES, x.style, ImGui::GetContentRegionAvail().x);
    struct Slider { const char* label; float* value; float lo, hi; const char* format; };
    Slider sliders[] = {{"Size", &x.size, 2, 40, "%.0f px"}, {"Thickness", &x.thickness, 1, 8, "%.0f px"}, {"Gap", &x.gap, 0, 20, "%.0f px"}};
    for (auto& sl : sliders) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(sl.label);
        ImGui::SameLine(S(150));
        ImGui::SetNextItemWidth(-1);
        changed |= ImGui::SliderFloat((std::string("##") + sl.label).c_str(), sl.value, sl.lo, sl.hi, sl.format);
    }
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Opacity");
    ImGui::SameLine(S(150));
    ImGui::SetNextItemWidth(-1);
    float percent = x.opacity * 100;
    if (ImGui::SliderFloat("##opacity", &percent, 10, 100, "%.0f%%")) x.opacity = percent / 100, changed = true;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Color");
    ImGui::SameLine(S(150));
    changed |= color_field("crosshair", x.color);
    ImGui::Dummy({0, S(2)});
    changed |= ImGui::Checkbox("Dark outline", &x.outline);
    ImGui::EndDisabled();
    auto game = config::cfg.hud_parts.find("crosshair");
    bool hide_game = game != config::cfg.hud_parts.end() && game->second.hidden;
    if (switch_row("Hide the game's crosshair", "Same as Hide next to Crosshair under HUD colors.", hide_game)) {
        auto& h = config::cfg.hud_parts["crosshair"];
        if (game == config::cfg.hud_parts.end()) memcpy(h.color, config::cfg.accent, sizeof h.color);
        h.hidden = hide_game, changed = true;
    }
    end_card();
    return changed;
}

inline bool fov_card() {
    auto& c = config::cfg;
    bool changed = false;
    begin_card("fov", "FIELD OF VIEW");
    float percent = c.fov * 100;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Field of view");
    ImGui::SameLine(S(150));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - S(96));
    if (ImGui::SliderFloat("##fov", &percent, config::FOV_MIN * 100, config::FOV_MAX * 100, percent == 100 ? "Game's own" : "%.0f%%")) {
        c.fov = std::round(percent) / 100, changed = true;
    }
    ImGui::SameLine(0, S(8));
    if (ImGui::Button("Reset", {-1, 0})) c.fov = 1.0f, changed = true;
    float game = visuals::game_fov, shown = visuals::shown_fov;
    if (std::isfinite(game) && std::isfinite(shown)) {
        char now[96];
        snprintf(now, sizeof now, "NOW %.0f DEGREES, THE GAME ASKS FOR %.0f", shown, game);
        ImGui::Dummy({0, S(2)});
        caps(now);
    }
    note("Scales the field of view the game picks, so sprinting and aiming still zoom like before. Changes live, also with the menu open.");
    end_card();
    return changed;
}

inline const char* FEATURED_TEXTS[] = {"btz_pounce_interrupted", "HUDMessage_BTZDeath_Killer", "HUDMessage_BTZDeath_Victim", "HUDMessage_BTZDeath_Observer",
                                        "HUDMessage_BTZZombiePounceInterrupt_Interrupter_AdditionalInterrupt",
                                        "HUDMessage_BTZZombiePounceInterrupt_Victim_SelfInterrupt", "btz_invasion_satus_human_killed"};
const size_t TEXT_RESULTS = 150;

inline std::string fit_text(ImFont* font, float size, const std::string& text, float width) {
    std::string s = text.substr(0, text.find('\n'));
    if (font->CalcTextSizeA(size, FLT_MAX, 0, s.c_str()).x <= width) return s;
    size_t cut = s.size();
    while (cut > 0) {
        do cut--;
        while (cut && ((unsigned char)s[cut] & 0xC0) == 0x80);
        std::string shorter = s.substr(0, cut) + "...";
        if (font->CalcTextSizeA(size, FLT_MAX, 0, shorter.c_str()).x <= width) return shorter;
    }
    return "...";
}

inline ImU32 hex_color(const std::string& hex, ImU32 fallback) {
    float rgb[3];
    return color::from_hex(hex, rgb) ? IM_COL32(color::to_byte(rgb[0]), color::to_byte(rgb[1]), color::to_byte(rgb[2]), 255) : fallback;
}

inline std::string with_names(const std::string& text) {
    std::string out;
    int n = 0;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '%' && i + 1 < text.size() && text[i + 1] == 's') out += SAMPLE_NAMES[n++ % 3], i++;
        else if (text[i] == '%' && i + 1 < text.size() && text[i + 1] == '%') out += '%', i++;
        else out += text[i];
    }
    return out;
}

inline float draw_rich(ImDrawList* dl, ImFont* font, float size, ImVec2 at, const std::string& text, ImU32 color, float width) {
    std::string line = text.substr(0, text.find('\n'));
    ImU32 now = color;
    float x = at.x, right = at.x + width;
    for (size_t i = 0; i < line.size();) {
        if (size_t n = visuals::color_sequence(line, i)) {
            std::string tone = line.substr(i + strlen(visuals::COLOR_OPEN), n - strlen(visuals::COLOR_OPEN) - 1);
            now = _stricmp(tone.c_str(), "reset") ? hex_color(tone, now) : color;
            i += n;
            continue;
        }
        size_t next = line.find(visuals::COLOR_OPEN, i + 1);
        std::string run = line.substr(i, (next == std::string::npos ? line.size() : next) - i);
        float run_w = font->CalcTextSizeA(size, FLT_MAX, 0, run.c_str()).x;
        if (x + run_w > right) {
            std::string cut = fit_text(font, size, run, right - x);
            dl->AddText(font, size, {x, at.y}, now, cut.c_str());
            return right - at.x;
        }
        dl->AddText(font, size, {x, at.y}, now, run.c_str());
        x += run_w;
        i += run.size();
    }
    return x - at.x;
}

inline void draw_preview() {
    if (GetTickCount() >= preview.until || preview.text.empty()) return;
    ImVec2 size = ImGui::GetIO().DisplaySize;
    auto* dl = ImGui::GetForegroundDrawList();
    std::string text = with_names(preview.text);
    float fs = font_size(f_strong) * 1.25f, pad = S(18);
    float w = std::min(size.x * 0.8f, f_strong->CalcTextSizeA(fs, FLT_MAX, 0, visuals::without_colors(text).c_str()).x + pad * 2);
    ImVec2 a{(size.x - w) / 2, size.y * 0.2f}, b{a.x + w, a.y + fs + pad * 2 + S(18)};
    dl->AddRectFilled(a, b, IM_COL32(12, 11, 10, 210), R(12));
    dl->AddRect(a, b, C(T.accent, 0.6f), R(12), 0, S(1.5f));
    brand::spaced_caps(dl, f_label, font_size(f_label), {a.x + pad, a.y + S(10)}, "PREVIEW", C(T.accent), S(1.4f));
    draw_rich(dl, f_strong, fs, {a.x + pad, a.y + S(14) + pad}, text, IM_COL32(242, 237, 228, 255), w - pad * 2);
}

enum RowClick { ROW_NONE, ROW_CLICKED };

inline RowClick text_row(const visuals::TextRow& r, bool selected) {
    auto& texts = config::cfg.texts;
    ImGui::PushID(r.key.c_str());
    auto mine = texts.find(r.key);
    const std::string& shown = mine != texts.end() ? mine->second : r.shown;
    bool english = !r.english.empty() && r.english != visuals::without_colors(shown);
    float w = ImGui::GetContentRegionAvail().x, body = ImGui::GetFontSize(), small = font_size(f_small), pad = S(12);
    float h = pad + body + S(5) + (english ? small + S(4) : 0) + small + pad;
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool picked = ImGui::InvisibleButton("row", {w, h});
    bool hovered = ImGui::IsItemHovered();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
    auto* dl = ImGui::GetWindowDrawList();
    if (selected || hovered) dl->AddRectFilled(p, {p.x + w, p.y + h}, selected ? C(T.accent, 0.10f) : C(IM_COL32(255, 255, 255, 255), 0.03f), R(10));
    const char* badge_text = selected ? "Editing" : mine != texts.end() ? "Changed" : hovered ? "Click to edit" : "";
    float badge = *badge_text ? f_small->CalcTextSizeA(small, FLT_MAX, 0, badge_text).x + S(16) : 0, text_w = w - pad * 2 - badge;
    float y = p.y + pad;
    draw_rich(dl, f_body, body, {p.x + pad, y}, shown, C(T.text), text_w);
    if (badge > 0) dl->AddText(f_small, small, {p.x + w - pad - badge + S(16), y + (body - small) / 2}, C(mine != texts.end() || selected ? T.accent : T.muted), badge_text);
    y += body + S(5);
    if (english) {
        dl->AddText(f_small, small, {p.x + pad, y}, C(T.soft), fit_text(f_small, small, "English: " + r.english, text_w).c_str());
        y += small + S(4);
    }
    std::string where = r.key + "   " + visuals::TEXT_KIND_INFO[r.kind].label;
    dl->AddText(f_small, small, {p.x + pad, y}, C(T.muted), fit_text(f_small, small, where, text_w).c_str());
    ImGui::PopID();
    return picked ? ROW_CLICKED : ROW_NONE;
}

inline const char* language_name(const std::string& code) {
    for (auto& l : visuals::LANGUAGES)
        if (code == l.code) return l.name;
    return nullptr;
}

struct Choice { std::string value, name; };

inline std::string steam_label() {
    const char* n = visuals::game_language_name(visuals::steam_language);
    return n ? std::string("Your Steam language (") + n + ")" : "Your Steam language";
}

inline bool choice_row(const char* id, const char* text, const char* hint, std::string& value, const std::vector<Choice>& options) {
    ImGui::PushID(id);
    float w = ImGui::GetContentRegionAvail().x, combo_w = S(300), body = ImGui::GetFontSize(), small = font_size(f_small), h = S(60);
    ImVec2 p = ImGui::GetCursorScreenPos();
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddText(f_body, body, {p.x + S(4), p.y + S(10)}, C(T.text), text);
    dl->AddText(f_small, small, {p.x + S(4), p.y + S(13) + body}, C(T.muted), fit_text(f_small, small, hint, w - combo_w - S(24)).c_str());
    const Choice* current = &options[0];
    for (auto& o : options)
        if (o.value == value) current = &o;
    ImGui::SetCursorScreenPos({p.x + w - combo_w, p.y + (h - ImGui::GetFrameHeight()) / 2});
    ImGui::SetNextItemWidth(combo_w);
    bool changed = false;
    if (ImGui::BeginCombo("##choice", current->name.c_str())) {
        for (auto& o : options)
            if (ImGui::Selectable(o.name.c_str(), o.value == value) && o.value != value) value = o.value, changed = true;
        ImGui::EndCombo();
    }
    ImGui::SetCursorScreenPos({p.x, p.y + h});
    ImGui::Dummy({w, 0});
    ImGui::PopID();
    return changed;
}

inline bool language_card() {
    auto& c = config::cfg;
    bool changed = false;
    begin_card("language", "LANGUAGE");
    std::vector<Choice> game{{"", steam_label()}}, voices{{"", steam_label()}, {visuals::FOLLOW_GAME, "Same as the game language"}},
        subtitles{{"", "Same as the game language"}, {config::STEAM_LANGUAGE, steam_label()}};
    for (auto& l : visuals::GAME_LANGUAGES) {
        game.push_back({l.code, l.name});
        if (l.voice) voices.push_back({l.code, l.name});
    }
    for (auto& l : visuals::LANGUAGES) subtitles.push_back({l.code, l.name});
    changed |= choice_row("game", "Game language", "Menus, HUD, items, quests, Night Hunter info, everything you read", c.game_language, game);
    changed |= choice_row("voices", "Voices", "The language the characters speak", c.voice_language, voices);
    changed |= choice_row("subtitles", "Subtitles", "What the characters say, as text", c.subtitle_language, subtitles);
    bool restart = c.game_language != visuals::startup_game_setting || c.voice_language != visuals::startup_voice_setting;
    std::string code = visuals::subtitle_code();
    ImGui::Dummy({0, S(4)});
    if (restart) caps("START THE GAME AGAIN TO CHANGE THE GAME LANGUAGE AND VOICES", T.accent);
    else if (!code.empty() && !visuals::library(code)) caps("LOADING THE SUBTITLES");
    else if (!visuals::started_language.empty()) {
        const std::string& shown_game = visuals::text_language.empty() ? visuals::started_language : visuals::text_language;
        const char* g = visuals::game_language_name(shown_game);
        const char* v = visuals::game_language_name(visuals::started_voice);
        std::string now = std::string("NOW: GAME ") + (g ? g : shown_game.c_str()) + (v ? std::string(", VOICES ") + v : std::string());
        for (auto& ch : now) ch = (char)toupper((unsigned char)ch);
        caps(now.c_str());
    }
    note("The game language and voices change the next time you start the game. Subtitles change right away; lines already on screen change the next time the game shows them.");
    end_card();
    return changed;
}

inline std::string callout_role(const std::string& key) {
    auto has = [&](const char* part) { return key.find(part) != std::string::npos; };
    if (has("AwardReason") || has("btz_pounce")) return "XP popup";
    if (has("Observer")) return "seen by others";
    if (has("Victim") || has("Vicitm")) return "done to you";
    if (has("Interrupter")) return "you saved someone";
    return "done by you";
}

inline std::string callout_line_label(const std::string& key) {
    for (auto& c : visuals::CALLOUTS)
        if (key == c.key) {
            std::string label = c.label;
            while (!label.empty() && label.back() == ':') label.pop_back();
            return label + " - " + callout_role(key);
        }
    return key;
}

inline bool callouts_card() {
    auto& texts = config::cfg.texts;
    static std::vector<std::string> names = visuals::callout_set_names();
    static std::string selected = visuals::BUILTIN_CALLOUT_SETS[1], loaded_name, renaming, confirm_delete;
    static config::CalloutTexts set;
    static bool naming = false, editing = getenv("DLT_CALLOUTS") != nullptr;
    static char name_buf[48] = "";
    static std::map<std::string, std::string> drafts;
    bool changed = false;
    auto reload = [&](bool list) {
        if (list) names = visuals::callout_set_names();
        if (std::find(names.begin(), names.end(), selected) == names.end()) selected = names.front();
        set = visuals::callout_set(selected), loaded_name = selected, drafts.clear();
    };
    if (loaded_name != selected) reload(false);
    int builtin = visuals::builtin_callout_set(selected);
    bool overridden = builtin >= 0 && GetFileAttributesA(config::callout_path(selected).c_str()) != INVALID_FILE_ATTRIBUTES;

    begin_card("callouts", "BE THE ZOMBIE CALLOUTS");
    for (size_t i = 0; i < names.size(); i++)
        if (flowing_chip(names[i].c_str(), names[i] == selected, i == 0)) selected = names[i], renaming.clear(), confirm_delete.clear();
    if (flowing_chip("+ New set", naming, false)) naming = !naming, snprintf(name_buf, sizeof name_buf, "%s copy", selected.c_str());
    if (naming) {
        ImGui::Dummy({0, S(2)});
        ImGui::SetNextItemWidth(S(260));
        bool enter = ImGui::InputText("##new_set", name_buf, sizeof name_buf, ImGuiInputTextFlags_EnterReturnsTrue);
        std::string clean = config::profile_name(name_buf);
        bool taken = std::find(names.begin(), names.end(), clean) != names.end();
        ImGui::SameLine(0, S(8));
        ImGui::BeginDisabled(clean.empty() || taken);
        if (accent_button("Create") || (enter && !clean.empty() && !taken)) {
            if (config::save_callout_set(clean, set)) selected = clean, reload(true), toast("Created callout set " + clean);
            else toast("Could not create the callout set");
            naming = false;
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, S(8));
        if (ImGui::Button("Cancel", {0, ImGui::GetFrameHeight()})) naming = false;
        note(taken ? "A set with this name already exists." : "Starts as a copy of the selected set. Letters, numbers, spaces, - and _.");
    }
    ImGui::Dummy({0, S(4)});

    int used = visuals::callouts_used(texts, set), count = (int)set.size();
    float w = ImGui::GetContentRegionAvail().x, row = font_size(f_body) + S(8);
    ImVec2 p = ImGui::GetCursorScreenPos();
    auto* dl = ImGui::GetWindowDrawList();
    const char* samples[] = {"HUDMessage_BTZDeath_Killer_Hunter_Highrage", "HUDMessage_BTZZombiePounceInterrupt_Interrupter_AdditionalInterrupt",
                             "HUDMessage_BTZDeath_Observer_Pvp_Human_Wrestling_Kick", "HUDMessage_BTZZombieSpitHit_ControlTheHorde_Victim"};
    std::vector<std::string> sample_lines;
    for (const char* key : samples)
        if (auto it = set.find(key); it != set.end()) sample_lines.push_back(it->second);
    int shown = std::max(1, (int)sample_lines.size());
    dl->AddRectFilled(p, {p.x + w, p.y + row * shown + S(16)}, C(T.ground), R(10));
    if (sample_lines.empty()) dl->AddText(f_body, font_size(f_body), {p.x + S(14), p.y + S(8)}, C(T.soft), "This set has no lines yet.");
    for (size_t i = 0; i < sample_lines.size(); i++)
        draw_rich(dl, f_body, font_size(f_body), {p.x + S(14), p.y + S(8) + row * i}, with_names(sample_lines[i]), C(T.text), w - S(28));
    ImGui::Dummy({w, row * shown + S(22)});

    ImGui::BeginDisabled(set.empty());
    if (accent_button(used == count && count ? "Update the callouts" : "Use these callouts")) {
        for (auto& [key, text] : set) texts[key] = text;
        changed = true;
        toast("Callouts from " + selected + " are on");
    }
    ImGui::EndDisabled();
    if (used) {
        ImGui::SameLine(0, S(8));
        if (ImGui::Button("Remove the callouts", {0, ImGui::GetFrameHeight()})) {
            for (auto& [key, text] : set)
                if (auto it = texts.find(key); it != texts.end() && it->second == text) texts.erase(it);
            changed = true;
        }
    }
    ImGui::SameLine(0, S(8));
    if (ImGui::Button("Show in the killfeed", {0, ImGui::GetFrameHeight()})) preview_lines(sample_lines);
    char status[200];
    snprintf(status, sizeof status, "%d of %d lines of this set are in your game. Your own edits below win.", used, count);
    note(status);

    ImGui::Dummy({0, S(4)});
    if (ImGui::Button(editing ? "Close the editor" : "Edit lines", {0, ImGui::GetFrameHeight()})) editing = !editing;
    if (builtin >= 0 && overridden) {
        ImGui::SameLine(0, S(8));
        if (ImGui::Button("Restore", {0, ImGui::GetFrameHeight()})) config::delete_callout_set(selected), reload(false), toast("Restored " + selected);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Put back the lines this set came with");
    }
    if (builtin < 0) {
        ImGui::SameLine(0, S(8));
        if (renaming == selected) {
            ImGui::SetNextItemWidth(S(200));
            bool enter = ImGui::InputText("##rename_set", name_buf, sizeof name_buf, ImGuiInputTextFlags_EnterReturnsTrue);
            std::string clean = config::profile_name(name_buf);
            bool taken = clean != selected && std::find(names.begin(), names.end(), clean) != names.end();
            ImGui::SameLine(0, S(8));
            ImGui::BeginDisabled(clean.empty() || taken);
            if (accent_button("Rename") || (enter && !clean.empty() && !taken)) {
                if (clean == selected || config::rename_callout_set(selected, clean)) selected = clean, reload(true);
                else toast("Could not rename the callout set");
                renaming.clear();
            }
            ImGui::EndDisabled();
        } else {
            if (ImGui::Button("Rename", {0, ImGui::GetFrameHeight()})) renaming = selected, snprintf(name_buf, sizeof name_buf, "%s", selected.c_str()), confirm_delete.clear();
            ImGui::SameLine(0, S(8));
            bool sure = confirm_delete == selected;
            if (ImGui::Button(sure ? "Sure?" : "Delete", {0, ImGui::GetFrameHeight()})) {
                if (sure) {
                    std::string gone = selected;
                    config::delete_callout_set(gone), confirm_delete.clear(), reload(true);
                    toast("Deleted callout set " + gone);
                } else {
                    confirm_delete = selected;
                }
            }
        }
    }
    if (editing) {
        ImGui::Dummy({0, S(4)});
        note("Keep %s where the game puts a name. %COLOR(rrggbb) starts a color, %COLOR(reset) ends it. Changes are saved when you leave a line.");
        std::vector<std::string> keys;
        for (auto& c : visuals::CALLOUTS)
            if (set.count(c.key)) keys.push_back(c.key);
        for (auto& [key, text] : set)
            if (std::find(keys.begin(), keys.end(), key) == keys.end()) keys.push_back(key);
        for (auto& key : keys) {
            ImGui::PushID(key.c_str());
            label(callout_line_label(key).c_str());
            std::string& draft = drafts.try_emplace(key, set[key]).first->second;
            char buf[512];
            snprintf(buf, sizeof buf, "%s", draft.c_str());
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##line", buf, sizeof buf)) draft = buf;
            if (ImGui::IsItemDeactivatedAfterEdit() && draft != set[key]) {
                bool was_on = texts.count(key) && texts[key] == set[key];
                set[key] = draft;
                if (config::save_callout_set(selected, set)) {
                    if (was_on) texts[key] = draft, changed = true;
                } else {
                    toast("Could not save the callout set");
                }
            }
            ImGui::PopID();
        }
    }
    end_card();
    return changed;
}

inline bool text_editor(const visuals::TextRow& r, char* draft, size_t size, bool& focus, bool& close) {
    auto& texts = config::cfg.texts;
    static float tone[3] = {1.0f, 0.302f, 0.427f};
    bool changed = false;
    ImGui::PushID("editor");
    ImGui::Indent(S(12));
    ImGui::SetNextItemWidth(-S(12));
    if (focus) ImGui::SetKeyboardFocusHere(), focus = false;
    ImGui::InputTextMultiline("##draft", draft, size, {-S(12), S(64)});
    if (accent_button("Save")) {
        if (visuals::safe_format(draft, r.english)) texts[r.key] = draft, changed = true, close = true;
        else toast("Keep the game's %s and write a percent sign as %%. Nothing was saved.");
    }
    ImGui::SameLine(0, S(8));
    if (ImGui::Button("Preview", {0, ImGui::GetFrameHeight()})) preview_text(draft);
    ImGui::SameLine(0, S(8));
    if (ImGui::Button("Use the game's text", {0, ImGui::GetFrameHeight()})) texts.erase(r.key), changed = true, close = true;
    ImGui::SameLine(0, S(8));
    if (ImGui::Button("Cancel", {0, ImGui::GetFrameHeight()})) close = true;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Color");
    ImGui::SameLine(S(72));
    color_field("text_tone", tone);
    ImGui::SameLine(0, S(8));
    if (ImGui::Button("Color all", {0, ImGui::GetFrameHeight()})) {
        std::string plain = visuals::without_colors(draft);
        snprintf(draft, size, "%s", visuals::colored(plain, color::to_hex(tone).c_str()).c_str());
    }
    ImGui::SameLine(0, S(8));
    if (ImGui::Button("Color the label", {0, ImGui::GetFrameHeight()})) {
        std::string plain = visuals::without_colors(draft);
        size_t colon = plain.find(':');
        size_t end = colon == std::string::npos ? plain.size() : colon + 1, start = plain.compare(0, 2, "%s") ? 0 : std::min(plain.size(), (size_t)3);
        if (end > start) snprintf(draft, size, "%s", (plain.substr(0, start) + visuals::colored(plain.substr(start, end - start), color::to_hex(tone).c_str()) + plain.substr(end)).c_str());
    }
    ImGui::SameLine(0, S(8));
    if (ImGui::Button("No colors", {0, ImGui::GetFrameHeight()})) snprintf(draft, size, "%s", visuals::without_colors(draft).c_str());
    ImGui::Unindent(S(12));
    ImGui::Dummy({0, S(6)});
    ImGui::PopID();
    return changed;
}

inline bool texts_card() {
    auto& texts = config::cfg.texts;
    static char filter[64] = "";
    static int kind = -1;
    static std::string searched = "\x01";
    static int searched_kind = -2;
    static std::shared_ptr<const visuals::TextRows> searched_rows;
    static std::vector<size_t> matches;
    static std::string editing, editing_in;
    static char draft[2048] = "";
    static bool focus = false;
    bool changed = false;
    auto all = visuals::rows();
    begin_card("texts", "YOUR TEXTS");
    if (!all) {
        note(visuals::game_folder().empty() ? "The game's text files were not found." : "Reading the game's texts...");
        end_card();
        return false;
    }
    search_box("##texts", "Search what you see in the game, like pounce or killed...", filter, sizeof filter);
    if (flowing_chip("All", kind < 0, true)) kind = -1;
    for (int k = 0; k < visuals::TEXT_KINDS; k++)
        if (flowing_chip(visuals::TEXT_KIND_INFO[k].label, kind == k, false)) kind = kind == k ? -1 : k;
    std::string f = visuals::lowered(filter);
    bool browsing = !f.empty() || kind >= 0;
    if (f != searched || kind != searched_kind || all != searched_rows) {
        searched = f, searched_kind = kind, searched_rows = all, matches.clear();
        for (size_t i = 0; browsing && i < all->rows.size() && matches.size() < TEXT_RESULTS; i++) {
            auto& r = all->rows[i];
            if ((kind < 0 || r.kind == kind) && (f.empty() || r.haystack.find(f) != std::string::npos)) matches.push_back(i);
        }
    }
    auto row_of = [&](const std::string& key) -> const visuals::TextRow* {
        auto it = all->index.find(key);
        return it == all->index.end() ? nullptr : &all->rows[it->second];
    };
    std::string open_now = editing, open_in = editing_in;
    auto row = [&](const visuals::TextRow& r, const char* section) {
        ImGui::PushID(section);
        bool here = open_now == r.key && open_in == section;
        if (text_row(r, here) == ROW_CLICKED && !here) {
            editing = r.key, editing_in = section, focus = true;
            auto mine = texts.find(r.key);
            snprintf(draft, sizeof draft, "%s", mine != texts.end() ? mine->second.c_str() : r.shown.c_str());
        }
        if (here) {
            bool close = false;
            changed |= text_editor(r, draft, sizeof draft, focus, close);
            if (close) editing.clear();
        }
        ImGui::PopID();
    };
    ImGui::Dummy({0, S(8)});
    caps(!browsing ? "COMMON IN BE THE ZOMBIE" : matches.empty() ? "NOTHING FOUND" : matches.size() >= TEXT_RESULTS ? "FIRST RESULTS" : "RESULTS");
    if (!browsing) {
        for (const char* key : FEATURED_TEXTS)
            if (const visuals::TextRow* r = row_of(key)) row(*r, "featured");
    } else {
        for (size_t i : matches) row(all->rows[i], "results");
    }
    if (!texts.empty()) {
        ImGui::Dummy({0, S(8)});
        caps("CHANGED BY YOU");
        std::vector<std::string> keys;
        for (auto& [key, text] : texts) keys.push_back(key);
        for (auto& key : keys)
            if (const visuals::TextRow* r = row_of(key)) row(*r, "mine");
        if (ImGui::Button("Use the game's texts again")) texts.clear(), editing.clear(), changed = true;
    }
    note("Click a text to edit it right there. Keep %s where the game puts a name. Only you see your texts; they win over the subtitles language.");
    end_card();
    return changed;
}

inline void texts_page() {
    bool changed = language_card();
    changed |= callouts_card();
    changed |= texts_card();
    if (changed) save_config();
}

inline void visuals_page() {
    bool changed = false;
#if !FATRAINER_VISUALS
    auto& e = config::cfg.esp;
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
    changed |= color_field("esp_hunter", e.hunter);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Survivors");
    ImGui::SameLine(S(150));
    changed |= color_field("esp_survivor", e.survivor);
    note("Allies and enemies come from the game's teams. The ESP keeps drawing when the menu is closed.");
    end_card();
#endif

    changed |= tint_card("uv_light", "UV LIGHT", "Custom UV light color", "Your UV flashlight shines in your own color.", config::cfg.uv,
                         "Glow scales the light's brightness and its visible beam. If the color does not change right away, switch the UV light off and on.");
    changed |= tint_card("hunter_glow", "NIGHT HUNTER GLOW", "Custom Night Hunter glow color",
                         "The veins that light up when UV hits the Night Hunter glow in your own color.", config::cfg.hunter_glow,
                         "Only you see it: as a survivor on the hunter you hit, as the Night Hunter on your own arms. Glow scales how bright the veins get.");
    changed |= fov_card();
    changed |= hud_colors_card();
    changed |= crosshair_card();
    changed |= survivor_glow_card();
    if (changed) save_config();
}

#if !FATRAINER_VISUALS
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

inline void save_position() {
    on_game_thread([] {
        std::lock_guard<std::mutex> l(game::mx);
        cheats::saved_position = cheats::player_position();
        cheats::has_saved_position = std::isfinite(cheats::saved_position.x);
        toast(cheats::has_saved_position ? "Position saved" : "Your position could not be read");
    });
}

inline void go_to_saved_position() {
    if (!cheats::has_saved_position) return toast("No saved position yet");
    go_to(cheats::saved_position);
    toast("Back at the saved position");
}

inline void refill_now() {
    on_game_thread([] {
        std::lock_guard<std::mutex> l(game::mx);
        cheats::refill();
    });
    toast("Health & stamina refilled");
}

inline void toggle_esp() {
    config::cfg.esp.on = !config::cfg.esp.on;
    save_config();
    toast(std::string("Player ESP ") + (config::cfg.esp.on ? "on" : "off"));
}

struct KeyAction { const char* key; const char* label; void (*run)(); };
inline const KeyAction KEY_ACTIONS[] = {
    {REFILL_KEY, "Refill health & stamina", refill_now},
    {"kill_nearby", "Kill all enemies nearby", kill_nearby},
    {"save_position", "Save position", save_position},
    {"go_saved_position", "Go to saved position", go_to_saved_position},
    {"esp", "Player ESP on / off", toggle_esp},
};

inline const KeyAction* find_action(const std::string& key) {
    for (auto& a : KEY_ACTIONS)
        if (key == a.key) return &a;
    return nullptr;
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
    if (ImGui::Button("Save position")) save_position();
    ImGui::SameLine();
    ImGui::BeginDisabled(!cheats::has_saved_position);
    if (accent_button("Go to saved position")) go_to_saved_position();
    ImGui::EndDisabled();
    end_card();
}

#endif

struct Key { int vk; const char* name; };
inline const Key KEYS[] = {{VK_INSERT, "Insert"}, {VK_F8, "F8"},   {VK_HOME, "Home"}, {VK_END, "End"},   {VK_DELETE, "Delete"},
                           {VK_PRIOR, "Page Up"}, {VK_NEXT, "Page Down"}, {VK_F1, "F1"}, {VK_F2, "F2"}, {VK_F3, "F3"},
                           {VK_F4, "F4"},         {VK_F5, "F5"},   {VK_F6, "F6"},     {VK_F7, "F7"},     {VK_F9, "F9"},
                           {VK_F10, "F10"},       {VK_F11, "F11"}, {VK_F12, "F12"}};

inline void menu_key_combo(bool& changed);

inline void keybinds_page();

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
#if FATRAINER_VISUALS
    {"visuals", "Visuals", "Colors, HUD, field of view and glow", visuals_page, always, "LOOK", {}},
    {"texts", "Texts", "Game language and your own texts", texts_page, always, "LOOK", {}},
#else
    {"player", "Player", "Health, stamina, gear and movement", player_page, always, "CHEATS",
     {"god", "stamina", "damage_taken", "hook", "uv", "uv_slow", "lockpick", "no_fall", "speed", "jump"}},
    {"combat", "Combat", "Enemies, ammo, supplies and weapons", combat_page, always, "CHEATS", {"one_hit", "ammo", "no_reload", "last_wish", "supplies", "durability", "item_dupe", "melee_speed"}},
    {"skills", "Skills", "Experience and skill tree levels", skills_page, always, "CHEATS", {"xp"}},
    {"zombie", "Night Hunter", "Be The Zombie abilities", zombie_page, always, "CHEATS", {"z_energy", "z_uv", "z_cooldowns", "z_spits", "z_pound_hits", "z_camo"}},
    {"pvp", "PvP", "Configs and how far your attacks reach in Be The Zombie", pvp_page, always, "MODES",
     {"z_pounce", "z_pound", "z_tackle", "z_claws", "z_spit", "dodge_spit", "one_hit_hunter", "h_dfa", "h_dfa_pull", "h_dfa_height", "h_dropkick", "h_dropkick_angle", "h_dropkick_height", "h_kicks", "h_melee", "h_recharge"}},
    {"visuals", "Visuals", "Player ESP, colors, HUD, field of view and glow", visuals_page, always, "MODES", {}},
    {"texts", "Texts", "Game language and your own texts", texts_page, always, "MODES", {}},
    {"prison", "Prison", "Harran Prison timers and teleports", prison_page, always, "MODES", {"prison_pause"}},
    {"cash", "Cash", "Your money", cash_page, always, "ITEMS", {}},
    {"backpack", "Backpack", "Items you carry. Press Edit to change a weapon", [] { inventory_page(game::K_BACKPACK); }, always, "ITEMS", {}},
    {"stash", "Stash", "Items stored in your stash. Press Edit to change a weapon", [] { inventory_page(game::K_STASH); }, always, "ITEMS", {}},
    {"materials", "Materials", "Crafting parts and consumables", [] { inventory_page(game::K_MATERIALS); }, always, "ITEMS", {}},
    {"tools", "Tools", "Special items", [] { inventory_page(game::K_TOOLS); }, has_tools, "ITEMS", {}},
    {"give", "Give items", "Spawn any item in the game", give_page, always, "ITEMS", {}},
    {"keys", "Keybinds", "Keys for cheats and sliders", keybinds_page, always, "", {}},
#endif
#if FATRAINER_VISUALS
    {"settings", "Settings", "Look, menu key and version", settings_page, always, "", {}},
#else
    {"settings", "Settings", "Look, controls, configs and version", settings_page, always, "", {}},
#endif
};

inline int active_on(const Page& p) {
    int n = 0;
#if !FATRAINER_VISUALS
    for (const char* k : p.keys) {
        if (auto* c = cheats::find(k)) n += c->on;
        if (auto* t = cheats::find_tweak(k)) n += t->factor != 1.0f;
    }
#endif
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

inline void setting_label(const char* text) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(text);
    ImGui::SameLine(S(190));
}

inline const char* edition_story() {
#if FATRAINER_VISUALS
    return "You have the Visuals Version: it only changes how the game looks to you, with the UV light, Night Hunter and survivor glow, HUD colors, a custom crosshair and texts. It has no cheats and never connects to the internet. "
           "Install and update it with the FaTrainer launcher, or put xinput1_3.dll next to DyingLightGame.exe yourself.";
#else
    return "You have the Fatum Version, downloaded from the FaTrainer website or GitHub. It runs only when you start the game with Play in FaTrainer, and then asks GitHub once which version is the newest. "
           "If a newer one is out, the trainer turns itself off until you press Play in FaTrainer again, unless Use older versions is on. "
           "With Beta builds on it also asks for the beta build. Nothing else is sent anywhere.";
#endif
}

inline void about_card() {
    begin_card("about_trainer", "ABOUT");
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float big = font_size(f_tile);
    std::string title = std::string("FaTrainer ") + VERSION;
    dl->AddText(f_tile, big, p, C(T.text), title.c_str());
    float tw = f_tile->CalcTextSizeA(big, FLT_MAX, 0, title.c_str()).x;
    float pulse = motion() > 0 ? fmodf(now(), 2.2f) / 2.2f : -1;
    pill(dl, {p.x + tw + S(16), p.y + (big - font_size(f_small) - S(10)) / 2}, FATRAINER_EDITION, T.accent, pulse);
    ImGui::Dummy({0, big + S(2)});
    ImGui::TextColored(V(T.accent), "%s", FATRAINER_NAME);
    label("FaTrainer | Dying Light");
    ImGui::Dummy({0, S(2)});
    note(edition_story());
    end_card();
}

inline const float ACCENTS[][3] = {{0.910f, 0.592f, 0.227f}, {0.93f, 0.38f, 0.33f}, {0.49f, 0.79f, 0.55f}, {0.38f, 0.63f, 0.96f},
                                   {0.67f, 0.50f, 0.96f},    {0.94f, 0.49f, 0.71f}, {0.85f, 0.82f, 0.76f}};

inline void accent_swatches(bool& changed) {
    auto& c = config::cfg;
    for (int i = 0; i < (int)(sizeof ACCENTS / sizeof ACCENTS[0]); i++) {
        ImGui::PushID(i);
        ImVec2 p = ImGui::GetCursorScreenPos();
        float d = ImGui::GetFrameHeight();
        if (ImGui::InvisibleButton("swatch", {d, d})) memcpy(c.accent, ACCENTS[i], sizeof c.accent), changed = true;
        bool hovered = ImGui::IsItemHovered();
        if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        bool selected = !memcmp(c.accent, ACCENTS[i], sizeof c.accent);
        float ring = animate(ImGui::GetID("ring"), selected ? 1.0f : hovered ? 0.45f : 0.0f, 16);
        auto* dl = ImGui::GetWindowDrawList();
        ImVec2 center{p.x + d / 2, p.y + d / 2};
        ImU32 color = IM_COL32((int)(ACCENTS[i][0] * 255), (int)(ACCENTS[i][1] * 255), (int)(ACCENTS[i][2] * 255), 255);
        dl->AddCircleFilled(center, d / 2 - S(4) - (1 - ring) * S(1), C(color));
        if (ring > 0.01f) dl->AddCircle(center, d / 2 - S(0.5f), C(T.text, ring), 0, S(1.6f));
        ImGui::SameLine(0, S(6));
        ImGui::PopID();
    }
    ImGui::NewLine();
    if (color_field("accent", c.accent)) changed = true;
}

inline void menu_key_combo(bool& changed) {
    auto& c = config::cfg;
    setting_label("Menu key");
    ImGui::SetNextItemWidth(S(180));
    const char* current = "Insert";
    for (auto& k : KEYS)
        if (k.vk == c.menu_key) current = k.name;
    if (ImGui::BeginCombo("##key", current)) {
        for (auto& k : KEYS)
            if (ImGui::Selectable(k.name, k.vk == c.menu_key)) {
                c.menu_key = k.vk, changed = true;
#if !FATRAINER_VISUALS
                take_key_from_others(k.vk, "");
#endif
            }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    label("F8 always works too.");
}

#if !FATRAINER_VISUALS
inline void keybinds_page() {
    bool changed = false;
    begin_card("menu_key", "MENU");
    menu_key_combo(changed);
    end_card();
    begin_card("keys_actions", "ACTIONS");
    for (auto& action : KEY_ACTIONS) {
        ImGui::PushID(action.key);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(action.label);
        ImGui::SameLine(S(300));
        auto found = config::cfg.cheat_keys.find(action.key);
        int vk = found == config::cfg.cheat_keys.end() ? 0 : found->second;
        if (key_button("bind", vk)) {
            if (vk) take_key_from_others(vk, action.key), config::cfg.cheat_keys[action.key] = vk;
            else config::cfg.cheat_keys.erase(action.key);
            changed = true;
        }
        ImGui::PopID();
    }
    end_card();
    capture_note();
    for (auto& page : PAGES) {
        bool any = false;
        for (const char* key : page.keys) any |= cheats::find(key) != nullptr || cheats::find_tweak(key) != nullptr;
        if (!any) continue;
        std::string title = page.title;
        for (char& ch : title) ch = (char)toupper((unsigned char)ch);
        begin_card((std::string("keys_") + page.id).c_str(), title.c_str());
        for (const char* key : page.keys) {
            auto* cheat = cheats::find(key);
            auto* tweak = cheat ? nullptr : cheats::find_tweak(key);
            if (!cheat && !tweak) continue;
            ImGui::PushID(key);
            ImGui::AlignTextToFramePadding();
            bool lit = cheat ? cheat->on.load() : tweak->factor != 1.0f;
            ImGui::PushStyleColor(ImGuiCol_Text, lit ? V(T.accent) : V(T.text));
            ImGui::TextUnformatted(cheat ? cheat->label : (std::string(tweak->label) + " to Max").c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine(S(300));
            auto found = config::cfg.cheat_keys.find(key);
            int vk = found == config::cfg.cheat_keys.end() ? 0 : found->second;
            if (key_button("bind", vk)) {
                if (vk) take_key_from_others(vk, key), config::cfg.cheat_keys[key] = vk;
                else config::cfg.cheat_keys.erase(key);
                changed = true;
            }
            ImGui::PopID();
        }
        end_card();
    }
    note("Press a cheat's key while the menu is closed to turn it on or off, or an action's key to run it. A slider's key sets it to Max and the next press puts back what you had. "
         "A message at the bottom of the screen says which. Spit keys are on the Night Hunter page.");
    if (changed) save_config();
}

#endif

inline void settings_page() {
    auto& c = config::cfg;
    auto& look = c.look;
    bool changed = false;
    about_card();

    begin_card("look", "APPEARANCE");
    setting_label("Accent");
    accent_swatches(changed);
    setting_label("Interface size");
    static float pending_scale = c.scale;
    ImGui::SetNextItemWidth(S(220));
    ImGui::SliderFloat("##scale", &pending_scale, 0.75f, 1.5f, "%.2fx", ImGuiSliderFlags_NoInput);
    ImGui::SameLine();
    ImGui::BeginDisabled(pending_scale == c.scale);
    if (accent_button("Apply")) c.scale = pending_scale, changed = true;
    ImGui::EndDisabled();
    setting_label("Window opacity");
    ImGui::SetNextItemWidth(S(300));
    float opacity = look.opacity * 100;
    if (ImGui::SliderFloat("##opacity", &opacity, 60, 100, "%.0f%%")) look.opacity = opacity / 100, changed = true;
    setting_label("Corner roundness");
    ImGui::SetNextItemWidth(S(300));
    float round = look.roundness * 100;
    if (ImGui::SliderFloat("##round", &round, 0, 160, round < 1 ? "Square" : "%.0f%%")) look.roundness = round / 100, changed = true;
    setting_label("Background dim");
    ImGui::SetNextItemWidth(S(300));
    float pct = c.dim * 100;
    if (ImGui::SliderFloat("##dim", &pct, 0, 95, "%.0f%%")) c.dim = pct / 100, changed = true;
    setting_label("Animations");
    const char* motions[] = {"Off", "Subtle", "Full"};
    if (segmented("motion", motions, 3, look.motion, S(300))) changed = true;
    ImGui::Dummy({0, S(2)});
    if (switch_row("Harran skyline", "The city at the bottom of the sidebar, with windows that light up.", look.skyline)) changed = true;
    if (switch_row("Accent glow", "A soft light in your accent color drifts over the page.", look.glow)) changed = true;
    ImGui::Dummy({0, S(2)});
    if (ImGui::Button("Reset appearance")) {
        config::Config fresh;
        memcpy(c.accent, fresh.accent, sizeof c.accent);
        c.dim = fresh.dim, c.look = fresh.look;
        changed = true;
    }
    end_card();

    begin_card("controls", "CONTROLS");
    menu_key_combo(changed);
#if !FATRAINER_VISUALS
    bool remember = c.remember_cheats;
    if (switch_row("Remember cheats", "Turn your cheats back on the next time the game starts.", remember)) c.remember_cheats = remember, changed = true;
    bool older = c.allow_older;
    if (switch_row("Use older versions", "Keep this version working after a newer one is out. The update notice still shows.", older)) c.allow_older = older, changed = true;
    bool beta = c.beta;
    if (switch_row("Beta builds", "Optional. Also update to the beta build, the version being worked on right now, when it is newer than the latest release. It may have bugs. Takes effect the next time the game starts.", beta))
        c.beta = beta, changed = true;
#endif
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

#if !FATRAINER_VISUALS
    configs_card();
#endif

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

#if FATRAINER_VISUALS
inline std::string g_page = "visuals";
#else
inline std::string g_page = "player";
#endif
inline float g_opened_at = -10;

inline void open_page(const char* id) {
    if (g_page == id) return;
    g_page = id;
    g_page_at = now();
}

inline bool nav_item(const Page& p, bool active, bool marked, float& active_y) {
    ImGui::PushID(p.id);
    float w = ImGui::GetContentRegionAvail().x, h = S(31);
    ImVec2 at = ImGui::GetCursorScreenPos();
    if (active) active_y = at.y - ImGui::GetWindowPos().y + ImGui::GetScrollY();
    bool clicked = ImGui::InvisibleButton("nav", {w, h});
    bool hovered = ImGui::IsItemHovered();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    float hover = animate(ImGui::GetID("h"), hovered && !active ? 1.0f : 0.0f);
    float on = animate(ImGui::GetID("a"), active ? 1.0f : 0.0f);
    auto* dl = ImGui::GetWindowDrawList();
    if (hover > 0.01f) dl->AddRectFilled(at, {at.x + w, at.y + h}, C(IM_COL32(255, 255, 255, 255), 0.035f * hover), R(9));
    float fs = ImGui::GetFontSize();
    float nudge = (on * S(6) + hover * S(3)) * motion();
    dl->AddText(on > 0.5f ? f_strong : f_body, fs, {at.x + S(16) + nudge, at.y + (h - fs) / 2}, C(brand::mix(brand::mix(T.muted, T.soft, hover), T.text, on)), p.title);
    if (marked) {
        ImVec2 dot{at.x + w - S(14), at.y + h / 2};
        float breathe = motion() > 0 ? 0.5f + 0.5f * sinf(now() * 2.4f) : 0;
        dl->AddCircleFilled(dot, S(6) + breathe * S(2), C(T.accent, 0.16f));
        dl->AddCircleFilled(dot, S(3.5f), C(T.accent));
    }
    ImGui::PopID();
    return clicked;
}

inline void sidebar_skyline(ImVec2 a, ImVec2 b) {
    static ImVec2 light{-1, -1};
    float t = scene_time();
    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool inside = mouse.x >= a.x && mouse.x <= b.x && mouse.y >= a.y - S(60) && mouse.y <= b.y;
    ImVec2 target = inside ? mouse : ImVec2{a.x + (b.x - a.x) * (0.5f + 0.32f * sinf(t * 0.29f)), a.y + (b.y - a.y) * (0.42f + 0.12f * sinf(t * 0.43f))};
    if (light.x < 0 || motion() == 0) light = target;
    brand::follow(light.x, target.x, inside ? 8.0f : 1.6f), brand::follow(light.y, target.y, inside ? 8.0f : 1.6f);
    brand::CityLook look;
    look.accent = T.accent, look.ground = T.side, look.light_radius = S(110), look.glow = config::cfg.look.glow ? 0.13f : 0, look.center = 1120, look.fog = 0.5f, look.glow_rings = 20;
    float rise = motion() >= 1 ? now() - g_opened_at + 0.15f : 100;
    auto* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(a, b, true);
    brand::draw_city(dl, a, b, light, {((light.x - a.x) / (b.x - a.x) - 0.5f) * -S(14) * motion(), 0}, t, rise, look);
    dl->AddRectFilledMultiColor(a, {b.x, a.y + (b.y - a.y) * 0.35f}, C(T.side), C(T.side), C(T.side, 0), C(T.side, 0));
    dl->PopClipRect();
}

inline void remember_window_size(ImVec2 now) {
    static ImVec2 last{-1, -1};
    static bool resized = false;
    if (last.x >= 0 && (now.x != last.x || now.y != last.y)) resized = true;
    last = now;
    if (!resized || ImGui::IsMouseDown(ImGuiMouseButton_Left)) return;
    resized = false;
    config::cfg.window[0] = roundf(now.x / config::cfg.scale), config::cfg.window[1] = roundf(now.y / config::cfg.scale);
    save_config();
}

inline void sidebar() {
    float opacity = config::cfg.look.opacity;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, V(T.side, opacity));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(16), S(22)});
    ImGui::BeginChild("side", {S(236), 0}, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    if (config::cfg.look.skyline) sidebar_skyline({wp.x, wp.y + ws.y - S(170)}, {wp.x + ws.x, wp.y + ws.y});
    dl->AddLine({wp.x + ws.x - 1, wp.y}, {wp.x + ws.x - 1, wp.y + ws.y}, C(T.line_soft));

    ImVec2 p = ImGui::GetCursorScreenPos();
    float brand_size = font_size(f_brand);
    float letters = motion() >= 1 ? now() - g_opened_at : 10;
    brand::draw_letters(dl, f_brand, brand_size, {p.x + S(4), p.y}, "FaTrainer", letters, C(T.text));
    float brand_w = f_brand->CalcTextSizeA(brand_size, FLT_MAX, 0, "FaTrainer").x;
    float dot_in = motion() >= 1 ? brand::ease_expo((letters - 0.55f) / 0.5f) : 1;
    dl->AddCircleFilled({p.x + S(4) + brand_w + S(5), p.y + brand_size * 0.78f}, S(3.2f) * dot_in, C(T.accent));
    ImGui::Dummy({0, brand_size + S(2)});
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(4));
    ImGui::PushFont(f_mono);
    ImGui::TextColored(V(T.muted), "Dying Light  v%s", VERSION);
    ImGui::PopFont();
    ImVec2 pill_at = ImGui::GetCursorScreenPos();
    pill(dl, {pill_at.x + S(2), pill_at.y + S(2)}, FATRAINER_EDITION, T.accent, motion() > 0 ? fmodf(now(), 2.6f) / 2.6f : -1);
    ImGui::Dummy({0, font_size(f_small) + S(18)});

#if FATRAINER_VISUALS
    int active = 0;
#else
    int active = cheats::active_count();
#endif
    float footer = ImGui::GetFrameHeight() * (active ? 2 : 1) + S(active ? 64 : 52);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0, 0, 0, 0});
    ImGui::BeginChild("nav", {0, ImGui::GetContentRegionAvail().y - footer}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleColor();
    auto* nav = ImGui::GetWindowDrawList();
    static float pill_y = -1;
    static float target_y = -1;
    if (target_y >= 0) {
        if (pill_y < 0 || motion() == 0) pill_y = target_y;
        brand::follow(pill_y, target_y, 16);
        ImVec2 o = ImGui::GetWindowPos();
        float y = o.y + pill_y - ImGui::GetScrollY(), w = ImGui::GetContentRegionAvail().x, h = S(31);
        nav->AddRectFilled({o.x, y}, {o.x + w, y + h}, C(T.accent, 0.12f), R(9));
        nav->AddRectFilled({o.x, y + h * 0.26f}, {o.x + S(3), y + h * 0.74f}, C(T.accent), S(2));
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {S(10), S(2)});
    std::string section;
    float found_y = -1;
    std::vector<std::string> sections;
    for (auto& pg : PAGES)
        if (std::find(sections.begin(), sections.end(), pg.section) == sections.end()) sections.push_back(pg.section);
    std::vector<const Page*> ordered;
    for (auto& sec : sections)
        for (auto& e : config::cfg.pages) {
            auto* pg = page_by_id(e.id);
            if (pg && e.visible && pg->shown() && sec == pg->section) ordered.push_back(pg);
        }
    for (const Page* pg : ordered) {
        if (*pg->section && pg->section != section) {
            ImGui::Dummy({0, section.empty() ? 0 : S(8)});
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(16));
            caps(pg->section);
            ImGui::Dummy({0, S(2)});
        }
        section = pg->section;
        if (nav_item(*pg, g_page == pg->id, active_on(*pg) > 0, found_y)) open_page(pg->id);
    }
    target_y = found_y;
    ImGui::PopStyleVar();
    ImGui::EndChild();
#if !FATRAINER_VISUALS
    if (active) {
        ImGui::Dummy({0, S(2)});
        if (ImGui::Button(("Turn all off (" + std::to_string(active) + ")").c_str(), {-1, 0})) cheats::all_off(), save_config();
    }
#endif
    ImGui::Dummy({0, S(4)});
#if FATRAINER_VISUALS
    bool in_game = visuals::hud_found() || !game::g.descs.empty();
#else
    bool in_game = cheats::player != 0;
#endif
    const char* st = game::g.scanning ? "Scanning..." : game::g.vt_money ? (in_game ? "Connected" : "Waiting for save") : "Game not ready";
    ImU32 sc = game::g.scanning ? T.accent : game::g.vt_money ? (in_game ? T.ok : T.accent) : T.bad;
    ImVec2 at = ImGui::GetCursorScreenPos();
    float fs = font_size(f_small);
    ImVec2 dot{at.x + S(10), at.y + fs / 2 + S(1)};
    float pulse = motion() > 0 ? fmodf(now(), game::g.scanning ? 0.9f : 2.2f) / (game::g.scanning ? 0.9f : 2.2f) : 1;
    ImGui::GetWindowDrawList()->AddCircle(dot, S(4) + S(6) * pulse, C(sc, 0.45f * (1 - pulse)), 0, S(1.2f));
    ImGui::GetWindowDrawList()->AddCircleFilled(dot, S(4), C(sc));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(22));
    ImGui::PushFont(f_small);
    ImGui::TextColored(V(sc), "%s", st);
    ImGui::PopFont();
    ImGui::BeginDisabled(game::g.scanning);
    if (ImGui::Button("Refresh", {-1, 0})) request_refresh();
    ImGui::EndDisabled();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

inline void draw_toast() {
    std::string msg;
    DWORD at;
    {
        std::lock_guard<std::mutex> q(g_qmx);
        msg = g_toast, at = g_toast_at;
    }
    const float SHOW = TOAST_MS / 1000.0f;
    float age = (GetTickCount() - at) / 1000.0f;
    if (msg.empty() || age > SHOW) return;
    float m = motion();
    float in = m > 0 ? brand::ease_expo(age / 0.35f) : 1, out = m > 0 ? brand::clamp01((SHOW - age) / 0.4f) : 1, a = std::min(in, out);
    ImGuiIO& io = ImGui::GetIO();
    auto* dl = ImGui::GetForegroundDrawList();
    float fs = font_size(f_body), tw = f_body->CalcTextSizeA(fs, FLT_MAX, 0, msg.c_str()).x;
    float w = tw + S(58), h = fs + S(26);
    ImVec2 c{io.DisplaySize.x / 2, io.DisplaySize.y - S(64) + (1 - in) * S(28) * m};
    ImVec2 p0{c.x - w / 2, c.y - h}, p1{c.x + w / 2, c.y};
    dl->AddRectFilled({p0.x, p0.y + S(6)}, {p1.x, p1.y + S(10)}, C(IM_COL32(0, 0, 0, 255), 0.35f * a), h / 2);
    dl->AddRectFilled(p0, p1, C(T.raised, a), h / 2);
    dl->AddRect(p0, p1, C(T.line, a), h / 2);
    float life = brand::clamp01(1 - age / SHOW);
    dl->AddLine({p0.x + h / 2, p1.y - 1}, {p0.x + h / 2 + (w - h) * life, p1.y - 1}, C(T.accent, 0.6f * a), S(1.5f));
    ImVec2 dot{p0.x + S(22), (p0.y + p1.y) / 2};
    dl->AddCircleFilled(dot, S(4), C(T.accent, a));
    if (m > 0) dl->AddCircle(dot, S(4) + S(7) * brand::clamp01(age / 0.8f), C(T.accent, 0.5f * a * (1 - brand::clamp01(age / 0.8f))), 0, S(1.5f));
    dl->AddText(f_body, fs, {p0.x + S(38), p0.y + (h - fs) / 2}, C(T.text, a), msg.c_str());
}

inline void draw_cursor() {
    ImGuiIO& io = ImGui::GetIO();
    if (!ImGui::IsMousePosValid()) return;
    ImVec2 m = io.MousePos;
    static ImVec2 halo{-1, -1};
    static float clicked_at = -10;
    if (halo.x < 0 || motion() == 0) halo = m;
    brand::follow(halo.x, m.x, 20), brand::follow(halo.y, m.y, 20);
    if (ImGui::IsMouseClicked(0)) clicked_at = now();
    auto* dl = ImGui::GetForegroundDrawList();
    float click = motion() > 0 ? brand::clamp01((now() - clicked_at) / 0.4f) : 1;
    if (click < 1) dl->AddCircle(m, S(6) + S(16) * brand::ease_out(click), C(T.accent, 0.6f * (1 - click)), 0, S(1.6f));
    dl->AddCircleFilled(halo, S(13), C(T.accent, 0.09f));
    ImGui::RenderMouseCursor(m, config::cfg.scale, ImGui::GetMouseCursor(), C(T.text), IM_COL32(0, 0, 0, 255), IM_COL32(0, 0, 0, 70));
}

inline float presence(bool open) {
    static float v = 0;
    static DWORD last = GetTickCount();
    DWORD t = GetTickCount();
    float dt = std::min(0.1f, (t - last) / 1000.0f);
    last = t;
    float m = motion(), speed = m == 0 ? 1000.0f : m < 1 ? 1 / 0.12f : 1 / 0.24f;
    v = open ? std::min(1.0f, v + dt * speed) : std::max(0.0f, v - dt * speed * 1.4f);
    return v;
}

inline void draw(float shown_raw, bool open) {
    ImGuiIO& io = ImGui::GetIO();
    if (config::cfg.rainbow.count("accent")) apply_style();
    static float last = 0;
    if (last == 0 && shown_raw > 0) g_opened_at = now(), g_page_at = now();
    last = shown_raw;
    float shown = brand::ease_out(shown_raw);
    if (config::cfg.dim > 0) ImGui::GetBackgroundDrawList()->AddRectFilled({0, 0}, io.DisplaySize, IM_COL32(0, 0, 0, (int)(config::cfg.dim * 255 * shown)));
    static bool first = true;
    if (first) {
        first = false;
        if (const char* p = getenv("DLT_PAGE")) g_page = p;
#if !FATRAINER_VISUALS
        if (getenv("DLT_SECTIONS"))
            for (int type : {1, 2, 2, 2, 2, 2, 3, 4}) cheats::prison_sections.push_back({0, type, (int)cheats::prison_sections.size(), false, {1, 2, 3}});
#endif
    }
    static ImVec2 rest{-1, -1};
    if (rest.x < 0) ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_FirstUseEver, {0.5f, 0.5f});
    else if (shown < 1) ImGui::SetNextWindowPos({rest.x, rest.y + (1 - shown) * S(22) * motion()});
    const ImVec2 DEFAULT_WINDOW{1200, 860};
    const float FIT = 0.92f;
    auto& saved = config::cfg.window;
    ImVec2 size = saved[0] > 0 ? ImVec2{S(saved[0]), S(saved[1])} : ImVec2{S(DEFAULT_WINDOW.x), S(DEFAULT_WINDOW.y)};
    ImGui::SetNextWindowSize({std::min(size.x, io.DisplaySize.x * FIT), std::min(size.y, io.DisplaySize.y * FIT)}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({S(780), S(500)}, {FLT_MAX, FLT_MAX});
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, shown);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (!open) flags |= ImGuiWindowFlags_NoInputs;
    ImGui::Begin(TITLE, nullptr, flags);
    if (shown >= 1 || rest.x < 0) rest = ImGui::GetWindowPos();
    remember_window_size(ImGui::GetWindowSize());
    {
        std::lock_guard<std::mutex> l(game::mx);
        const Page* page = page_by_id(g_page);
        if (!page || !page->shown()) page = &PAGES[0], g_page = page->id;
        sidebar();
        ImGui::SameLine(0, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(32), S(26)});
        ImGui::BeginChild("content", {0, 0}, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();
        auto* dl = ImGui::GetWindowDrawList();
        ImVec2 cp = ImGui::GetWindowPos(), cs = ImGui::GetWindowSize();
        float t = scene_time();
        if (config::cfg.look.glow)
            brand::soft_glow(dl, {cp.x + cs.x - S(90) + sinf(t * 0.21f) * S(60), cp.y - S(150) + sinf(t * 0.33f) * S(20)}, S(480), T.accent, 0.075f, 24);
        float since = motion() >= 1 ? now() - g_page_at : 10;
        ImVec2 hp = ImGui::GetCursorScreenPos();
        float hs = font_size(f_head);
        brand::draw_letters(dl, f_head, hs, hp, page->title, since, C(T.text));
        ImGui::Dummy({0, hs * 1.02f});
        ImGui::PushFont(f_small);
        ImGui::TextColored(V(T.soft), "%s", page->subtitle);
        ImGui::PopFont();
        ImVec2 up = ImGui::GetCursorScreenPos();
        float line = motion() > 0 ? brand::ease_expo((since - 0.15f) / 0.6f) : 1;
        dl->AddRectFilled({up.x, up.y + S(4)}, {up.x + S(48) * line, up.y + S(6)}, C(T.accent), S(1));
        ImGui::Dummy({0, S(14)});
        float header_wheel = ImGui::IsWindowHovered() ? ImGui::GetIO().MouseWheel : 0;
        ImGui::BeginChild(page->id, {0, 0});
        if (header_wheel != 0) ImGui::SetScrollY(ImGui::GetScrollY() - header_wheel * ImGui::GetFontSize() * 5);
        static const char* preview_scroll = getenv("DLT_SCROLL");
        if (preview_scroll && ImGui::GetFrameCount() > 5 && ImGui::GetFrameCount() < 30) ImGui::SetScrollY((float)atof(preview_scroll));
        g_card = 0;
        g_card_offsets.clear();
        page->draw();
        ImGui::EndChild();
        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleVar();
    draw_toast();
    static const bool cursor = getenv("DLT_NO_CURSOR") == nullptr;
    if (open && cursor) draw_cursor();
}

enum Notice { NOTICE_UPDATE, NOTICE_OLDER, NOTICE_BUILD_DOWNLOADING, NOTICE_BUILD_READY, NOTICE_BUILD_FAILED };

inline void draw_update_notice(const std::string& latest, float alpha, int kind) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(30), S(24)});
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, S(48) - (1 - alpha) * S(24)}, ImGuiCond_Always, {0.5f, 0});
    ImGui::SetNextWindowSize({S(580), 0});
    ImGui::Begin("##update", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings);
    ImVec2 p = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize();
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled({p.x, p.y + S(20)}, {p.x + S(4), p.y + sz.y - S(20)}, C(T.accent), S(2));
    brand::soft_glow(dl, {p.x + sz.x, p.y}, S(200), T.accent, 0.06f, 20);
    bool build = kind >= NOTICE_BUILD_DOWNLOADING;
    caps(build ? "NEW BUILD" : "UPDATE");
    ImGui::PushFont(f_head);
    ImGui::TextUnformatted(("FaTrainer " + latest + " is out").c_str());
    ImGui::PopFont();
    ImGui::Dummy({0, S(2)});
    ImGui::PushTextWrapPos(0);
    std::string mine = VERSION;
    std::string text =
        kind == NOTICE_OLDER ? "You are playing with an older version (" + mine + ") because Use older versions is on. To update, close the game, open FaTrainer and press Play."
        : kind == NOTICE_BUILD_DOWNLOADING ? "Downloading the new build. This build (" + mine + ") keeps working until then."
        : kind == NOTICE_BUILD_READY ? "The new build is downloaded and starts the next time you open the game. This build (" + mine + ") keeps working until then."
        : kind == NOTICE_BUILD_FAILED ? "The new build could not be downloaded. This build (" + mine + ") keeps working; FaTrainer updates it the next time you press Play."
        : "This version (" + mine + ") is turned off until you update. Close the game, open FaTrainer and press Play; it updates the trainer first. "
          "The game itself works as usual. To keep playing with this version, add the line allow_older=1 to fatrainer.ini next to the game.";
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopTextWrapPos();
    if (kind == NOTICE_UPDATE) {
        ImGui::PushFont(f_small);
        ImGui::TextColored(V(T.muted), "Press your menu key or F8 to show this again.");
        ImGui::PopFont();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

inline void load_fonts() {
    f_body = brand::load_font(brand::BODY, 18);
    f_strong = brand::load_font(brand::STRONG, 18);
    f_small = brand::load_font(brand::BODY, 14.5f);
    f_label = brand::load_font(brand::STRONG, 11.5f);
    f_mono = brand::load_font(brand::MONO, 12.5f);
    f_head = brand::load_font(brand::DISPLAY, 36);
    f_tile = brand::load_font(brand::HEADING, 28);
    f_brand = brand::load_font(brand::DISPLAY, 24);
    f_big = brand::load_font(brand::DISPLAY, 58);
    ImGui::GetIO().FontDefault = f_body;
}

inline void startup() {
    config::load(config::default_path());
    config::normalize(page_ids());
#if !FATRAINER_VISUALS
    config::Profile p;
    if (config::cfg.remember_cheats) p.cheats = config::cfg.cheats_on;
    p.tweaks = config::cfg.tweaks;
    cheats::apply_profile(p);
    cheats::set_weapon_edits(config::cfg.weapon_edits);
#endif
    if (getenv("DLT_ESP")) config::cfg.esp.on = config::cfg.esp.health_text = config::cfg.esp.rank = config::cfg.esp.rage = true;
}

}
