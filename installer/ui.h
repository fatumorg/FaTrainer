#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "brand.h"
#include "imgui.h"
#include "platform.h"

EMBED(changelog_md, "CHANGELOG.md")
#ifndef _WIN32
EMBED(icon_png, "installer/icon.png")
#endif

namespace app {

using namespace brand;
const char* LAUNCH_OPTION = "WINEDLLOVERRIDES=\"xinput1_3=n,b\" %command%";

inline float scale = 1;
inline float S(float v) { return v * scale; }
struct Fonts { ImFont *intro, *brand, *hero, *heading, *body, *strong, *small, *label, *mono; };
inline Fonts F;

inline std::unordered_map<ImGuiID, float> anims;
inline float& anim(const std::string& key, float initial = 0) {
    auto it = anims.try_emplace(ImGui::GetID(key.c_str()), initial).first;
    return it->second;
}
inline void label_caps(ImDrawList* dl, ImVec2 pos, const char* text, ImU32 color) {
    spaced_caps(dl, F.label, F.label->FontSize, pos, text, C(color), S(1.4f));
}

enum class Kind { primary, ghost, quiet };

inline bool button(const char* id, const std::string& text, ImVec2 size, Kind kind, bool enabled = true, float progress = -1) {
    ImGui::PushID(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton("button", size) && enabled;
    bool hovered = enabled && ImGui::IsItemHovered(), held = enabled && ImGui::IsItemActive();
    float h = follow(anim("hover"), hovered ? 1.0f : 0.0f, 16), d = follow(anim("down"), held ? 1.0f : 0.0f, 30);
    float e = follow(anim("enabled", enabled ? 1.0f : 0.0f), enabled ? 1.0f : 0.0f, 10);
    ImGui::PopID();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float lift = (h - d) * S(1.5f);
    ImVec2 a{p.x, p.y - lift}, b{p.x + size.x, p.y + size.y - lift};
    float r = S(10);
    ImU32 ink = TEXT;
    if (kind == Kind::primary) {
        if (progress >= 0) {
            dl->AddRectFilled(a, b, C(RAISED), r);
            float fill = a.x + (b.x - a.x) * clamp01(progress);
            if (fill > a.x + 1) {
                dl->PushClipRect(a, {fill, b.y}, true);
                dl->AddRectFilled(a, b, C(ACCENT), r);
                float sheen = a.x + fmodf((float)ImGui::GetTime() * S(320), (b.x - a.x) + S(160)) - S(80);
                dl->AddRectFilledMultiColor({sheen - S(60), a.y}, {sheen, b.y}, C(IM_COL32(255, 255, 255, 0)), C(IM_COL32(255, 255, 255, 60)),
                                            C(IM_COL32(255, 255, 255, 60)), C(IM_COL32(255, 255, 255, 0)));
                dl->AddRectFilledMultiColor({sheen, a.y}, {sheen + S(60), b.y}, C(IM_COL32(255, 255, 255, 60)), C(IM_COL32(255, 255, 255, 0)),
                                            C(IM_COL32(255, 255, 255, 0)), C(IM_COL32(255, 255, 255, 60)));
                dl->PopClipRect();
            }
            dl->AddRect(a, b, C(LINE), r);
            ink = progress > 0.55f ? ACCENT_INK : TEXT;
        } else {
            for (int i = 1; i <= 4; i++) dl->AddRect({a.x - i * S(2), a.y - i * S(2)}, {b.x + i * S(2), b.y + i * S(2)}, C(ACCENT, h * 0.09f / i), r + i * S(2), 0, S(2));
            ImU32 fill = mix(mix(IM_COL32(70, 62, 52, 255), ACCENT, e), IM_COL32(246, 181, 104, 255), h * 0.6f);
            dl->AddRectFilled(a, b, C(fill), r);
            ink = mix(MUTED, ACCENT_INK, e);
        }
    } else if (kind == Kind::ghost) {
        dl->AddRectFilled(a, b, C(RAISED, h), r);
        dl->AddRect(a, b, C(mix(LINE, ACCENT, h * 0.8f)), r);
        ink = enabled ? TEXT : MUTED;
    } else {
        ink = mix(MUTED, TEXT, h);
        float tw = text_w(F.strong, text.c_str());
        float ux = (a.x + b.x - tw) / 2;
        dl->AddLine({ux, b.y - S(6)}, {ux + tw * h, b.y - S(6)}, C(ACCENT), S(1.5f));
    }
    float tw = text_w(F.strong, text.c_str());
    dl->AddText(F.strong, F.strong->FontSize, {(a.x + b.x - tw) / 2, (a.y + b.y - F.strong->FontSize) / 2}, C(ink), text.c_str());
    return pressed;
}

enum class Fetch { loading, ready, failed };
enum class Job { idle, working, done, failed };

struct Model {
    std::mutex mx;
    Fetch fetch = Fetch::loading;
    std::string fetch_error;
    bool on_beta = false, refetch = false;
    ReleaseInfo latest, installer_update;
    bool restart = false;
    std::vector<logic::ChangelogEntry> changelog;
    fs::path game;
    std::string installed;
    Job job = Job::idle;
    std::string step;
    float progress = 0;
    double finished_at = -10;
    std::string message;
    bool message_bad = false;
    bool picking = false, picked_ready = false, picker_missing = false;
    std::string picked;
    bool beta = false;
    logic::Announcement announcement;
};
inline Model M;

inline fs::path installer_ini() { return platform::config_dir() / "installer.ini"; }

inline void fetch_announcement() {
    std::thread([] {
        std::string text, error;
        if (!platform::net::get_text(ANNOUNCEMENT_URL, text, error)) return;
        logic::Announcement a = logic::parse_announcement(text);
        if (a.id.empty() || a.id == logic::ini_value(platform::read_file(installer_ini()), "announcement")) return;
        std::lock_guard<std::mutex> l(M.mx);
        M.announcement = a;
    }).detach();
}

inline void dismiss_announcement(const std::string& id) {
    std::error_code ec;
    fs::create_directories(platform::config_dir(), ec);
    std::string text = platform::read_file(installer_ini());
    std::ofstream(installer_ini(), std::ios::binary | std::ios::trunc) << logic::with_ini_value(text, "announcement", id);
    std::lock_guard<std::mutex> l(M.mx);
    M.announcement = {};
}

inline void refresh_installed() {
    std::error_code ec;
    fs::path dll = M.game / platform::DLL_NAME;
    M.installed = !M.game.empty() && fs::exists(dll, ec) ? logic::trainer_version_in(platform::read_file(dll)) : "";
    fs::path ini = M.game / "fatrainer.ini";
    std::string text = !M.game.empty() && fs::exists(ini, ec) ? platform::read_file(ini) : "";
    bool beta = logic::ini_value(text, "beta") == "1";
    if (beta != M.beta) M.beta = beta, M.refetch = true;
}

inline bool set_ini_flag(const char* key, bool on) {
    fs::path ini = M.game / "fatrainer.ini";
    std::error_code ec;
    std::string text = fs::exists(ini, ec) ? platform::read_file(ini) : "";
    {
        std::ofstream out(ini, std::ios::binary | std::ios::trunc);
        out << logic::with_ini_value(text, key, on ? "1" : "0");
        if (!out.flush()) return false;
    }
    refresh_installed();
    return true;
}

inline void fetch_release() {
    {
        std::lock_guard<std::mutex> l(M.mx);
        M.fetch = Fetch::loading;
    }
    bool beta;
    {
        std::lock_guard<std::mutex> l(M.mx);
        beta = M.beta, M.refetch = false;
    }
    std::thread([beta] {
        std::string text, error, changes, ignored;
        bool ok = platform::net::get_text(std::string(RELEASE_DOWNLOADS) + "version.txt", text, error);
        ReleaseInfo info = parse_release_info(text);
        if (ok && (info.version.empty() || info.sha256.size() != 64)) ok = false, error = "the release has no valid version.txt";
        bool have_changes = ok && platform::net::get_text(std::string(RELEASE_DOWNLOADS) + "CHANGELOG.md", changes, ignored);
        bool on_beta = false;
        std::string beta_text, beta_changes;
        if (beta && platform::net::get_text(std::string(BETA_DOWNLOADS) + "version.txt", beta_text, ignored)) {
            ReleaseInfo build = parse_release_info(beta_text);
            if (build.sha256.size() == 64 && !build.version.empty() && (!ok || newer_version(build.version, info.version))) {
                info = build, ok = on_beta = true, error.clear();
                if (platform::net::get_text(std::string(BETA_DOWNLOADS) + "CHANGELOG.md", beta_changes, ignored)) changes = beta_changes, have_changes = true;
            }
        }
        std::string installer_text;
        platform::net::get_text(std::string(INSTALLER_DOWNLOADS) + platform::INSTALLER_INFO, installer_text, ignored);
        ReleaseInfo installer = parse_release_info(installer_text);
        bool installer_newer = newer_version(installer.version, INSTALLER_VERSION) && installer.sha256.size() == 64;
        std::lock_guard<std::mutex> l(M.mx);
        M.installer_update = installer_newer ? installer : ReleaseInfo{};
        M.on_beta = on_beta;
        M.fetch = ok ? Fetch::ready : Fetch::failed;
        M.fetch_error = error;
        if (ok) M.latest = info;
        if (have_changes) {
            auto parsed = logic::parse_changelog(changes);
            if (!parsed.empty()) M.changelog = parsed;
        }
    }).detach();
}

inline void finish(Job job, const std::string& message) {
    std::lock_guard<std::mutex> l(M.mx);
    M.job = job;
    M.message = message;
    M.message_bad = job == Job::failed;
    M.finished_at = ImGui::GetTime();
    if (job == Job::done) M.progress = 1;
}

inline std::string replace_hint() {
    return platform::LINUX ? "Check that you can write to the game folder."
                           : "Close Dying Light if it is running. If it is not, right-click FaTrainer and choose Run as administrator.";
}

inline void start_play() {
    fs::path game;
    bool on_beta, install;
    ReleaseInfo latest;
    std::string installed;
    {
        std::lock_guard<std::mutex> l(M.mx);
        if (M.job == Job::working) return;
        game = M.game, latest = M.latest, on_beta = M.on_beta, installed = M.installed;
        install = logic::needs_install(M.installed, M.latest.version, M.fetch == Fetch::ready, M.beta);
        M.job = Job::working, M.step = install ? "Downloading" : "Starting Dying Light", M.progress = 0, M.message.clear();
    }
    std::thread([game, latest, on_beta, install, installed]() mutable {
        std::error_code ec;
        if (install) {
            const unsigned long long LIMIT = 64ull << 20;
            std::string data, error, base = on_beta ? BETA_DOWNLOADS : RELEASE_DOWNLOADS;
            bool ok = platform::net::get(base + platform::DLL_NAME, [&](const char* d, size_t n, unsigned long long total) {
                data.append(d, n);
                unsigned long long expected = latest.size ? latest.size : total;
                std::lock_guard<std::mutex> l(M.mx);
                M.progress = expected ? 0.88f * std::min(1.0f, (float)data.size() / expected) : 0.5f;
                return data.size() <= LIMIT;
            }, error);
            if (!ok) return finish(Job::failed, "Download failed: " + error + ". Nothing was changed.");
            {
                std::lock_guard<std::mutex> l(M.mx);
                M.step = "Checking", M.progress = 0.92f;
            }
            if ((latest.size && data.size() != latest.size) || logic::sha256_hex(data) != latest.sha256 || logic::trainer_version_in(data).empty())
                return finish(Job::failed, "The download did not match the release checksum, so nothing was installed. Try again in a minute.");
            {
                std::lock_guard<std::mutex> l(M.mx);
                M.step = "Installing", M.progress = 0.96f;
            }
            fs::path target = game / platform::DLL_NAME, temp = game / platform::DOWNLOAD_NAME, backup = game / platform::BACKUP_NAME;
            if (fs::exists(target, ec) && logic::trainer_version_in(platform::read_file(target)).empty() && !fs::exists(backup, ec)) {
                fs::copy_file(target, backup, ec);
                if (ec) return finish(Job::failed, "Could not keep a copy of the xinput1_3.dll that was already there. " + replace_hint());
            }
            {
                std::ofstream out(temp, std::ios::binary | std::ios::trunc);
                out.write(data.data(), (std::streamsize)data.size());
                if (!out) {
                    out.close();
                    fs::remove(temp, ec);
                    return finish(Job::failed, "Could not write to the game folder. " + replace_hint());
                }
            }
            fs::rename(temp, target, ec);
            if (ec) {
                fs::remove(temp, ec);
                return finish(Job::failed, "Could not replace xinput1_3.dll. " + replace_hint());
            }
            installed = latest.version;
            std::lock_guard<std::mutex> l(M.mx);
            if (M.game == game) M.installed = installed;
        }
        fs::path arm = game / LAUNCH_FILE;
        bool armed = !newer_version(FIRST_ARMED_VERSION, installed);
        if (armed && !logic::write_launch_file(game, (long long)time(nullptr))) return finish(Job::failed, "Could not write to the game folder. " + replace_hint());
        {
            std::lock_guard<std::mutex> l(M.mx);
            M.step = "Starting Dying Light", M.progress = 0.98f;
        }
        platform::launch_game();
        std::string running = "Dying Light is starting with FaTrainer " + installed + ". Press Insert or F8 in game. Starting the game from Steam plays it without the trainer.";
        if (!armed) return finish(Job::done, running);
        for (long long waited = 0; waited < LAUNCH_MAX_AGE * 1000; waited += 500) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (!fs::exists(arm, ec) && !ec) return finish(Job::done, running);
            std::lock_guard<std::mutex> l(M.mx);
            M.step = "Waiting for Dying Light";
        }
        fs::remove(arm, ec);
        finish(Job::failed, platform::LINUX ? "Dying Light did not load FaTrainer within 3 minutes. Add the Steam launch option below once, then press Play again."
                                            : "Dying Light did not load FaTrainer within 3 minutes. If the game was already running, close it and press Play again.");
    }).detach();
}

inline void start_self_update() {
    ReleaseInfo update;
    {
        std::lock_guard<std::mutex> l(M.mx);
        if (M.job == Job::working || M.installer_update.version.empty()) return;
        update = M.installer_update;
        M.job = Job::working, M.step = "Updating FaTrainer", M.progress = 0, M.message.clear();
    }
    std::thread([update] {
        std::error_code ec;
        std::string data, error;
        bool ok = platform::net::get(std::string(INSTALLER_DOWNLOADS) + platform::INSTALLER_ASSET, [&](const char* d, size_t n, unsigned long long total) {
                                                                data.append(d, n);
                                                                unsigned long long expected = update.size ? update.size : total;
                                                                std::lock_guard<std::mutex> l(M.mx);
                                                                M.progress = expected ? 0.9f * std::min(1.0f, (float)data.size() / expected) : 0.5f;
                                                                return data.size() <= (128ull << 20);
                                                            }, error);
        if (!ok) return finish(Job::failed, "Could not download the new FaTrainer: " + error + ". This one keeps working.");
        if ((update.size && data.size() != update.size) || logic::sha256_hex(data) != update.sha256)
            return finish(Job::failed, "The new FaTrainer did not match its checksum, so nothing was changed. Try again in a minute.");
        if (!platform::replace_self(data, error)) return finish(Job::failed, "Could not update FaTrainer: " + error + ". Download it again from the website.");
        {
            std::lock_guard<std::mutex> l(M.mx);
            M.restart = true;
        }
        finish(Job::done, "Updated FaTrainer to " + update.version + ". Restarting.");
    }).detach();
}

inline void uninstall() {
    std::lock_guard<std::mutex> l(M.mx);
    std::error_code ec;
    fs::path target = M.game / platform::DLL_NAME, backup = M.game / platform::BACKUP_NAME;
    fs::remove(target, ec);
    if (ec) {
        M.message = "Could not remove xinput1_3.dll. " + replace_hint(), M.message_bad = true;
        return;
    }
    bool restored = fs::exists(backup, ec) && (fs::rename(backup, target, ec), !ec);
    refresh_installed();
    M.job = Job::idle;
    M.message = restored ? "FaTrainer was removed from the game folder and the xinput1_3.dll that was there before is back." : "FaTrainer was removed from the game folder. Your fatrainer.ini settings stay there. Press Play to put it back.";
    M.message_bad = false;
}

inline void use_folder(const std::string& utf8) {
    if (M.job == Job::working) return;
    fs::path chosen = fs::u8path(utf8);
    if (!platform::is_game_dir(chosen) && platform::is_game_dir(chosen.parent_path())) chosen = chosen.parent_path();
    if (!platform::is_game_dir(chosen) && platform::is_game_dir(chosen / "Dying Light")) chosen = chosen / "Dying Light";
    if (!platform::is_game_dir(chosen)) {
        M.message = "That folder has no DyingLightGame.exe. Choose the Dying Light folder itself.", M.message_bad = true;
        return;
    }
    M.game = chosen;
    M.job = Job::idle;
    M.message.clear();
    refresh_installed();
}

inline void choose_folder() {
    {
        std::lock_guard<std::mutex> l(M.mx);
        if (M.picking) return;
        M.picking = true;
    }
    platform::pick_folder([](std::string picked) {
        std::lock_guard<std::mutex> l(M.mx);
        M.picking = false;
        M.picked_ready = true;
        M.picked = picked;
    });
}

inline void init(float dpi_scale) {
    scale = dpi_scale;
    ImGui::GetIO().IniFilename = nullptr;
    F.intro = load_font(DISPLAY, S(92));
    F.brand = load_font(DISPLAY, S(26));
    F.hero = load_font(DISPLAY, S(46));
    F.heading = load_font(HEADING, S(21));
    F.body = load_font(BODY, S(16));
    F.strong = load_font(STRONG, S(16));
    F.small = load_font(BODY, S(13.5f));
    F.label = load_font(STRONG, S(11.5f));
    F.mono = load_font(MONO, S(12.5f));
    ImGui::GetIO().FontDefault = F.body;
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = {0, 0}, s.WindowBorderSize = 0, s.ChildBorderSize = 0, s.FrameRounding = S(8), s.FramePadding = {S(12), S(9)};
    s.ItemSpacing = {S(10), S(10)}, s.ScrollbarSize = S(6), s.ScrollbarRounding = S(3);
    s.Colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(TEXT);
    s.Colors[ImGuiCol_WindowBg] = s.Colors[ImGuiCol_ChildBg] = {0, 0, 0, 0};
    s.Colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(RAISED);
    s.Colors[ImGuiCol_FrameBgHovered] = s.Colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(30, 30, 34, 255));
    s.Colors[ImGuiCol_TextSelectedBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(232, 151, 58, 90));
    s.Colors[ImGuiCol_ScrollbarBg] = {0, 0, 0, 0};
    s.Colors[ImGuiCol_ScrollbarGrab] = ImGui::ColorConvertU32ToFloat4(LINE);
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = s.Colors[ImGuiCol_ScrollbarGrabActive] = ImGui::ColorConvertU32ToFloat4(MUTED);
    s.Colors[ImGuiCol_NavHighlight] = ImGui::ColorConvertU32ToFloat4(ACCENT);
    s.Colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(24, 24, 28, 255));
    s.Colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(LINE);
    s.Colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(IM_COL32(232, 151, 58, 60));
    s.Colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(232, 151, 58, 40));
    s.Colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(232, 151, 58, 80));
    s.Colors[ImGuiCol_Button] = s.Colors[ImGuiCol_FrameBg];
    s.Colors[ImGuiCol_ButtonHovered] = s.Colors[ImGuiCol_FrameBgHovered];
    s.PopupRounding = S(10), s.PopupBorderSize = 1;
    platform::self_path();
    M.changelog = logic::parse_changelog(std::string((const char*)changelog_md, (size_t)(changelog_md_end - changelog_md)));
    M.game = platform::find_game();
    refresh_installed();
    fetch_release();
    fetch_announcement();
}

