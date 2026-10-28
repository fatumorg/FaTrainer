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
    if (name == std::string::npos || name < 5) return false;
    cheats::Vec3 p;
    int end = 0;
    const char* c = line.c_str() + coords + 1;
    if (sscanf(c, "%f,%f,%f %n", &p.x, &p.y, &p.z, &end) != 3 || c[end]) return false;
    for (float v : {p.x, p.y, p.z})
        if (!std::isfinite(v) || std::fabs(v) > FARTHEST) return false;
    level = line.substr(5, name - 5);
    out = {config::profile_name(line.substr(name + 1, coords - name - 1)), p};
    return true;
}

inline std::string spots_path() {
    std::string ini = config::default_path();
    return ini.substr(0, ini.find_last_of("\\/") + 1) + "fatrainer_positions.ini";
}

inline std::string next_name(const std::vector<Spot>& list) {
    for (int n = 1;; n++) {
        std::string name = "Position " + std::to_string(n);
        bool taken = false;
        for (auto& s : list) taken |= s.name == name;
        if (!taken) return name;
    }
}

inline bool load_spots(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::map<std::string, std::vector<Spot>> out;
    std::string line, level;
    Spot s;
    while (std::getline(in, line)) {
        if (!line.compare(0, 3, "\xEF\xBB\xBF")) line.erase(0, 3);
        if (!parse_spot(line, level, s)) continue;
        if (s.name.empty()) s.name = next_name(out[level]);
        out[level].push_back(s);
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

const cheats::Vec3 NORTH{0, 0, 1}, EAST{-1, 0, 0};

inline std::string compass(cheats::Vec3 from, cheats::Vec3 to) {
    static const char* const WORDS[] = {"north", "north-east", "east", "south-east", "south", "south-west", "west", "north-west"};
    const float TURN = 6.2831853f;
    float dx = to.x - from.x, dz = to.z - from.z;
    float north = dx * NORTH.x + dz * NORTH.z, east = dx * EAST.x + dz * EAST.z;
    int part = (int)std::lround(std::atan2(east, north) / TURN * 8);
    return WORDS[(part % 8 + 8) % 8];
}

inline bool sane(cheats::Vec3 p) {
    for (float v : {p.x, p.y, p.z})
        if (!std::isfinite(v) || std::fabs(v) > FARTHEST) return false;
    return p.x != 0 || p.y != 0 || p.z != 0;
}

inline bool waypoint(cheats::Vec3& out) {
    uintptr_t lp = cheats::alive(cheats::player) ? cheats::own_logical_player(cheats::player) : 0;
    bool shown = false;
    if (!lp || !cheats::field_bool || !cheats::field_vec3 || !cheats::reflected(lp)) return false;
    return cheats::field_bool(lp, "m_WaypointVisible", &shown) && shown && cheats::field_vec3(lp, "m_WaypointPos", &out) && sane(out);
}

const size_t GOAL_MANAGER = 0x8e0, TRACKED_GOAL = 0x48, GOAL_QUEST = 0x10, QUEST_NAME = 0x18, LONGEST_QUEST_NAME = 96;

inline std::string tracked_quest_title() {
    uintptr_t game_di = cheats::profile_root ? game::rdv<uintptr_t>(cheats::profile_root) : 0;
    uintptr_t goals = game_di ? game::rdv<uintptr_t>(game_di + GOAL_MANAGER) : 0, goal = goals ? game::rdv<uintptr_t>(goals + TRACKED_GOAL) : 0;
    uintptr_t quest = goal && cheats::vt_quest_goals && game::rdv<uintptr_t>(goal) == cheats::vt_quest_goals ? game::rdv<uintptr_t>(goal + GOAL_QUEST) : 0;
    if (!quest || !cheats::vt_quest || game::rdv<uintptr_t>(quest) != cheats::vt_quest || !visuals::texts_api.find) return "";
    auto name = game::rdv<visuals::TtlString<char>>(quest + QUEST_NAME);
    char id[LONGEST_QUEST_NAME + 1] = {};
    if (!name.text || !name.size || name.size > LONGEST_QUEST_NAME || !game::rd((uintptr_t)name.text, id, name.size)) return "";
    for (char ch : std::string(id))
        if (!isalnum((unsigned char)ch) && ch != '_') return "";
    const wchar_t* text = visuals::shown_text(std::string(id) + "_Name");
    int bytes = text ? WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr) : 0;
    std::string out(bytes > 1 ? bytes - 1 : 0, '\0');
    if (bytes > 1) WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), bytes, nullptr, nullptr);
    for (char& ch : out)
        if ((unsigned char)ch < 32) ch = ' ';
    return out;
}

