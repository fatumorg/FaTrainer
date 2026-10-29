#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "imgui.h"

#ifdef _WIN32
#define EMBED_SECTION ".section .rdata,\"dr\"\n"
#else
#define EMBED_SECTION ".section .rodata\n"
#endif
#define EMBED(name, file)                                     \
    extern "C" const unsigned char name[], name##_end[];      \
    asm(EMBED_SECTION ".global " #name "\n.balign 16\n" #name \
                      ":\n.incbin \"" file "\"\n.global " #name "_end\n" #name "_end:\n.byte 0\n.text\n");

EMBED(font_display, "fonts/display-latin.ttf")
EMBED(font_display_ext, "fonts/display-latin-ext.ttf")
EMBED(font_heading, "fonts/heading-latin.ttf")
EMBED(font_heading_ext, "fonts/heading-latin-ext.ttf")
EMBED(font_body, "fonts/body-latin.ttf")
EMBED(font_body_ext, "fonts/body-latin-ext.ttf")
EMBED(font_strong, "fonts/strong-latin.ttf")
EMBED(font_strong_ext, "fonts/strong-latin-ext.ttf")
EMBED(font_mono, "fonts/mono-latin.ttf")
EMBED(font_mono_ext, "fonts/mono-latin-ext.ttf")

namespace brand {

const ImU32 GROUND = IM_COL32(17, 17, 19, 255), RAISED = IM_COL32(24, 24, 27, 255), LINE = IM_COL32(42, 42, 46, 255),
            LINE_SOFT = IM_COL32(30, 30, 34, 255), TEXT = IM_COL32(236, 230, 218, 255), SOFT = IM_COL32(185, 178, 165, 255),
            MUTED = IM_COL32(132, 125, 114, 255), ACCENT = IM_COL32(100, 200, 100, 255), ACCENT_INK = IM_COL32(8, 24, 8, 255),
            GOOD = IM_COL32(126, 201, 140, 255), BAD = IM_COL32(236, 112, 96, 255);
const float PI = 3.14159265f;

enum Face { DISPLAY, HEADING, BODY, STRONG, MONO };

inline ImFont* load_font(Face face, float size) {
    static const ImWchar LATIN[] = {0x0020, 0x00FF, 0x2010, 0x2027, 0x2030, 0x205E, 0x20AC, 0x20AC, 0x2122, 0x2122, 0};
    static const ImWchar LATIN_EXT[] = {0x0100, 0x02FF, 0x1E00, 0x1EFF, 0x20A0, 0x20C0, 0};
    struct Pair { const unsigned char *latin, *latin_end, *ext, *ext_end; };
    const Pair faces[] = {{font_display, font_display_end, font_display_ext, font_display_ext_end},
                          {font_heading, font_heading_end, font_heading_ext, font_heading_ext_end},
                          {font_body, font_body_end, font_body_ext, font_body_ext_end},
                          {font_strong, font_strong_end, font_strong_ext, font_strong_ext_end},
                          {font_mono, font_mono_end, font_mono_ext, font_mono_ext_end}};
    const Pair& f = faces[face];
    ImFontConfig c;
    c.FontDataOwnedByAtlas = false;
    c.OversampleH = 2;
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)f.latin, (int)(f.latin_end - f.latin), size, &c, LATIN);
    c.MergeMode = true;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)f.ext, (int)(f.ext_end - f.ext), size, &c, LATIN_EXT);
    return font;
}

inline float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
inline float ease_out(float t) { t = clamp01(t); return 1 - powf(1 - t, 3); }
inline float ease_expo(float t) { t = clamp01(t); return t >= 1 ? 1 : 1 - powf(2, -10 * t); }
inline float ease_in_out(float t) { t = clamp01(t); return t < 0.5f ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2; }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline ImVec2 lerp(ImVec2 a, ImVec2 b, float t) { return {lerp(a.x, b.x, t), lerp(a.y, b.y, t)}; }
inline ImU32 C(ImU32 c, float alpha = 1) { return ImGui::GetColorU32(c, alpha); }
inline ImU32 mix(ImU32 a, ImU32 b, float t) {
    auto ch = [&](int shift) { return (ImU32)lerp((float)((a >> shift) & 0xff), (float)((b >> shift) & 0xff), clamp01(t)) << shift; };
    return ch(0) | ch(8) | ch(16) | ch(24);
}
inline float follow(float& v, float target, float rate) {
    v += (target - v) * (1 - expf(-rate * ImGui::GetIO().DeltaTime));
    return v;
}
inline float text_w(ImFont* f, const char* s, const char* e = nullptr, float size = 0) {
    return f->CalcTextSizeA(size ? size : f->FontSize, FLT_MAX, 0, s, e).x;
}