inline int tab = 0;
inline double tab_changed_at = 0;
const float CONTENT_AT = 1.45f;

inline float reveal(int index) {
    double base = std::max((double)CONTENT_AT, tab_changed_at);
    return ease_out((float)(ImGui::GetTime() - base - index * 0.07) / 0.6f);
}

struct Reveal {
    float offset;
    explicit Reveal(int index) {
        float a = reveal(index);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, a);
        offset = (1 - a) * S(18);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
    }
    ~Reveal() {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - offset);
        ImGui::PopStyleVar();
    }
};

inline void tabs(ImVec2 pos) {
    const char* names[] = {"Games", "Changelog"};
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float x = pos.x;
    static float line_x = -1, line_w = 0;
    for (int i = 0; i < 2; i++) {
        float w = text_w(F.strong, names[i]);
        ImGui::SetCursorScreenPos({x - S(10), pos.y - S(10)});
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("tab", {w + S(20), F.strong->FontSize + S(20)}) && tab != i) tab = i, tab_changed_at = ImGui::GetTime();
        platform::caption_holes.push_back({ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y, ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y});
        bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        float h = follow(anim(std::string("tab") + names[i]), tab == i ? 1.0f : hovered ? 0.6f : 0.0f, 14);
        dl->AddText(F.strong, F.strong->FontSize, {x, pos.y}, C(mix(MUTED, TEXT, h)), names[i]);
        if (tab == i) {
            if (line_x < 0) line_x = x, line_w = w;
            follow(line_x, x, 16), follow(line_w, w, 16);
        }
        x += w + S(30);
    }
    dl->AddRectFilled({line_x, pos.y + F.strong->FontSize + S(8)}, {line_x + line_w, pos.y + F.strong->FontSize + S(10)}, C(ACCENT), S(1));
}

