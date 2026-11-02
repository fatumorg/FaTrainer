#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <map>
#include <mutex>
#include <string>
#include "game.h"

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

// The channel: a member attribute KEY in the game's own Epic Online Services lobby (the engine's sessions are EOS
// lobbies, not Steam lobbies). Every player in the session can read it, the stock game never looks at the key, and it
// changes nothing in the match. EOS is not thread-safe, so everything runs inside the game's own EOS_Platform_Tick.
// Option structs and ApiVersion 1 are the ones the engine itself passes (engine 1806d5f24, 1806a94b5, 1806bc5aa).
const char* const KEY = "fatrainer";
const char* const EOS_DLL = "EOSSDK-Win64-Shipping.dll";
const DWORD RESEND_MS = 30000, TICK_MS = 2000;
const int MAX_MEMBERS = 16;
struct Member { void* user; bool owner, you, fatrainer; Peer peer; };

struct LobbyIdOptions { int32_t api; const char* lobby; void* user; };
struct ModificationOptions { int32_t api; void* user; const char* lobby; };
struct AttributeData { int32_t api; const char* key; union { int64_t i; double d; int32_t b; const char* text; } value; int32_t type; };
struct AddMemberAttributeOptions { int32_t api; const AttributeData* attribute; int32_t visibility; };
struct UpdateLobbyOptions { int32_t api; void* modification; };
struct MemberCountOptions { int32_t api; };
struct MemberByIndexOptions { int32_t api; uint32_t index; };
struct CopyMemberAttributeOptions { int32_t api; void* target; const char* key; };
struct Attribute { int32_t api; AttributeData* data; int32_t visibility; };
struct OwnerOptions { int32_t api; };
const int32_t TYPE_STRING = 3, VISIBILITY_PUBLIC = 0, EOS_SUCCESS = 0;

struct Eos {
    int (*copy_details)(void*, const LobbyIdOptions*, void**) = nullptr;
    int (*update_modification)(void*, const ModificationOptions*, void**) = nullptr;
    void (*platform_tick)(void*) = nullptr;
    int (*add_member_attribute)(void*, const AddMemberAttributeOptions*) = nullptr;
    void (*update_lobby)(void*, const UpdateLobbyOptions*, void*, void (*)(const void*)) = nullptr;
    void (*modification_release)(void*) = nullptr;
    uint32_t (*member_count)(void*, const MemberCountOptions*) = nullptr;
    void* (*member_by_index)(void*, const MemberByIndexOptions*) = nullptr;
    int (*copy_member_attribute)(void*, const CopyMemberAttributeOptions*, Attribute**) = nullptr;
    void (*attribute_release)(Attribute*) = nullptr;
    void (*details_release)(void*) = nullptr;
    void* (*owner)(void*, const OwnerOptions*) = nullptr;
};
inline Eos eos;

inline std::mutex lobby_mx;
inline void* lobby_interface = nullptr;
inline void* local_user = nullptr;
inline std::string lobby_id;
inline std::string wire_out;
inline std::vector<Member> lobby_members;
inline void (*log_line)(const char*, ...) = nullptr;

inline void remember(void* iface, const char* lobby, void* user) {
    if (!iface || !lobby || !user) return;
    std::lock_guard<std::mutex> l(lobby_mx);
    if (lobby_id != lobby && log_line) log_line("announce: in an EOS lobby");
    lobby_interface = iface, lobby_id = lobby, local_user = user;
}

inline int copy_details_hook(void* iface, const LobbyIdOptions* o, void** out) {
    if (o) remember(iface, o->lobby, o->user);
    return eos.copy_details(iface, o, out);
}

inline int update_modification_hook(void* iface, const ModificationOptions* o, void** out) {
    if (o) remember(iface, o->lobby, o->user);
    return eos.update_modification(iface, o, out);
}

inline void update_done(const void*) {}

