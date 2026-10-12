#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <vector>
#include "game.h"

namespace input {

using game::hook_import;

inline std::atomic<bool> blocked{false};
inline std::atomic<long> wheel{0};

using GetStateFn = HRESULT(WINAPI*)(void*, DWORD, void*);
using GetDataFn = HRESULT(WINAPI*)(void*, DWORD, DIDEVICEOBJECTDATA*, DWORD*, DWORD);
using AsyncKeyFn = SHORT(WINAPI*)(int);
using KeyboardFn = BOOL(WINAPI*)(BYTE*);

struct Patched { void** vtable; GetStateFn get_state; GetDataFn get_data; };
inline Patched patched[8];
inline int patched_count = 0;
inline std::mutex kinds_mx;
inline std::map<void*, bool> is_mouse;
inline AsyncKeyFn real_async_key, real_key_state;
inline KeyboardFn real_keyboard_state;

struct Injected { BYTE dik; DWORD until; bool down_sent, up_sent; };
inline std::mutex inject_mx;
inline std::vector<Injected> injected;
inline DWORD injected_sequence = 0x40000000;
const DWORD INJECT_FORGET = 1000;

inline bool held(BYTE dik) { return GetAsyncKeyState(MapVirtualKeyW(dik, MAPVK_VSC_TO_VK)) & 0x8000; }

inline void press(BYTE dik, DWORD ms) {
    std::lock_guard<std::mutex> l(inject_mx);
    injected.push_back({dik, GetTickCount() + ms, false, false});
}

inline void add_injected_state(BYTE* keys) {
    std::lock_guard<std::mutex> l(inject_mx);
    DWORD now = GetTickCount();
    for (auto& k : injected)
        if ((LONG)(k.until - now) > 0) keys[k.dik] = 0x80;
}

inline void add_injected_events(BYTE* data, DWORD size, DWORD* count, DWORD capacity) {
    std::lock_guard<std::mutex> l(inject_mx);
    DWORD now = GetTickCount();
    auto emit = [&](BYTE dik, bool down) {
        if (*count >= capacity || size < 16) return false;
        BYTE* e = data + *count * size;
        memset(e, 0, size);
        DWORD fields[4] = {dik, down ? 0x80u : 0u, now, injected_sequence++};
        memcpy(e, fields, sizeof fields);
        ++*count;
        return true;
    };
    for (auto& k : injected) {
        if (!k.down_sent) k.down_sent = emit(k.dik, true);
        if (k.down_sent && !k.up_sent && (LONG)(k.until - now) <= 0) k.up_sent = held(k.dik) || emit(k.dik, false);
    }
    injected.erase(std::remove_if(injected.begin(), injected.end(),
                                  [&](const Injected& k) { return k.up_sent || (LONG)(now - k.until) > (LONG)INJECT_FORGET; }),
                   injected.end());
}

inline Patched* patch_for(void* device) {
    void** vt = *(void***)device;
    for (int i = 0; i < patched_count; i++)
        if (patched[i].vtable == vt) return &patched[i];
    return nullptr;
}

inline bool mouse_device(void* device) {
    std::lock_guard<std::mutex> l(kinds_mx);
    auto it = is_mouse.find(device);
    if (it != is_mouse.end()) return it->second;
    DIDEVCAPS caps{sizeof caps};
    bool mouse = SUCCEEDED(((IDirectInputDevice8W*)device)->GetCapabilities(&caps)) && GET_DIDEVICE_TYPE(caps.dwDevType) == DI8DEVTYPE_MOUSE;
    return is_mouse[device] = mouse;
}

inline HRESULT WINAPI blocked_state(void* device, DWORD size, void* data) {
    HRESULT r = patch_for(device)->get_state(device, size, data);
    if (FAILED(r) || !data) return r;
    if (!blocked) {
        if (size == 256 && !mouse_device(device)) add_injected_state((BYTE*)data);
        return r;
    }
    if (size >= sizeof(DIMOUSESTATE) && mouse_device(device)) wheel += ((DIMOUSESTATE*)data)->lZ;
    memset(data, 0, size);
    return r;
}

inline HRESULT WINAPI blocked_data(void* device, DWORD size, DIDEVICEOBJECTDATA* data, DWORD* count, DWORD flags) {
    DWORD capacity = count ? *count : 0;
    HRESULT r = patch_for(device)->get_data(device, size, data, count, flags);
    if (FAILED(r) || !data || !count || (flags & DIGDD_PEEK)) return r;
    bool mouse = mouse_device(device);
    if (!blocked) {
        if (!mouse) add_injected_events((BYTE*)data, size, count, capacity);
        return r;
    }
    DWORD kept = 0;
    for (DWORD i = 0; i < *count; i++) {
        auto* e = (DIDEVICEOBJECTDATA*)((BYTE*)data + i * size);
        if (mouse && e->dwOfs == DIMOFS_Z) wheel += (LONG)e->dwData;
        bool release = !(e->dwData & 0x80) && (!mouse || e->dwOfs >= DIMOFS_BUTTON0);
        if (release) memmove((BYTE*)data + kept++ * size, e, size);
    }
    *count = kept;
    return r;
}

inline SHORT WINAPI blocked_async_key(int vk) { return blocked ? 0 : real_async_key(vk); }
inline SHORT WINAPI blocked_key_state(int vk) { return blocked ? 0 : real_key_state(vk); }
inline BOOL WINAPI blocked_keyboard_state(BYTE* keys) {
    BOOL r = real_keyboard_state(keys);
    if (blocked && r && keys) memset(keys, 0, 256);
    return r;
}

inline void patch_vtable(void* device) {
    void** vt = *(void***)device;
    if (patch_for(device) || patched_count >= 8) return;
    patched[patched_count++] = {vt, (GetStateFn)vt[9], (GetDataFn)vt[10]};
    DWORD old;
    VirtualProtect(&vt[9], 2 * sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
    vt[9] = (void*)blocked_state;
    vt[10] = (void*)blocked_data;
    VirtualProtect(&vt[9], 2 * sizeof(void*), old, &old);
}

inline int install(HMODULE engine) {
    for (REFIID iid : {IID_IDirectInput8W, IID_IDirectInput8A}) {
        IDirectInput8W* di = nullptr;
        if (FAILED(DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, iid, (void**)&di, nullptr))) continue;
        for (REFGUID dev : {GUID_SysMouse, GUID_SysKeyboard}) {
            IDirectInputDevice8W* d = nullptr;
            if (SUCCEEDED(di->CreateDevice(dev, &d, nullptr))) {
                patch_vtable(d);
                d->Release();
            }
        }
        di->Release();
    }
    if (auto p = hook_import(engine, "user32.dll", "GetAsyncKeyState", (void*)blocked_async_key)) real_async_key = (AsyncKeyFn)p;
    if (auto p = hook_import(engine, "user32.dll", "GetKeyState", (void*)blocked_key_state)) real_key_state = (AsyncKeyFn)p;
    if (auto p = hook_import(engine, "user32.dll", "GetKeyboardState", (void*)blocked_keyboard_state)) real_keyboard_state = (KeyboardFn)p;
    return patched_count;
}

}
