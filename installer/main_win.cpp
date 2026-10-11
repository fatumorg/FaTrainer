#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "ui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static ID3D11Device* device;
static ID3D11DeviceContext* context;
static IDXGISwapChain* swap_chain;
static ID3D11RenderTargetView* target;
static UINT resize_w, resize_h;

static void create_target() {
    ID3D11Texture2D* back = nullptr;
    swap_chain->GetBuffer(0, IID_PPV_ARGS(&back));
    device->CreateRenderTargetView(back, nullptr, &target);
    back->Release();
}

static bool create_device(HWND window) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = window;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    for (D3D_DRIVER_TYPE driver : {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP})
        if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, driver, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd, &swap_chain, &device, nullptr, &context))) {
            create_target();
            return true;
        }
    return false;
}

static int frame_size(HWND window) {
    UINT dpi = GetDpiForWindow(window);
    return GetSystemMetricsForDpi(SM_CXFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

static LRESULT hit_test(HWND window, LPARAM l) {
    POINT p{(short)LOWORD(l), (short)HIWORD(l)};
    ScreenToClient(window, &p);
    RECT r;
    GetClientRect(window, &r);
    int edge = IsZoomed(window) ? 0 : frame_size(window);
    bool left = p.x < edge, right = p.x >= r.right - edge, top = p.y < edge, bottom = p.y >= r.bottom - edge;
    if (top && left) return HTTOPLEFT;
    if (top && right) return HTTOPRIGHT;
    if (bottom && left) return HTBOTTOMLEFT;
    if (bottom && right) return HTBOTTOMRIGHT;
    if (left) return HTLEFT;
    if (right) return HTRIGHT;
    if (bottom) return HTBOTTOM;
    if (p.y < platform::caption_height) {
        for (const platform::Rect& hole : platform::caption_holes)
            if (p.x >= hole.x0 && p.x < hole.x1 && p.y >= hole.y0 && p.y < hole.y1) return HTCLIENT;
        return top ? HTTOP : HTCAPTION;
    }
    return HTCLIENT;
}

static LRESULT WINAPI window_proc(HWND window, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_NCCALCSIZE && w) {
        if (IsZoomed(window)) {
            RECT* r = &((NCCALCSIZE_PARAMS*)l)->rgrc[0];
            int inset = frame_size(window);
            r->left += inset, r->top += inset, r->right -= inset, r->bottom -= inset;
        }
        return 0;
    }
    if (msg == WM_NCHITTEST) return hit_test(window, l);
    if (ImGui_ImplWin32_WndProcHandler(window, msg, w, l)) return true;
    switch (msg) {
        case WM_SIZE:
            if (w != SIZE_MINIMIZED) resize_w = LOWORD(l), resize_h = HIWORD(l);
            return 0;
        case WM_GETMINMAXINFO: {
            float dpi = GetDpiForWindow(window) / 96.0f;
            ((MINMAXINFO*)l)->ptMinTrackSize = {(LONG)(900 * dpi), (LONG)(640 * dpi)};
            return 0;
        }
        case WM_DPICHANGED: {
            RECT* r = (RECT*)l;
            SetWindowPos(window, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        case WM_SYSCOMMAND:
            if ((w & 0xfff0) == SC_KEYMENU) return 0;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(window, msg, w, l);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    WNDCLASSEXW wc{sizeof wc};
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"FaTrainer";
    RegisterClassExW(&wc);
    platform::remove_old_self();
    POINT cursor{0, 0};
    GetCursorPos(&cursor);
    HWND focused = GetForegroundWindow();
    HMONITOR monitor = focused ? MonitorFromWindow(focused, MONITOR_DEFAULTTONULL) : nullptr;
    if (!monitor) monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{sizeof info};
    GetMonitorInfoW(monitor, &info);
    UINT dpi = GetDpiForSystem();
    float scale = dpi / 96.0f;
    int width = (int)(1120 * scale), height = (int)(740 * scale);
    RECT full = info.rcMonitor, work = info.rcWork;
    logic::Screen screen{{full.left, full.top, full.right - full.left, full.bottom - full.top}, {work.left, work.top, work.right - work.left, work.bottom - work.top}};
    logic::Area spot = logic::centered(screen, width, height);
    HWND window = CreateWindowExW(0, wc.lpszClassName, L"FaTrainer (" FATRAINER_EDITION ")", WS_OVERLAPPEDWINDOW, spot.x, spot.y, width, height, nullptr, nullptr, instance, nullptr);
    BOOL dark = TRUE;
    const DWORD IMMERSIVE_DARK_MODE = 20;
    DwmSetWindowAttribute(window, IMMERSIVE_DARK_MODE, &dark, sizeof dark);
    MARGINS shadow{0, 0, 1, 0};
    DwmExtendFrameIntoClientArea(window, &shadow);
    SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    if (!create_device(window)) {
        MessageBoxW(window, L"FaTrainer could not start Direct3D 11 on this PC.", L"FaTrainer", MB_ICONERROR);
        return 1;
    }
    platform::window = window;
    ShowWindow(window, show);
    UpdateWindow(window);
    ImGui::CreateContext();
    app::init(GetDpiForWindow(window) / 96.0f);
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, context);
    for (bool running = true; running;) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;
        if (IsIconic(window)) {
            Sleep(50);
            continue;
        }
        if (resize_w && resize_h) {
            target->Release();
            swap_chain->ResizeBuffers(0, resize_w, resize_h, DXGI_FORMAT_UNKNOWN, 0);
            resize_w = resize_h = 0;
            create_target();
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app::frame();
        ImGui::Render();
        const float ground[4] = {17 / 255.0f, 17 / 255.0f, 19 / 255.0f, 1};
        context->OMSetRenderTargets(1, &target, nullptr);
        context->ClearRenderTargetView(target, ground);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swap_chain->Present(1, 0);
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    target->Release();
    swap_chain->Release();
    context->Release();
    device->Release();
    DestroyWindow(window);
    return 0;
}
