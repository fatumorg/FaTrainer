#pragma once
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

inline const char* REFILL_KEY = "refill";

namespace config {

struct PageEntry { std::string id; bool visible = true; };

struct Esp {
    bool on = false, box = true, role = true, health_bar = true, health_text = false, distance = true, rank = false, rage = false,
         snaplines = false, allies = true;
    float max_distance = 300;
    float hunter[3] = {0.95f, 0.30f, 0.25f};
    float survivor[3] = {0.30f, 0.75f, 1.00f};
};

struct HudColor {
    bool on = false;
    float color[3] = {1, 1, 1};
    bool hidden = false;
};

enum CrosshairStyle { CROSS, DOT, CIRCLE, CROSS_DOT, CROSSHAIR_STYLES };

struct Crosshair {
    bool on = false, outline = true;
    int style = CROSS;
    float size = 8, thickness = 2, gap = 4, opacity = 1;
    float color[3] = {0.30f, 1.0f, 0.45f};
};

const float RAINBOW_FASTEST = 0.5f, RAINBOW_SLOWEST = 60.0f, RAINBOW_DEFAULT = 6.0f;

struct LegacyHudPart { const char* key; std::vector<const char*> parts; };
inline const LegacyHudPart LEGACY_HUD_PARTS[] = {
    {"health", {"health", "level", "boosters", "medkits"}},
    {"crosshair", {"crosshair", "rope", "throw"}},
    {"weapons", {"weapon", "equipment", "grenade", "reload"}},
    {"xp", {"xp", "driver_xp", "night_reward"}},
    {"messages", {"killfeed", "big_messages"}},
    {"map", {"radar", "objectives", "marker"}},
    {"enemies", {"enemy_hp", "boss_hp"}},
    {"hunter", {"hunter_info", "players", "nests"}},
};

struct UvLight {
    bool on = false;
    float color[3] = {50 / 255.0f, 0, 1};
    float glow = 1;
};

const int SPIT_KEYS = 4;

enum Motion { MOTION_OFF, MOTION_SUBTLE, MOTION_FULL };

struct Look {
    float opacity = 0.97f, roundness = 1.0f;
    int motion = MOTION_FULL;
    bool skyline = true, glow = true;
};

struct Config {
    float accent[3] = {0.910f, 0.592f, 0.227f};
    float dim = 0.6f;
    float scale = 1.0f;
    float window[2] = {};
    int menu_key = VK_INSERT;
    bool remember_cheats = true;
    bool allow_older = false;
    bool beta = false;
    std::vector<std::string> hidden_configs;
    std::vector<PageEntry> pages;
    std::vector<std::string> cheats_on;
    std::vector<std::pair<std::string, float>> tweaks;
    int spit_keys[SPIT_KEYS] = {};
    std::map<std::string, int> cheat_keys;
    Esp esp;
    UvLight uv, hunter_glow;
    HudColor hud_tint;
    std::map<std::string, HudColor> hud_parts;
    std::map<std::string, HudColor> survivor_glow;
    std::map<std::string, float> rainbow;
    Crosshair crosshair;
    std::map<std::string, std::string> texts;
    Look look;
};

inline Config cfg;

inline std::string default_path() {
    char p[MAX_PATH];
    GetModuleFileNameA(nullptr, p, MAX_PATH);
    std::string s = p;
    return s.substr(0, s.find_last_of("\\/") + 1) + "fatrainer.ini";
}

struct Profile {
    std::vector<std::string> cheats;
    std::vector<std::pair<std::string, float>> tweaks;
};

inline std::string profile_dir() {
    std::string ini = default_path();
    return ini.substr(0, ini.find_last_of("\\/") + 1) + "fatrainer_configs";
}

inline std::string profile_name(const std::string& raw) {
    std::string out;
    for (char ch : raw)
        if (isalnum((unsigned char)ch) || ch == ' ' || ch == '-' || ch == '_') out += ch;
    while (!out.empty() && out.back() == ' ') out.pop_back();
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    return out.substr(0, 40);
}

inline std::string profile_path(const std::string& name) { return profile_dir() + "\\" + name + ".ini"; }

inline std::vector<std::string> list_profiles() {
    std::vector<std::string> out;
    WIN32_FIND_DATAA d;
    HANDLE h = FindFirstFileA((profile_dir() + "\\*.ini").c_str(), &d);
    if (h == INVALID_HANDLE_VALUE) return out;
    do {
        std::string n = d.cFileName;
        if (!(d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) out.push_back(n.substr(0, n.size() - 4));
    } while (FindNextFileA(h, &d));
    FindClose(h);
    std::sort(out.begin(), out.end());
    return out;
}

inline bool save_profile(const std::string& name, const Profile& p) {
    if (name.empty()) return false;
    CreateDirectoryA(profile_dir().c_str(), nullptr);
    FILE* f = fopen(profile_path(name).c_str(), "w");
    if (!f) return false;
    for (auto& c : p.cheats) fprintf(f, "cheat=%s\n", c.c_str());
    for (auto& [k, v] : p.tweaks) fprintf(f, "tweak=%s,%.2f\n", k.c_str(), v);
    fclose(f);
    return true;
}

inline bool read_cheat_line(const std::string& k, const std::string& v, Profile& p) {
    if (k == "cheat") p.cheats.push_back(v);
    else if (k == "tweak") {
        size_t c = v.find(',');
        if (c != std::string::npos) p.tweaks.push_back({v.substr(0, c), (float)atof(v.substr(c + 1).c_str())});
    } else return false;
    return true;
}

inline bool load_profile(const std::string& name, Profile& p) {
    FILE* f = fopen(profile_path(name).c_str(), "r");
    if (!f) return false;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        size_t eq = s.find('=');
        if (eq != std::string::npos) read_cheat_line(s.substr(0, eq), s.substr(eq + 1), p);
    }
    fclose(f);
    return true;
}

inline bool delete_profile(const std::string& name) { return DeleteFileA(profile_path(name).c_str()); }

inline bool rename_profile(const std::string& from, const std::string& to) {
    return !to.empty() && MoveFileA(profile_path(from).c_str(), profile_path(to).c_str());
}

inline void normalize(const std::vector<std::string>& known) {
    std::vector<PageEntry> out;
    for (auto& p : cfg.pages)
        if (std::find(known.begin(), known.end(), p.id) != known.end() &&
            std::none_of(out.begin(), out.end(), [&](auto& o) { return o.id == p.id; }))
            out.push_back(p);
    for (size_t k = 0; k < known.size(); k++) {
        auto has = [&](const std::string& id) { return std::find_if(out.begin(), out.end(), [&](auto& o) { return o.id == id; }); };
        if (has(known[k]) != out.end()) continue;
        auto at = out.begin();
        for (size_t j = k; j-- > 0;)
            if (has(known[j]) != out.end()) { at = has(known[j]) + 1; break; }
        out.insert(known[k] == "settings" ? out.end() : at, {known[k], true});
    }
    for (auto& p : out)
        if (p.id == "settings") p.visible = true;
    cfg.pages = out;
    cfg.scale = std::clamp(cfg.scale, 0.75f, 1.5f);
    cfg.dim = std::clamp(cfg.dim, 0.0f, 0.95f);
    cfg.look.opacity = std::clamp(cfg.look.opacity, 0.6f, 1.0f);
    cfg.look.roundness = std::clamp(cfg.look.roundness, 0.0f, 1.6f);
    cfg.look.motion = std::clamp(cfg.look.motion, (int)MOTION_OFF, (int)MOTION_FULL);
}

inline std::string escape_text(const std::string& text) {
    std::string out;
    for (char c : text) {
        if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c != '\r') out += c;
    }
    return out;
}