inline void status_pill(ImVec2 right_top) {
    std::string text;
    ImU32 dot = MUTED;
    Fetch fetch;
    {
        std::lock_guard<std::mutex> l(M.mx);
        fetch = M.fetch;
        if (fetch == Fetch::ready && M.on_beta) text = "Beta build " + M.latest.version, dot = ACCENT;
        else if (fetch == Fetch::ready) text = FATRAINER_EDITION ", latest " + M.latest.version, dot = GOOD;
        else if (fetch == Fetch::failed) text = "GitHub not reachable", dot = BAD;
        else text = "Checking GitHub";
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float retry_w = fetch == Fetch::failed ? text_w(F.strong, "Retry") + S(24) : 0;
    float w = text_w(F.small, text.c_str()) + S(40) + retry_w, h = S(32);
    ImVec2 a{right_top.x - w, right_top.y}, b{right_top.x, right_top.y + h};
    dl->AddRectFilled(a, b, C(RAISED), h / 2);
    dl->AddRect(a, b, C(LINE_SOFT), h / 2);
    float pulse = fetch == Fetch::loading ? 0.45f + 0.55f * (0.5f + 0.5f * sinf((float)ImGui::GetTime() * 5)) : 1;
    dl->AddCircleFilled({a.x + S(17), a.y + h / 2}, S(3.5f), C(dot, pulse));
    if (fetch == Fetch::ready) dl->AddCircle({a.x + S(17), a.y + h / 2}, S(3.5f) + S(6) * fmodf((float)ImGui::GetTime(), 2.2f) / 2.2f, C(dot, 0.5f * (1 - fmodf((float)ImGui::GetTime(), 2.2f) / 2.2f)));
    dl->AddText(F.small, F.small->FontSize, {a.x + S(30), a.y + (h - F.small->FontSize) / 2}, C(SOFT), text.c_str());
    if (fetch == Fetch::failed) {
        ImGui::SetCursorScreenPos({b.x - retry_w - S(6), a.y});
        platform::caption_holes.push_back({b.x - retry_w - S(6), a.y, b.x, b.y});
        if (button("retry", "Retry", {retry_w, h}, Kind::quiet)) fetch_release();
    }
}

inline float window_controls(float width) {
    if (!platform::CUSTOM_CAPTION) return width;
    const float W = S(46), H = S(34);
    float left = width - 3 * W;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int i = 0; i < 3; i++) {
        ImVec2 a{left + i * W, 0}, b{a.x + W, H}, c{(a.x + b.x) / 2, (a.y + b.y) / 2};
        ImGui::SetCursorScreenPos(a);
        ImGui::PushID(i);
        bool pressed = ImGui::InvisibleButton("window", {W, H});
        float h = follow(anim("hover"), ImGui::IsItemHovered() ? 1.0f : 0.0f, 18);
        ImGui::PopID();
        bool close = i == platform::CLOSE;
        dl->AddRectFilled(a, b, close ? C(IM_COL32(196, 43, 28, 255), h) : C(RAISED, h));
        ImU32 ink = C(close ? mix(SOFT, IM_COL32(255, 255, 255, 255), h) : mix(SOFT, TEXT, h));
        float r = S(5);
        if (i == platform::MINIMIZE) dl->AddLine({c.x - r, c.y}, {c.x + r, c.y}, ink, S(1));
        else if (i == platform::MAXIMIZE && platform::maximized()) {
            dl->AddRect({c.x - r + S(2), c.y - r - S(1)}, {c.x + r + S(1), c.y + r - S(2)}, ink, 0, 0, S(1));
            dl->AddRectFilled({c.x - r - S(1), c.y - r + S(2)}, {c.x + r - S(2), c.y + r + S(1)}, C(mix(GROUND, RAISED, h)));
            dl->AddRect({c.x - r - S(1), c.y - r + S(2)}, {c.x + r - S(2), c.y + r + S(1)}, ink, 0, 0, S(1));
        } else if (i == platform::MAXIMIZE) dl->AddRect({c.x - r, c.y - r}, {c.x + r, c.y + r}, ink, 0, 0, S(1));
        else dl->AddLine({c.x - r, c.y - r}, {c.x + r, c.y + r}, ink, S(1)), dl->AddLine({c.x - r, c.y + r}, {c.x + r, c.y - r}, ink, S(1));
        if (pressed) platform::window_command((platform::WindowCommand)i);
    }
    platform::caption_holes.push_back({left, 0, width, H});
    return left;
}

