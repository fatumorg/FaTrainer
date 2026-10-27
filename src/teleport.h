#pragma once
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include "cheats.h"
#include "config.h"

namespace teleport {

struct Spot { std::string name; cheats::Vec3 pos; };

inline std::map<std::string, std::vector<Spot>> spots;

const float FARTHEST = 100000;

inline std::string format_spot(const std::string& level, const Spot& s) {
    char b[128];
    snprintf(b, sizeof b, "|%.2f,%.2f,%.2f", s.pos.x, s.pos.y, s.pos.z);
    return "spot=" + level + "|" + config::profile_name(s.name) + b;
}

inline bool parse_spot(const std::string& line, std::string& level, Spot& out) {
    if (line.compare(0, 5, "spot=")) return false;
    size_t coords = line.rfind('|'), name = coords == std::string::npos || coords < 6 ? std::string::npos : line.rfind('|', coords - 1);
    if (name == std::string::npos || name < 5 || coords - name < 2) return false;
    cheats::Vec3 p;
    int end = 0;
    const char* c = line.c_str() + coords + 1;
    if (sscanf(c, "%f,%f,%f%n", &p.x, &p.y, &p.z, &end) != 3 || c[end]) return false;
    for (float v : {p.x, p.y, p.z})
        if (!std::isfinite(v) || std::fabs(v) > FARTHEST) return false;
    level = line.substr(5, name - 5);
    out = {line.substr(name + 1, coords - name - 1), p};
    return true;
}

inline std::string spots_path() {
    std::string ini = config::default_path();
    return ini.substr(0, ini.find_last_of("\\/") + 1) + "fatrainer_positions.ini";
}

inline bool load_spots(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::map<std::string, std::vector<Spot>> out;
    std::string line, level;
    Spot s;
    while (std::getline(in, line)) {
        while (!line.empty() && line.back() == '\r') line.pop_back();
        if (parse_spot(line, level, s)) out[level].push_back(s);
    }
    spots = out;
    return true;
}

inline bool save_spots(const std::string& path) {
    std::string tmp = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::trunc);
        for (auto& [level, list] : spots)
            for (auto& s : list) f << format_spot(level, s) << "\n";
        if (!f.flush()) return false;
    }
    return MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
}

inline std::string level_name() {
    static const HMODULE engine = GetModuleHandleA("engine_x64_rwdi.dll");
    static const uintptr_t getter = engine ? (uintptr_t)GetProcAddress(engine, "?GetLevelName@ILevel@@QEAAPEBDXZ") : 0;
    static const uint8_t START[] = {0x48, 0x8B, 0x41, 0x08, 0x48, 0x05};
    uint8_t code[11];
    uintptr_t level = getter ? cheats::current_level() : 0;
    if (!level || !game::rd(getter, code, sizeof code) || memcmp(code, START, sizeof START) || code[10] != 0xC3) return "";
    int32_t at;
    memcpy(&at, code + 6, sizeof at);
    uintptr_t impl = game::rdv<uintptr_t>(level + code[3]);
    char name[128] = {};
    if (!impl || !game::rd(impl + at, name, sizeof name - 1)) return "";
    std::string s = name;
    for (char ch : s)
        if ((unsigned char)ch < 32 || ch == '|') return "";
    return s;
}

inline std::string next_name(const std::vector<Spot>& list) {
    for (int n = 1;; n++) {
        std::string name = "Position " + std::to_string(n);
        bool taken = false;
        for (auto& s : list) taken |= s.name == name;
        if (!taken) return name;
    }
}

inline std::string distance_text(cheats::Vec3 from, cheats::Vec3 to) {
    float d = cheats::distance(from, to);
    char b[32];
    if (d < 999.5f) snprintf(b, sizeof b, "%.0f m", d);
    else snprintf(b, sizeof b, "%.1f km", d / 1000);
    return b;
}

}
