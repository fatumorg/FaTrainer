#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <map>
#include <mutex>
#include <string>

namespace announce {

// What a FaTrainer game tells the others in the match: "FTR1|<version>|<edition>". Anything else is dropped.
struct Peer { std::string version, edition; };

const char* const PREFIX = "FTR1|";
const size_t WIRE_MAX = 48, VERSION_MAX = 16;

inline std::string encode(const char* version, const char* edition) { return std::string(PREFIX) + version + "|" + edition; }

inline bool decode(const std::string& wire, Peer& out) {
    size_t prefix = strlen(PREFIX);
    if (wire.size() > WIRE_MAX || wire.compare(0, prefix, PREFIX)) return false;
    size_t bar = wire.find('|', prefix);
    if (bar == std::string::npos) return false;
    std::string version = wire.substr(prefix, bar - prefix), edition = wire.substr(bar + 1);
    if (version.empty() || version.size() > VERSION_MAX) return false;
    for (char c : version)
        if (!(c >= '0' && c <= '9') && c != '.' && c != 'b' && c != ' ') return false;
    if (edition != "Fatum" && edition != "Visuals") return false;
    out = {version, edition};
    return true;
}

inline std::string badge(const Peer& p) { return "FaTrainer " + p.version + " \xC2\xB7 " + p.edition + " Version"; }

// The channel: Steam lobby member data under KEY. Every player in the match's Steam lobby can read it, the stock
// game never looks at the key, and it changes nothing in the match.
const char* const KEY = "fatrainer";
const DWORD RESEND_MS = 30000, TICK_MS = 2000;
struct GameInfo { uint64_t game; uint32_t ip; uint16_t port, query_port; uint64_t lobby; };
struct Member { uint64_t steam_id; std::string name; bool owner, you, fatrainer; Peer peer; };

struct Api {
    void* friends = nullptr;
    void* matchmaking = nullptr;
    void* user = nullptr;
    uint64_t (*steam_id)(void*) = nullptr;
    bool (*game_played)(void*, uint64_t, GameInfo*) = nullptr;
    const char* (*persona)(void*, uint64_t) = nullptr;
    void (*overlay_to_user)(void*, const char*, uint64_t) = nullptr;
    int (*members)(void*, uint64_t) = nullptr;
    uint64_t (*member)(void*, uint64_t, int) = nullptr;
    uint64_t (*owner)(void*, uint64_t) = nullptr;
    const char* (*member_data)(void*, uint64_t, uint64_t, const char*) = nullptr;
    void (*set_member_data)(void*, uint64_t, const char*, const char*) = nullptr;
};
inline Api api;

inline bool install() {
    HMODULE steam = GetModuleHandleA("steam_api64.dll");
    if (!steam) return false;
    auto get = [&](const char* name) { return (void*)GetProcAddress(steam, name); };
    auto accessor = [&](const char* name) { auto f = (void* (*)())get(name); return f ? f() : nullptr; };
    api.friends = accessor("SteamAPI_SteamFriends_v017"), api.matchmaking = accessor("SteamAPI_SteamMatchmaking_v009"), api.user = accessor("SteamAPI_SteamUser_v023");
    api.steam_id = (decltype(api.steam_id))get("SteamAPI_ISteamUser_GetSteamID");
    api.game_played = (decltype(api.game_played))get("SteamAPI_ISteamFriends_GetFriendGamePlayed");
    api.persona = (decltype(api.persona))get("SteamAPI_ISteamFriends_GetFriendPersonaName");
    api.overlay_to_user = (decltype(api.overlay_to_user))get("SteamAPI_ISteamFriends_ActivateGameOverlayToUser");
    api.members = (decltype(api.members))get("SteamAPI_ISteamMatchmaking_GetNumLobbyMembers");
    api.member = (decltype(api.member))get("SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex");
    api.owner = (decltype(api.owner))get("SteamAPI_ISteamMatchmaking_GetLobbyOwner");
    api.member_data = (decltype(api.member_data))get("SteamAPI_ISteamMatchmaking_GetLobbyMemberData");
    api.set_member_data = (decltype(api.set_member_data))get("SteamAPI_ISteamMatchmaking_SetLobbyMemberData");
    return api.friends && api.matchmaking && api.user && api.steam_id && api.game_played && api.persona && api.members && api.member &&
           api.owner && api.member_data && api.set_member_data;
}

inline std::mutex members_mx;
inline std::vector<Member> lobby_members;
inline uint64_t current_lobby = 0;

// Sends our line once per lobby and again every RESEND_MS; reads every member's line. Window thread only.
inline void tick(const char* version, const char* edition, void (*log)(const char*, ...)) {
    static DWORD last_tick = 0, last_sent = 0;
    static uint64_t sent_to = 0;
    if (!api.set_member_data || GetTickCount() - last_tick < TICK_MS) return;
    last_tick = GetTickCount();
    uint64_t me = api.steam_id(api.user);
    GameInfo info{};
    uint64_t lobby = api.game_played(api.friends, me, &info) ? info.lobby : 0;
    if (lobby != current_lobby && log) log("announce: %s", lobby ? "in a Steam lobby" : "no Steam lobby");
    current_lobby = lobby;
    std::vector<Member> out;
    if (lobby) {
        if (lobby != sent_to || GetTickCount() - last_sent > RESEND_MS)
            api.set_member_data(api.matchmaking, lobby, KEY, encode(version, edition).c_str()), sent_to = lobby, last_sent = GetTickCount();
#if !FATRAINER_VISUALS
        uint64_t owner = api.owner(api.matchmaking, lobby);
        int n = api.members(api.matchmaking, lobby);
        for (int i = 0; i < n && i < 16; i++) {
            Member m{};
            m.steam_id = api.member(api.matchmaking, lobby, i);
            const char* name = api.persona(api.friends, m.steam_id);
            m.name = name ? name : "", m.owner = m.steam_id == owner, m.you = m.steam_id == me;
            const char* wire = api.member_data(api.matchmaking, lobby, m.steam_id, KEY);
            m.fatrainer = wire && decode(wire, m.peer);
            out.push_back(m);
        }
#endif
    }
    std::lock_guard<std::mutex> l(members_mx);
    lobby_members.swap(out);
}

inline std::vector<Member> members() {
    std::lock_guard<std::mutex> l(members_mx);
    return lobby_members;
}

inline void open_profile(uint64_t steam_id) {
    if (api.overlay_to_user) api.overlay_to_user(api.friends, "steamid", steam_id);
}

#if !FATRAINER_VISUALS
inline std::mutex heard_mx;
inline std::map<uintptr_t, Peer> heard_from;

inline std::map<uintptr_t, Peer> heard() {
    std::lock_guard<std::mutex> l(heard_mx);
    return heard_from;
}
#endif

}
