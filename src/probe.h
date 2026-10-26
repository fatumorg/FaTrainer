#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "cheats.h"
#include "game.h"
#include "input.h"

namespace probe {

using SetLevelFn = void(__fastcall*)(uintptr_t, int16_t, int);
inline SetLevelFn set_level_original = nullptr;

inline void __fastcall traced_set_level(uintptr_t container, int16_t level, int tree) {
    uintptr_t from = (uintptr_t)__builtin_return_address(0) - game::g.base;
    uint16_t before = tree >= 0 && tree < 8 ? game::rdv<uint16_t>(game::rdv<uintptr_t>(container + 0x40) + tree * 0x20 + 0x14) : 0;
    cheats::logf_hook("trace: SetLevel tree %d %u -> %d from gamedll+%llx container %llx", tree, before, level, (unsigned long long)from, (unsigned long long)container);
    set_level_original(container, level, tree);
}

using RestoreFn = void(__fastcall*)(uintptr_t, uintptr_t);
inline RestoreFn restore_original = nullptr;

inline void __fastcall traced_restore(uintptr_t container, uintptr_t src) {
    char line[256];
    int n = snprintf(line, sizeof line, "trace: restore from gamedll+%llx src %llx levels", (unsigned long long)((uintptr_t)__builtin_return_address(0) - game::g.base), (unsigned long long)src);
    for (int t = 0; t < 8; t++) n += snprintf(line + n, sizeof line - n, " %u/%u", game::rdv<uint16_t>(src + 0x890 + t * 2), game::rdv<uint32_t>(src + 0xa0c + t * 4));
    cheats::logf_hook("%s", line);
    restore_original(container, src);
}

inline bool trace_restore() {
    const uint8_t prologue[14] = {0x48, 0x8B, 0xC4, 0x48, 0x89, 0x50, 0x10, 0x48, 0x89, 0x48, 0x08, 0x56, 0x41, 0x54};
    uintptr_t fn = game::g.base + 0xc41950;
    if (restore_original) return true;
    if (memcmp((const void*)fn, prologue, sizeof prologue)) return false;
    auto* trampoline = (uint8_t*)VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memcpy(trampoline, prologue, sizeof prologue);
    cheats::absolute_jump(trampoline + sizeof prologue, fn + sizeof prologue);
    restore_original = (RestoreFn)trampoline;
    uint8_t patch[sizeof prologue];
    cheats::absolute_jump(patch, (uintptr_t)&traced_restore);
    cheats::write_code(fn, patch, sizeof patch);
    return true;
}

inline bool trace_set_level() {
    const uint8_t prologue[15] = {0x48, 0x89, 0x5C, 0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57, 0x48, 0x83, 0xEC, 0x20};
    uintptr_t fn = cheats::set_level_fn;
    if (set_level_original) return true;
    if (!fn || memcmp((const void*)fn, prologue, sizeof prologue)) return false;
    auto* trampoline = (uint8_t*)VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memcpy(trampoline, prologue, sizeof prologue);
    cheats::absolute_jump(trampoline + sizeof prologue, fn + sizeof prologue);
    set_level_original = (SetLevelFn)trampoline;
    cheats::set_level_fn = (uintptr_t)trampoline;
    uint8_t patch[sizeof prologue];
    cheats::absolute_jump(patch, (uintptr_t)&traced_set_level);
    patch[14] = 0x90;
    cheats::write_code(fn, patch, sizeof patch);
    return true;
}

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
        std::string name;
        while (isalnum((unsigned char)*s) || *s == '_' || *s == ':' || *s == '@' || *s == '?' || *s == '$') name += *s++;
        bool hex = !name.empty() && name.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
        if (hex && name != "g") return strtoull(name.c_str(), nullptr, 16);
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

struct Watch { uintptr_t at = 0; std::vector<int> offsets; DWORD until = 0; std::vector<uint32_t> last; FILE* out = nullptr; };
inline Watch watch;
inline std::mutex watch_mx;

inline void watch_frame() {
    std::lock_guard<std::mutex> l(watch_mx);
    if (!watch.out) return;
    if (GetTickCount() > watch.until) {
        fclose(watch.out), watch.out = nullptr;
        return;
    }
    std::vector<uint32_t> now;
    for (int off : watch.offsets) now.push_back(rdv<uint32_t>(watch.at + off, 0xdeadbeef));
    if (now == watch.last) return;
    watch.last = now;
    fprintf(watch.out, "%lu", (unsigned long)GetTickCount());
    for (size_t i = 0; i < now.size(); i++) {
        float f;
        memcpy(&f, &now[i], 4);
        fprintf(watch.out, " +%x=%d/%.4g", watch.offsets[i], (int)now[i], f);
    }
    fprintf(watch.out, "\n");
    fflush(watch.out);
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
        uintptr_t args[6] = {};
        for (int i = 0; i < 6; i++) {
            p.skip();
            if (!*p.s) break;
            args[i] = p.expr();
        }
        uintptr_t gamedll = (uintptr_t)GetModuleHandleA("gamedll_x64_rwdi.dll");
        if (!p.ok || !fn || fn == gamedll) return out + "  bad address\n";
        auto f = (uintptr_t(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t))fn;
        uintptr_t r = f(args[0], args[1], args[2], args[3], args[4], args[5]);
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
    if (c == "findtext") {
        std::string want = p.s;
        for (auto& ch : want) ch = (char)tolower((unsigned char)ch);
        auto found = game::scan({visuals::engine_text_vtable});
        int shown = 0;
        for (uintptr_t t : found.empty() ? std::vector<uintptr_t>{} : found[0]) {
            visuals::TtlText raw = rdv<visuals::TtlText>(t + visuals::TEXT_RAW), wide = rdv<visuals::TtlText>(t + visuals::TEXT_RAW + 0x10);
            std::string r(raw.text && raw.size < 400 ? raw.size : 0, '\0'), w;
            if (!r.empty()) game::rd((uintptr_t)raw.text, r.data(), r.size());
            for (uint32_t i = 0; wide.text && i < wide.size && i < 400; i++) w += (char)rdv<uint16_t>((uintptr_t)wide.text + i * 2);
            std::string lw = w;
            for (auto& ch : lw) ch = (char)tolower((unsigned char)ch);
            if (lw.find(want) == std::string::npos || shown++ > 30) continue;
            snprintf(b, sizeof b, "  %llx raw [%s] wide [%s]\n", (unsigned long long)t, r.c_str(), w.c_str());
            out += b;
        }
        snprintf(b, sizeof b, "  %zu texts\n", found.empty() ? (size_t)0 : found[0].size());
        return out + b;
    }
    if (c == "findwide") {
        std::string want = p.s;
        std::vector<uint8_t> needle;
        for (char ch : want) needle.push_back((uint8_t)ch), needle.push_back(0);
        int n = 0;
        game::each_heap_chunk([&](const uint64_t* q, uintptr_t at, size_t len) {
            const uint8_t* bytes = (const uint8_t*)q;
            for (size_t i = 0; n < 20 && i + needle.size() <= len; i += 2)
                if (!memcmp(bytes + i, needle.data(), needle.size())) {
                    uintptr_t hit = at + i;
                    std::string owner = "?";
                    for (size_t back = 0; back < 0x1000 && owner == "?"; back += 8) {
                        uintptr_t v = rdv<uintptr_t>((hit & ~(uintptr_t)7) - back);
                        std::string name = v ? game::class_name(v) : "?";
                        if (name != "?") snprintf(b, sizeof b, "%s at -%zx (%llx)", name.c_str(), back, (unsigned long long)((hit & ~(uintptr_t)7) - back)), owner = b;
                    }
                    snprintf(b, sizeof b, "  %llx in %s\n", (unsigned long long)hit, owner.c_str());
                    out += b, n++;
                }
        });
        return out;
    }
    if (c == "enemies") {
        cheats::Vec3 me = cheats::player_position();
        std::lock_guard<std::mutex> l(cheats::modules_mx);
        int n = 0;
        for (uintptr_t m : cheats::enemy_modules) {
            uintptr_t owner = rdv<uintptr_t>(m + cheats::MODULE_OWNER);
            cheats::Vec3 at;
            float health = rdv<float>(m + cheats::MODULE_HEALTH, NAN);
            if (!(health > 0) || !cheats::position_of(owner + game::g.human_control, &at) || n++ > 30) continue;
            snprintf(b, sizeof b, "  %llx %s health %.0f at %.1f %.1f %.1f (%.1f m)\n", (unsigned long long)owner, game::class_name(owner).c_str(), health, at.x, at.y, at.z, cheats::distance(me, at));
            out += b;
        }
        return out;
    }
    if (c == "aim") {
        float m[16];
        cheats::Vec3 me = cheats::player_position();
        if (!cheats::view_matrix(m)) return out + "  no camera\n";
        std::lock_guard<std::mutex> l(cheats::modules_mx);
        float best = 1e9f, bx = 0, by = 0, bd = 0, bh = 0;
        for (uintptr_t mod : cheats::enemy_modules) {
            uintptr_t owner = rdv<uintptr_t>(mod + cheats::MODULE_OWNER);
            cheats::Vec3 at;
            float health = rdv<float>(mod + cheats::MODULE_HEALTH, NAN), sx, sy;
            if (!(health > 0) || !cheats::position_of(owner + game::g.human_control, &at) || fabsf(at.y - me.y) > 3) continue;
            at.y += 1.5f;
            float d = cheats::distance(me, at);
            if (d < 1.5f || d > 30 || !cheats::to_screen(m, at, 2560, 1440, sx, sy)) continue;
            float off = fabsf(sx - 1280);
            if (off < best) best = off, bx = sx, by = sy, bd = d, bh = health;
        }
        snprintf(b, sizeof b, "  %s %.0f %.0f %.1f %.0f\n", best < 1e9f ? "target" : "none", bx - 1280, by - 720, bd, bh);
        return out + b;
    }
    if (c == "give") {
        char id[96] = {};
        int n = 1, kind = game::K_BACKPACK;
        sscanf(p.s, "%95s %d %d", id, &n, &kind);
        auto d = game::g.descs.find(id);
        int into = d == game::g.descs.end() ? -2 : game::give_anywhere((game::Kind)std::clamp(kind, 0, game::K_COUNT - 1), d->second, n);
        snprintf(b, sizeof b, "  gave into %d\n", into);
        return out + b;
    }
    if (c == "cheat") {
        char key[64] = {};
        int on = 1;
        sscanf(p.s, "%63s %d", key, &on);
        auto* ch = cheats::find(key);
        if (!ch) return out + "  no such cheat\n";
        ch->on = on != 0;
        return out + "  set\n";
    }
    if (c == "tweak") {
        char key[64] = {};
        float f = 1;
        sscanf(p.s, "%63s %f", key, &f);
        auto* t = cheats::find_tweak(key);
        if (!t) return out + "  no such tweak\n";
        t->factor = f;
        return out + "  set\n";
    }
    if (c == "level") {
        int tree = 0, level = -1;
        sscanf(p.s, "%d %d", &tree, &level);
        if (level >= 0) cheats::set_tree_level(tree, level);
        snprintf(b, sizeof b, "  tree %d level %d xp %u start %u span %u\n", tree, cheats::tree_level(tree), cheats::tree_xp(tree), cheats::tree_level_start(tree), cheats::tree_span(tree));
        return out + b;
    }
    if (c == "providers") {
        if (p.s[0] == 0) {
            cheats::probe_provider_id = -1;
            std::lock_guard<std::mutex> l(cheats::probe_provider_mx);
            for (auto& [provider, count] : cheats::probe_provider_counts) {
                snprintf(b, sizeof b, "  provider %llx reads %d\n", (unsigned long long)provider, count);
                out += b;
                uintptr_t holders[2] = {cheats::player, cheats::profile_player()};
                for (uintptr_t holder : holders)
                    for (size_t off = 0; holder && off < 0x2000; off += 8)
                        if (rdv<uintptr_t>(holder + off) == provider) {
                            snprintf(b, sizeof b, "    held by %llx +%zx\n", (unsigned long long)holder, off);
                            out += b;
                        }
            }
            return out;
        }
        char name[96] = {};
        sscanf(p.s, "%95s", name);
        auto it = cheats::param_ids.find(name);
        if (it == cheats::param_ids.end() && strcmp(name, "*")) return out + "  no such param\n";
        {
            std::lock_guard<std::mutex> l(cheats::probe_provider_mx);
            cheats::probe_provider_counts.clear();
        }
        cheats::probe_provider_id = it == cheats::param_ids.end() ? -2 : it->second + 1;
        return out + "  counting\n";
    }
    if (c == "tracelevel") return out + (trace_set_level() && trace_restore() ? "  tracing\n" : "  failed\n");
    if (c == "pos") {
        cheats::Vec3 at = cheats::player_position();
        snprintf(b, sizeof b, "  %.2f %.2f %.2f\n", at.x, at.y, at.z);
        return out + b;
    }
    if (c == "tp") {
        cheats::Vec3 at = cheats::player_position(), d{};
        bool relative = p.s[0] == '+';
        sscanf(p.s + relative, "%f %f %f", &d.x, &d.y, &d.z);
        cheats::Vec3 to = relative ? cheats::Vec3{at.x + d.x, at.y + d.y, at.z + d.z} : d;
        cheats::teleport(to);
        snprintf(b, sizeof b, "  %.2f %.2f %.2f -> %.2f %.2f %.2f\n", at.x, at.y, at.z, to.x, to.y, to.z);
        return out + b;
    }
    if (c == "param") {
        char name[96] = {};
        float v = NAN;
        sscanf(p.s, "%95s %f", name, &v);
        for (uintptr_t container : cheats::param_containers()) {
            uintptr_t at = cheats::param_value(container, name);
            if (at && !std::isnan(v)) game::wr<float>(at, v);
            snprintf(b, sizeof b, "  container %llx: %llx = %g (byte %d)\n", (unsigned long long)container, (unsigned long long)at, at ? rdv<float>(at) : NAN, at ? rdv<uint8_t>(at) : -1);
            out += b;
        }
        return out;
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
                snprintf(b, sizeof b, "  %llx visible %d shown %d opacity %.2f children %d\n", (unsigned long long)e, visuals::ui.visible(e), visuals::ui.shown(e),
                         visuals::ui.get_opacity(e), visuals::ui.children_count(e));
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
    if (c == "watch") {
        uintptr_t a = p.expr();
        char* end;
        double seconds = strtod(p.s, &end);
        std::vector<int> offsets;
        for (const char* q = end; *q;) {
            while (*q == ' ') q++;
            if (!*q) break;
            offsets.push_back((int)strtol(q, &end, 16));
            if (end == q) break;
            q = end;
        }
        std::lock_guard<std::mutex> l(watch_mx);
        if (watch.out) fclose(watch.out);
        watch = {a, offsets, GetTickCount() + (DWORD)(seconds * 1000), {}, fopen((folder() + "fatrainer_watch.txt").c_str(), "wb")};
        snprintf(b, sizeof b, "  watching %llx, %zu offsets for %.0f s\n", (unsigned long long)a, offsets.size(), seconds);
        return out + b;
    }
    if (c == "near") {
        float radius = (float)atof(p.s);
        uintptr_t level = cheats::current_level();
        cheats::Vec3 me{};
        bool at = game::g.player_control >= 0 && cheats::position_of(cheats::player + game::g.player_control, &me);
        cheats::EngineVector found{};
        bool ok = level && at && cheats::find_in_radius && cheats::find_in_radius(level, &found, &me, radius > 0 ? radius : 8, 0, false, nullptr);
        snprintf(b, sizeof b, "  level %llx at %d (%.1f %.1f %.1f) returned %d, %u objects\n", (unsigned long long)level, at, me.x, me.y, me.z, ok, found.size);
        out += b;
        for (uint32_t i = 0; i < found.size && i < 30 && found.data; i++) {
            uintptr_t control = found.data[i], vt = rdv<uintptr_t>(control);
            uintptr_t whole = vt ? control - rdv<uint32_t>(rdv<uintptr_t>(vt - 8) + 4) : 0;
            snprintf(b, sizeof b, "    %llx %s\n", (unsigned long long)whole, game::class_name(whole).c_str());
            out += b;
        }
        return out;
    }
    if (c == "scan") {
        uintptr_t vt = p.expr();
        auto found = game::scan({vt});
        snprintf(b, sizeof b, "  %zu objects\n", found[0].size());
        out += b;
        for (size_t i = 0; i < found[0].size() && i < 40; i++) snprintf(b, sizeof b, "  %llx\n", (unsigned long long)found[0][i]), out += b;
        return out;
    }
    if (c == "find32") {
        uint32_t target = (uint32_t)strtoul(p.s, nullptr, 10);
        size_t n = 0;
        game::each_heap_chunk([&](const uint64_t* q, uintptr_t at, size_t len) {
            const uint32_t* d = (const uint32_t*)q;
            for (size_t i = 0; i < len / 4 && n < 60; i++)
                if (d[i] == target) {
                    char l[64];
                    snprintf(l, sizeof l, "  %llx\n", (unsigned long long)(at + i * 4));
                    out += l, n++;
                }
        });
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
