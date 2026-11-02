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

inline std::vector<Row> rows() {
    using namespace cheats;
    std::vector<EspTarget> seen;
    {
        std::lock_guard<std::mutex> l(esp_mx);
        seen = seen_players;
    }
    auto heard = announce::heard();
    std::vector<Row> out;
    int n = 1;
    for (auto& t : seen) {
        Row r{};
        r.who.p = t.p, r.who.hunter = t.hunter, r.who.name = (t.hunter ? "Night Hunter " : "Survivor ") + std::to_string(n++);
        r.health = t.health, r.max_health = t.max_health, r.distance = t.distance, r.rank = t.rank;
        if (auto h = heard.find(t.p); h != heard.end()) r.fatrainer = true, r.peer = h->second;
        std::lock_guard<std::mutex> l(detect::players_mx);
        if (auto d = detect::players.find(t.p); d != detect::players.end()) r.findings = d->second.findings();
        out.push_back(r);
    }
    return out;
}

}
