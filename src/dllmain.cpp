#include <windows.h>
#include <tlhelp32.h>
#include <d3d11.h>
#include <dxgi.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <string>
#include <thread>
#include "font.h"
#include "input.h"
#include "menu.h"
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
static std::atomic<long> g_dx{0}, g_dy{0}, g_wheel{0};
static std::atomic<long> g_frames{0};

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

static void feed_mouse() {
    ImGuiIO& io = ImGui::GetIO();
    static float vx = -1, vy = -1;
    static POINT last_os{-1, -1};
    static bool btn[3];
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
    menu::load_fonts(FONT_TTF, sizeof FONT_TTF);
    menu::apply_style();
    logf("overlay: imgui ok");
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);
    logf("overlay: backends ok");
    oWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
    g_ready = true;
    logf("overlay ready (window %p, render thread %lu)", (void*)g_hwnd, (unsigned long)GetCurrentThreadId());
}

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

static HRESULT WINAPI hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (g_frames++ == 0) logf("first frame (thread %lu)", (unsigned long)GetCurrentThreadId());
    static bool prev = false;
    bool key = (GetAsyncKeyState(config::cfg.menu_key) | GetAsyncKeyState(VK_F8)) & 0x8000;
    if (key && !prev) g_open = !g_open;
    prev = key;
    input::blocked = g_open.load();
    if (!g_ready) init_imgui(sc);
    static DWORD last_tick = 0;
    static std::atomic<bool> tick_queued{false};
    if (g_ready && GetTickCount() - last_tick > 100 && !tick_queued.exchange(true)) {
        last_tick = GetTickCount();
        on_game_thread([] {
            tick_queued = false;
            cheats::tick();
        });
    }
    const DWORD MENU_RESCAN = 3000, CHEAT_RESCAN = 15000, CHEAT_RESCAN_MAX = 120000;
    static DWORD cheat_rescan = CHEAT_RESCAN;
    DWORD since_scan = GetTickCount() - g_last_scan;
    if (!cheats::objects_missing) cheat_rescan = CHEAT_RESCAN;
    if (cheats::respawned) {
        std::lock_guard<std::mutex> l(game::mx);
        if (!game::g.scanning) cheats::respawned = false, cheat_rescan = CHEAT_RESCAN, request_refresh();
    }
    if ((g_open && since_scan > MENU_RESCAN) || (cheats::objects_missing && since_scan > cheat_rescan)) {
        std::lock_guard<std::mutex> l(game::mx);
        bool menu_needs = game::g.wallets.empty() || game::g.invs.empty() || game::g.descs.empty() || !cheats::player;
        bool cheats_need = cheats::objects_missing && since_scan > cheat_rescan;
        if (!game::g.scanning && ((g_open && menu_needs) || cheats_need)) {
            if (cheats_need) cheat_rescan = std::min(cheat_rescan * 2, CHEAT_RESCAN_MAX);
            request_refresh();
        }
    }
    bool esp = false;
    if (g_ready && esp) cheats::track_players();
    if (g_ready && (g_open || esp)) {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = g_open;
        ImGui_ImplDX11_NewFrame();
        if (g_open) {
            ImGui_ImplWin32_NewFrame();
            feed_mouse();
            feed_keyboard();
        } else {
            DXGI_SWAP_CHAIN_DESC d{};
            sc->GetDesc(&d);
            io.DisplaySize = {(float)d.BufferDesc.Width, (float)d.BufferDesc.Height};
            io.DeltaTime = 1.0f / 60.0f;
        }
        ImGui::NewFrame();
        if (esp) menu::draw_esp();
        if (g_open) menu::draw();
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

static LONG CALLBACK crash_logger(EXCEPTION_POINTERS* e) {
    DWORD code = e->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_STACK_OVERFLOW &&
        code != EXCEPTION_INT_DIVIDE_BY_ZERO && code != 0xC0000409)
        return EXCEPTION_CONTINUE_SEARCH;
    void* at = e->ExceptionRecord->ExceptionAddress;
    HMODULE m = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)at, &m);
    char path[MAX_PATH] = "?";
    if (m) GetModuleFileNameA(m, path, MAX_PATH);
    const char* name = strrchr(path, '\\') ? strrchr(path, '\\') + 1 : path;
    if (!_stricmp(name, "kernelbase.dll") || !_stricmp(name, "kernel32.dll")) return EXCEPTION_CONTINUE_SEARCH;
    static std::atomic<int> n{0};
    if (n++ < 30)
        logf("exception %08lx in %s+%llx (data %llx) thread %lu menu_open=%d  [may be handled by the game]",
             (unsigned long)code, name, (unsigned long long)((uintptr_t)at - (uintptr_t)m),
             (unsigned long long)(e->ExceptionRecord->NumberParameters > 1 ? e->ExceptionRecord->ExceptionInformation[1] : 0),
             (unsigned long)GetCurrentThreadId(), (int)g_open.load());
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

static void main_thread() {
    logf("--- %s %s loaded", TITLE, VERSION);
    log_environment();
    HMODULE gamedll = nullptr;
    while (!(gamedll = GetModuleHandleA("gamedll_x64_rwdi.dll"))) Sleep(200);
    if (game::resolve_classes((uintptr_t)gamedll))
        logf("classes: money +%llx, inventory +%llx, item manager +%llx", (unsigned long long)(game::g.vt_money - game::g.base),
             (unsigned long long)(game::g.vt_inv[0] - game::g.base), (unsigned long long)(game::g.vt_manager - game::g.base));
    else
        logf("ERROR: %s", game::g.status.c_str());
    menu::startup();
    cheats::locate((uintptr_t)gamedll);
    cheats::install_update_hooks();
    logf("settings cache hook: %s", cheats::install_cache_hook() ? "ok" : cheats::cache_get_fn ? "unexpected code, skipped" : "not found");
    logf("cheats: %s", cheats::describe().c_str());
    HMODULE engine = GetModuleHandleA("engine_x64_rwdi.dll");
    cheats::locate_engine(engine);
    logf("input blocking: %d directinput vtables", input::install(engine));
    if (getenv("DLT_OPEN")) g_open = true;
    std::thread([] {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
        const DWORD ENEMY_RESCAN = 3000, PLAYER_RESCAN = 10000;
        DWORD last_players = 0;
        for (;;) {
            bool players = false;
            if (players) last_players = GetTickCount();
            cheats::scan_targets(cheats::is_on("one_hit"), players);
            Sleep(ENEMY_RESCAN);
        }
    }).detach();
    Sleep(4000);
    hook_d3d();
    watch_first_frame();
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        AddVectoredExceptionHandler(1, crash_logger);
        CloseHandle(CreateThread(nullptr, 0, [](LPVOID) -> DWORD { main_thread(); return 0; }, nullptr, 0, nullptr));
    }
    return TRUE;
}
