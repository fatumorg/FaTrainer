#include <windows.h>
#include <tlhelp32.h>
#include <d3d11.h>
#include <dxgi.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <functional>
#include <string>
#include <thread>
#include "input.h"
#include "menu.h"
#include "crash.h"
#if FATRAINER_DEV
#include "probe.h"
#endif
#if !FATRAINER_VISUALS
#include "net_win.h"
#include "../installer/logic.h"
#endif
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
ImGuiKey ImGui_ImplWin32_KeyEventToImGuiKey(WPARAM wParam, LPARAM lParam);

void dlt_assert_fail(const char* expr, const char* file, int line) {
    static std::atomic<int> n{0};
    if (n++ < 20) logf("imgui assert: %s (%s:%d)", expr, file, line);
}

static FARPROC real(const char* name) {
    static HMODULE h = [] {
        char p[MAX_PATH];
        GetSystemDirectoryA(p, MAX_PATH);
        strcat(p, "\\xinput1_3.dll");
        return LoadLibraryA(p);
    }();
    return h ? GetProcAddress(h, name) : nullptr;
}
#define FWD(ret, name, fail, params, args)                  \
    extern "C" ret WINAPI name params {                     \
        static auto fn_ = (ret(WINAPI*) params)real(#name); \
        return fn_ ? fn_ args : fail;                       \
    }
FWD(DWORD, XInputGetState, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, void* s), (i, s))
FWD(DWORD, XInputSetState, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, void* s), (i, s))
FWD(DWORD, XInputGetCapabilities, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, DWORD f, void* c), (i, f, c))
FWD(DWORD, XInputGetDSoundAudioDeviceGuids, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, GUID* a, GUID* b), (i, a, b))
FWD(DWORD, XInputGetBatteryInformation, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, BYTE t, void* b), (i, t, b))
FWD(DWORD, XInputGetKeystroke, ERROR_DEVICE_NOT_CONNECTED, (DWORD i, DWORD r, void* k), (i, r, k))
extern "C" void WINAPI XInputEnable(BOOL e) {
    static auto fn_ = (void(WINAPI*)(BOOL))real("XInputEnable");
    if (fn_) fn_(e);
}

using PresentFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using ResizeFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
static PresentFn oPresent;
static ResizeFn oResize;
static ID3D11Device* g_dev;
static ID3D11DeviceContext* g_ctx;
static WNDPROC oWndProc;
static bool g_ready;
static std::atomic<DWORD> g_wndproc_at{0};
static std::atomic<long> g_dx{0}, g_dy{0}, g_wheel{0};
static std::atomic<long> g_frames{0};
static std::string g_outdated;
static std::atomic<int> g_build = menu::NOTICE_UPDATE;

static void on_raw_input(LPARAM l) {
    RAWINPUT ri;
    UINT sz = sizeof ri;
    if (GetRawInputData((HRAWINPUT)l, RID_INPUT, &ri, &sz, sizeof(RAWINPUTHEADER)) == (UINT)-1) return;
    if (ri.header.dwType != RIM_TYPEMOUSE) return;
    auto& m = ri.data.mouse;
    if (!(m.usFlags & MOUSE_MOVE_ABSOLUTE)) { g_dx += m.lLastX; g_dy += m.lLastY; }
    if (m.usButtonFlags & RI_MOUSE_WHEEL) g_wheel += (SHORT)m.usButtonData;
}

static LRESULT CALLBACK hkWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    static bool logged = false;
    if (!logged) { logged = true; logf("window thread %lu", (unsigned long)GetCurrentThreadId()); }
    g_wndproc_at = GetTickCount();
    drain_queue();
    if (m == WM_FATRAINER) return 0;
    if (g_open) {
        switch (m) {
            case WM_INPUT: on_raw_input(l); return DefWindowProcW(h, m, w, l);
            case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP: case WM_CHAR:
                return 0;
            case WM_MOUSEWHEEL:
                g_wheel += GET_WHEEL_DELTA_WPARAM(w);
                return 0;
            case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_RBUTTONDOWN: case WM_RBUTTONUP:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_LBUTTONDBLCLK: case WM_RBUTTONDBLCLK: case WM_MOUSEHWHEEL:
                return 0;
        }
    }
    return CallWindowProcW(oWndProc, h, m, w, l);
}

static bool g_just_opened = false;

