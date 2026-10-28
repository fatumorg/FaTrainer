#pragma once
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
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
const float FOV_MIN = 0.6f, FOV_MAX = 2.0f;
inline const char STEAM_LANGUAGE[] = "steam";

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
enum Density { DENSITY_COMFORTABLE, DENSITY_COMPACT };
enum MenuFont { FONT_DEFAULT, FONT_WIDE, FONT_MONO, MENU_FONTS };
enum CardStyle { CARD_FILLED, CARD_OUTLINED, CARD_FLAT };
enum OpenAt { OPEN_CENTER, OPEN_REMEMBER, OPEN_LEFT, OPEN_RIGHT };
enum MenuStyle { STYLE_CLASSIC, STYLE_RAIL, STYLE_TABS, STYLE_DOCK };
const float TEXT_SIZE_MIN = 0.8f, TEXT_SIZE_MAX = 1.3f, SIDEBAR_MIN = 190, SIDEBAR_MAX = 320;

struct Look {
    float opacity = 0.97f, roundness = 1.0f;
    int motion = MOTION_FULL;
    bool skyline = true, glow = true;
    int density = DENSITY_COMFORTABLE, font = FONT_DEFAULT;
    float text_size = 1.0f;
    int card_style = CARD_FILLED;
    bool checkboxes = false, descriptions = true;
    int open_at = OPEN_REMEMBER;
    float sidebar_width = 236;
    int style = STYLE_CLASSIC;
};

struct Config {
    float accent[3] = {0.910f, 0.592f, 0.227f};
    float dim = 0.6f;
    float scale = 1.0f;
    float window[2] = {};
    float window_at[2] = {0.5f, 0.5f};
    int menu_key = VK_INSERT;
    bool remember_cheats = true;
    bool allow_older = false;
    bool beta = false;
    bool all_resolutions = false;
    bool cheat_engine = false;
    std::vector<std::string> hidden_configs;
    std::vector<PageEntry> pages;
    std::vector<std::string> cheats_on;
    std::vector<std::pair<std::string, float>> tweaks;
    int spit_keys[SPIT_KEYS] = {};
    std::map<std::string, int> cheat_keys;
    std::map<std::string, std::pair<float, float>> key_levels;
    std::map<std::string, int> config_keys;
    Esp esp;
    UvLight uv, hunter_glow;
    HudColor hud_tint;
    std::map<std::string, HudColor> hud_parts;
    std::map<std::string, HudColor> survivor_glow, btz_colors;
    std::map<std::string, float> rainbow;
    Crosshair crosshair;
    std::map<std::string, std::string> texts;
    std::map<std::string, std::map<std::string, float>> weapon_edits;
    std::string game_language, voice_language, subtitle_language;
    std::map<std::string, std::string> kind_languages;
    float fov = 1.0f;
    Look look;
};

inline Config cfg;

inline bool read_float(const char* text, float& out) {
    char* end = nullptr;
    float f = strtof(text, &end);
    if (end == text || !std::isfinite(f)) return false;
    out = f;
    return true;
}

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

