#pragma once
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <ctime>
#include <string>
#include <thread>

namespace crash {

const size_t REPORT_TAIL = 24000;
const int WIDTH = 820, HEIGHT = 560, MARGIN = 14, BUTTON_W = 140, BUTTON_H = 32, ID_COPY = 1, ID_CLOSE = 2;
inline const char SESSION_MARK[] = "--- FaTrainer", SHOWN_MARK[] = "crash: dialog shown", SESSION_TIME[] = " at ";
inline const wchar_t CLASS_NAME[] = L"FaTrainerCrashLog";

inline std::wstring widen(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    if (n) MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    std::wstring out;
    for (wchar_t c : w) {
        if (c == L'\n' && (out.empty() || out.back() != L'\r')) out += L'\r';
        out += c;
    }
    return out;
}

inline std::string read_text(const std::string& path) {
    std::string out;
    if (FILE* f = fopen(path.c_str(), "rb")) {
        char b[8192];
        size_t n;
        while ((n = fread(b, 1, sizeof b, f)) > 0) out.append(b, n);
        fclose(f);
    }
    return out;
}

inline std::string tail(const std::string& text, size_t limit) {
    if (text.size() <= limit) return text;
    size_t from = text.find('\n', text.size() - limit);
    return text.substr(from == std::string::npos ? text.size() - limit : from + 1);
}

inline std::string previous_session(const std::string& log) {
    size_t last = log.rfind(SESSION_MARK);
    if (last == std::string::npos || last == 0) return "";
    size_t before = log.rfind(SESSION_MARK, last - 1);
    return before == std::string::npos ? "" : log.substr(before, last - before);
}

inline long long session_time(const std::string& session) {
    size_t eol = session.find('\n'), at = session.rfind(SESSION_TIME, eol);
    return at == std::string::npos ? 0 : atoll(session.c_str() + at + strlen(SESSION_TIME));
}

inline long long unix_time(const FILETIME& t) {
    ULARGE_INTEGER u{{t.dwLowDateTime, t.dwHighDateTime}};
    const unsigned long long EPOCH = 116444736000000000ull, TICKS = 10000000ull;
    return u.QuadPart < EPOCH ? 0 : (long long)((u.QuadPart - EPOCH) / TICKS);
}

inline long long newest_dump(const std::string& folder) {
    WIN32_FIND_DATAA d;
    HANDLE h = FindFirstFileA((folder + "dumps\\*.dmp").c_str(), &d);
    if (h == INVALID_HANDLE_VALUE) return 0;
    long long newest = 0;
    do newest = std::max(newest, unix_time(d.ftLastWriteTime));
    while (FindNextFileA(h, &d));
    FindClose(h);
    return newest;
}

inline bool crashed_after(const std::string& session, long long dump) { return session.find(SHOWN_MARK) == std::string::npos && session_time(session) && dump >= session_time(session); }

inline void copy_text(HWND owner, const std::wstring& text) {
    if (!OpenClipboard(owner)) return;
    EmptyClipboard();
    if (HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t))) {
        memcpy(GlobalLock(mem), text.c_str(), (text.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(mem);
        if (!SetClipboardData(CF_UNICODETEXT, mem)) GlobalFree(mem);
    }
    CloseClipboard();
}

struct Window { std::wstring intro, report; HWND edit = nullptr, copy = nullptr, close = nullptr; HFONT font = nullptr, mono = nullptr; };

inline LRESULT CALLBACK window_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    auto* self = (Window*)GetWindowLongPtrW(w, GWLP_USERDATA);
    switch (msg) {
    case WM_CREATE: {
        self = (Window*)((CREATESTRUCTW*)lp)->lpCreateParams;
        SetWindowLongPtrW(w, GWLP_USERDATA, (LONG_PTR)self);
        HINSTANCE inst = ((CREATESTRUCTW*)lp)->hInstance;
        self->font = CreateFontW(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        self->mono = CreateFontW(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
        HWND intro = CreateWindowExW(0, L"STATIC", self->intro.c_str(), WS_CHILD | WS_VISIBLE, MARGIN, MARGIN, WIDTH - 3 * MARGIN, 44, w, nullptr, inst, nullptr);
        self->edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", self->report.c_str(), WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, MARGIN,
                                     MARGIN + 52, WIDTH - 3 * MARGIN, HEIGHT - 4 * MARGIN - 52 - BUTTON_H - 40, w, nullptr, inst, nullptr);
        self->copy = CreateWindowExW(0, L"BUTTON", L"Copy log", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, MARGIN, HEIGHT - BUTTON_H - 3 * MARGIN - 28, BUTTON_W, BUTTON_H, w,
                                     (HMENU)(INT_PTR)ID_COPY, inst, nullptr);
        self->close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE, MARGIN * 2 + BUTTON_W, HEIGHT - BUTTON_H - 3 * MARGIN - 28, BUTTON_W, BUTTON_H, w,
                                      (HMENU)(INT_PTR)ID_CLOSE, inst, nullptr);
        for (HWND c : {intro, self->copy, self->close}) SendMessageW(c, WM_SETFONT, (WPARAM)self->font, TRUE);
        SendMessageW(self->edit, WM_SETFONT, (WPARAM)self->mono, TRUE);
        SendMessageW(self->edit, EM_SETSEL, (WPARAM)self->report.size(), (LPARAM)self->report.size());
        SendMessageW(self->edit, EM_SCROLLCARET, 0, 0);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == ID_COPY) {
            copy_text(w, self->report);
            SetWindowTextW(self->copy, L"Copied");
        } else if (LOWORD(wp) == ID_CLOSE) DestroyWindow(w);
        return 0;
    case WM_DESTROY:
        if (self) DeleteObject(self->font), DeleteObject(self->mono);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(w, msg, wp, lp);
}

inline void run_window(const std::string& title, const std::string& intro, const std::string& report) {
    HINSTANCE inst = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.lpfnWndProc = window_proc, wc.hInstance = inst, wc.lpszClassName = CLASS_NAME, wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);
    Window self{widen(intro), widen(report)};
    int x = (GetSystemMetrics(SM_CXSCREEN) - WIDTH) / 2, y = (GetSystemMetrics(SM_CYSCREEN) - HEIGHT) / 2;
    HWND w = CreateWindowExW(WS_EX_TOPMOST, CLASS_NAME, widen(title).c_str(), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, x, y, WIDTH, HEIGHT, nullptr, nullptr, inst,
                             &self);
    if (!w) return;
    SetForegroundWindow(w);
    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        if (m.message == WM_KEYDOWN && m.wParam == 'C' && (GetKeyState(VK_CONTROL) & 0x8000)) copy_text(w, self.report);
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
}

inline void show(const std::string& title, const std::string& intro, const std::string& report, bool wait) {
    std::thread t([=] { run_window(title, intro, report); });
    if (wait) t.join();
    else t.detach();
}

}
