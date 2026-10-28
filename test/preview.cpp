#include <d3d11.h>
#include <cstdio>
#include <string>
#include <vector>
#include "fake.h"

static void save_frame(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc, const char* path) {
    ID3D11Texture2D *bb, *copy;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    D3D11_TEXTURE2D_DESC d;
    bb->GetDesc(&d);
    d.Usage = D3D11_USAGE_STAGING, d.BindFlags = 0, d.CPUAccessFlags = D3D11_CPU_ACCESS_READ, d.MiscFlags = 0;
    dev->CreateTexture2D(&d, nullptr, &copy);
    ctx->CopyResource(copy, bb);
    D3D11_MAPPED_SUBRESOURCE m;
    ctx->Map(copy, 0, D3D11_MAP_READ, 0, &m);
    std::string tmp = std::string(path) + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    unsigned w = d.Width, h = d.Height, size = w * h * 4;
    unsigned char head[54] = {'B', 'M'};
    auto put = [&](int at, unsigned v) { for (int i = 0; i < 4; i++) head[at + i] = (unsigned char)(v >> (8 * i)); };
    put(2, 54 + size), put(10, 54), put(14, 40), put(18, w), put(22, (unsigned)-(int)h), put(26, 1 | (32 << 16));
    fwrite(head, 1, 54, f);
    std::vector<unsigned char> line(w * 4);
    for (unsigned y = 0; y < h; y++) {
        unsigned char* row = (unsigned char*)m.pData + y * m.RowPitch;
        for (unsigned x = 0; x < w; x++) line[x * 4] = row[x * 4 + 2], line[x * 4 + 1] = row[x * 4 + 1], line[x * 4 + 2] = row[x * 4], line[x * 4 + 3] = 255;
        fwrite(line.data(), 1, line.size(), f);
    }
    fclose(f);
    MoveFileExA(tmp.c_str(), path, MOVEFILE_REPLACE_EXISTING);
    ctx->Unmap(copy, 0);
    copy->Release();
    bb->Release();
}

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
        static const char* shot = getenv("DLT_SHOT");
        static DWORD started = GetTickCount(), saved = 0;
        if (shot && GetTickCount() - started > 1500 && GetTickCount() - saved > 400) {
            save_frame(dev, ctx, sc, shot);
            saved = GetTickCount();
        }
        Sleep(16);
    }
    return 0;
}