inline std::string unescape_text(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] != '\\' || i + 1 == text.size()) out += text[i];
        else out += text[++i] == 'n' ? '\n' : text[i];
    }
    return out;
}

inline int read_hud_color(const char* text, HudColor& h) {
    int on = 0, hidden = 0;
    int fields = sscanf(text, "%d,%f,%f,%f,%d", &on, &h.color[0], &h.color[1], &h.color[2], &hidden);
    if (fields < 4) return 0;
    h.on = on != 0, h.hidden = hidden != 0;
    return fields;
}

inline void read_hud_part(const std::string& key, const HudColor& h, int fields) {
    for (auto& legacy : LEGACY_HUD_PARTS)
        if (fields == 4 && key == legacy.key) {
            for (const char* part : legacy.parts) cfg.hud_parts[part] = h;
            return;
        }
    cfg.hud_parts[key] = h;
}

inline float* color_of(const std::string& key) {
    if (key == "accent") return cfg.accent;
    if (key == "esp_hunter") return cfg.esp.hunter;
    if (key == "esp_survivor") return cfg.esp.survivor;
    if (key == "uv_light") return cfg.uv.color;
    if (key == "hunter_glow") return cfg.hunter_glow.color;
    if (key == "hud_tint") return cfg.hud_tint.color;
    if (key == "crosshair") return cfg.crosshair.color;
    auto in = [&](std::map<std::string, HudColor>& m, const char* prefix) -> float* {
        size_t n = strlen(prefix);
        if (key.compare(0, n, prefix)) return nullptr;
        auto it = m.find(key.substr(n));
        return it == m.end() ? nullptr : it->second.color;
    };
    if (float* c = in(cfg.hud_parts, "hud.")) return c;
    return in(cfg.survivor_glow, "glow.");
}

