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
    uint64_t steam_id;
    std::string name;
    int level, kills, deaths, latency_ms;
    bool voice, you, host, hunter;
};

inline std::string role(const Player& pl) {
    std::string side = pl.hunter ? "Night Hunter" : "Survivor";
    return pl.host ? "Host \xC2\xB7 " + side : side;
}

// Every other player the trainer tracks, and you. Names, kills, deaths, latency and voice are not read yet.
struct Row {
    Player who;
    float health, max_health, distance;
    int rank;
    bool fatrainer;
    announce::Peer peer;
    std::vector<detect::Finding> findings;
};

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
    announce::Peer shared;
    bool everyone = everyone_runs(announce::members(), shared);
    std::vector<Row> out;
    int n = 1;
    for (auto& t : seen) {
        Row r{};
        r.who.p = t.p, r.who.hunter = t.hunter, r.who.name = (t.hunter ? "Night Hunter " : "Survivor ") + std::to_string(n++);
        r.health = t.health, r.max_health = t.max_health, r.distance = t.distance, r.rank = t.rank;
        r.fatrainer = everyone, r.peer = shared;
        std::lock_guard<std::mutex> l(detect::players_mx);
        if (auto d = detect::players.find(t.p); d != detect::players.end()) r.findings = d->second.findings();
        out.push_back(r);
    }
    return out;
}

}