static void feed_mouse() {
    ImGuiIO& io = ImGui::GetIO();
    static float vx = -1, vy = -1;
    static POINT last_os{-1, -1};
    static bool btn[3];
    static bool held_at_open[3];
    RECT cr;
    GetClientRect(g_hwnd, &cr);
    if (vx < 0) { vx = cr.right / 2.0f; vy = cr.bottom / 2.0f; }
    RECT clip;
    GetClipCursor(&clip);
    bool locked = (clip.right - clip.left) <= 8 || (clip.bottom - clip.top) <= 8;
    POINT p;
    GetCursorPos(&p);
    long dx = g_dx.exchange(0), dy = g_dy.exchange(0);
    if (!locked && (p.x != last_os.x || p.y != last_os.y)) {
        POINT c = p;
        ScreenToClient(g_hwnd, &c);
        vx = (float)c.x, vy = (float)c.y;
    } else {
        vx += dx, vy += dy;
    }
    last_os = p;
    vx = std::clamp(vx, 0.0f, (float)cr.right - 1), vy = std::clamp(vy, 0.0f, (float)cr.bottom - 1);
    io.AddMousePosEvent(vx, vy);
    const int vk[3] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON};
    for (int i = 0; i < 3; i++) {
        bool down = GetAsyncKeyState(vk[i]) & 0x8000;
        if (g_just_opened) held_at_open[i] = down;
        if (held_at_open[i]) {
            held_at_open[i] = down;
            continue;
        }
        if (down != btn[i]) io.AddMouseButtonEvent(i, btn[i] = down);
    }
    long wh = g_wheel.exchange(0), direct = input::wheel.exchange(0);
    if (!wh) wh = direct;
    if (wh) io.AddMouseWheelEvent(0, wh / (float)WHEEL_DELTA);
}

static bool typing_key(int vk) {
    return (vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z') || (vk >= VK_NUMPAD0 && vk <= VK_DIVIDE) || (vk >= VK_OEM_1 && vk <= VK_OEM_3) ||
           (vk >= VK_OEM_4 && vk <= VK_OEM_8) || vk == VK_SPACE || vk == VK_BACK || vk == VK_DELETE || vk == VK_RETURN ||
           vk == VK_ESCAPE || vk == VK_TAB || (vk >= VK_PRIOR && vk <= VK_DOWN);
}

static void feed_keyboard() {
    ImGuiIO& io = ImGui::GetIO();
    static bool down[256];
    bool shift = GetAsyncKeyState(VK_SHIFT) & 0x8000, ctrl = GetAsyncKeyState(VK_CONTROL) & 0x8000;
    io.AddKeyEvent(ImGuiMod_Shift, shift);
    io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
    for (int vk = 8; vk < 256; vk++) {
        if (!typing_key(vk)) continue;
        bool d = GetAsyncKeyState(vk) & 0x8000;
        if (g_just_opened) {
            if (down[vk]) io.AddKeyEvent(ImGui_ImplWin32_KeyEventToImGuiKey(vk, (LPARAM)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC) << 16), false);
            down[vk] = d;
            continue;
        }
        if (d == down[vk]) continue;
        down[vk] = d;
        UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        ImGuiKey key = ImGui_ImplWin32_KeyEventToImGuiKey(vk, (LPARAM)scan << 16);
        if (key != ImGuiKey_None) io.AddKeyEvent(key, d);
        if (!d || ctrl) continue;
        BYTE state[256] = {};
        if (shift) state[VK_SHIFT] = 0x80;
        WCHAR text[4];
        int n = ToUnicode(vk, scan, state, text, 4, 0);
        for (int i = 0; i < n; i++)
            if (text[i] >= 32) io.AddInputCharacterUTF16(text[i]);
    }
}

static void init_imgui(IDXGISwapChain* sc) {
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&g_dev))) return logf("overlay: swap chain has no d3d11 device");
    g_dev->GetImmediateContext(&g_ctx);
    DXGI_SWAP_CHAIN_DESC d;
    sc->GetDesc(&d);
    g_hwnd = d.OutputWindow;
    logf("overlay: device ok, window %p, %ux%u, windowed %d", (void*)g_hwnd, d.BufferDesc.Width, d.BufferDesc.Height, (int)d.Windowed);
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;
    menu::load_fonts();
    menu::apply_style();
    logf("overlay: imgui ok");
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);
    logf("overlay: backends ok");
    oWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
    g_ready = true;
    logf("overlay ready (window %p, render thread %lu)", (void*)g_hwnd, (unsigned long)GetCurrentThreadId());
}

