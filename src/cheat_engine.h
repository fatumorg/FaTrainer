#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <string>
#include "app.h"
#include "../installer/logic.h"

namespace cheat_engine {

inline const char ATTACH_SCRIPT[] =
    "getAutoAttachList().add(\"DyingLightGame.exe\")\n"
    "local fatrainerSawGame = false\n"
    "local fatrainerTimer = createTimer(nil, false)\n"
    "fatrainerTimer.Interval = 2000\n"
    "fatrainerTimer.OnTimer = function()\n"
    "  if getProcessIDFromProcessName(\"DyingLightGame.exe\") ~= nil then\n"
    "    fatrainerSawGame = true\n"
    "  elseif fatrainerSawGame then\n"
    "    closeCE()\n"
    "  end\n"
    "end\n"
    "fatrainerTimer.Enabled = true\n";

const DWORD INSTALL_WAIT_MS = 300000;
inline const wchar_t EXE[] = L"cheatengine-x86_64.exe", MARKER[] = L"fatrainer-installed.txt", SETTINGS[] = L"Software\\Cheat Engine";

inline std::wstring wide(const std::string& s) { return std::wstring(s.begin(), s.end()); }

inline std::wstring install_dir() {
    wchar_t base[MAX_PATH] = L"";
    GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
    return std::wstring(base) + L"\\FaTrainer\\Cheat Engine";
}

inline std::wstring installer_arguments(const std::wstring& dir) { return L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /ZBDIST /DIR=\"" + dir + L"\""; }

inline std::string read_all(const std::wstring& path) {
    std::string out;
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) return out;
    char buf[1 << 16];
    for (size_t n; (n = fread(buf, 1, sizeof buf, f)) > 0;) out.append(buf, n);
    fclose(f);
    return out;
}

inline bool write_all(const std::wstring& path, const std::string& data) {
    FILE* f = _wfopen(path.c_str(), L"wb");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    return fclose(f) == 0 && ok;
}

inline bool exists(const std::wstring& path) { return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES; }

inline bool running() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W e{sizeof e};
    bool found = false;
    for (BOOL more = Process32FirstW(snap, &e); more && !found; more = Process32NextW(snap, &e)) found = !_wcsicmp(e.szExeFile, EXE);
    CloseHandle(snap);
    return found;
}

inline HANDLE launch(const std::wstring& file, const std::wstring& arguments, int show, bool keep_handle) {
    SHELLEXECUTEINFOW info{sizeof info};
    info.fMask = keep_handle ? SEE_MASK_NOCLOSEPROCESS : 0;
    info.lpVerb = L"open", info.lpFile = file.c_str(), info.lpParameters = arguments.empty() ? nullptr : arguments.c_str(), info.nShow = show;
    if (!ShellExecuteExW(&info)) return INVALID_HANDLE_VALUE;
    return keep_handle ? info.hProcess : nullptr;
}

inline void quiet_first_start() {
    DWORD zero = 0;
    RegSetKeyValueW(HKEY_CURRENT_USER, SETTINGS, L"First Time User", REG_DWORD, &zero, sizeof zero);
    std::wstring ceshare = std::wstring(SETTINGS) + L"\\ceshare";
    if (RegGetValueW(HKEY_CURRENT_USER, ceshare.c_str(), L"enabled", RRF_RT_REG_SZ, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
        RegSetKeyValueW(HKEY_CURRENT_USER, ceshare.c_str(), L"enabled", REG_SZ, L"0", 2 * sizeof(wchar_t));
}

inline bool install(const std::wstring& installer, const std::wstring& dir, const std::string& version) {
    HANDLE process = launch(installer, installer_arguments(dir), SW_HIDE, true);
    if (process == INVALID_HANDLE_VALUE) {
        logf("cheat engine: installer did not start (error %lu)", GetLastError());
        return false;
    }
    DWORD waited = process ? WaitForSingleObject(process, INSTALL_WAIT_MS) : WAIT_FAILED;
    if (process) CloseHandle(process);
    if (waited != WAIT_OBJECT_0 || !exists(dir + L"\\" + EXE)) {
        logf("cheat engine: install of %s did not finish (wait %lu)", version.c_str(), waited);
        return false;
    }
    write_all(dir + L"\\" + MARKER, version);
    logf("cheat engine: installed %s", version.c_str());
    return true;
}

inline void run(const std::wstring& game_dir) {
    std::wstring folder = game_dir + L"\\" + wide(CHEAT_ENGINE_DIR);
    logic::CheatEngineInfo info = logic::parse_cheat_engine_info(read_all(folder + L"\\" + wide(CHEAT_ENGINE_INFO_FILE)));
    std::wstring installer = folder + L"\\CheatEngine" + wide(info.version) + L".exe";
    if (info.version.empty() || !logic::cheat_engine_file_ok(read_all(installer), info)) {
        logf("cheat engine: installer missing or does not match, skipped");
        return;
    }
    std::wstring dir = install_dir();
    std::string installed = read_all(dir + L"\\" + MARKER);
    if ((!exists(dir + L"\\" + EXE) || installed.empty() || newer_version(info.version, installed)) && !install(installer, dir, info.version)) return;
    std::wstring script = dir + L"\\autorun\\fatrainer.lua";
    if (read_all(script) != ATTACH_SCRIPT && !write_all(script, ATTACH_SCRIPT)) logf("cheat engine: could not write the attach script");
    quiet_first_start();
    if (running()) {
        logf("cheat engine: already running, it attaches by itself");
        return;
    }
    if (launch(dir + L"\\" + EXE, L"", SW_SHOWMINNOACTIVE, false) == INVALID_HANDLE_VALUE) logf("cheat engine: did not start (error %lu)", GetLastError());
    else logf("cheat engine: started %s", info.version.c_str());
}

}