inline void load(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return;
    cfg.pages.clear();
    cfg.cheats_on.clear();
    cfg.tweaks.clear();
    cfg.hidden_configs.clear();
    cfg.cheat_keys.clear();
    cfg.hud_parts.clear();
    cfg.survivor_glow.clear();
    cfg.rainbow.clear();
    cfg.texts.clear();
    char line[4096];
    while (fgets(line, sizeof line, f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        size_t eq = s.find('=');
        if (eq == std::string::npos) continue;
        std::string k = s.substr(0, eq), v = s.substr(eq + 1);
        if (k == "accent") sscanf(v.c_str(), "%f,%f,%f", &cfg.accent[0], &cfg.accent[1], &cfg.accent[2]);
        else if (k == "dim") cfg.dim = (float)atof(v.c_str());
        else if (k == "scale") cfg.scale = (float)atof(v.c_str());
        else if (k == "window") sscanf(v.c_str(), "%f,%f", &cfg.window[0], &cfg.window[1]);
        else if (k == "menu_key") cfg.menu_key = atoi(v.c_str());
        else if (k == "spit_key") {
            int i = -1, vk = 0;
            if (sscanf(v.c_str(), "%d,%d", &i, &vk) == 2 && i >= 0 && i < SPIT_KEYS) cfg.spit_keys[i] = vk;
        }
        else if (k == "cheat_key") {
            size_t c = v.find(',');
            int vk = c == std::string::npos ? 0 : atoi(v.c_str() + c + 1);
            if (vk > 0 && vk < 256) cfg.cheat_keys[v.substr(0, c)] = vk;
        }
        else if (k == "esp") {
            int f[10] = {};
            auto& e = cfg.esp;
            if (sscanf(v.c_str(), "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f", &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6], &f[7], &f[8], &f[9],
                       &e.max_distance) == 11) {
                bool* flags[10] = {&e.on, &e.box, &e.role, &e.health_bar, &e.health_text, &e.distance, &e.rank, &e.rage, &e.snaplines, &e.allies};
                for (int i = 0; i < 10; i++) *flags[i] = f[i] != 0;
            }
        }
        else if (k == "esp_hunter") sscanf(v.c_str(), "%f,%f,%f", &cfg.esp.hunter[0], &cfg.esp.hunter[1], &cfg.esp.hunter[2]);
        else if (k == "esp_survivor") sscanf(v.c_str(), "%f,%f,%f", &cfg.esp.survivor[0], &cfg.esp.survivor[1], &cfg.esp.survivor[2]);
        else if (k == "uv_light" || k == "hunter_glow") {
            int on = 0;
            auto& u = k == "uv_light" ? cfg.uv : cfg.hunter_glow;
            if (sscanf(v.c_str(), "%d,%f,%f,%f,%f", &on, &u.color[0], &u.color[1], &u.color[2], &u.glow) == 5) u.on = on != 0;
        }
        else if (k == "look") {
            int motion = MOTION_FULL, skyline = 1, glow = 1;
            auto& l = cfg.look;
            if (sscanf(v.c_str(), "%f,%f,%d,%d,%d", &l.opacity, &l.roundness, &motion, &skyline, &glow) == 5)
                l.motion = motion, l.skyline = skyline != 0, l.glow = glow != 0;
        }
        else if (k == "remember_cheats") cfg.remember_cheats = v == "1";
        else if (k == "allow_older") cfg.allow_older = v == "1";
        else if (k == "beta") cfg.beta = v == "1";
        else if (k == "hud_tint") read_hud_color(v.c_str(), cfg.hud_tint);
        else if (k == "crosshair") {
            auto& x = cfg.crosshair;
            int on = 0, outline = 1;
            if (sscanf(v.c_str(), "%d,%d,%f,%f,%f,%f,%d,%f,%f,%f", &on, &x.style, &x.size, &x.thickness, &x.gap, &x.opacity, &outline, &x.color[0],
                       &x.color[1], &x.color[2]) == 10)
                x.on = on != 0, x.outline = outline != 0, x.style = std::clamp(x.style, 0, CROSSHAIR_STYLES - 1);
        }
        else if (k == "hud" || k == "text" || k == "survivor_glow" || k == "rainbow") {
            size_t c = v.find(',');
            if (c == std::string::npos || !c) continue;
            std::string key = v.substr(0, c);
            HudColor h;
            if (k == "text") cfg.texts[key] = unescape_text(v.substr(c + 1));
            else if (k == "rainbow") cfg.rainbow[key] = std::clamp((float)atof(v.c_str() + c + 1), RAINBOW_FASTEST, RAINBOW_SLOWEST);
            else if (int fields = read_hud_color(v.c_str() + c + 1, h); fields && k == "hud") read_hud_part(key, h, fields);
            else if (fields) cfg.survivor_glow[key] = h;
        }
        else if (k == "hidden_config") {
            if (std::find(cfg.hidden_configs.begin(), cfg.hidden_configs.end(), v) == cfg.hidden_configs.end()) cfg.hidden_configs.push_back(v);
        }
        else if (Profile p; read_cheat_line(k, v, p)) {
            cfg.cheats_on.insert(cfg.cheats_on.end(), p.cheats.begin(), p.cheats.end());
            cfg.tweaks.insert(cfg.tweaks.end(), p.tweaks.begin(), p.tweaks.end());
        }
        else if (k == "page") {
            size_t c = v.find(',');
            cfg.pages.push_back({v.substr(0, c), c == std::string::npos || v.substr(c + 1) != "0"});
        }
    }
    fclose(f);
}