inline void soft_glow(ImDrawList* dl, ImVec2 c, float radius, ImU32 color, float strength, int rings = 48) {
    for (int i = 0; i < rings; i++) {
        float f = 1 - i / (float)rings;
        dl->AddCircleFilled(c, radius * f, C(color, strength / rings * 2.2f * (0.4f + 0.6f * f)), 72);
    }
}

inline void round_corners(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, ImU32 color) {
    struct Corner { ImVec2 corner, center; float from; };
    const Corner corners[4] = {{a, {a.x + r, a.y + r}, PI}, {{b.x, a.y}, {b.x - r, a.y + r}, 1.5f * PI}, {b, {b.x - r, b.y - r}, 0}, {{a.x, b.y}, {a.x + r, b.y - r}, 0.5f * PI}};
    const int STEPS = 16;
    for (const Corner& c : corners)
        for (int i = 0; i < STEPS; i++) {
            float a0 = c.from + PI / 2 * i / STEPS, a1 = c.from + PI / 2 * (i + 1) / STEPS;
            dl->AddTriangleFilled(c.corner, {c.center.x + cosf(a0) * r, c.center.y + sinf(a0) * r}, {c.center.x + cosf(a1) * r, c.center.y + sinf(a1) * r}, color);
        }
}

inline void draw_letters(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, const char* word, float t, ImU32 color) {
    float x = pos.x;
    dl->PushClipRect({pos.x - size, pos.y - size * 0.1f}, {pos.x + text_w(font, word, nullptr, size) + size, pos.y + size * 1.12f}, true);
    for (int i = 0; word[i]; i++) {
        float a = ease_expo((t - i * 0.055f) / 0.7f);
        dl->AddText(font, size, {x, pos.y + (1 - a) * size}, C(color, a), word + i, word + i + 1);
        x += text_w(font, word + i, word + i + 1, size);
    }
    dl->PopClipRect();
}

inline void spaced_caps(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, const char* text, ImU32 color, float spacing) {
    float x = pos.x;
    for (const char* c = text; *c; c++) {
        dl->AddText(font, size, {x, pos.y}, color, c, c + 1);
        x += text_w(font, c, c + 1, size) + spacing;
    }
}

struct Window { float x, y, phase; };
struct Building { float x, w, h, roof_w, roof_h, antenna; std::vector<Window> windows; };
struct City { std::vector<Building> back, front; float crane_x = 0, crane_h = 0; };

inline uint32_t rng_state;
inline float rnd() {
    rng_state = rng_state * 1664525u + 1013904223u;
    return (rng_state >> 8) / 16777216.0f;
}

inline std::vector<Building> make_layer(uint32_t seed, float min_h, float max_h, float min_w, float max_w, float window_chance) {
    rng_state = seed;
    std::vector<Building> out;
    for (float x = -60; x < 1660;) {
        Building b{x, min_w + rnd() * (max_w - min_w), min_h + rnd() * (max_h - min_h), 0, 0, 0, {}};
        if (rnd() < 0.45f) b.roof_w = b.w * (0.2f + rnd() * 0.3f), b.roof_h = 8 + rnd() * 14;
        if (rnd() < 0.3f) b.antenna = 20 + rnd() * 40;
        for (float wy = 16; wy < b.h - 12; wy += 21)
            for (float wx = 9; wx < b.w - 14; wx += 15)
                if (rnd() < window_chance) b.windows.push_back({wx, wy, rnd() * 100});
        out.push_back(b);
        x += b.w + 2 + rnd() * 6;
    }
    return out;
}

inline City make_city() {
    City c;
    c.back = make_layer(7, 120, 270, 60, 125, 0.16f);
    c.front = make_layer(19, 40, 165, 70, 150, 0.13f);
    rng_state = 41;
    Building tower{1150, 58, 340, 0, 0, 0, {}};
    for (float wy = 20; wy < 330; wy += 21)
        for (float wx = 10; wx < 46; wx += 15)
            if (rnd() < 0.2f) tower.windows.push_back({wx, wy, rnd() * 100});
    c.back.push_back(tower);
    c.crane_x = 1460, c.crane_h = 330;
    return c;
}

inline const City& city() {
    static City c = make_city();
    return c;
}

struct CityLook { ImU32 accent = ACCENT, ground = GROUND; float light_radius = 190, glow = 0.28f, center = 800, fog = 0.32f; int glow_rings = 48; };

