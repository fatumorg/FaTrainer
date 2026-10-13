#pragma once
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include "logic.h"
#include "version.h"

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <shobjidl.h>
#include <shlobj.h>
#include "net_win.h"
#else
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace fs = std::filesystem;

namespace platform {

const std::string FOLDER_PICKER_MISSING = "\x01";

struct Rect { float x0, y0, x1, y1; };
inline float caption_height = 0;
inline std::vector<Rect> caption_holes;

#ifdef _WIN32
const bool LINUX = false, CUSTOM_CAPTION = true;
inline HWND window = nullptr;

enum WindowCommand { MINIMIZE, MAXIMIZE, CLOSE };
inline bool maximized() { return window && IsZoomed(window); }
inline void window_command(WindowCommand c) {
    if (c == MINIMIZE) ShowWindow(window, SW_MINIMIZE);
    else if (c == MAXIMIZE) ShowWindow(window, maximized() ? SW_RESTORE : SW_MAXIMIZE);
    else PostMessageW(window, WM_CLOSE, 0, 0);
}

inline fs::path config_dir() {
    const wchar_t* data = _wgetenv(L"APPDATA");
    return data ? fs::path(data) / "FaTrainer" : fs::path();
}

const char* INSTALLER_ASSET = "FaTrainer.exe";
const char* INSTALLER_INFO = "installer-windows.txt";

inline fs::path self_path() {
    std::wstring path(32768, L'\0');
    path.resize(GetModuleFileNameW(nullptr, &path[0], (DWORD)path.size()));
    return path;
}

inline fs::path old_self() { return self_path().wstring() + L".old"; }
inline void remove_old_self() {
    std::error_code ec;
    fs::remove(old_self(), ec);
}

inline bool replace_self(const std::string& data, std::string& error) {
    std::error_code ec;
    fs::path self = self_path(), fresh = self.wstring() + L".new";
    std::ofstream(fresh, std::ios::binary | std::ios::trunc).write(data.data(), (std::streamsize)data.size());
    if (fs::file_size(fresh, ec) != data.size()) return fs::remove(fresh, ec), error = "could not write next to FaTrainer", false;
    fs::remove(old_self(), ec);
    fs::rename(self, old_self(), ec);
    if (ec) return fs::remove(fresh, ec), error = "could not move the running FaTrainer aside", false;
    fs::rename(fresh, self, ec);
    if (ec) return fs::rename(old_self(), self, ec), error = "could not put the new FaTrainer in place", false;
    return true;
}

inline void restart_self() {
    STARTUPINFOW startup{sizeof startup};
    PROCESS_INFORMATION process{};
    std::wstring path = self_path().wstring();
    if (CreateProcessW(path.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process))
        CloseHandle(process.hThread), CloseHandle(process.hProcess);
    PostMessageW(window, WM_CLOSE, 0, 0);
}
namespace net = ::net;

inline std::string utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], n, nullptr, nullptr);
    return out;
}

inline std::string registry_string(HKEY root, const wchar_t* key, const wchar_t* name) {
    wchar_t value[MAX_PATH] = {};
    DWORD size = sizeof value;
    if (RegGetValueW(root, key, name, RRF_RT_REG_SZ, nullptr, value, &size) != ERROR_SUCCESS) return {};
    return utf8(value);
}

inline std::vector<fs::path> steam_roots() {
    std::vector<fs::path> roots;
    for (const std::string& r : {registry_string(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath"),
                                 registry_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath"),
                                 registry_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath")})
        if (!r.empty()) roots.push_back(fs::u8path(r));
    roots.push_back("C:\\Program Files (x86)\\Steam");
    return roots;
}

inline void pick_folder(const std::function<void(std::string)>& done) {
    std::string picked;
    IFileOpenDialog* dialog = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        FILEOPENDIALOGOPTIONS options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        dialog->SetTitle(L"Choose the Dying Light folder (the one with DyingLightGame.exe)");
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->Show(window)) && SUCCEEDED(dialog->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) picked = utf8(path), CoTaskMemFree(path);
            item->Release();
        }
        dialog->Release();
    }
    done(picked);
}