inline std::vector<std::string> ini_names(const std::string& dir) {
    std::vector<std::string> out;
    WIN32_FIND_DATAA d;
    HANDLE h = FindFirstFileA((dir + "\\*.ini").c_str(), &d);
    if (h == INVALID_HANDLE_VALUE) return out;
    do {
        std::string n = d.cFileName;
        if (!(d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) out.push_back(n.substr(0, n.size() - 4));
    } while (FindNextFileA(h, &d));
    FindClose(h);
    std::sort(out.begin(), out.end());
    return out;
}

inline bool path_exists(const std::string& path) { return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES; }

inline std::vector<std::string> list_profiles() { return ini_names(profile_dir()); }

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

inline std::string callout_dir() {
    std::string ini = default_path();
    return ini.substr(0, ini.find_last_of("\\/") + 1) + "fatrainer_callouts";
}

inline std::string callout_path(const std::string& name) { return callout_dir() + "\\" + name + ".ini"; }

inline std::vector<std::string> list_callout_sets() { return ini_names(callout_dir()); }

inline bool delete_callout_set(const std::string& name) { return DeleteFileA(callout_path(name).c_str()); }

inline bool rename_callout_set(const std::string& from, const std::string& to) {
    return !to.empty() && MoveFileA(callout_path(from).c_str(), callout_path(to).c_str());
}

inline std::string migrate_page_id(const std::string& id) {
    return id == "pvp" ? "survivor" : id == "prison" ? "dlc" : id;
}

inline void normalize(const std::vector<std::string>& known) {
    std::vector<PageEntry> out;
    for (auto& p : cfg.pages) {
        p.id = migrate_page_id(p.id);
        if (std::find(known.begin(), known.end(), p.id) != known.end() &&
            std::none_of(out.begin(), out.end(), [&](auto& o) { return o.id == p.id; }))
            out.push_back(p);
    }
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

inline std::string game_colors(std::string text) {
    for (size_t at; (at = text.find("<font")) != std::string::npos;) {
        size_t end = text.find('>', at), hash = text.find('#', at);
        std::string tone = hash != std::string::npos && hash < end ? text.substr(hash + 1, 6) : "";
        bool hex = tone.size() == 6 && tone.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
        text.replace(at, end == std::string::npos ? std::string::npos : end - at + 1, hex ? "%COLOR(" + tone + ")" : "");
    }
    for (size_t at; (at = text.find("</font>")) != std::string::npos;) text.replace(at, 7, "%COLOR(reset)");
    return text;
}

inline std::string unescape_text(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] != '\\' || i + 1 == text.size()) out += text[i];
        else out += text[++i] == 'n' ? '\n' : text[i];
    }
    return out;
}

using CalloutTexts = std::map<std::string, std::string>;

inline bool save_callout_set(const std::string& name, const CalloutTexts& texts) {
    if (name.empty()) return false;
    CreateDirectoryA(callout_dir().c_str(), nullptr);
    FILE* f = fopen(callout_path(name).c_str(), "w");
    if (!f) return false;
    for (auto& [key, text] : texts) fprintf(f, "text=%s,%s\n", key.c_str(), escape_text(text).c_str());
    fclose(f);
    return true;
}