inline void shortcut_card(float width) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + width, a.y + S(108)};
    dl->AddRectFilled(a, b, C(RAISED), S(12));
    label_caps(dl, {a.x + S(20), a.y + S(18)}, "SHORTCUT", MUTED);
    static std::string note;
    static bool bad = false;
    std::error_code ec;
    bool exists = fs::exists(platform::shortcut_file(), ec);
    if (note.empty())
        note = platform::LINUX ? exists ? "FaTrainer is in your app menu. Press again after you move this file." : "Adds FaTrainer to your app menu, so you can search for it."
                               : exists ? "FaTrainer is on your desktop and in the Start menu. Press again after you move this file." : "Adds FaTrainer to your desktop and the Start menu.";
    ImGui::PushClipRect(a, {b.x - S(190), b.y}, true);
    dl->AddText(F.small, F.small->FontSize, {a.x + S(20), a.y + S(42)}, C(bad ? BAD : SOFT), note.c_str());
    ImGui::PopClipRect();
    ImGui::SetCursorScreenPos({b.x - S(178), a.y + S(32)});
    if (button("shortcut", exists ? "Update shortcut" : "Add shortcut", {S(158), S(44)}, Kind::ghost)) {
        std::string error;
#ifdef _WIN32
        std::string icon;
#else
        std::string icon((const char*)icon_png, (size_t)(icon_png_end - icon_png));
#endif
        bad = !platform::add_shortcut(icon, error);
        note = bad ? "Could not add the shortcut: " + error + "." : "Added. Start FaTrainer from it to play with the trainer.";
    }
    ImGui::SetCursorScreenPos({a.x, b.y});
    ImGui::Dummy({width, 0});
}

