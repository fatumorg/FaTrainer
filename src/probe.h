#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "cheats.h"
#include "game.h"

namespace probe {

using game::rd;
using game::rdv;

inline std::string folder() {
    char p[MAX_PATH];
    GetModuleFileNameA(nullptr, p, MAX_PATH);
    std::string s = p;
    return s.substr(0, s.find_last_of("\\/") + 1);
}

struct Parser {
    const char* s;
    bool ok = true;
    void skip() { while (*s == ' ') s++; }
    uintptr_t symbol(const std::string& name) {
        uintptr_t gamedll = (uintptr_t)GetModuleHandleA("gamedll_x64_rwdi.dll"), engine = (uintptr_t)GetModuleHandleA("engine_x64_rwdi.dll");
        if (name == "g") return gamedll;
        if (name == "eng") return engine;
        if (name == "player") return cheats::player;
        if (name == "local") return cheats::local_object();
        if (name == "game") return cheats::profile_root ? rdv<uintptr_t>(cheats::profile_root) : 0;
        if (name == "profile") return cheats::game_profile();
        if (name == "params") return cheats::params_root;
        if (name == "tools") return visuals::tools_global ? rdv<uintptr_t>(visuals::tools_global) : 0;
        if (name.rfind("vt:", 0) == 0) return game::find_vtable(gamedll, name.substr(3).c_str());
        ok = false;
        return 0;
    }
    uintptr_t term() {
        skip();
        if (*s == '[') {
            s++;
            uintptr_t v = expr();
            skip();
            if (*s == ']') s++;
            else ok = false;
            return rdv<uintptr_t>(v);
        }
        if (isxdigit((unsigned char)*s) && !(isalpha((unsigned char)*s) && isalpha((unsigned char)s[1]))) {
            char* end;
            uintptr_t v = strtoull(s, &end, 16);
            s = end;
            return v;
        }
        std::string name;
        while (isalnum((unsigned char)*s) || *s == '_' || *s == ':' || *s == '@' || *s == '?' || *s == '$') name += *s++;
        return symbol(name);
    }
    uintptr_t expr() {
        uintptr_t v = term();
        for (;;) {
            skip();
            if (*s == '+') s++, v += term();
            else if (*s == '-') s++, v -= term();
            else return v;
        }
    }
};

inline std::string hex_dump(uintptr_t a, size_t n) {
    std::vector<uint8_t> b(n);
    if (!rd(a, b.data(), n)) return "  unreadable\n";
    std::string out;
    for (size_t i = 0; i < n; i += 16) {
        char line[200];
        int k = snprintf(line, sizeof line, "  +%03zx:", i);
        for (size_t j = 0; j < 16 && i + j < n; j += 8) {
            uint64_t q;
            memcpy(&q, &b[i + j], 8);
            k += snprintf(line + k, sizeof line - k, " %016llx", (unsigned long long)q);
        }
        k += snprintf(line + k, sizeof line - k, "  |");
        for (size_t j = 0; j < 16 && i + j < n; j += 4) {
            float f;
            memcpy(&f, &b[i + j], 4);
            k += snprintf(line + k, sizeof line - k, " %.4g", f);
        }
        out += line;
        out += '\n';
    }
    return out;
}

inline std::string run(const std::string& line) {
    char cmd[16] = {};
    int used = 0;
    if (sscanf(line.c_str(), "%15s %n", cmd, &used) < 1) return "";
    Parser p{line.c_str() + used};
    std::string c = cmd, out = "> " + line + "\n";
    char b[512];
    if (c == "r") {
        uintptr_t a = p.expr();
        size_t n = strtoul(p.s, nullptr, 16);
        snprintf(b, sizeof b, "  %llx %s\n", (unsigned long long)a, game::class_name(a).c_str());
        return out + b + hex_dump(a, n ? n : 0x40);
    }
    if (c == "c") {
        uintptr_t a = p.expr();
        snprintf(b, sizeof b, "  %llx -> %llx %s\n", (unsigned long long)a, (unsigned long long)rdv<uintptr_t>(a), game::class_name(rdv<uintptr_t>(a)).c_str());
        return out + b;
    }
    if (c == "classes") {
        uintptr_t a = p.expr();
        size_t n = strtoul(p.s, nullptr, 16);
        for (size_t off = 0; off < (n ? n : 0x100); off += 8) {
            uintptr_t v = rdv<uintptr_t>(a + off);
            std::string name = v ? game::class_name(v) : "?";
            if (name != "?") snprintf(b, sizeof b, "  +%zx %llx %s\n", off, (unsigned long long)v, name.c_str()), out += b;
        }
        return out;
    }
    if (c == "f" || c == "i" || c == "q" || c == "b") {
        uintptr_t a = p.expr();
        bool done = c == "f" ? game::wr<float>(a, (float)atof(p.s)) : c == "i" ? game::wr<int>(a, atoi(p.s)) : c == "b" ? game::wr<uint8_t>(a, (uint8_t)strtoul(p.s, nullptr, 16)) : game::wr<uint64_t>(a, strtoull(p.s, nullptr, 16));
        return out + (done ? "  written\n" : "  failed\n");
    }
    if (c == "call") {
        uintptr_t fn = p.expr();
        uintptr_t args[4] = {};
        for (int i = 0; i < 4; i++) {
            p.skip();
            if (!*p.s) break;
            args[i] = p.expr();
        }
        auto f = (uintptr_t(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))fn;
        uintptr_t r = f(args[0], args[1], args[2], args[3]);
        snprintf(b, sizeof b, "  returned %llx\n", (unsigned long long)r);
        return out + b;
    }
    if (c == "cheat") {
        char key[64] = {};
        int on = 0;
        sscanf(p.s, "%63s %d", key, &on);
        cheats::Cheat* ch = cheats::find(key);
        if (ch) ch->on = on != 0;
        return out + (ch ? "  set\n" : "  no such cheat\n");
    }
    if (c == "tweak") {
        char key[64] = {};
        float f = 1;
        sscanf(p.s, "%63s %f", key, &f);
        cheats::Tweak* t = cheats::find_tweak(key);
        if (t) t->factor = f;
        return out + (t ? "  set\n" : "  no such tweak\n");
    }
    if (c == "hud") {
        char key[64] = {};
        int on = 0, hidden = 0;
        float r = 1, g = 1, bl = 1;
        sscanf(p.s, "%63s %d %f %f %f %d", key, &on, &r, &g, &bl, &hidden);
        auto& h = config::cfg.hud_parts[key];
        h.on = on != 0, h.hidden = hidden != 0, h.color[0] = r, h.color[1] = g, h.color[2] = bl;
        return out + "  set\n";
    }
    if (c == "show") {
        char key[64] = {};
        sscanf(p.s, "%63s", key);
        for (int i = 0; i < visuals::HUD_PART_COUNT; i++)
            if (!strcmp(key, visuals::HUD_PARTS[i].key)) visuals::preview_part = i, visuals::preview_until = GetTickCount() + 6000;
        float r[4] = {};
        bool rect = visuals::preview_rect(r);
        snprintf(b, sizeof b, "  rect %d %.1f %.1f %.1f %.1f\n", rect, r[0], r[1], r[2], r[3]);
        return out + b;
    }
    if (c == "ui") {
        char key[64] = {};
        sscanf(p.s, "%63s", key);
        std::vector<std::pair<uintptr_t, int>> stack{{visuals::hud_root, 0}};
        int shown = 0;
        while (!stack.empty() && shown < 20) {
            auto [e, depth] = stack.back();
            stack.pop_back();
            if (!e || depth > 40) continue;
            std::string name = visuals::ui.name_of(e);
            if (name == key) {
                float lo[3] = {}, hi[3] = {};
                if (visuals::element_bounds) visuals::element_bounds(e, lo, hi);
                snprintf(b, sizeof b, "  %llx visible %d shown %d opacity %.2f children %d bounds %.0f,%.0f %.0f,%.0f\n", (unsigned long long)e, visuals::ui.visible(e), visuals::ui.shown(e),
                         visuals::ui.get_opacity(e), visuals::ui.children_count(e), lo[0], lo[1], hi[0], hi[1]);
                out += b, shown++;
                for (int i = 0; i < visuals::ui.children_count(e) && i < 12; i++) {
                    uintptr_t ch = visuals::ui.child(e, i);
                    snprintf(b, sizeof b, "    %s visible %d opacity %.2f\n", visuals::ui.name_of(ch).c_str(), visuals::ui.visible(ch), visuals::ui.get_opacity(ch));
                    out += b;
                }
            }
            for (int i = 0, n = visuals::ui.children_count(e); i < n && i < 400; i++) stack.push_back({visuals::ui.child(e, i), depth + 1});
        }
        return out + (shown ? "" : "  not found\n");
    }
    if (c == "scan") {
        uintptr_t vt = p.expr();
        auto found = game::scan({vt});
        snprintf(b, sizeof b, "  %zu objects\n", found[0].size());
        out += b;
        for (size_t i = 0; i < found[0].size() && i < 40; i++) snprintf(b, sizeof b, "  %llx\n", (unsigned long long)found[0][i]), out += b;
        return out;
    }
    if (c == "refs") {
        uintptr_t target = p.expr();
        size_t n = 0;
        game::each_heap_chunk([&](const uint64_t* q, uintptr_t at, size_t len) {
            for (size_t i = 0; i < len / 8 && n < 40; i++)
                if (q[i] == target) {
                    char l[64];
                    snprintf(l, sizeof l, "  %llx\n", (unsigned long long)(at + i * 8));
                    out += l, n++;
                }
        });
        return out;
    }
    return out + "  unknown command\n";
}

inline void poll() {
    static std::string in = folder() + "fatrainer_probe.txt", result = folder() + "fatrainer_probe.out";
    FILE* f = fopen(in.c_str(), "rb");
    if (!f) return;
    std::string text;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) text.append(buf, n);
    fclose(f);
    DeleteFileA(in.c_str());
    std::string out;
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find('\n', at);
        if (end == std::string::npos) end = text.size();
        std::string line = text.substr(at, end - at);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (!line.empty()) out += run(line);
        at = end + 1;
    }
    if (FILE* o = fopen(result.c_str(), "wb")) fwrite(out.data(), 1, out.size(), o), fclose(o);
}

}