static void keep_window_hooked() {
    const DWORD SILENT_MS = 1000;
    if (GetTickCount() - g_wndproc_at < SILENT_MS) return;
    if ((WNDPROC)GetWindowLongPtrW(g_hwnd, GWLP_WNDPROC) == hkWndProc) return;
    oWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
    g_wndproc_at = GetTickCount();
    logf("overlay: the game replaced the window procedure, hooking it again");
    PostMessageW(g_hwnd, WM_FATRAINER, 0, 0);
}

#if !FATRAINER_VISUALS
static const BYTE SPIT_GAME_KEYS[config::SPIT_KEYS] = {DIK_1, DIK_2, DIK_3, DIK_4};

static void poll_spit_keys() {
    static bool held[config::SPIT_KEYS] = {};
    const DWORD SPIT_PRESS_MS = 120;
    for (int i = 0; i < config::SPIT_KEYS; i++) {
        int vk = config::cfg.spit_keys[i];
        bool down = vk && (GetAsyncKeyState(vk) & 0x8000);
        if (down && !held[i] && !g_open) input::press(SPIT_GAME_KEYS[i], SPIT_PRESS_MS);
        held[i] = down;
    }
}

static void poll_cheat_keys() {
    static std::map<std::string, bool> held;
    for (auto& [key, vk] : config::cfg.cheat_keys) {
        bool down = GetAsyncKeyState(vk) & 0x8000;
        bool pressed = !held[key] && down && !g_open;
        held[key] = down;
        if (pressed && key == REFILL_KEY) {
            on_game_thread([] {
                std::lock_guard<std::mutex> l(game::mx);
                cheats::refill();
            });
            toast("Health & stamina refilled");
            continue;
        }
        auto* cheat = pressed ? cheats::find(key) : nullptr;
        if (!cheat) continue;
        cheat->on = !cheat->on;
        save_config();
        toast(std::string(cheat->label) + (cheat->on ? " on" : " off"));
    }
}

#endif

static void begin_passive_frame(IDXGISwapChain* sc) {
    ImGuiIO& io = ImGui::GetIO();
    DXGI_SWAP_CHAIN_DESC d{};
    sc->GetDesc(&d);
    io.DisplaySize = {(float)d.BufferDesc.Width, (float)d.BufferDesc.Height};
    io.DeltaTime = 1.0f / 60.0f;
}

static void render_overlay(IDXGISwapChain* sc) {
    ImGui::Render();
    ID3D11Texture2D* bb = nullptr;
    if (SUCCEEDED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb))) {
        ID3D11RenderTargetView* rtv = nullptr;
        if (SUCCEEDED(g_dev->CreateRenderTargetView(bb, nullptr, &rtv))) {
            g_ctx->OMSetRenderTargets(1, &rtv, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            rtv->Release();
        }
        bb->Release();
    }
}

static float update_notice_alpha() {
    const DWORD NOTICE_MS = 20000, FADE_IN_MS = 400, FADE_OUT_MS = 800;
    static DWORD shown_at = GetTickCount();
    static bool prev = false;
    bool key = (GetAsyncKeyState(config::cfg.menu_key) | GetAsyncKeyState(VK_F8)) & 0x8000;
    if (key && !prev && !g_open) shown_at = GetTickCount();
    prev = key;
    DWORD age = GetTickCount() - shown_at;
    if (age >= NOTICE_MS) return 0;
    return std::min({1.0f, age / (float)FADE_IN_MS, (NOTICE_MS - age) / (float)FADE_OUT_MS});
}

static bool trainer_off() { return !g_outdated.empty() && g_build == menu::NOTICE_UPDATE && !config::cfg.allow_older; }

static HRESULT present_update_notice(IDXGISwapChain* sc, UINT sync, UINT flags) {
    float alpha = update_notice_alpha();
    if (alpha > 0) {
        if (!g_ready) init_imgui(sc);
        if (g_ready) {
            ImGui_ImplDX11_NewFrame();
            begin_passive_frame(sc);
            ImGui::NewFrame();
            menu::draw_update_notice(g_outdated, alpha, menu::NOTICE_UPDATE);
            render_overlay(sc);
        }
    }
    return oPresent(sc, sync, flags);
}

static void keep_crash_filter();