inline void game_list(ImVec2 a, float width, const std::string& state) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float r0 = reveal(0), r1 = reveal(1), r2 = reveal(2);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, r0);
    label_caps(dl, {a.x, a.y + (1 - r0) * S(12)}, "GAMES", MUTED);
    ImGui::PopStyleVar();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, r1);
    ImVec2 c0{a.x, a.y + S(30) + (1 - r1) * S(16)}, c1{a.x + width, c0.y + S(92)};
    ImGui::SetCursorScreenPos(c0);
    ImGui::InvisibleButton("game_dying_light", {width, S(92)});
    float h = follow(anim("game_card"), ImGui::IsItemHovered() ? 1.0f : 0.0f, 12);
    dl->AddRectFilled(c0, c1, C(mix(RAISED, IM_COL32(30, 29, 31, 255), h)), S(12));
    dl->AddRect(c0, c1, C(mix(LINE_SOFT, LINE, h)), S(12));
    dl->AddRectFilled({c0.x, c0.y + S(18)}, {c0.x + S(3), c1.y - S(18)}, C(ACCENT), S(2));
    dl->PushClipRect({c0.x + S(12), c0.y}, {c1.x - S(12), c1.y}, true);
    float sky_k = width / 260;
    for (const Building& bd : city().front) {
        float x0 = c0.x + (bd.x - 300) * sky_k * 0.42f, x1 = x0 + bd.w * sky_k * 0.42f;
        if (x1 < c0.x || x0 > c1.x) continue;
        dl->AddRectFilled({x0, c1.y - bd.h * 0.16f * sky_k}, {x1, c1.y}, C(IM_COL32(34, 30, 28, 255), 0.55f + 0.45f * h));
    }
    dl->PopClipRect();
    dl->AddText(F.heading, F.heading->FontSize, {c0.x + S(20), c0.y + S(18)}, C(TEXT), "Dying Light");
    dl->AddText(F.small, F.small->FontSize, {c0.x + S(20), c0.y + S(50)}, C(state == "update" ? ACCENT : state == "installed" ? GOOD : MUTED),
                state == "update" ? "Update available" : state == "installed" ? "Installed" : "Not installed");
    ImGui::PopStyleVar();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, r2);
    ImVec2 d0{a.x, c1.y + S(14) + (1 - r2) * S(16)}, d1{a.x + width, d0.y + S(64)};
    for (float x = d0.x + S(12); x < d1.x - S(12); x += S(10)) {
        dl->AddLine({x, d0.y}, {std::min(x + S(5), d1.x - S(12)), d0.y}, C(LINE));
        dl->AddLine({x, d1.y}, {std::min(x + S(5), d1.x - S(12)), d1.y}, C(LINE));
    }
    for (float y = d0.y + S(12); y < d1.y - S(12); y += S(10)) {
        dl->AddLine({d0.x, y}, {d0.x, std::min(y + S(5), d1.y - S(12))}, C(LINE));
        dl->AddLine({d1.x, y}, {d1.x, std::min(y + S(5), d1.y - S(12))}, C(LINE));
    }
    dl->AddText(F.small, F.small->FontSize, {d0.x + S(20), d0.y + (S(64) - F.small->FontSize) / 2}, C(MUTED), "More games are on the way");
    ImGui::PopStyleVar();
}

inline void check_mark(ImDrawList* dl, ImVec2 c, float size, float t, ImU32 color) {
    ImVec2 p0{c.x - size * 0.5f, c.y}, p1{c.x - size * 0.12f, c.y + size * 0.38f}, p2{c.x + size * 0.55f, c.y - size * 0.42f};
    float first = clamp01(t / 0.4f), second = clamp01((t - 0.4f) / 0.6f);
    if (first > 0) dl->AddLine(p0, lerp(p0, p1, first), color, S(2.5f));
    if (second > 0) dl->AddLine(p1, lerp(p1, p2, second), color, S(2.5f));
}

inline bool check_box(ImDrawList* dl, const char* id, const char* text, ImVec2 box, bool value, bool enabled) {
    ImGui::BeginDisabled(!enabled);
    ImGui::SetCursorScreenPos({box.x, box.y - S(6)});
    bool clicked = ImGui::InvisibleButton(id, {S(220), S(32)});
    bool hovered = ImGui::IsItemHovered();
    ImGui::EndDisabled();
    if (hovered && enabled) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    float on = follow(anim(id), value ? 1.0f : 0.0f, 16);
    ImVec2 b0{box.x, box.y}, b1{box.x + S(20), box.y + S(20)};
    dl->AddRectFilled(b0, b1, C(mix(RAISED, ACCENT, on)), S(5));
    dl->AddRect(b0, b1, C(mix(hovered ? MUTED : LINE, ACCENT, on)), S(5));
    if (on > 0.01f) check_mark(dl, {box.x + S(10), box.y + S(10)}, S(9), on, C(ACCENT_INK));
    dl->AddText(F.strong, F.strong->FontSize, {box.x + S(30), box.y + (S(20) - F.strong->FontSize) / 2}, C(enabled ? TEXT : MUTED), text);
    return clicked && enabled;
}

inline void toggle_ini_flag(const char* key, bool on) {
    std::lock_guard<std::mutex> l(M.mx);
    if (!set_ini_flag(key, on)) M.message = "Could not write fatrainer.ini. " + replace_hint(), M.message_bad = true;
}

inline void muted_note(float width, const char* text) {
    ImGui::PushFont(F.small);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(MUTED), "%s", text);
    ImGui::PopTextWrapPos();
    ImGui::PopFont();
}