inline std::vector<fs::path> shortcut_files() {
    std::vector<fs::path> out;
    for (REFKNOWNFOLDERID id : {FOLDERID_Desktop, FOLDERID_Programs}) {
        PWSTR dir = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &dir))) out.push_back(fs::path(dir) / L"FaTrainer.lnk");
        CoTaskMemFree(dir);
    }
    return out;
}

inline fs::path shortcut_file() {
    std::vector<fs::path> files = shortcut_files();
    return files.empty() ? fs::path() : files[0];
}

inline bool add_shortcut(const std::string&, std::string& error) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    fs::path exe = self_path();
    for (const fs::path& file : shortcut_files()) {
        IShellLinkW* link = nullptr;
        IPersistFile* persist = nullptr;
        bool ok = SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link)));
        if (ok) {
            link->SetPath(exe.c_str());
            link->SetWorkingDirectory(exe.parent_path().c_str());
            link->SetDescription(L"Play Dying Light with FaTrainer");
            link->SetIconLocation(exe.c_str(), 0);
            ok = SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&persist))) && SUCCEEDED(persist->Save(file.c_str(), TRUE));
        }
        if (persist) persist->Release();
        if (link) link->Release();
        if (!ok) return error = "could not write " + file.u8string(), false;
    }
    return true;
}

#else
const bool LINUX = true, CUSTOM_CAPTION = false;
enum WindowCommand { MINIMIZE, MAXIMIZE, CLOSE };
inline bool maximized() { return false; }
inline void window_command(WindowCommand) {}

const int NOT_STARTED = -1, NOT_FOUND = 127;