static HRESULT WINAPI hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (g_frames++ == 0) logf("first frame (thread %lu)", (unsigned long)GetCurrentThreadId());
    if (trainer_off()) return present_update_notice(sc, sync, flags);
    static bool prev = false;
    bool key = (GetAsyncKeyState(config::cfg.menu_key) | GetAsyncKeyState(VK_F8)) & 0x8000;
    if (key && !prev) g_open = !g_open, g_just_opened = g_open;
    prev = key;
    input::blocked = g_open.load();
    if (!g_ready) init_imgui(sc);
    if (g_ready) keep_window_hooked();
    static DWORD last_tick = 0;
    static std::atomic<bool> tick_queued{false};
    if (g_ready && GetTickCount() - last_tick > 100 && !tick_queued.exchange(true)) {
        last_tick = GetTickCount();
        keep_crash_filter();
        on_game_thread([] {
            tick_queued = false;
#if FATRAINER_VISUALS
            std::lock_guard<std::mutex> l(game::mx);
            visuals::install_var_hook(visuals::tools_global, false, nullptr);
            visuals::apply_uv_light(config::cfg.uv.on, config::cfg.uv.color, config::cfg.uv.glow);
#else
            cheats::tick();
#endif
#if FATRAINER_DEV
            probe::poll();
#endif
            visuals::keep_texts();
            visuals::keep_text_rows();
        });
    }
#if FATRAINER_VISUALS
    const DWORD VISUALS_RESCAN = 15000, VISUALS_RESCAN_MAX = 120000;
    static DWORD visuals_rescan = VISUALS_RESCAN;
    if (!g_ready || !visuals::looks_missing()) visuals_rescan = VISUALS_RESCAN;
    else if (GetTickCount() - g_last_scan > visuals_rescan && !game::g.scanning) visuals_rescan = std::min(visuals_rescan * 2, VISUALS_RESCAN_MAX), request_refresh();
    bool esp = false;
#else
    const DWORD MENU_RESCAN = 3000, CHEAT_RESCAN = 15000, CHEAT_RESCAN_MAX = 120000;
    static DWORD cheat_rescan = CHEAT_RESCAN;
    DWORD since_scan = GetTickCount() - g_last_scan;
    bool looks_missing = g_ready && visuals::looks_missing();
    if (!cheats::objects_missing && !looks_missing) cheat_rescan = CHEAT_RESCAN;
    if (cheats::respawned) {
        std::lock_guard<std::mutex> l(game::mx);
        if (!game::g.scanning) cheats::respawned = false, cheat_rescan = CHEAT_RESCAN, visuals::forget_hud(), request_refresh();
    }
    if ((g_open && since_scan > MENU_RESCAN) || ((cheats::objects_missing || looks_missing) && since_scan > cheat_rescan)) {
        std::lock_guard<std::mutex> l(game::mx);
        bool menu_needs = game::g.wallets.empty() || game::g.invs.empty() || game::g.descs.empty() || !cheats::player;
        bool cheats_need = (cheats::objects_missing || looks_missing) && since_scan > cheat_rescan;
        if (!game::g.scanning && ((g_open && menu_needs) || cheats_need)) {
            if (cheats_need) cheat_rescan = std::min(cheat_rescan * 2, CHEAT_RESCAN_MAX);
            request_refresh();
        }
    }
    poll_spit_keys();
    poll_cheat_keys();
    bool esp = config::cfg.esp.on;
    if (g_ready && esp) cheats::track_players();
    if (g_ready && !g_open && cheats::is_on("dodge_spit")) cheats::dodge_spits();
#endif
    if (g_ready) visuals::spin_rainbows(GetTickCount64() / 1000.0);
    if (g_ready) visuals::keep_hud(logf);
    bool previewing = g_ready && menu::previewing();
    float shown = g_ready ? menu::presence(g_open && !previewing) : 0;
    float notice = g_outdated.empty() ? 0 : update_notice_alpha();
    bool toast_only = g_ready && shown <= 0 && toast_showing();
    bool crosshair = g_ready && config::cfg.crosshair.on && visuals::game_crosshair_shown();
    if (g_ready && (shown > 0 || esp || notice > 0 || toast_only || crosshair || previewing)) {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        ImGui_ImplDX11_NewFrame();
        if (g_open) {
            ImGui_ImplWin32_NewFrame();
            feed_mouse();
            feed_keyboard();
            g_just_opened = false;
        } else {
            begin_passive_frame(sc);
        }
        ImGui::NewFrame();
        if (crosshair) menu::draw_crosshair_overlay();
#if !FATRAINER_VISUALS
        if (esp) menu::draw_esp();
#endif
        if (shown > 0) menu::draw(shown, g_open && !previewing);
        if (previewing) menu::draw_preview();
        if (toast_only) menu::draw_toast();
        if (notice > 0) menu::draw_update_notice(g_outdated, notice, g_build == menu::NOTICE_UPDATE ? menu::NOTICE_OLDER : g_build.load());
        render_overlay(sc);
    }
    return oPresent(sc, sync, flags);
}