inline void detail(float width) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Model view;
    bool refetch;
    {
        std::lock_guard<std::mutex> l(M.mx);
        refetch = M.refetch && M.fetch != Fetch::loading;
    }
    if (refetch) fetch_release();
    static bool self_update_started = false;
    bool must_update;
    {
        std::lock_guard<std::mutex> l(M.mx);
        must_update = !M.installer_update.version.empty() && M.job == Job::idle && !self_update_started;
    }
    if (must_update) self_update_started = true, start_self_update();
    {
        std::lock_guard<std::mutex> l(M.mx);
        if (M.picked_ready) {
            M.picked_ready = false;
            if (M.picked == platform::FOLDER_PICKER_MISSING) M.picker_missing = true;
            else if (!M.picked.empty()) use_folder(M.picked);
        }
        view.fetch = M.fetch, view.fetch_error = M.fetch_error, view.latest = M.latest, view.game = M.game, view.installed = M.installed;
        view.job = M.job, view.step = M.step, view.progress = M.progress, view.finished_at = M.finished_at, view.message = M.message;
        view.message_bad = M.message_bad, view.picking = M.picking, view.picker_missing = M.picker_missing;
        view.installer_update = M.installer_update;
        view.beta = M.beta, view.on_beta = M.on_beta;
    }
    bool found = platform::is_game_dir(view.game);
    bool ready = view.fetch == Fetch::ready;
    bool outdated = !view.installed.empty() && ready && (view.installed == logic::LEGACY_VERSION || newer_version(view.latest.version, view.installed));
    bool current = !view.installed.empty() && ready && view.installed == view.latest.version;
    float t = (float)ImGui::GetTime();

    {
        Reveal r(0);
        ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + width, a.y + S(212)};
        ImGui::InvisibleButton("hero", {width, S(212)});
        static ImVec2 light{-1, -1};
        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool inside = mouse.x >= a.x && mouse.x <= b.x && mouse.y >= a.y && mouse.y <= b.y;
        ImVec2 target = inside ? mouse : ImVec2{a.x + width * (0.62f + 0.22f * sinf(t * 0.33f)), a.y + S(212) * (0.55f + 0.18f * sinf(t * 0.47f))};
        if (light.x < 0) light = target;
        follow(light.x, target.x, inside ? 9.0f : 2.0f), follow(light.y, target.y, inside ? 9.0f : 2.0f);
        ImVec2 parallax{((light.x - a.x) / width - 0.5f) * -S(26), 0};
        dl->PushClipRect(a, b, true);
        dl->AddRectFilledMultiColor(a, b, C(IM_COL32(20, 20, 24, 255)), C(IM_COL32(20, 20, 24, 255)), C(IM_COL32(28, 24, 22, 255)), C(IM_COL32(28, 24, 22, 255)));
        CityLook look;
        look.light_radius = S(190);
        draw_city(dl, a, b, light, parallax, t, t - CONTENT_AT + 0.1f, look);
        dl->AddRectFilledMultiColor({a.x, a.y}, {a.x + width * 0.6f, b.y}, C(GROUND, 0.55f), C(GROUND, 0), C(GROUND, 0), C(GROUND, 0.55f));
        dl->PopClipRect();
        round_corners(dl, a, b, S(14), C(GROUND));
        dl->AddRect(a, b, C(LINE_SOFT), S(14));
        draw_letters(dl, F.hero, F.hero->FontSize, {a.x + S(26), b.y - S(26) - F.hero->FontSize * 1.05f}, "Dying Light", t - CONTENT_AT - 0.25f, TEXT);
        label_caps(dl, {a.x + S(28), a.y + S(24)}, "STEAM  /  THE FOLLOWING  /  HARRAN PRISON", SOFT);
    }
    ImGui::Dummy({0, S(6)});

    {
        Reveal r(1);
        ImVec2 a = ImGui::GetCursorScreenPos();
        float cell = (width - S(24)) / 3;
        struct Stat { const char* label; std::string value; ImU32 color; };
        Stat stats[3] = {
            {"INSTALLED", view.installed.empty() ? "Not yet" : view.installed == logic::LEGACY_VERSION ? "Older than 2.0" : view.installed,
             outdated ? ACCENT : current ? GOOD : SOFT},
            {"LATEST", ready ? view.latest.version : view.fetch == Fetch::loading ? "Checking" : "Unknown", TEXT},
            {"RUNS ON", platform::LINUX ? "Linux, Proton" : "Windows", TEXT}};
        for (int i = 0; i < 3; i++) {
            ImVec2 c0{a.x + i * (cell + S(12)), a.y}, c1{c0.x + cell, c0.y + S(78)};
            dl->AddRectFilled(c0, c1, C(RAISED), S(12));
            label_caps(dl, {c0.x + S(18), c0.y + S(16)}, stats[i].label, MUTED);
            dl->AddText(F.heading, F.heading->FontSize, {c0.x + S(18), c0.y + S(38)}, C(stats[i].color), stats[i].value.c_str());
        }
        ImGui::Dummy({width, S(78)});
    }
    ImGui::Dummy({0, S(4)});

    {
        Reveal r(2);
        ImVec2 a = ImGui::GetCursorScreenPos();
        label_caps(dl, a, "GAME FOLDER", MUTED);
        ImGui::Dummy({0, S(14)});
        ImVec2 f0 = ImGui::GetCursorScreenPos(), f1{f0.x + width - S(140), f0.y + S(46)};
        dl->AddRectFilled(f0, f1, C(RAISED), S(10));
        dl->AddRect(f0, f1, C(found ? LINE_SOFT : mix(LINE, BAD, 0.6f)), S(10));
        dl->AddCircleFilled({f0.x + S(18), (f0.y + f1.y) / 2}, S(4), C(found ? GOOD : BAD));
        std::string shown = found ? view.game.u8string() : "Dying Light was not found in your Steam libraries";
        dl->PushClipRect(f0, {f1.x - S(14), f1.y}, true);
        dl->AddText(found ? F.mono : F.small, (found ? F.mono : F.small)->FontSize, {f0.x + S(34), (f0.y + f1.y - (found ? F.mono : F.small)->FontSize) / 2}, C(found ? SOFT : MUTED),
                    shown.c_str());
        dl->PopClipRect();
        ImGui::SetCursorScreenPos({f1.x + S(12), f0.y});
        if (button("choose", view.picking ? "Choosing" : found ? "Change" : "Choose", {S(128), S(46)}, Kind::ghost, !view.picking && view.job != Job::working))
            choose_folder();
        ImGui::SetCursorScreenPos({f0.x, f1.y + S(10)});
        static char typed[1024] = "";
        static bool typing = false;
        if (view.picker_missing || typing) {
            typing = true;
            ImGui::PushFont(F.mono);
            ImGui::SetNextItemWidth(width - S(140));
            bool enter = ImGui::InputTextWithHint("##typed", "Paste the folder path here", typed, sizeof typed, ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopFont();
            ImGui::SameLine(0, S(12));
            if ((button("use", "Use folder", {S(128), S(36)}, Kind::ghost, typed[0] != 0) || enter) && typed[0]) {
                std::lock_guard<std::mutex> l(M.mx);
                use_folder(typed);
            }
            if (view.picker_missing) {
                ImGui::PushFont(F.small);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(MUTED), "No folder picker (zenity or kdialog) was found, so type or paste the folder instead.");
                ImGui::PopFont();
            }
        } else {
            ImGui::PushFont(F.small);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(MUTED), found ? "Found automatically. Change it if you have more than one copy of the game."
                                                                             : "Choose the folder that contains DyingLightGame.exe.");
            ImGui::PopFont();
            ImGui::SameLine(0, S(8));
            ImGui::PushFont(F.small);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(SOFT), "Type it instead");
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsItemClicked()) typing = true, snprintf(typed, sizeof typed, "%s", found ? view.game.u8string().c_str() : "");
        }
    }
    ImGui::Dummy({0, S(8)});

    {
        Reveal r(3);
        ImVec2 a = ImGui::GetCursorScreenPos();
        label_caps(dl, a, "UPDATES", MUTED);
        ImGui::Dummy({0, S(14)});
        ImVec2 beta_box = ImGui::GetCursorScreenPos();
        bool beta = view.beta;
        if (check_box(dl, "beta", "Beta builds", beta_box, beta, found)) toggle_ini_flag("beta", !beta);
        ImGui::SetCursorScreenPos({beta_box.x, beta_box.y + S(20) + S(8)});
        muted_note(width, beta ? "Play uses the beta build when it is newer than the latest release. Beta builds are what is being worked on right now and may have bugs."
                               : "Optional. Beta builds are the version being worked on right now, open to everyone. They may have bugs, so Play uses the published release unless you tick this.");
        ImGui::Dummy({0, S(8)});
    }

    {
        Reveal r(4);
        std::string text;
        float progress = -1;
        bool enabled = false;
        bool celebrate = view.job == Job::done && ImGui::GetTime() - view.finished_at < 2.6;
        bool install = logic::needs_install(view.installed, view.latest.version, ready, view.beta);
        bool playable = found && (ready || (!view.installed.empty() && view.installed != logic::LEGACY_VERSION));
        if (view.job == Job::working) text = view.step + (view.progress < 0.97f ? "  " + std::to_string((int)(view.progress * 100)) + "%" : ""), progress = view.progress;
        else if (celebrate) text = "Running", progress = 1;
        else if (!view.installer_update.version.empty()) text = "Update FaTrainer to " + view.installer_update.version, enabled = true;
        else if (view.fetch == Fetch::loading) text = "Checking for updates";
        else if (!found) text = "Choose the game folder first";
        else if (!playable) text = "Cannot reach GitHub";
        else text = !install ? "Play" : (newer_version(view.latest.version, view.installed) ? "Update to " : "Install ") + view.latest.version + " and play", enabled = true;
        ImVec2 at = ImGui::GetCursorScreenPos();
        float shown_progress = progress >= 0 ? follow(anim("progress"), progress, 10) : (anim("progress") = 0);
        if (button("install", text, {S(300), S(52)}, Kind::primary, enabled, progress >= 0 ? shown_progress : -1))
            view.installer_update.version.empty() ? start_play() : start_self_update();
        if (celebrate) check_mark(dl, {at.x + S(300) / 2 - text_w(F.strong, "Running") / 2 - S(22), at.y + S(26)}, S(14), (float)(ImGui::GetTime() - view.finished_at) * 2.2f, C(ACCENT_INK));
        if (!view.installed.empty() && view.job != Job::working && view.installer_update.version.empty()) {
            ImGui::SameLine(0, S(12));
            if (button("uninstall", "Remove from game", {S(160), S(52)}, Kind::quiet)) uninstall();
        }
        std::string message = view.message;
        if (message.empty() && view.fetch == Fetch::failed) message = "Could not check for the latest release: " + view.fetch_error + ".", view.message_bad = true;
        if (message.empty() && !view.installer_update.version.empty())
            message = "FaTrainer " + view.installer_update.version + " is out (this is " INSTALLER_VERSION "). It updates itself first. Your game is not touched.";
        if (!message.empty()) {
            float shown = ease_out((float)(ImGui::GetTime() - view.finished_at) / 0.5f);
            if (view.finished_at < 0) shown = 1;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * shown);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(view.message_bad ? BAD : SOFT), "%s", message.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleVar();
        }
    }

    if (platform::LINUX) {
        ImGui::Dummy({0, S(10)});
        Reveal r(5);
        ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + width, a.y + S(132)};
        dl->AddRectFilled(a, b, C(RAISED), S(12));
        label_caps(dl, {a.x + S(20), a.y + S(18)}, "STEAM LAUNCH OPTION", MUTED);
        dl->AddText(F.small, F.small->FontSize, {a.x + S(20), a.y + S(40)}, C(SOFT),
                    "Proton needs this once: Steam, right-click Dying Light, Properties, Launch options.");
        ImVec2 c0{a.x + S(20), a.y + S(68)}, c1{b.x - S(130), a.y + S(112)};
        dl->AddRectFilled(c0, c1, C(GROUND), S(8));
        dl->AddText(F.mono, F.mono->FontSize, {c0.x + S(14), (c0.y + c1.y - F.mono->FontSize) / 2}, C(ACCENT), LAUNCH_OPTION);
        static double copied_at = -10;
        ImGui::SetCursorScreenPos({c1.x + S(12), c0.y});
        if (button("copy", ImGui::GetTime() - copied_at < 1.8 ? "Copied" : "Copy", {S(98), S(44)}, Kind::ghost)) {
            ImGui::SetClipboardText(LAUNCH_OPTION);
            copied_at = ImGui::GetTime();
        }
        ImGui::SetCursorScreenPos({a.x, b.y});
        ImGui::Dummy({width, 0});
        ImGui::Dummy({0, S(2)});
        shortcut_card(width);
    } else {
        ImGui::Dummy({0, S(10)});
        Reveal r(5);
        shortcut_card(width);
    }
    ImGui::Dummy({0, S(24)});
}