inline int run(const std::vector<std::string>& args, const std::function<bool(const char*, size_t)>& out) {
    int pipe_fd[2];
    if (pipe(pipe_fd)) return NOT_STARTED;
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipe_fd[1], 1);
    posix_spawn_file_actions_addclose(&actions, pipe_fd[0]);
    posix_spawn_file_actions_addclose(&actions, pipe_fd[1]);
    posix_spawn_file_actions_addopen(&actions, 2, "/dev/null", O_WRONLY, 0);
    std::vector<char*> argv;
    for (const std::string& a : args) argv.push_back((char*)a.c_str());
    argv.push_back(nullptr);
    pid_t pid = 0;
    int started = posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    close(pipe_fd[1]);
    if (started) {
        close(pipe_fd[0]);
        return NOT_STARTED;
    }
    char buffer[16384];
    bool wanted = true;
    for (ssize_t n; (n = read(pipe_fd[0], buffer, sizeof buffer)) > 0;)
        if (wanted && !out(buffer, (size_t)n)) wanted = false, kill(pid, SIGTERM);
    close(pipe_fd[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : NOT_STARTED;
}

inline std::string home() {
    const char* h = getenv("HOME");
    return h ? h : "";
}

inline fs::path xdg(const char* variable, const char* fallback) {
    const char* v = getenv(variable);
    return v && *v ? fs::path(v) : fs::path(home()) / fallback;
}

inline fs::path config_dir() { return xdg("XDG_CONFIG_HOME", ".config") / "fatrainer"; }

const char* INSTALLER_ASSET = "FaTrainer";
const char* INSTALLER_INFO = "installer-linux.txt";

inline fs::path self_path() {
    static const fs::path at_start = [] {
        std::error_code ec;
        return fs::read_symlink("/proc/self/exe", ec);
    }();
    return at_start;
}

inline void remove_old_self() {}

inline bool replace_self(const std::string& data, std::string& error) {
    std::error_code ec;
    fs::path self = self_path(), fresh = self.string() + ".new";
    if (self.empty()) return error = "could not find where FaTrainer is", false;
    std::ofstream(fresh, std::ios::binary | std::ios::trunc).write(data.data(), (std::streamsize)data.size());
    if (fs::file_size(fresh, ec) != data.size()) return fs::remove(fresh, ec), error = "could not write next to FaTrainer", false;
    fs::permissions(fresh, fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read | fs::perms::others_exec, ec);
    fs::rename(fresh, self, ec);
    if (ec) return fs::remove(fresh, ec), error = "could not replace the FaTrainer file", false;
    return true;
}

inline void restart_self() {
    std::string path = self_path().string();
    execl(path.c_str(), path.c_str(), (char*)nullptr);
}
inline fs::path applications_dir() { return xdg("XDG_DATA_HOME", ".local/share") / "applications"; }
inline fs::path icons_dir() { return xdg("XDG_DATA_HOME", ".local/share") / "icons" / "hicolor" / "256x256" / "apps"; }
inline fs::path shortcut_file() { return applications_dir() / "fatrainer.desktop"; }
inline fs::path shortcut_icon() { return icons_dir() / "fatrainer.png"; }

inline bool on_nixos() {
    std::ifstream release("/etc/os-release");
    std::string text((std::istreambuf_iterator<char>(release)), std::istreambuf_iterator<char>());
    return text.find("ID=nixos") != std::string::npos;
}

inline std::string desktop_quoted(const std::string& path) {
    std::string out = "\"";
    for (char c : path) {
        if (c == '"' || c == '`' || c == '$' || c == '\\') out += "\\\\";
        out += c;
    }
    return out + "\"";
}

inline bool add_shortcut(const std::string& icon_png, std::string& error) {
    std::error_code ec;
    fs::path exe = self_path();
    if (exe.empty()) return error = "could not find where FaTrainer is", false;
    fs::create_directories(applications_dir(), ec);
    fs::create_directories(icons_dir(), ec);
    fs::remove(applications_dir() / "fatrainer-installer.desktop", ec);
    fs::remove(icons_dir() / "fatrainer-installer.png", ec);
    std::ofstream(shortcut_icon(), std::ios::binary | std::ios::trunc).write(icon_png.data(), (std::streamsize)icon_png.size());
    std::ofstream desktop(shortcut_file(), std::ios::trunc);
    desktop << "[Desktop Entry]\nType=Application\nName=FaTrainer\nGenericName=Dying Light trainer\n"
            << "Comment=Play Dying Light with FaTrainer | Dying Light\n"
            << "Exec=" << (on_nixos() ? "steam-run " : "") << desktop_quoted(exe.string()) << "\n"
            << "Path=" << exe.parent_path().string() << "\nIcon=fatrainer\nTerminal=false\nCategories=Game;\n"
            << "Keywords=Dying Light;trainer;FaTrainer;cheats;\nStartupWMClass=fatrainer\n";
    if (!desktop) return error = "could not write " + shortcut_file().string(), false;
    return true;
}

inline std::vector<fs::path> steam_roots() {
    std::vector<fs::path> roots;
    if (const char* data = getenv("XDG_DATA_HOME")) roots.push_back(fs::path(data) / "Steam");
    for (const char* relative : {".steam/steam", ".steam/root", ".local/share/Steam", ".var/app/com.valvesoftware.Steam/.local/share/Steam",
                                 "snap/steam/common/.local/share/Steam"})
        roots.push_back(fs::path(home()) / relative);
    return roots;
}

inline void pick_folder(const std::function<void(std::string)>& done) {
    std::thread([done] {
        const char* title = "Choose the Dying Light folder (the one with DyingLightGame.exe)";
        const std::vector<std::vector<std::string>> pickers = {
            {"zenity", "--file-selection", "--directory", std::string("--title=") + title},
            {"kdialog", "--getexistingdirectory", home(), "--title", title},
            {"qarma", "--file-selection", "--directory", std::string("--title=") + title}};
        for (const auto& picker : pickers) {
            std::string out;
            int status = run(picker, [&](const char* d, size_t n) { out.append(d, n); return true; });
            if (status == NOT_STARTED || status == NOT_FOUND) continue;
            while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
            return done(status == 0 ? out : std::string());
        }
        done(FOLDER_PICKER_MISSING);
    }).detach();
}

namespace net {
using Sink = std::function<bool(const char* data, size_t n, unsigned long long total)>;

inline bool get(const std::string& url, const Sink& sink, std::string& error) {
    const std::vector<std::vector<std::string>> tools = {
        {"curl", "--fail", "--location", "--silent", "--proto", "=https", "--max-time", "300", url},
        {"wget", "--quiet", "--https-only", "--timeout=30", "--output-document=-", url}};
    for (const auto& tool : tools) {
        int status = run(tool, [&](const char* d, size_t n) { return sink(d, n, 0); });
        if (status == 0) return true;
        if (status != NOT_STARTED && status != NOT_FOUND) return error = "could not download from GitHub (" + tool[0] + " error " + std::to_string(status) + ")", false;
    }
    return error = "downloading needs curl or wget, install one of them", false;
}

inline bool get_text(const std::string& url, std::string& out, std::string& error) {
    const size_t LIMIT = 1 << 20;
    out.clear();
    return get(url, [&](const char* d, size_t n, unsigned long long) { out.append(d, n); return out.size() < LIMIT; }, error);
}
}
#endif

const char* GAME_EXE = "DyingLightGame.exe";
const char* GAME_LAUNCH_URL = "steam://rungameid/239140";
const char* APP_ID = "239140";

inline void launch_game() {
#ifdef _WIN32
    std::string steam = registry_string(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamExe");
    std::error_code ec;
    if (steam.empty() || !fs::exists(fs::u8path(steam), ec) ||
        (INT_PTR)ShellExecuteW(nullptr, L"open", fs::u8path(steam).c_str(), L"-applaunch 239140", nullptr, SW_SHOWNORMAL) <= 32)
        ShellExecuteA(nullptr, "open", GAME_LAUNCH_URL, nullptr, nullptr, SW_SHOWNORMAL);
#else
    std::thread([] {
        const std::vector<std::vector<std::string>> launchers = {{"steam", "-applaunch", APP_ID},
                                                                 {"flatpak", "run", "com.valvesoftware.Steam", "-applaunch", APP_ID},
                                                                 {"xdg-open", GAME_LAUNCH_URL}};
        for (const auto& launcher : launchers) {
            int status = run(launcher, [](const char*, size_t) { return true; });
            if (status != NOT_STARTED && status != NOT_FOUND) return;
        }
    }).detach();
#endif
}
const char* DLL_NAME = "xinput1_3.dll";
const char* BACKUP_NAME = "xinput1_3.dll.before-fatrainer";
const char* DOWNLOAD_NAME = "xinput1_3.dll.fatrainer-download";

inline std::string read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f), {});
}