inline void draw_city(ImDrawList* dl, ImVec2 a, ImVec2 b, ImVec2 light, ImVec2 parallax, float t, float rise_t, const CityLook& look = {}) {
    const float VISIBLE_UNITS = 1180;
    float k = std::max((b.x - a.x) / VISIBLE_UNITS, (b.y - a.y) / 380);
    float ox = (a.x + b.x) / 2 - look.center * k;
    float radius = look.light_radius;
    auto light_on = [&](float x0, float y0, float x1, float y1) {
        float cx = std::clamp(light.x, x0, x1), cy = std::clamp(light.y, y0, y1);
        float f = 1 - sqrtf((light.x - cx) * (light.x - cx) + (light.y - cy) * (light.y - cy)) / radius;
        return f > 0 ? f * f : 0.0f;
    };
    ImU32 window_dim = (mix(IM_COL32(0, 0, 0, 255), look.accent, 0.72f) & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, 150);
    ImU32 window_lit = mix(look.accent, IM_COL32(255, 255, 255, 255), 0.6f);
    auto layer = [&](const std::vector<Building>& buildings, ImU32 wall, ImU32 wall_lit, float shift, float delay) {
        for (const Building& bd : buildings) {
            float x0 = ox + bd.x * k + shift, x1 = x0 + bd.w * k;
            float rise = ease_expo((rise_t - delay - (bd.x + 60) / 1700 * 0.55f) / 0.7f);
            if (x1 < a.x - 40 || x0 > b.x + 40 || rise <= 0) continue;
            float y1 = b.y, y0 = y1 - bd.h * k * rise;
            float lit = light_on(x0, y0, x1, y1);
            ImU32 color = mix(wall, wall_lit, lit);
            dl->AddRectFilled({x0, y0}, {x1, y1}, C(color));
            if (bd.roof_w > 0) {
                float rx = x0 + (bd.w - bd.roof_w) * 0.35f * k;
                dl->AddRectFilled({rx, y0 - bd.roof_h * k}, {rx + bd.roof_w * k, y0 + 1}, C(color));
            }
            if (bd.antenna > 0) dl->AddLine({x0 + bd.w * 0.7f * k, y0}, {x0 + bd.w * 0.7f * k, y0 - bd.antenna * k}, C(color), std::max(1.0f, 2 * k));
            float wake = clamp01((rise_t - delay - 0.5f) / 1.2f);
            for (const Window& w : bd.windows) {
                float show = clamp01(wake * 3 - w.phase / 50);
                if (show <= 0) continue;
                float cycle = fmodf(t * 0.37f + w.phase, 17.0f);
                if ((int)w.phase % 6 == 0 && cycle < 0.35f) continue;
                float wx = x0 + w.x * k, wy = y1 - (w.y + 10) * k * rise;
                if (wy < y0) continue;
                float wl = light_on(wx, wy, wx + 6 * k, wy + 9 * k);
                ImU32 window = mix(window_dim, window_lit, wl * 1.6f);
                dl->AddRectFilled({wx, wy}, {wx + std::max(2.0f, 6 * k), wy + std::max(3.0f, 9 * k)}, C(window, show));
            }
        }
    };
    if (look.glow > 0) soft_glow(dl, light, radius * 1.5f, look.accent, look.glow, look.glow_rings);
    ImU32 back = IM_COL32(31, 31, 38, 255), front = IM_COL32(13, 13, 16, 255);
    layer(city().back, back, mix(back, look.accent, 0.22f), parallax.x * 0.4f, 0);
    float crane_rise = ease_expo((rise_t - 0.55f) / 0.8f);
    if (crane_rise > 0) {
        float cx = ox + city().crane_x * k + parallax.x * 0.4f, top = b.y - city().crane_h * k * crane_rise;
        ImU32 steel = C(IM_COL32(52, 50, 54, 255));
        dl->AddLine({cx, b.y}, {cx, top}, steel, std::max(1.5f, 3 * k));
        dl->AddLine({cx - 70 * k, top + 8 * k}, {cx + 150 * k, top + 8 * k}, steel, std::max(1.0f, 2.5f * k));
        float sway = sinf(t * 0.6f) * 6 * k;
        dl->AddLine({cx + 120 * k, top + 8 * k}, {cx + 120 * k + sway, top + 90 * k}, steel, 1);
        float blink = fmodf(t, 2.4f) < 0.5f ? 1.0f : 0.25f;
        dl->AddCircleFilled({cx + 150 * k, top + 8 * k}, std::max(1.5f, 2.5f * k), C(IM_COL32(230, 70, 50, 255), blink * crane_rise));
    }
    layer(city().front, front, mix(front, look.accent, 0.2f), parallax.x, 0.18f);
    float fog = (b.y - a.y) * look.fog;
    dl->AddRectFilledMultiColor({a.x, b.y - fog}, b, C(look.ground, 0), C(look.ground, 0), C(look.ground, 0.92f), C(look.ground, 0.92f));
}

}