inline void changelog(float width) {
    std::vector<logic::ChangelogEntry> entries;
    {
        std::lock_guard<std::mutex> l(M.mx);
        entries = M.changelog;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (size_t i = 0; i < entries.size(); i++) {
        Reveal r((int)std::min<size_t>(i, 6));
        ImVec2 a = ImGui::GetCursorScreenPos();
        float version_size = F.hero->FontSize * (i == 0 ? 1.0f : 0.7f);
        float version_w = std::max(S(150), F.hero->CalcTextSizeA(version_size, FLT_MAX, 0, entries[i].version.c_str()).x + S(24));
        dl->AddText(F.hero, version_size, {a.x, a.y - S(4)}, C(i == 0 ? ACCENT : TEXT), entries[i].version.c_str());
        float caps_y = a.y + version_size + S(4), caps_step = F.label->FontSize + S(5);
        if (!entries[i].name.empty()) label_caps(dl, {a.x + S(2), caps_y}, entries[i].name.c_str(), i == 0 ? ACCENT : MUTED), caps_y += caps_step;
        if (i == 0) label_caps(dl, {a.x + S(2), caps_y}, "NEWEST", MUTED), caps_y += caps_step;
        ImGui::SetCursorScreenPos({a.x + version_w, a.y + S(4)});
        ImGui::BeginGroup();
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width - version_w - S(10));
        for (const std::string& line : entries[i].lines) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            dl->AddCircleFilled({p.x + S(3), p.y + F.body->FontSize * 0.55f}, S(2.5f), C(line.rfind("New", 0) == 0 ? ACCENT : line.rfind("Fixed", 0) == 0 ? GOOD : MUTED));
            ImGui::SetCursorScreenPos({p.x + S(16), p.y});
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(i == 0 ? TEXT : SOFT));
            ImGui::TextWrapped("%s", line.c_str());
            ImGui::PopStyleColor();
            ImGui::Dummy({0, S(2)});
        }
        ImGui::PopTextWrapPos();
        ImGui::EndGroup();
        ImVec2 end = ImGui::GetCursorScreenPos();
        float bottom = std::max(end.y, caps_y + S(6));
        dl->AddLine({a.x, bottom + S(14)}, {a.x + width, bottom + S(14)}, C(LINE_SOFT));
        ImGui::SetCursorScreenPos({a.x, bottom + S(34)});
        ImGui::Dummy({width, 0});
    }
}