static HRESULT WINAPI hkResize(IDXGISwapChain* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT fl) {
    return oResize(sc, n, w, h, f, fl);
}

static bool hook_d3d() {
    WNDCLASSEXA wc{sizeof wc};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "fatrainer_dummy";
    RegisterClassExA(&wc);
    HWND w = CreateWindowExA(0, wc.lpszClassName, "", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = w;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                               &sd, &sc, &dev, nullptr, &ctx);
    bool ok = SUCCEEDED(hr);
    if (ok) {
        void** vt = *(void***)sc;
        DWORD old;
        VirtualProtect(&vt[8], sizeof(void*) * 6, PAGE_EXECUTE_READWRITE, &old);
        oPresent = (PresentFn)vt[8];
        oResize = (ResizeFn)vt[13];
        vt[8] = (void*)hkPresent;
        vt[13] = (void*)hkResize;
        VirtualProtect(&vt[8], sizeof(void*) * 6, old, &old);
        sc->Release();
        ctx->Release();
        dev->Release();
    }
    DestroyWindow(w);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);
    logf("d3d11 hook: %s (hr=%08lx)", ok ? "ok" : "FAILED", (unsigned long)hr);
    return ok;
}

static std::string code_place(uintptr_t at) {
    HMODULE m = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)at, &m);
    char path[MAX_PATH] = "?";
    if (m) GetModuleFileNameA(m, path, MAX_PATH);
    const char* name = strrchr(path, '\\') ? strrchr(path, '\\') + 1 : path;
    char out[MAX_PATH + 32];
    snprintf(out, sizeof out, "%s+%llx", name, (unsigned long long)(at - (uintptr_t)m));
    return out;
}

static std::string call_stack(const CONTEXT* fault) {
    const int FRAMES = 12;
    CONTEXT c = *fault;
    std::string out;
    for (int i = 0; i < FRAMES && c.Rip; i++) {
        DWORD64 base = 0;
        auto* fn = RtlLookupFunctionEntry(c.Rip, &base, nullptr);
        if (i) out += " < ";
        out += code_place(c.Rip);
        if (fn) {
            void* handler_data = nullptr;
            DWORD64 frame = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, c.Rip, fn, &c, &handler_data, &frame, nullptr);
        } else {
            if (IsBadReadPtr((void*)c.Rsp, 8)) break;
            c.Rip = *(DWORD64*)c.Rsp, c.Rsp += 8;
        }
    }
    return out;
}

static std::string game_folder_path() {
    char exe[MAX_PATH];
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    std::string s = exe;
    return s.substr(0, s.find_last_of("\\/") + 1);
}

static const char CRASH_INTRO[] =
    "Dying Light crashed. Press Copy log and post the log in the FaTrainer forum (fatrainer.fatum.cc) or send it to contact@fatum.cc, so the crash can be fixed.";
static LPTOP_LEVEL_EXCEPTION_FILTER g_game_filter = nullptr;
static std::atomic<bool> g_crash_shown{false};

static LONG WINAPI crash_filter(EXCEPTION_POINTERS* e) {
    if (!g_crash_shown.exchange(true)) {
        char head[2048];
        snprintf(head, sizeof head, "exception %08lx in %s\nstack: %s\n\n", (unsigned long)e->ExceptionRecord->ExceptionCode,
                 code_place((uintptr_t)e->ExceptionRecord->ExceptionAddress).c_str(), call_stack(e->ContextRecord).c_str());
        logf("%s while the game crashed", crash::SHOWN_MARK);
        std::string log = crash::read_text(game_folder_path() + "fatrainer.log");
        size_t start = log.rfind(crash::SESSION_MARK);
        crash::show(std::string(TITLE) + ": the game crashed", CRASH_INTRO, head + crash::tail(start == std::string::npos ? log : log.substr(start), crash::REPORT_TAIL), true);
    }
    return g_game_filter ? g_game_filter(e) : EXCEPTION_CONTINUE_SEARCH;
}