const size_t MOST_LOCATIONS = 50000;
const int LOCATION_FLAGS = 0x60, LOCATION_TYPE = 0x70, LOCATION_STATE = 0x74, LOCATION_SPAN = 0x78, LOCATION_SHOWN = 1, STATE_TRACKED = 2;
const int TYPE_WAYPOINT = 0x11, TYPE_COOP_WAYPOINT = 0x5a, LOCATION_TYPES = 0x98, SLOT_LOCATION_POSITION = 8;

inline bool tracked_location(cheats::Vec3 me, cheats::Vec3& out) {
    static const std::string FAMILY = "Location@Locations@@";
    auto list = cheats::locations_root ? game::rdv<cheats::EngineVector>(cheats::locations_root) : cheats::EngineVector{};
    if (!std::isfinite(me.x) || !list.data || !list.size || list.size > MOST_LOCATIONS) return false;
    std::vector<uintptr_t> all(list.size);
    if (!game::rd((uintptr_t)list.data, all.data(), all.size() * sizeof(uintptr_t))) return false;
    float best = INFINITY;
    for (uintptr_t loc : all) {
        uint8_t head[LOCATION_SPAN];
        int type, state;
        if (!game::rd(loc, head, sizeof head)) continue;
        memcpy(&type, head + LOCATION_TYPE, sizeof type);
        memcpy(&state, head + LOCATION_STATE, sizeof state);
        if (!(head[LOCATION_FLAGS] & LOCATION_SHOWN) || state != STATE_TRACKED || type < 0 || type >= LOCATION_TYPES || type == TYPE_WAYPOINT || type == TYPE_COOP_WAYPOINT) continue;
        std::string cls = game::class_name(loc);
        uintptr_t get = cheats::slot(loc, SLOT_LOCATION_POSITION);
        if (cls.size() <= FAMILY.size() || cls.compare(cls.size() - FAMILY.size(), FAMILY.size(), FAMILY) || !cheats::in_game_module(get)) continue;
        cheats::Vec3 at{NAN, NAN, NAN};
        ((cheats::Vec3 * (*)(uintptr_t, cheats::Vec3*)) get)(loc, &at);
        float d = sane(at) ? cheats::distance(me, at) : INFINITY;
        if (d < best) best = d, out = at;
    }
    return std::isfinite(best);
}

inline bool objective(cheats::Vec3& out, std::string& title) {
    if (!tracked_location(cheats::player_position(), out)) return false;
    title = tracked_quest_title();
    return true;
}

const float LAND_ABOVE = 0.5f, SKY = 500, SKY_DEPTH = 2000, NEAR_ABOVE = 2, NEAR_DEPTH = 50;

inline bool land(cheats::Vec3& at, bool height_known) {
    float ground, terrain;
    cheats::Vec3 from = at;
    from.y = height_known ? at.y + NEAR_ABOVE : std::max(at.y, cheats::player_position().y) + SKY;
    bool hit = cheats::ground_below(from, height_known ? NEAR_DEPTH : SKY_DEPTH, &ground), soil = cheats::terrain_below(at, &terrain);
    if (hit) at.y = ground;
    else if (height_known) at.y = soil ? std::max(at.y, terrain) : at.y;
    else if (soil) at.y = terrain;
    else return false;
    at.y += LAND_ABOVE;
    return true;
}

inline std::string distance_text(cheats::Vec3 from, cheats::Vec3 to) {
    float d = cheats::distance(from, to);
    char b[32];
    if (d < 999.5f) snprintf(b, sizeof b, "%.0f m", d);
    else snprintf(b, sizeof b, "%.1f km", d / 1000);
    return b;
}

}
