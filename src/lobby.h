#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "announce.h"
#include "cheats.h"
#include "detect.h"

namespace lobby {

struct Player {
    uintptr_t p;
    std::string name;
    bool you, host, hunter;
};

inline std::string role(const Player& pl) {
    std::string side = pl.hunter ? "Night Hunter" : "Survivor";
    return pl.host ? "Host \xC2\xB7 " + side : side;
}

struct Row {
    Player who;
    float health, max_health, distance;
    int rank;
    bool fatrainer;
    announce::Peer peer;
    std::vector<detect::Finding> findings;
};

// The engine's lobby, the object behind IGame::LobbyIsNetworkIdValid (mov rax, [global]): the other clients at +630
// (0x130 bytes each, count +638) with the EOS product user id as text at +4 and the profile name at +50 (engine
// 18023c840, 18023cb10); your own profile name at +3a0.
const int CLIENTS = 0x630, CLIENT_COUNT = 0x638, CLIENT_SIZE = 0x130, CLIENT_USER_ID = 4, CLIENT_NAME = 0x50, OWN_NAME = 0x3a0;
const uint32_t CLIENTS_MAX = 32, NAME_MAX = 64;
const int USER_ID_LENGTH = 32;

inline uintptr_t engine_lobby() {
    static const uintptr_t global = [] {
        auto fn = (uintptr_t)GetProcAddress(GetModuleHandleA("engine_x64_rwdi.dll"), "?LobbyIsNetworkIdValid@IGame@@QEAA_NI@Z");
        bool load = fn && game::rdv<uint8_t>(fn) == 0x48 && game::rdv<uint8_t>(fn + 1) == 0x8B && game::rdv<uint8_t>(fn + 2) == 0x05;
        return load ? game::rip_target(fn, 3, 7) : 0;
    }();
    return global ? game::rdv<uintptr_t>(global) : 0;
}

// A ttl::string_base<wchar_t> (text, size, capacity) as UTF-8.
inline std::string wide_text_at(uintptr_t at) {
    uintptr_t text = game::rdv<uintptr_t>(at);
    uint32_t size = game::rdv<uint32_t>(at + 8);
    if (!text || !size || size > NAME_MAX) return "";
    std::wstring w(size, L'\0');
    return game::rd(text, w.data(), size * sizeof(wchar_t)) ? visuals::utf8(w.c_str()) : "";
}

inline bool same_id(const char* a, const std::string& b) { return b.size() == USER_ID_LENGTH && !_strnicmp(a, b.c_str(), USER_ID_LENGTH); }

inline std::string own_name() {
    uintptr_t l = engine_lobby();
    return l ? wide_text_at(l + OWN_NAME) : "";
}

inline std::string name_of_user(const std::string& id) {
    uintptr_t l = engine_lobby(), list = l ? game::rdv<uintptr_t>(l + CLIENTS) : 0;
    uint32_t n = l ? game::rdv<uint32_t>(l + CLIENT_COUNT) : 0;
    for (uint32_t i = 0; list && i < n && i < CLIENTS_MAX; i++) {
        char got[USER_ID_LENGTH + 1] = {};
        if (game::rd(list + i * CLIENT_SIZE + CLIENT_USER_ID, got, USER_ID_LENGTH) && same_id(got, id)) return wide_text_at(list + i * CLIENT_SIZE + CLIENT_NAME);
    }
    return "";
}

inline std::string member_name(const announce::Member& m) { return m.you ? own_name() : name_of_user(m.id); }

// Which field of a LogicalPlayer names its player, learned from yours: a wide string with your profile name, or your
// EOS user id as text. NO_FIELD after a search without a hit, searched again every RETRY_MS.
const int UNKNOWN_FIELD = -1, NO_FIELD = -2, PLAYER_FIELDS_SPAN = 0x1000;
inline std::atomic<int> name_field{UNKNOWN_FIELD}, id_field{UNKNOWN_FIELD};

inline void learn_name_fields(uintptr_t mine, const std::string& name, const std::string& id) {
    const DWORD RETRY_MS = 10000;
    static DWORD tried_at = 0;
    if (!mine || name_field >= 0 || id_field >= 0 || (name.empty() && id.size() != USER_ID_LENGTH)) return;
    if (name_field == NO_FIELD && GetTickCount() - tried_at < RETRY_MS) return;
    bool first = name_field == UNKNOWN_FIELD;
    tried_at = GetTickCount();
    std::vector<char> bytes(PLAYER_FIELDS_SPAN);
    if (!game::rd(mine, bytes.data(), bytes.size())) return;
    for (int off = 8; off + 16 <= PLAYER_FIELDS_SPAN && !name.empty(); off += 8)
        if (wide_text_at(mine + off) == name) {
            name_field = off;
            cheats::logf_hook("lobby: a LogicalPlayer names its player at +%x", off);
            return;
        }
    for (int off = 0; off + USER_ID_LENGTH <= PLAYER_FIELDS_SPAN && id.size() == USER_ID_LENGTH; off++)
        if (same_id(bytes.data() + off, id)) {
            id_field = off;
            cheats::logf_hook("lobby: a LogicalPlayer carries its EOS user id at +%x", off);
            return;
        }
    name_field = id_field = NO_FIELD;
    if (first) cheats::logf_hook("lobby: no name or EOS user id found in your LogicalPlayer yet");
}

inline std::string body_name(uintptr_t p) {
    uintptr_t lp = cheats::logical_player(p);
    if (!lp) return "";
    if (name_field >= 0) return wide_text_at(lp + name_field);
    char id[USER_ID_LENGTH + 1] = {};
    return id_field >= 0 && game::rd(lp + id_field, id, USER_ID_LENGTH) ? name_of_user(id) : "";
}

// ponytail: EOS members carry no in-game player, so rows get the badge only when every other member runs the same
// FaTrainer; mixed sessions show it on the session card alone. Per-row needs an EOS user to player mapping.
inline bool everyone_runs(const std::vector<announce::Member>& members, announce::Peer& shared) {
    bool any = false;
    for (auto& m : members) {
        if (m.you) continue;
        if (!m.fatrainer || (any && (m.peer.version != shared.version || m.peer.edition != shared.edition))) return false;
        any = true, shared = m.peer;
    }
    return any;
}

inline std::vector<Row> rows() {
    using namespace cheats;
    std::vector<EspTarget> seen;
    {
        std::lock_guard<std::mutex> l(esp_mx);
        seen = seen_players;
    }
    auto members = announce::members();
    std::string my_id;
    for (auto& m : members)
        if (m.you) my_id = m.id;
    learn_name_fields(logical_player(player), own_name(), my_id);
    std::vector<announce::Member> others;
    std::vector<std::string> other_names;
    for (auto& m : members)
        if (!m.you) others.push_back(m), other_names.push_back(member_name(m));
    announce::Peer shared;
    bool everyone = everyone_runs(members, shared);
    std::vector<Row> out;
    int survivors = 1;
    for (auto& t : seen) {
        Row r{};
        r.who.p = t.p, r.who.hunter = t.hunter, r.who.name = body_name(t.p);
        if (r.who.name.empty() && seen.size() == 1 && others.size() == 1) r.who.name = other_names[0];
        r.fatrainer = everyone, r.peer = shared;
        for (size_t i = 0; i < others.size(); i++)
            if (!r.who.name.empty() && other_names[i] == r.who.name) r.who.host = others[i].owner, r.fatrainer = others[i].fatrainer, r.peer = others[i].peer;
        if (r.who.name.empty()) r.who.name = t.hunter ? "Night Hunter" : "Survivor " + std::to_string(survivors++);
        r.health = t.health, r.max_health = t.max_health, r.distance = t.distance, r.rank = t.rank;
        std::lock_guard<std::mutex> l(detect::players_mx);
        if (auto d = detect::players.find(t.p); d != detect::players.end()) r.findings = d->second.findings();
        out.push_back(r);
    }
    return out;
}

}