inline void announcement_dialog(ImVec2 size, float t) {
    logic::Announcement a;
    {
        std::lock_guard<std::mutex> l(M.mx);
        a = M.announcement;
    }
    if (a.id.empty() || t < CONTENT_AT + 0.6f) return;
    float shown = ease_out(follow(anim("announcement"), 1.0f, 6));
    const float W = std::min(size.x - S(48), S(560)), PAD = S(28);
    ImGui::SetNextWindowPos({size.x / 2, size.y / 2 + (1 - shown) * S(24)}, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({W, 0});
    ImGui::SetNextWindowSizeConstraints({W, 0}, {W, size.y - S(80)});
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImGui::ColorConvertU32ToFloat4(C(GROUND, 0.72f * shown)));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(RAISED));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {PAD, PAD});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, S(16));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, shown);
    if (!ImGui::IsPopupOpen("announcement")) ImGui::OpenPopup("announcement");
    if (ImGui::BeginPopupModal("announcement", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        dl->AddRectFilled({p.x, p.y + S(3)}, {p.x + S(8), p.y + S(11)}, C(ACCENT), S(4));
        label_caps(dl, {p.x + S(16), p.y}, "ANNOUNCEMENT", ACCENT);
        ImGui::Dummy({0, S(22)});
        ImGui::PushFont(F.heading);
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(a.title.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopFont();
        if (!a.text.empty()) {
            ImGui::Dummy({0, S(2)});
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(SOFT));
            ImGui::PushTextWrapPos(0);
            ImGui::TextUnformatted(a.text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }
        ImGui::Dummy({0, S(10)});
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - PAD - S(140));
        if (button("announcement_ok", "Got it", {S(140), S(46)}, Kind::primary)) {
            dismiss_announcement(a.id);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(2);
}

inline void frame() {
    ImGuiIO& io = ImGui::GetIO();
    float t = (float)ImGui::GetTime();
    {
        std::lock_guard<std::mutex> l(M.mx);
        if (M.restart && ImGui::GetTime() - M.finished_at > 1.2) M.restart = false, platform::restart_self();
    }
    ImVec2 size = io.DisplaySize;
    ImGui::GetBackgroundDrawList()->AddRectFilled({0, 0}, size, GROUND);

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(size);
    ImGui::Begin("FaTrainer", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float MARGIN = S(36), BAR = S(84);
    platform::caption_height = BAR;
    platform::caption_holes.clear();
    float controls_left = window_controls(size.x);
    const char* WORD = "FaTrainer";
    float move = ease_in_out((t - 0.95f) / 0.8f);
    float font_size = lerp(F.intro->FontSize, F.brand->FontSize, move);
    ImFont* font = move >= 1 ? F.brand : F.intro;
    ImVec2 start{(size.x - text_w(F.intro, WORD)) / 2, size.y / 2 - F.intro->FontSize * 0.62f}, end{MARGIN, S(28)};
    draw_letters(dl, font, font_size, lerp(start, end, move), WORD, t - 0.15f, TEXT);
    float intro_line = ease_expo((t - 0.55f) / 0.6f) * (1 - move);
    if (intro_line > 0) {
        float w = text_w(F.intro, WORD) * intro_line;
        dl->AddRectFilled({size.x / 2 - w / 2, start.y + F.intro->FontSize * 1.18f}, {size.x / 2 + w / 2, start.y + F.intro->FontSize * 1.18f + S(3)}, C(ACCENT));
    }
    float shown = ease_out((t - CONTENT_AT) / 0.7f);
    if (shown > 0) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, shown);
        float brand_w = text_w(F.brand, WORD);
        const char* SUBTITLE = "Launcher " INSTALLER_VERSION "  \xc2\xb7  " FATRAINER_EDITION;
        dl->AddText(F.small, F.small->FontSize, {MARGIN + brand_w + S(12), S(28) + F.brand->FontSize - F.small->FontSize - S(2)}, C(MUTED), SUBTITLE);
        tabs({MARGIN + brand_w + S(12) + text_w(F.small, SUBTITLE) + S(44), S(32)});
        status_pill({std::min(size.x - MARGIN, controls_left - S(16)), S(24)});
        dl->AddLine({MARGIN, BAR}, {MARGIN + (size.x - 2 * MARGIN) * shown, BAR}, C(LINE_SOFT));
        ImGui::PopStyleVar();

        float top = BAR + S(28), height = size.y - top;
        if (tab == 0) {
            float list_w = std::clamp(size.x * 0.24f, S(220), S(290));
            std::string state;
            {
                std::lock_guard<std::mutex> l(M.mx);
                bool ready = M.fetch == Fetch::ready;
                if (M.installed == logic::LEGACY_VERSION || (!M.installed.empty() && ready && newer_version(M.latest.version, M.installed))) state = "update";
                else if (!M.installed.empty()) state = "installed";
            }
            game_list({MARGIN, top}, list_w, state);
            float x = MARGIN + list_w + S(32), w = size.x - x - MARGIN;
            ImGui::SetCursorScreenPos({x, top});
            ImGui::BeginChild("detail", {w + S(12), height}, 0, ImGuiWindowFlags_NoBackground);
            detail(w);
            ImGui::EndChild();
        } else {
            float w = std::min(size.x - 2 * MARGIN, S(860));
            ImGui::SetCursorScreenPos({(size.x - w) / 2, top});
            ImGui::BeginChild("changelog", {w + S(12), height}, 0, ImGuiWindowFlags_NoBackground);
            changelog(w);
            ImGui::EndChild();
        }
    }
    ImGui::End();
    announcement_dialog(size, t);
    soft_glow(ImGui::GetForegroundDrawList(), {size.x * 0.82f + sinf(t * 0.21f) * S(60), -S(120)}, S(520), ACCENT, 0.08f);
}

}