inline std::vector<long> game_processes() {
    std::vector<long> out;
#ifdef _WIN32
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return out;
    PROCESSENTRY32W e{sizeof e};
    for (BOOL more = Process32FirstW(snap, &e); more; more = Process32NextW(snap, &e))
        if (logic::game_process(utf8(e.szExeFile))) out.push_back((long)e.th32ProcessID);
    CloseHandle(snap);
#else
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/proc", ec)) {
        std::string pid = entry.path().filename().string();
        if (pid.empty() || pid.find_first_not_of("0123456789") != std::string::npos || atol(pid.c_str()) == (long)getpid()) continue;
        std::string cmdline = read_file(entry.path() / "cmdline");
        if (logic::game_process(cmdline.substr(0, cmdline.find('\0')))) out.push_back(atol(pid.c_str()));
    }
#endif
    return out;
}

inline void stop_processes(const std::vector<long>& pids) {
    for (long pid : pids) {
#ifdef _WIN32
        if (HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid)) TerminateProcess(h, 1), CloseHandle(h);
#else
        kill((pid_t)pid, SIGKILL);
#endif
    }
}

inline bool is_game_dir(const fs::path& dir) {
    std::error_code ec;
    return !dir.empty() && fs::is_regular_file(dir / GAME_EXE, ec);
}

inline fs::path find_game() {
    for (const fs::path& root : steam_roots()) {
        std::vector<fs::path> libraries = {root};
        for (const std::string& lib : logic::vdf_values(read_file(root / "steamapps" / "libraryfolders.vdf"), "path"))
            libraries.push_back(fs::u8path(lib));
        for (const fs::path& lib : libraries) {
            auto dirs = logic::vdf_values(read_file(lib / "steamapps" / ("appmanifest_" + std::string(APP_ID) + ".acf")), "installdir");
            fs::path game = lib / "steamapps" / "common" / fs::u8path(dirs.empty() ? "Dying Light" : dirs[0]);
            if (is_game_dir(game)) return game;
        }
    }
    return {};
}

}