static void keep_crash_filter() {
    LPTOP_LEVEL_EXCEPTION_FILTER previous = SetUnhandledExceptionFilter(crash_filter);
    if (previous && previous != crash_filter) g_game_filter = previous;
}

static void check_last_crash() {
    std::string folder = game_folder_path(), session = crash::previous_session(crash::read_text(folder + "fatrainer.log"));
    if (!crash::crashed_after(session, crash::newest_dump(folder))) return;
    logf("%s at the next start", crash::SHOWN_MARK);
    crash::show(std::string(TITLE) + ": the game crashed last time", CRASH_INTRO, crash::tail(session, crash::REPORT_TAIL), false);
}

static LONG CALLBACK crash_logger(EXCEPTION_POINTERS* e) {
    DWORD code = e->ExceptionRecord->ExceptionCode;
    const DWORD HEAP_CORRUPTION = 0xC0000374, STACK_BUFFER_OVERRUN = 0xC0000409, FAIL_FAST = 0xC0000602;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_STACK_OVERFLOW &&
        code != EXCEPTION_INT_DIVIDE_BY_ZERO && code != STACK_BUFFER_OVERRUN && code != HEAP_CORRUPTION && code != FAIL_FAST)
        return EXCEPTION_CONTINUE_SEARCH;
    std::string place = code_place((uintptr_t)e->ExceptionRecord->ExceptionAddress);
    bool system = !_strnicmp(place.c_str(), "kernelbase.dll", 14) || !_strnicmp(place.c_str(), "kernel32.dll", 12);
    if (system && code == EXCEPTION_ACCESS_VIOLATION) return EXCEPTION_CONTINUE_SEARCH;
    static std::atomic<int> n{0};
    if (n++ < 30) {
        logf("exception %08lx in %s (data %llx) thread %lu menu_open=%d  [may be handled by the game]", (unsigned long)code, place.c_str(),
             (unsigned long long)(e->ExceptionRecord->NumberParameters > 1 ? e->ExceptionRecord->ExceptionInformation[1] : 0),
             (unsigned long)GetCurrentThreadId(), (int)g_open.load());
        logf("  stack: %s", call_stack(e->ContextRecord).c_str());
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void log_environment() {
    using WineVersionFn = const char*(__cdecl*)();
    auto wine = (WineVersionFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version");
    using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOW*);
    OSVERSIONINFOW v{sizeof v};
    if (auto get = (RtlGetVersionFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion")) get(&v);
    logf("system: %s, Windows %lu.%lu build %lu", wine ? (std::string("Wine ") + wine()).c_str() : "Windows", v.dwMajorVersion,
         v.dwMinorVersion, v.dwBuildNumber);
    char windir[MAX_PATH];
    GetWindowsDirectoryA(windir, MAX_PATH);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) return;
    MODULEENTRY32 m{sizeof m};
    std::string extra;
    for (BOOL ok = Module32First(snap, &m); ok; ok = Module32Next(snap, &m))
        if (_strnicmp(m.szExePath, windir, strlen(windir))) extra += std::string(" ") + m.szModule;
    CloseHandle(snap);
    logf("modules outside Windows:%s", extra.c_str());
}

static void watch_first_frame() {
    for (int seconds = 10; seconds <= 60 && !g_frames; seconds += 10) {
        Sleep(10000);
        if (g_frames) break;
        std::string windows;
        EnumWindows([](HWND w, LPARAM out) -> BOOL {
            DWORD pid;
            GetWindowThreadProcessId(w, &pid);
            if (pid != GetCurrentProcessId() || !IsWindowVisible(w)) return TRUE;
            char title[64] = "";
            GetWindowTextA(w, title, sizeof title);
            *(std::string*)out += std::string(" [") + title + (IsHungAppWindow(w) ? ", not responding]" : ", responding]");
            return TRUE;
        }, (LPARAM)&windows);
        logf("no frame yet after %d s, game windows:%s", seconds, windows.empty() ? " none" : windows.c_str());
        if (seconds == 30) log_environment();
    }
}

#if !FATRAINER_VISUALS
static std::string g_release_base = RELEASE_DOWNLOADS;

static ReleaseInfo latest_release() {
    std::string text, error;
    if (!net::get_text(std::string(RELEASE_DOWNLOADS) + "version.txt", text, error)) {
        logf("update: could not check for a new version (%s), the trainer stays on", error.c_str());
        return {};
    }
    ReleaseInfo latest = parse_release_info(text);
    logf("update: latest release %s, this is %s", latest.version.empty() ? "unknown" : latest.version.c_str(), VERSION);
    config::load(config::default_path());
    std::string beta_text;
    if (config::cfg.beta && net::get_text(std::string(BETA_DOWNLOADS) + "version.txt", beta_text, error)) {
        ReleaseInfo beta = parse_release_info(beta_text);
        logf("update: beta builds are on, beta build %s", beta.version.empty() ? "unknown" : beta.version.c_str());
        if (beta.sha256.size() == 64 && newer_version(beta.version, latest.version)) latest = beta, g_release_base = BETA_DOWNLOADS;
    }
    return latest;
}

static std::wstring own_path() {
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&own_path, &self);
    wchar_t path[MAX_PATH] = L"";
    GetModuleFileNameW(self, path, MAX_PATH);
    return path;
}

static void download_build(ReleaseInfo latest) {
    const unsigned long long LIMIT = 64ull << 20;
    std::string data, error;
    bool ok = net::get(g_release_base + "xinput1_3.dll", [&](const char* d, size_t n, unsigned long long) {
        data.append(d, n);
        return data.size() <= LIMIT;
    }, error);
    if (ok && ((latest.size && data.size() != latest.size) || logic::sha256_hex(data) != latest.sha256)) ok = false, error = "the download did not match the release checksum";
    std::wstring path = own_path(), old = path + L".old";
    if (ok && !MoveFileExW(path.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING)) ok = false, error = "could not move the running DLL aside";
    if (ok) {
        FILE* f = _wfopen(path.c_str(), L"wb");
        ok = f && fwrite(data.data(), 1, data.size(), f) == data.size();
        if (f) fclose(f);
        if (!ok) DeleteFileW(path.c_str()), MoveFileExW(old.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING), error = "could not write the new DLL";
    }
    if (ok) logf("update: build %s downloaded, it starts with the next game start", latest.version.c_str());
    else logf("update: build %s could not be installed (%s), press Play in FaTrainer to update", latest.version.c_str(), error.c_str());
    g_build = ok ? menu::NOTICE_BUILD_READY : menu::NOTICE_BUILD_FAILED;
}

#endif

static void main_thread() {
    logf("--- %s %s, %s loaded (%s)%s%lld", TITLE, VERSION, FATRAINER_EDITION, EDITION_TAG, crash::SESSION_TIME, (long long)time(nullptr));
    check_last_crash();
    keep_crash_filter();
    log_environment();
#if FATRAINER_VISUALS
    HMODULE gamedll = nullptr;
    while (!(gamedll = GetModuleHandleA("gamedll_x64_rwdi.dll"))) Sleep(200);
    if (!game::resolve_classes((uintptr_t)gamedll)) logf("ERROR: %s", game::g.status.c_str());
    menu::startup();
    HMODULE engine = GetModuleHandleA("engine_x64_rwdi.dll");
    visuals::locate((uintptr_t)gamedll);
    logf("hud colors: %s, color hooks %d/5", visuals::locate_engine(engine) && visuals::hud_element_offset >= 0 ? "ready" : "not found", visuals::install_color_hooks(engine, gamedll));
    logf("fov hooks: %s", visuals::install_fov_hooks(gamedll) ? "ok" : "not found");
    visuals::locate_texts((uintptr_t)gamedll, engine);
    logf("texts: %s", visuals::texts_api.find && visuals::tools_global ? "ready" : "not found");
    logf("input blocking: %d directinput vtables", input::install(engine));
    if (getenv("DLT_OPEN")) g_open = true;
    Sleep(4000);
    visuals::learn_steam_language();
    logf("language: %s", visuals::language_report().c_str());
    hook_d3d();
    watch_first_frame();
}
#else
    DeleteFileW((own_path() + L".old").c_str());
    ReleaseInfo release = latest_release();
    const std::string& latest = release.version;
    HMODULE gamedll = nullptr;
    while (!(gamedll = GetModuleHandleA("gamedll_x64_rwdi.dll"))) Sleep(200);
    if (newer_version(latest, VERSION)) {
        config::load(config::default_path());
        g_outdated = latest;
        if (same_version(latest, VERSION) && release.sha256.size() == 64) {
            logf("update: build %s is out, downloading it, this build keeps running", latest.c_str());
            g_build = menu::NOTICE_BUILD_DOWNLOADING;
            std::thread(download_build, release).detach();
        } else if (!config::cfg.allow_older) {
            logf("update: FaTrainer %s is out, this version stays off until you press Play in FaTrainer", latest.c_str());
            Sleep(4000);
            hook_d3d();
            return;
        } else {
            logf("update: FaTrainer %s is out, this version keeps running because Use older versions is on", latest.c_str());
        }
    }
    if (game::resolve_classes((uintptr_t)gamedll))
        logf("classes: money +%llx, inventory +%llx, item manager +%llx", (unsigned long long)(game::g.vt_money - game::g.base),
             (unsigned long long)(game::g.vt_inv[0] - game::g.base), (unsigned long long)(game::g.vt_manager - game::g.base));
    else
        logf("ERROR: %s", game::g.status.c_str());
    menu::startup();
    cheats::locate((uintptr_t)gamedll);
    cheats::install_update_hooks();
    logf("settings cache hook: %s", cheats::install_cache_hook() ? "ok" : cheats::cache_get_fn ? "unexpected code, skipped" : "not found");
    logf("kick filter hook: %s", cheats::install_grab_filter_hook() ? "ok" : "not found");
    logf("cheats: %s", cheats::describe().c_str());
    HMODULE engine = GetModuleHandleA("engine_x64_rwdi.dll");
    cheats::locate_engine(engine);
    visuals::locate((uintptr_t)gamedll);
    logf("hud colors: %s, color hooks %d/5", visuals::locate_engine(engine) && visuals::hud_element_offset >= 0 ? "ready" : "not found", visuals::install_color_hooks(engine, gamedll));
    logf("fov hooks: %s", visuals::install_fov_hooks(gamedll) ? "ok" : "not found");
    visuals::locate_texts((uintptr_t)gamedll, engine);
    logf("texts: %s", visuals::texts_api.find && visuals::tools_global ? "ready" : "not found");
    logf("input blocking: %d directinput vtables", input::install(engine));
    if (getenv("DLT_OPEN")) g_open = true;
    std::thread([] {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
        const DWORD ENEMY_RESCAN = 3000, PLAYER_RESCAN = 10000;
        DWORD last_players = 0;
        for (;;) {
            bool players = (config::cfg.esp.on || cheats::is_on("one_hit_hunter") || cheats::human_targeting_set()) && GetTickCount() - last_players >= PLAYER_RESCAN;
            if (players) last_players = GetTickCount();
            cheats::scan_targets(cheats::is_on("one_hit"), players);
            Sleep(ENEMY_RESCAN);
        }
    }).detach();
    Sleep(4000);
    visuals::learn_steam_language();
    logf("language: %s", visuals::language_report().c_str());
    hook_d3d();
    watch_first_frame();
}
#endif