inline bool save(const std::string& path) {
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return false;
    fprintf(f, "accent=%.3f,%.3f,%.3f\n", cfg.accent[0], cfg.accent[1], cfg.accent[2]);
    fprintf(f, "dim=%.2f\nscale=%.2f\nmenu_key=%d\nremember_cheats=%d\nallow_older=%d\nbeta=%d\n", cfg.dim, cfg.scale, cfg.menu_key,
            (int)cfg.remember_cheats, (int)cfg.allow_older, (int)cfg.beta);
    if (cfg.window[0] > 0 && cfg.window[1] > 0) fprintf(f, "window=%.0f,%.0f\n", cfg.window[0], cfg.window[1]);
    for (auto& h : cfg.hidden_configs) fprintf(f, "hidden_config=%s\n", h.c_str());
    for (auto& p : cfg.pages) fprintf(f, "page=%s,%d\n", p.id.c_str(), (int)p.visible);
    if (cfg.remember_cheats)
        for (auto& c : cfg.cheats_on) fprintf(f, "cheat=%s\n", c.c_str());
    for (auto& [k, v] : cfg.tweaks) fprintf(f, "tweak=%s,%.2f\n", k.c_str(), v);
    for (int i = 0; i < SPIT_KEYS; i++)
        if (cfg.spit_keys[i]) fprintf(f, "spit_key=%d,%d\n", i, cfg.spit_keys[i]);
    for (auto& [cheat, vk] : cfg.cheat_keys) fprintf(f, "cheat_key=%s,%d\n", cheat.c_str(), vk);
    auto& e = cfg.esp;
    fprintf(f, "esp=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.0f\n", e.on, e.box, e.role, e.health_bar, e.health_text, e.distance, e.rank, e.rage,
            e.snaplines, e.allies, e.max_distance);
    fprintf(f, "esp_hunter=%.3f,%.3f,%.3f\nesp_survivor=%.3f,%.3f,%.3f\n", e.hunter[0], e.hunter[1], e.hunter[2], e.survivor[0],
            e.survivor[1], e.survivor[2]);
    for (auto [key, u] : {std::pair{"uv_light", &cfg.uv}, std::pair{"hunter_glow", &cfg.hunter_glow}})
        fprintf(f, "%s=%d,%.3f,%.3f,%.3f,%.2f\n", key, u->on, u->color[0], u->color[1], u->color[2], u->glow);
    fprintf(f, "hud_tint=%d,%.3f,%.3f,%.3f\n", cfg.hud_tint.on, cfg.hud_tint.color[0], cfg.hud_tint.color[1], cfg.hud_tint.color[2]);
    for (auto& [part, h] : cfg.hud_parts) fprintf(f, "hud=%s,%d,%.3f,%.3f,%.3f,%d\n", part.c_str(), h.on, h.color[0], h.color[1], h.color[2], h.hidden);
    for (auto& [variant, h] : cfg.survivor_glow) fprintf(f, "survivor_glow=%s,%d,%.3f,%.3f,%.3f\n", variant.c_str(), h.on, h.color[0], h.color[1], h.color[2]);
    for (auto& [key, seconds] : cfg.rainbow) fprintf(f, "rainbow=%s,%.1f\n", key.c_str(), seconds);
    const auto& x = cfg.crosshair;
    fprintf(f, "crosshair=%d,%d,%.1f,%.1f,%.1f,%.2f,%d,%.3f,%.3f,%.3f\n", x.on, x.style, x.size, x.thickness, x.gap, x.opacity, x.outline, x.color[0], x.color[1],
            x.color[2]);
    for (auto& [key, text] : cfg.texts) fprintf(f, "text=%s,%s\n", key.c_str(), escape_text(text).c_str());
    fprintf(f, "look=%.2f,%.2f,%d,%d,%d\n", cfg.look.opacity, cfg.look.roundness, cfg.look.motion, cfg.look.skyline, cfg.look.glow);
    fclose(f);
    return true;
}

}
