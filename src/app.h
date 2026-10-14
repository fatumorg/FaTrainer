#pragma once
#include <windows.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#if !FATRAINER_VISUALS
#include "cheats.h"
#endif
#include "visuals.h"
#include "config.h"
#include "game.h"
#include "version.h"

inline const char* TITLE = "FaTrainer | Dying Light";


inline void logf(const char* fmt, ...) {
    static std::string path;
    if (path.empty()) {
        char p[MAX_PATH];
        GetModuleFileNameA(nullptr, p, MAX_PATH);
        path = p;
        path = path.substr(0, path.find_last_of("\\/") + 1) + "fatrainer.log";
    }
    if (FILE* f = fopen(path.c_str(), "a")) {
        va_list a;
        va_start(a, fmt);
        vfprintf(f, fmt, a);
        va_end(a);
        fputc('\n', f);
        fclose(f);
    }
}

#if !FATRAINER_VISUALS
inline const bool logf_wired = (cheats::logf_hook = logf, true);
#endif
inline const bool texts_log_wired = (visuals::texts_log = logf, true);

inline std::atomic<bool> g_open{false};
inline DWORD g_last_scan = 0;
inline HWND g_hwnd;
inline const UINT WM_FATRAINER = WM_APP + 0x4F;
inline std::mutex g_qmx;
inline std::vector<std::function<void()>> g_queue;
inline std::string g_toast;
inline DWORD g_toast_at = 0;
const DWORD TOAST_MS = 3500;

inline void toast(const std::string& s) {
    logf("toast: %s", s.c_str());
    std::lock_guard<std::mutex> l(g_qmx);
    g_toast = s;
    g_toast_at = GetTickCount();
}

inline bool toast_showing() {
    std::lock_guard<std::mutex> l(g_qmx);
    return !g_toast.empty() && GetTickCount() - g_toast_at < TOAST_MS;
}

inline void on_game_thread(std::function<void()> f) {
    {
        std::lock_guard<std::mutex> l(g_qmx);
        g_queue.push_back(std::move(f));
    }
    PostMessageW(g_hwnd, WM_FATRAINER, 0, 0);
}

inline void drain_queue() {
    std::vector<std::function<void()>> q;
    {
        std::lock_guard<std::mutex> l(g_qmx);
        q.swap(g_queue);
    }
    for (auto& f : q) f();
}

inline void request_refresh() {
    g_last_scan = GetTickCount();
    std::thread([] {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
        game::refresh();
        std::lock_guard<std::mutex> l(game::mx);
        logf("refresh: %s", game::g.status.c_str());
#if !FATRAINER_VISUALS
        cheats::player = cheats::find_player();
        logf("inventories:%s", game::inventory_report().c_str());
        if (cheats::sections_wanted.exchange(false))
            on_game_thread([] {
                std::lock_guard<std::mutex> l(game::mx);
                cheats::read_sections();
            });
        logf("stats: %s", game::stat_report().c_str());
        logf("cheats: %s", cheats::describe().c_str());
#else
        logf("hud screens: %zu", game::g.huds.size());
#endif
    }).detach();
}

inline std::string route_path() {
    std::string ini = config::default_path();
    return ini.substr(0, ini.find_last_of("\\/") + 1) + "fatrainer_route.txt";
}

inline void save_config() {
#if !FATRAINER_VISUALS
    auto p = cheats::current_profile();
    config::cfg.cheats_on = p.cheats;
    config::cfg.tweaks = p.tweaks;
#endif
    config::save(config::default_path());
}