inline bool load_callout_set(const std::string& name, CalloutTexts& texts) {
    FILE* f = fopen(callout_path(name).c_str(), "r");
    if (!f) return false;
    char line[2048];
    while (fgets(line, sizeof line, f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        size_t comma = s.find(',');
        if (s.compare(0, 5, "text=") || comma == std::string::npos || comma == 5) continue;
        texts[s.substr(5, comma - 5)] = unescape_text(s.substr(comma + 1));
    }
    fclose(f);
    return true;
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

inline bool read_visual_line(const std::string& k, const std::string& v) {
    if (k == "esp") {
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
    else if (k == "all_resolutions") cfg.all_resolutions = v == "1";
    else if (k == "hud_tint") read_hud_color(v.c_str(), cfg.hud_tint);
    else if (k == "fov") {
        if (read_float(v.c_str(), cfg.fov)) cfg.fov = std::clamp(cfg.fov, FOV_MIN, FOV_MAX);
    }
    else if (k == "crosshair") {
        auto& x = cfg.crosshair;
        int on = 0, outline = 1;
        if (sscanf(v.c_str(), "%d,%d,%f,%f,%f,%f,%d,%f,%f,%f", &on, &x.style, &x.size, &x.thickness, &x.gap, &x.opacity, &outline, &x.color[0],
                   &x.color[1], &x.color[2]) == 10)
            x.on = on != 0, x.outline = outline != 0, x.style = std::clamp(x.style, 0, CROSSHAIR_STYLES - 1);
    }
    else if (k == "hud" || k == "survivor_glow" || k == "btz_color" || k == "rainbow") {
        size_t c = v.find(',');
        if (c == std::string::npos || !c) return true;
        std::string key = v.substr(0, c);
        HudColor h;
        if (k == "rainbow") cfg.rainbow[key] = std::clamp((float)atof(v.c_str() + c + 1), RAINBOW_FASTEST, RAINBOW_SLOWEST);
        else if (int fields = read_hud_color(v.c_str() + c + 1, h); fields && k == "hud") read_hud_part(key, h, fields);
        else if (fields) (k == "btz_color" ? cfg.btz_colors : cfg.survivor_glow)[key] = h;
    }
    else return false;
    return true;
}

inline std::string visual_lines() {
    std::string out;
    auto add = [&](const char* format, auto... args) {
        int n = snprintf(nullptr, 0, format, args...);
        std::string line(n + 1, '\0');
        snprintf(line.data(), n + 1, format, args...);
        line.pop_back();
        out += line;
    };
    auto& e = cfg.esp;
    add("esp=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.0f\n", e.on, e.box, e.role, e.health_bar, e.health_text, e.distance, e.rank, e.rage, e.snaplines,
        e.allies, e.max_distance);
    add("esp_hunter=%.3f,%.3f,%.3f\nesp_survivor=%.3f,%.3f,%.3f\n", e.hunter[0], e.hunter[1], e.hunter[2], e.survivor[0], e.survivor[1], e.survivor[2]);
    for (auto [key, u] : {std::pair{"uv_light", &cfg.uv}, std::pair{"hunter_glow", &cfg.hunter_glow}})
        add("%s=%d,%.3f,%.3f,%.3f,%.2f\n", key, u->on, u->color[0], u->color[1], u->color[2], u->glow);
    add("hud_tint=%d,%.3f,%.3f,%.3f\n", cfg.hud_tint.on, cfg.hud_tint.color[0], cfg.hud_tint.color[1], cfg.hud_tint.color[2]);
    for (auto& [part, h] : cfg.hud_parts) add("hud=%s,%d,%.3f,%.3f,%.3f,%d\n", part.c_str(), h.on, h.color[0], h.color[1], h.color[2], h.hidden);
    for (auto& [variant, h] : cfg.survivor_glow) add("survivor_glow=%s,%d,%.3f,%.3f,%.3f\n", variant.c_str(), h.on, h.color[0], h.color[1], h.color[2]);
    for (auto& [part, h] : cfg.btz_colors) add("btz_color=%s,%d,%.3f,%.3f,%.3f\n", part.c_str(), h.on, h.color[0], h.color[1], h.color[2]);
    for (auto& [key, seconds] : cfg.rainbow)
        if (key != "accent") add("rainbow=%s,%.1f\n", key.c_str(), seconds);
    const auto& x = cfg.crosshair;
    add("crosshair=%d,%d,%.1f,%.1f,%.1f,%.2f,%d,%.3f,%.3f,%.3f\n", x.on, x.style, x.size, x.thickness, x.gap, x.opacity, x.outline, x.color[0], x.color[1],
        x.color[2]);
    if (cfg.fov != 1.0f) add("fov=%.2f\n", cfg.fov);
    add("all_resolutions=%d\n", (int)cfg.all_resolutions);
    return out;
}

inline void reset_visuals() {
    Config fresh;
    cfg.esp = fresh.esp, cfg.uv = fresh.uv, cfg.hunter_glow = fresh.hunter_glow, cfg.hud_tint = fresh.hud_tint, cfg.crosshair = fresh.crosshair;
    cfg.fov = fresh.fov, cfg.all_resolutions = fresh.all_resolutions;
    cfg.hud_parts.clear(), cfg.survivor_glow.clear(), cfg.btz_colors.clear();
    for (auto it = cfg.rainbow.begin(); it != cfg.rainbow.end();) it = it->first == "accent" ? std::next(it) : cfg.rainbow.erase(it);
}

inline std::string visual_config_dir() {
    std::string ini = default_path();
    return ini.substr(0, ini.find_last_of("\\/") + 1) + "fatrainer_visual_configs";
}

inline std::string visual_config_path(const std::string& name) {
    std::string clean = profile_name(name);
    return clean.empty() ? "" : visual_config_dir() + "\\" + clean + ".ini";
}

inline std::vector<std::string> list_visual_configs() {
    std::vector<std::string> out;
    for (auto& n : ini_names(visual_config_dir()))
        if (profile_name(n) == n) out.push_back(n);
    return out;
}

inline bool save_visual_config(const std::string& name) {
    std::string path = visual_config_path(name), tmp = path + ".tmp", text = visual_lines();
    if (path.empty()) return false;
    CreateDirectoryA(visual_config_dir().c_str(), nullptr);
    FILE* f = fopen(tmp.c_str(), "w");
    if (!f) return false;
    bool written = fputs(text.c_str(), f) >= 0;
    if (fclose(f) != 0 || !written || !MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) return DeleteFileA(tmp.c_str()), false;
    return true;
}

inline void apply_visual_lines(const std::string& text) {
    reset_visuals();
    std::istringstream in(text);
    for (std::string s; std::getline(in, s);) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        size_t eq = s.find('=');
        if (eq == std::string::npos || s.compare(0, 15, "rainbow=accent,") == 0) continue;
        read_visual_line(s.substr(0, eq), s.substr(eq + 1));
    }
}

inline bool load_visual_config(const std::string& name) {
    std::string path = visual_config_path(name);
    std::ifstream in(path);
    if (!in) return false;
    std::ostringstream text;
    text << in.rdbuf();
    apply_visual_lines(text.str());
    return true;
}

inline std::string visual_key_id, visual_key_before;

inline bool press_visual_key(const std::string& id) {
    std::string name = id.compare(0, 7, "visual:") ? "" : id.substr(7);
    if (!path_exists(visual_config_path(name))) return false;
    if (visual_key_id == id) return apply_visual_lines(visual_key_before), visual_key_id.clear(), true;
    std::string before = visual_key_id.empty() ? visual_lines() : visual_key_before;
    if (!load_visual_config(name)) return false;
    visual_key_before = before, visual_key_id = id;
    return true;
}

inline bool rename_visual_config(const std::string& from, const std::string& to) {
    std::string a = visual_config_path(from), b = visual_config_path(to);
    return !a.empty() && !b.empty() && MoveFileA(a.c_str(), b.c_str());
}

inline bool delete_visual_config(const std::string& name) {
    std::string path = visual_config_path(name);
    return !path.empty() && DeleteFileA(path.c_str());
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
    if (float* c = in(cfg.btz_colors, "btz.")) return c;
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
    cfg.key_levels.clear();
    cfg.config_keys.clear();
    cfg.hud_parts.clear();
    cfg.survivor_glow.clear();
    cfg.btz_colors.clear();
    cfg.rainbow.clear();
    cfg.texts.clear();
    cfg.game_language.clear(), cfg.voice_language.clear(), cfg.subtitle_language.clear(), cfg.kind_languages.clear();
    cfg.fov = 1.0f;
    std::string old_language;
    bool old_spoken_kept = false;
    char line[4096];
    while (fgets(line, sizeof line, f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        size_t eq = s.find('=');
        if (eq == std::string::npos) continue;
        std::string k = s.substr(0, eq), v = s.substr(eq + 1);
        if (read_visual_line(k, v)) continue;
        if (k == "accent") sscanf(v.c_str(), "%f,%f,%f", &cfg.accent[0], &cfg.accent[1], &cfg.accent[2]);
        else if (k == "dim") read_float(v.c_str(), cfg.dim);
        else if (k == "scale") read_float(v.c_str(), cfg.scale);
        else if (k == "window") sscanf(v.c_str(), "%f,%f", &cfg.window[0], &cfg.window[1]);
        else if (k == "window_at") {
            float x, y;
            if (sscanf(v.c_str(), "%f,%f", &x, &y) == 2 && std::isfinite(x) && std::isfinite(y))
                cfg.window_at[0] = std::clamp(x, 0.0f, 1.0f), cfg.window_at[1] = std::clamp(y, 0.0f, 1.0f);
        }
        else if (k == "menu_key") cfg.menu_key = atoi(v.c_str());
        else if (k == "spit_key") {
            int i = -1, vk = 0;
            if (sscanf(v.c_str(), "%d,%d", &i, &vk) == 2 && i >= 0 && i < SPIT_KEYS) cfg.spit_keys[i] = vk;
        }
        else if (k == "cheat_key") {
            size_t c = v.find(',');
            int vk = c == std::string::npos ? 0 : atoi(v.c_str() + c + 1);
            std::string key = v.substr(0, c);
            if (key == "go_saved_position") key = "go_last_position";
            if (vk > 0 && vk < 256) cfg.cheat_keys[key] = vk;
        }
        else if (k == "key_levels") {
            size_t c = v.find(',');
            float on = 0, off = 0;
            if (c != std::string::npos && c && sscanf(v.c_str() + c + 1, "%f,%f", &on, &off) == 2 && std::isfinite(on) && std::isfinite(off))
                cfg.key_levels[v.substr(0, c)] = {on, off};
        }
        else if (k == "config_key") {
            size_t c = v.rfind(',');
            int vk = c == std::string::npos ? 0 : atoi(v.c_str() + c + 1);
            if (vk > 0 && vk < 256 && c > 0) cfg.config_keys[v.substr(0, c)] = vk;
        }
        else if (k == "look") {
            float opacity, roundness;
            int motion, skyline, glow;
            auto& l = cfg.look;
            if (sscanf(v.c_str(), "%f,%f,%d,%d,%d", &opacity, &roundness, &motion, &skyline, &glow) == 5 && std::isfinite(opacity) && std::isfinite(roundness))
                l.opacity = opacity, l.roundness = roundness, l.motion = motion, l.skyline = skyline != 0, l.glow = glow != 0;
        }
        else if (k == "menu") {
            int density, font, card, checkboxes, descriptions, open_at, style;
            float text_size, sidebar;
            auto& l = cfg.look;
            if (sscanf(v.c_str(), "%d,%d,%f,%d,%d,%d,%d,%f,%d", &density, &font, &text_size, &card, &checkboxes, &descriptions, &open_at, &sidebar, &style) == 9 &&
                std::isfinite(text_size) && std::isfinite(sidebar)) {
                l.density = std::clamp(density, (int)DENSITY_COMFORTABLE, (int)DENSITY_COMPACT), l.font = std::clamp(font, 0, MENU_FONTS - 1);
                l.text_size = std::clamp(text_size, TEXT_SIZE_MIN, TEXT_SIZE_MAX), l.card_style = std::clamp(card, (int)CARD_FILLED, (int)CARD_FLAT);
                l.checkboxes = checkboxes != 0, l.descriptions = descriptions != 0, l.open_at = std::clamp(open_at, (int)OPEN_CENTER, (int)OPEN_RIGHT);
                l.sidebar_width = std::clamp(sidebar, SIDEBAR_MIN, SIDEBAR_MAX), l.style = std::clamp(style, (int)STYLE_CLASSIC, (int)STYLE_DOCK);
            }
        }
        else if (k == "remember_cheats") cfg.remember_cheats = v == "1";
        else if (k == "allow_older") cfg.allow_older = v == "1";
        else if (k == "beta") cfg.beta = v == "1";
        else if (k == "cheat_engine") cfg.cheat_engine = v == "1";
        else if (k == "text_language") {
            size_t c = v.find(',');
            if (c == std::string::npos) old_language = v;
            else if (c + 1 < v.size()) cfg.kind_languages[v.substr(0, c)] = v.substr(c + 1);
        }
        else if (k == "text_keep") old_spoken_kept = ("," + v + ",").find(",spoken,") != std::string::npos;
        else if (k == "game_language") cfg.game_language = v;
        else if (k == "voice_language") cfg.voice_language = v;
        else if (k == "subtitle_language") cfg.subtitle_language = v;
        else if (k == "weapon_edit") {
            size_t a = v.find(','), b = a == std::string::npos ? a : v.find(',', a + 1);
            if (b != std::string::npos) cfg.weapon_edits[v.substr(0, a)][v.substr(a + 1, b - a - 1)] = (float)atof(v.c_str() + b + 1);
        }
        else if (k == "text") {
            size_t c = v.find(',');
            if (c != std::string::npos && c) cfg.texts[v.substr(0, c)] = game_colors(unescape_text(v.substr(c + 1)));
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
            cfg.pages.push_back({migrate_page_id(v.substr(0, c)), c == std::string::npos || v.substr(c + 1) != "0"});
        }
    }
    fclose(f);
    if (!old_language.empty() && cfg.game_language.empty()) {
        cfg.game_language = old_language;
        if (old_spoken_kept) cfg.subtitle_language = STEAM_LANGUAGE;
    }
}

inline std::string peek(const std::string& path, const std::string& key) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return "";
    char line[512];
    std::string found;
    while (fgets(line, sizeof line, f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        if (s.size() > key.size() && !s.compare(0, key.size(), key) && s[key.size()] == '=') found = s.substr(key.size() + 1);
    }
    fclose(f);
    return found;
}

inline bool save(const std::string& path) {
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return false;
    fprintf(f, "accent=%.3f,%.3f,%.3f\n", cfg.accent[0], cfg.accent[1], cfg.accent[2]);
    fprintf(f, "dim=%.2f\nscale=%.2f\nmenu_key=%d\nremember_cheats=%d\nallow_older=%d\nbeta=%d\n", cfg.dim, cfg.scale, cfg.menu_key,
            (int)cfg.remember_cheats, (int)cfg.allow_older, (int)cfg.beta);
    fprintf(f, "cheat_engine=%d\n", (int)cfg.cheat_engine);
    if (cfg.window[0] > 0 && cfg.window[1] > 0) fprintf(f, "window=%.0f,%.0f\n", cfg.window[0], cfg.window[1]);
    fprintf(f, "window_at=%.3f,%.3f\n", cfg.window_at[0], cfg.window_at[1]);
    for (auto& h : cfg.hidden_configs) fprintf(f, "hidden_config=%s\n", h.c_str());
    for (auto& p : cfg.pages) fprintf(f, "page=%s,%d\n", p.id.c_str(), (int)p.visible);
    if (cfg.remember_cheats)
        for (auto& c : cfg.cheats_on) fprintf(f, "cheat=%s\n", c.c_str());
    for (auto& [k, v] : cfg.tweaks) fprintf(f, "tweak=%s,%.2f\n", k.c_str(), v);
    for (int i = 0; i < SPIT_KEYS; i++)
        if (cfg.spit_keys[i]) fprintf(f, "spit_key=%d,%d\n", i, cfg.spit_keys[i]);
    for (auto& [cheat, vk] : cfg.cheat_keys) fprintf(f, "cheat_key=%s,%d\n", cheat.c_str(), vk);
    for (auto& [key, lv] : cfg.key_levels) fprintf(f, "key_levels=%s,%.2f,%.2f\n", key.c_str(), lv.first, lv.second);
    for (auto& [id, vk] : cfg.config_keys) fprintf(f, "config_key=%s,%d\n", id.c_str(), vk);
    fputs(visual_lines().c_str(), f);
    if (auto accent = cfg.rainbow.find("accent"); accent != cfg.rainbow.end()) fprintf(f, "rainbow=accent,%.1f\n", accent->second);
    for (auto& [key, text] : cfg.texts) fprintf(f, "text=%s,%s\n", key.c_str(), escape_text(text).c_str());
    if (!cfg.game_language.empty()) fprintf(f, "game_language=%s\n", cfg.game_language.c_str());
    if (!cfg.voice_language.empty()) fprintf(f, "voice_language=%s\n", cfg.voice_language.c_str());
    if (!cfg.subtitle_language.empty()) fprintf(f, "subtitle_language=%s\n", cfg.subtitle_language.c_str());
    for (auto& [kind, code] : cfg.kind_languages) fprintf(f, "text_language=%s,%s\n", kind.c_str(), code.c_str());
    for (auto& [id, stats] : cfg.weapon_edits)
        for (auto& [stat, value] : stats) fprintf(f, "weapon_edit=%s,%s,%g\n", id.c_str(), stat.c_str(), value);
    const auto& l = cfg.look;
    fprintf(f, "look=%.2f,%.2f,%d,%d,%d\n", l.opacity, l.roundness, l.motion, l.skyline, l.glow);
    fprintf(f, "menu=%d,%d,%.2f,%d,%d,%d,%d,%.0f,%d\n", l.density, l.font, l.text_size, l.card_style, l.checkboxes, l.descriptions, l.open_at, l.sidebar_width,
            l.style);
    fclose(f);
    return true;
}

}
