#include <d3d11.h>
#include "fake.h"

int main(int argc, char** argv) {
    HMODULE gd = LoadLibraryExA(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    game::resolve_classes((uintptr_t)gd);
    static fake::World w;
    w.build();
    SetEnvironmentVariableA("FATRAINER_ARMED", "1");
    LoadLibraryA(argv[2]);

    WNDCLASSA wc{};
    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "preview";
    RegisterClassA(&wc);
    HWND hw = CreateWindowA("preview", "preview", WS_POPUP | WS_VISIBLE, 0, 0, 1280, 900, nullptr, nullptr, nullptr, nullptr);
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hw;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    IDXGISwapChain* sc;
    ID3D11Device* dev;
    ID3D11DeviceContext* ctx;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd,
                                             &sc, &dev, nullptr, &ctx)))
        return 2;
    for (int f = 0; f < 60 * 16; f++) {
        MSG m;
        while (PeekMessageA(&m, nullptr, 0, 0, PM_REMOVE)) DispatchMessageA(&m);
        ID3D11Texture2D* bb;
        ID3D11RenderTargetView* rtv;
        sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
        dev->CreateRenderTargetView(bb, nullptr, &rtv);
        float bg[4] = {0.23f, 0.27f, 0.21f, 1};
        ctx->ClearRenderTargetView(rtv, bg);
        rtv->Release();
        bb->Release();
        sc->Present(1, 0);
        Sleep(16);
    }
    return 0;
}