inline void send_and_read() {
    void* iface;
    void* user;
    std::string lobby, wire;
    {
        std::lock_guard<std::mutex> l(lobby_mx);
        iface = lobby_interface, user = local_user, lobby = lobby_id, wire = wire_out;
    }
    if (!iface || lobby.empty() || wire.empty()) return;
    static DWORD last_sent = 0;
    static std::string sent_to;
    if (sent_to != lobby || GetTickCount() - last_sent > RESEND_MS) {
        ModificationOptions mo{1, user, lobby.c_str()};
        void* modification = nullptr;
        if (eos.update_modification(iface, &mo, &modification) == EOS_SUCCESS && modification) {
            AttributeData data{1, KEY, {}, TYPE_STRING};
            data.value.text = wire.c_str();
            AddMemberAttributeOptions ao{1, &data, VISIBILITY_PUBLIC};
            if (eos.add_member_attribute(modification, &ao) == EOS_SUCCESS) {
                UpdateLobbyOptions uo{1, modification};
                eos.update_lobby(iface, &uo, nullptr, &update_done);
            }
            eos.modification_release(modification);
        }
        sent_to = lobby, last_sent = GetTickCount();
    }
#if !FATRAINER_VISUALS
    LobbyIdOptions co{1, lobby.c_str(), user};
    void* details = nullptr;
    std::vector<Member> out;
    if (eos.copy_details(iface, &co, &details) == EOS_SUCCESS && details) {
        MemberCountOptions mc{1};
        OwnerOptions oo{1};
        void* owner = eos.owner(details, &oo);
        uint32_t n = eos.member_count(details, &mc);
        for (uint32_t i = 0; i < n && i < MAX_MEMBERS; i++) {
            MemberByIndexOptions mi{1, i};
            Member m{};
            m.user = eos.member_by_index(details, &mi);
            m.owner = m.user && m.user == owner, m.you = m.user == user;
            CopyMemberAttributeOptions ca{1, m.user, KEY};
            Attribute* a = nullptr;
            if (m.user && eos.copy_member_attribute(details, &ca, &a) == EOS_SUCCESS && a) {
                if (a->data && a->data->type == TYPE_STRING && a->data->value.text) m.fatrainer = decode(a->data->value.text, m.peer);
                eos.attribute_release(a);
            }
            out.push_back(m);
        }
        eos.details_release(details);
    }
    std::lock_guard<std::mutex> l(lobby_mx);
    lobby_members.swap(out);
#endif
}

inline void platform_tick_hook(void* platform) {
    eos.platform_tick(platform);
    static DWORD last = 0;
    if (GetTickCount() - last < TICK_MS) return;
    last = GetTickCount();
    send_and_read();
}

inline bool install(const char* version, const char* edition, void (*log)(const char*, ...)) {
    HMODULE engine = GetModuleHandleA("engine_x64_rwdi.dll"), sdk = GetModuleHandleA(EOS_DLL);
    if (!engine || !sdk) return false;
    log_line = log;
    {
        std::lock_guard<std::mutex> l(lobby_mx);
        wire_out = encode(version, edition);
    }
    auto get = [&](const char* name) { return (void*)GetProcAddress(sdk, name); };
    eos.add_member_attribute = (decltype(eos.add_member_attribute))get("EOS_LobbyModification_AddMemberAttribute");
    eos.update_lobby = (decltype(eos.update_lobby))get("EOS_Lobby_UpdateLobby");
    eos.modification_release = (decltype(eos.modification_release))get("EOS_LobbyModification_Release");
    eos.member_count = (decltype(eos.member_count))get("EOS_LobbyDetails_GetMemberCount");
    eos.member_by_index = (decltype(eos.member_by_index))get("EOS_LobbyDetails_GetMemberByIndex");
    eos.copy_member_attribute = (decltype(eos.copy_member_attribute))get("EOS_LobbyDetails_CopyMemberAttributeByKey");
    eos.attribute_release = (decltype(eos.attribute_release))get("EOS_Lobby_Attribute_Release");
    eos.details_release = (decltype(eos.details_release))get("EOS_LobbyDetails_Release");
    eos.owner = (decltype(eos.owner))get("EOS_LobbyDetails_GetLobbyOwner");
    if (!eos.add_member_attribute || !eos.update_lobby || !eos.modification_release || !eos.member_count || !eos.member_by_index ||
        !eos.copy_member_attribute || !eos.attribute_release || !eos.details_release || !eos.owner)
        return false;
    eos.copy_details = (decltype(eos.copy_details))get("EOS_Lobby_CopyLobbyDetailsHandle");
    eos.update_modification = (decltype(eos.update_modification))get("EOS_Lobby_UpdateLobbyModification");
    eos.platform_tick = (decltype(eos.platform_tick))get("EOS_Platform_Tick");
    if (!eos.copy_details || !eos.update_modification || !eos.platform_tick) return false;
    return game::hook_import(engine, EOS_DLL, "EOS_Lobby_CopyLobbyDetailsHandle", (void*)&copy_details_hook) &&
           game::hook_import(engine, EOS_DLL, "EOS_Lobby_UpdateLobbyModification", (void*)&update_modification_hook) &&
           game::hook_import(engine, EOS_DLL, "EOS_Platform_Tick", (void*)&platform_tick_hook);
}

inline std::vector<Member> members() {
    std::lock_guard<std::mutex> l(lobby_mx);
    return lobby_members;
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