#if !FATRAINER_VISUALS
static std::wstring launch_file() {
    std::wstring path = own_path();
    return path.substr(0, path.find_last_of(L"\\/") + 1) + std::wstring(LAUNCH_FILE, LAUNCH_FILE + strlen(LAUNCH_FILE));
}

static bool armed(bool consume) {
    char flag[8] = "";
    if (GetEnvironmentVariableA("FATRAINER_ARMED", flag, sizeof flag) && !strcmp(flag, "1")) return true;
    std::wstring path = launch_file();
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) return false;
    long long written = 0;
    if (fscanf(f, "%lld", &written) != 1) written = 0;
    fclose(f);
    if (consume) DeleteFileW(path.c_str());
    return launch_armed(written, (long long)time(nullptr));
}

static bool launched_from_launcher() { return armed(true); }
#endif

static void choose_languages() {
#if !FATRAINER_VISUALS
    if (!armed(false)) return;
#endif
    char exe[MAX_PATH];
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    std::string ini = exe;
    ini = ini.substr(0, ini.find_last_of("\\/") + 1) + "fatrainer.ini";
    std::string game = config::peek(ini, "game_language");
    if (game.empty()) game = config::peek(ini, "text_language");
    visuals::install_language_hooks(game, config::peek(ini, "voice_language"));
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        choose_languages();
        CloseHandle(CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
#if !FATRAINER_VISUALS
            if (!launched_from_launcher()) {
                logf("launch: not started from FaTrainer, trainer off");
                return 0;
            }
            logf("launch: started from FaTrainer");
#endif
            AddVectoredExceptionHandler(1, crash_logger);
            main_thread();
            return 0;
        }, nullptr, 0, nullptr));
    }
    return TRUE;
}
