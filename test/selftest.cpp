#include <cstdio>
#include <fstream>
#include <list>
#include "cheats.h"
#include "color.h"
#include "visuals.h"
#include "crash.h"
#include "config.h"
#include "fake.h"
#include "input.h"
#include "version.h"
#include "../installer/logic.h"
#include "cheat_engine.h"
#include "teleport.h"
#include "dlc.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static fake::World w;

static void __fastcall fake_add(uintptr_t inv, void* data, int, bool) {
    int count;
    uintptr_t desc;
    memcpy(&count, (uint8_t*)data, 4);
    memcpy(&desc, (uint8_t*)data + 0x20, 8);
    uintptr_t arr = game::rdv<uintptr_t>(inv + 0x40);
    uint32_t n = game::rdv<uint32_t>(inv + 0x48);
    uintptr_t it = w.alloc(0x100);
    w.put<int>(it + 0x40, count);
    w.put<uintptr_t>(it + 0x60, desc);
    w.put<uintptr_t>(arr + n * 8, it);
    w.put<uint32_t>(inv + 0x48, n + 1);
}

static void erase_item(uintptr_t inv, uintptr_t item) {
    uintptr_t arr = game::rdv<uintptr_t>(inv + 0x40);
    uint32_t n = game::rdv<uint32_t>(inv + 0x48);
    for (uint32_t i = 0; i < n; i++)
        if (game::rdv<uintptr_t>(arr + i * 8) == item) {
            memmove((void*)(arr + i * 8), (void*)(arr + (i + 1) * 8), (n - i - 1) * 8);
            w.put<uint32_t>(inv + 0x48, n - 1);
            return;
        }
}

static bool __fastcall fake_remove_one(uintptr_t inv, uintptr_t item, int) {
    int count = game::rdv<int>(item + 0x40);
    if (count > 1) w.put<int>(item + 0x40, count - 1);
    else erase_item(inv, item);
    return true;
}

static int __fastcall fake_remove_stack(uintptr_t inv, uintptr_t item, int, int) {
    int count = game::rdv<int>(item + 0x40);
    if (count <= 1) return 0;
    erase_item(inv, item);
    return count;
}

struct FakeElement {
    uintptr_t vt = 1;
    const char* id;
    int kind;
    uintptr_t engine = 0;
    float color[3] = {1, 1, 1}, original[3] = {1, 1, 1};
    std::vector<FakeElement*> children;
    float alpha = 1, written_alpha = -1, opacity = 1;
    bool visible = true;
};
static int dead_writes = 0;
static FakeElement* as_element(uintptr_t e) { return (FakeElement*)e; }
static int __fastcall fake_children_count(uintptr_t e) { return (int)as_element(e)->children.size(); }
static uintptr_t __fastcall fake_child(uintptr_t e, int i) { return (uintptr_t)as_element(e)->children[i]; }
static float* __fastcall fake_get_color(uintptr_t e, float* out) { memcpy(out, as_element(e)->color, 12); return out; }
static void __fastcall fake_set_color(uintptr_t e, const float* rgb) {
    if (as_element(e)->vt != 1) dead_writes++;
    memcpy(as_element(e)->color, rgb, 12);
}
static void __fastcall fake_set_text_color(uintptr_t e, const float* rgba) {
    fake_set_color(e, rgba);
    as_element(e)->written_alpha = rgba[3];
}
static float fake_text_alpha(uintptr_t e) { return as_element(e)->alpha; }
static std::string fake_name_of(uintptr_t e) { return as_element(e)->id; }
static int fake_kind_of(uintptr_t e, uintptr_t* at) { *at = e; return as_element(e)->kind; }
static uintptr_t fake_vtable_of(uintptr_t e) { return as_element(e)->vt; }
static float __fastcall fake_get_opacity(uintptr_t e) { return as_element(e)->opacity; }
static void __fastcall fake_engine_opacity(uintptr_t engine, float v) { as_element(engine)->opacity = v; }
static void __fastcall fake_set_opacity(uintptr_t e, float v) {
    if (visuals::opacity_original && as_element(e)->engine) visuals::opacity_hook(as_element(e)->engine, v);
    else as_element(e)->opacity = v;
}
static bool __fastcall fake_shown(uintptr_t e) { return as_element(e)->visible; }

static std::list<std::wstring> fake_text_storage;
static std::map<std::string, visuals::TextEntry> fake_text_entries;
static visuals::TextEntry* __fastcall fake_find_text(uintptr_t, const char* const* key) {
    auto it = fake_text_entries.find(*key);
    return it == fake_text_entries.end() ? nullptr : &it->second;
}
static bool __fastcall fake_add_text(uintptr_t, const visuals::TtlString<char>* key, const visuals::TtlString<wchar_t>* text, bool) {
    fake_text_storage.push_back(std::wstring(text->text, text->size));
    fake_text_entries[key->text].text = fake_text_storage.back().data();
    return true;
}
static void fake_load_text(const char* key, const wchar_t* text) {
    fake_text_storage.push_back(text);
    fake_text_entries[key] = {key, fake_text_storage.back().data()};
}

static bool __fastcall fake_yes(uintptr_t) { return true; }
static bool __fastcall fake_no(uintptr_t) { return false; }

static float fake_var_float(uintptr_t, uintptr_t name, uintptr_t, uintptr_t) {
    const char* s = *(const char**)name;
    if (!strcmp(s, "f_btz_zombie_grab_range")) return 10;
    if (!strcmp(s, "f_btz_wrestling_kick_angle_max")) return 22;
    if (!strcmp(s, "f_btz_pvp_grab_below_angle_threshold")) return -50;
    return 7;
}
static float* fake_var_vec3(uintptr_t, float* out, uintptr_t name, uintptr_t, uintptr_t) {
    const char* s = *(const char**)name;
    float veins[3] = {255, 50, 0}, other[3] = {1, 2, 3};
    memcpy(out, strstr(s, "veins") && !strstr(s, "not_drained") ? veins : other, sizeof veins);
    return out;
}
static uintptr_t fake_var_vtable[1100];
static uintptr_t fake_var_object = (uintptr_t)&fake_var_vtable[1];
static uintptr_t fake_var_holder = (uintptr_t)&fake_var_object;

static float read_var(const char* name) {
    auto fn = (cheats::VarFloatFn)cheats::slot((uintptr_t)&fake_var_object, cheats::SLOT_VAR_FLOAT);
    return fn((uintptr_t)&fake_var_object, (uintptr_t)&name, 0, 0);
}

static std::vector<float> read_vec3(const char* name) {
    auto fn = (cheats::VarVec3Fn)cheats::slot((uintptr_t)&fake_var_object, cheats::SLOT_VAR_VEC3);
    float out[3];
    float* v = fn((uintptr_t)&fake_var_object, out, (uintptr_t)&name, 0, 0);
    return {v[0], v[1], v[2]};
}

static void __fastcall refuse_add(uintptr_t, void*, int, bool) {}
static uintptr_t fake_cache_base;
static uintptr_t fake_cache_get(uintptr_t, int id) { return fake_cache_base + id * 40 + 8; }
static int fake_level_calls = 0;
static int ownership_requests = 0;
static void fake_request_ownership(uintptr_t repl) { ownership_requests++; *(uint8_t*)(repl + 0x28) = 1; }
static void __fastcall fake_level_from_xp(uintptr_t, int) { fake_level_calls++; }
static void __fastcall fake_set_level(uintptr_t container, int16_t level, int type) {
    uintptr_t record = game::rdv<uintptr_t>(container + 0x40) + type * 0x20;
    game::wr<uint16_t>(record + 0x14, (uint16_t)level);
    game::wr<uint32_t>(record + 0xc, level * 1000);
    game::wr<uint32_t>(record + 0x10, 1000);
}

static std::map<int, int> fake_learned;
static std::vector<std::vector<uint8_t>> fake_nodes;
static int fake_skill_calls = 0;

static void fake_store_learned(uintptr_t container) {
    const size_t NODE = 0x30, AT = 0x10;
    fake_nodes.clear();
    for (size_t i = 0; i < fake_learned.size(); i++) fake_nodes.emplace_back(NODE);
    uintptr_t next = 0;
    auto it = fake_learned.rbegin();
    for (size_t i = 0; i < fake_nodes.size(); i++, ++it) {
        uintptr_t node = (uintptr_t)fake_nodes[i].data() + AT;
        *(int16_t*)(node - 0x10) = (int16_t)it->first;
        *(int16_t*)(node - 0xa) = (int16_t)it->second;
        *(uintptr_t*)(node + 0x10) = next;
        next = node;
    }
    *(uintptr_t*)(container + 0xd8) = next;
}
static int __fastcall fake_add_skill(uintptr_t container, uint16_t index, int, bool, bool skip_checks) {
    fake_skill_calls++;
    if (!skip_checks) return 3;
    fake_learned[index]++;
    fake_store_learned(container);
    return 0;
}
static void __fastcall fake_remove_skill(uintptr_t container, uint16_t index) {
    fake_skill_calls++;
    fake_learned.erase(index);
    fake_store_learned(container);
}

static uintptr_t fake_part_store[game::MAX_UPGRADES], fake_cache_object[4], fake_random_mod = 0;
static uintptr_t __fastcall fake_context_ctor(void* self, uintptr_t desc, uint32_t word, uintptr_t mod, uintptr_t blueprint, const game::TtlVector* parts) {
    if (!mod) mod = fake_random_mod;
    auto* c = (uint8_t*)self;
    memcpy(c + game::CONTEXT_DESC, &desc, 8);
    memcpy(c + game::CONTEXT_WORD, &word, 4);
    *(int*)(c + 0x14) = 7;
    memcpy(c + game::CONTEXT_MOD, &mod, 8);
    memcpy(c + game::CONTEXT_BLUEPRINT, &blueprint, 8);
    uint32_t n = std::max<uint32_t>(parts->size, 2);
    memset(fake_part_store, 0, sizeof fake_part_store);
    memcpy(fake_part_store, parts->data, parts->size * 8);
    uintptr_t store = (uintptr_t)fake_part_store, cache = parts->size ? (uintptr_t)fake_cache_object : 0;
    memcpy(c + game::CONTEXT_PARTS, &store, 8);
    memcpy(c + game::CONTEXT_PART_COUNT, &n, 4);
    memcpy(c + game::CONTEXT_PART_COUNT + 4, &n, 4);
    *(uint32_t*)(c + 0x40) = 0x4c;
    memcpy(c + 0x48, &cache, 8);
    return (uintptr_t)self;
}

static void quiet(const char*, ...) {}

static uintptr_t fake_tree_node(uintptr_t key_back, uint32_t key, uintptr_t less, uintptr_t more) {
    uintptr_t block = w.alloc(0x200), node = block + 0x180;
    w.put<uint32_t>(node - key_back, key);
    w.put<uintptr_t>(node + game::TREE_LESS, less);
    w.put<uintptr_t>(node + game::TREE_MORE, more);
    return node;
}

int main(int argc, char** argv) {
    CHECK(newer_version("2.1", "2.0") && newer_version("2.0.1", "2.0") && newer_version("10.0", "9.9"));
    CHECK(!newer_version("2.0", "2.0") && !newer_version("2.0.0", "2.0") && !newer_version("1.9", "2.0") && !newer_version("", "2.0") && !newer_version("2.x", "2.0"));
    CHECK(newer_version("2.2", "2.2 b3") && newer_version("2.2 b2", "2.2 b1") && newer_version("2.2 b10", "2.2 b9") && newer_version("2.3", "2.2 b9"));
    CHECK(newer_version("2.3", "2.2") && newer_version("2.3 b1", "2.2") && newer_version("2.2 b999", "2.2 b2") && !newer_version("2.3 b1", "2.3"));
    CHECK(!newer_version("2.2 b1", "2.2") && !newer_version("2.2", "2.2") && !newer_version("2.2 b1", "2.2 b1") && !newer_version("2.2 b0", "2.1") && !newer_version("2.2 bx", "2.1") && !newer_version("2.2b1", "2.1"));
    CHECK(version_slug("2.2 b1") == "2.2-b1" && parse_release_info("version 2.2 b3\nsha256 ab\n").version == "2.2 b3");
    CHECK(parse_release_info("build 2.2 b3\nversion 2.2\n").version == "2.2 b3" && parse_release_info("version 2.2\nbuild 2.2 b3\n").version == "2.2 b3");
    CHECK(parse_release_info("version 2.2\nbuild 2.2 b999\nrelease 2.2\n").version == "2.2" && parse_release_info("release 2.2\nbuild 2.2 b999\n").version == "2.2");
    CHECK(logic::trainer_version_in(std::string("xx\0FaTrainer-version:2.2 b3\0yy", 31)) == "2.2 b3");
    CHECK(launch_armed(1000, 1000) && launch_armed(1000, 1180) && !launch_armed(1000, 1181) && launch_armed(1030, 1000) && !launch_armed(1031, 1000));
    CHECK(!launch_armed(0, 1700000000) && !newer_version(FIRST_ARMED_VERSION, VERSION) && newer_version(FIRST_ARMED_VERSION, "2.3 b3"));
    ReleaseInfo info = parse_release_info("version 2.1\r\nsha256 ab12\nsize 2598750\n");
    CHECK(info.version == "2.1" && info.sha256 == "ab12" && info.size == 2598750);
    CHECK(!strcmp(VERSION, FATRAINER_VERSION));
    CHECK(logic::sha256_hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    {
        std::string good = "version=7.7\r\nurl=https://d1dj9aohuk02ls.cloudfront.net/f/CheatEngine/2129/CheatEngine77.exe\nsize=34690856\nsha256=cf0f4b6002555677984233c95856683959be83a5def3dc12175a8296eb22676b\n";
        auto info = logic::parse_cheat_engine_info(good);
        CHECK(info.version == "7.7" && info.size == 34690856 && info.url.rfind("https://", 0) == 0 && info.sha256.size() == 64);
        std::string hash = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
        CHECK(logic::parse_cheat_engine_info("version=7.7\nurl=http://x/y.exe\nsize=3\nsha256=" + hash).version.empty());
        CHECK(logic::parse_cheat_engine_info("version=7.7\nurl=https://x/y.exe\nsize=0\nsha256=" + hash).version.empty());
        CHECK(logic::parse_cheat_engine_info("version=7.7\nurl=https://x/y.exe\nsize=3\nsha256=" + hash.substr(1)).version.empty());
        CHECK(logic::parse_cheat_engine_info("version=7.7\nurl=https://x/y.exe\nsize=3\n").version.empty());
        CHECK(logic::parse_cheat_engine_info("version=7.7/../x\nurl=https://x/y.exe\nsize=3\nsha256=" + hash).version.empty());
        CHECK(logic::parse_cheat_engine_info("").version.empty() && logic::parse_cheat_engine_info("<html>404</html>").version.empty());
        logic::CheatEngineInfo abc{"1", "https://x/y.exe", hash, 3};
        CHECK(logic::cheat_engine_file_ok("abc", abc) && !logic::cheat_engine_file_ok("abd", abc) && !logic::cheat_engine_file_ok("abcd", abc));
    }
    CHECK(logic::game_process("Z:\\mnt\\data\\Gaming\\Steam\\steamapps\\common\\Dying Light\\DyingLightGame.exe") && logic::game_process("DyingLightGame.exe"));
    CHECK(logic::game_process("/home/x/.steam/steamapps/common/Dying Light/dyinglightgame.EXE") && !logic::game_process("grep DyingLightGame.exe"));
    CHECK(!logic::game_process("/usr/bin/steam") && !logic::game_process("") && !logic::game_process("C:\\DyingLightGame.exe.bak"));
    CHECK(logic::sha256_hex(std::string(1000, 'a')) == "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");
    auto libraries = logic::vdf_values("\"0\"\n{\n\t\t\"path\"\t\t\"D:\\\\Games\\\\Steam\"\n}\n\"1\" { \"path\" \"/home/a b/Steam\" }", "path");
    CHECK(libraries.size() == 2 && libraries[0] == "D:\\Games\\Steam" && libraries[1] == "/home/a b/Steam");
    CHECK(logic::trainer_version_in(std::string("xx\0FaTrainer-version:2.0\0yy", 28)) == "2.0");
    CHECK(logic::trainer_version_in("..FaTrainer | Dying Light..") == logic::LEGACY_VERSION && logic::trainer_version_in("MZ other mod").empty());
    {
        const char* hypr = "Monitor DP-5 (ID 1):\n\t1920x1080@179.96400 at 0x500\n\treserved: 0 0 56 0\n\tscale: 1\n\tfocused: no\n"
                           "Monitor DP-6 (ID 0):\n\t2560x1440@180.00000 at 1920x0\n\treserved: 0 40 0 0\n\tscale: 1.25\n\tfocused: yes\n";
        logic::Screen screen = logic::hyprland_focused_monitor(hypr);
        CHECK(screen.work.x == 1920 && screen.work.y == 40 && screen.work.w == 2048 && screen.work.h == 1112);
        CHECK(screen.full.x == 1920 && screen.full.y == 0 && screen.full.w == 2048 && screen.full.h == 1152);
        CHECK(logic::hyprland_focused_monitor("garbage").full.w == 0);
        logic::Screen desk{{1920, 0, 2560, 1440}, {1976, 0, 2504, 1440}};
        logic::Area spot = logic::centered(desk, 1120, 740);
        CHECK(spot.x == 2640 && spot.y == 350);
        logic::Screen small{{0, 0, 1366, 768}, {0, 40, 1366, 728}};
        spot = logic::centered(small, 1120, 740);
        CHECK(spot.x == 123 && spot.y == 40);
        logic::Screen side{{0, 0, 1200, 900}, {300, 0, 900, 900}};
        spot = logic::centered(side, 1120, 740);
        CHECK(spot.x == 300 && spot.y == 80);
        CHECK(newer_version("9.9", INSTALLER_VERSION) && !newer_version(INSTALLER_VERSION, INSTALLER_VERSION));
    }
    {
        std::string json = "[{\"tag_name\": \"installer\", \"assets\": [{\"browser_download_url\": \"https://x/installer-linux.txt\"}]},"
                           "{\"tag_name\": \"v2.2-b1\", \"assets\": [{\"browser_download_url\": \"https://x/v2.2-b1/xinput1_3.dll\"}, {\"browser_download_url\": \"https://x/v2.2-b1/version.txt\"}]},"
                           "{\"tag_name\": \"v1.4\", \"assets\": [{\"browser_download_url\": \"https://x/v1.4/xinput1_3.dll\"}]},"
                           "{\"tag_name\": \"v2.1\", \"assets\": [{\"browser_download_url\": \"https://x/v2.1/version.txt\"}, {\"browser_download_url\": \"https://x/v2.1/xinput1_3.dll\"}]}]";
        auto releases = logic::installable_releases(json);
        CHECK(releases.size() == 2 && releases[0].tag == "v2.2-b1" && releases[0].version == "2.2 b1" && releases[1].version == "2.1");
        std::string builds = "[{\"tag_name\": \"v2.2-b1\", \"a\": \"/xinput1_3.dll\" \"/version.txt\"},{\"tag_name\": \"v2.2-b3\", \"a\": \"/xinput1_3.dll\" \"/version.txt\"},"
                             "{\"tag_name\": \"v2.2\", \"a\": \"/xinput1_3.dll\" \"/version.txt\"},{\"tag_name\": \"v2.3\", \"a\": \"/xinput1_3.dll\" \"/version.txt\"}]";
        auto newest = logic::installable_releases(builds);
        CHECK(newest.size() == 2 && newest[0].version == "2.3" && newest[1].version == "2.2" && newest[1].tag == "v2.2");
        CHECK(logic::needs_install("", "2.3", true, false) && logic::needs_install(logic::LEGACY_VERSION, "2.3", true, false) && logic::needs_install("2.2", "2.3", true, false));
        CHECK(logic::trainer_edition_in(std::string("xx FaTrainer-version:2.5 b6\0FaTrainer-edition:Visuals\0", 54)) == "Visuals");
        CHECK(logic::trainer_edition_in("FaTrainer-version:2.4 FaTrainer | Dying Light") == "Fatum" && logic::trainer_edition_in("").empty());
        CHECK(logic::edition_differs("Fatum", true) && logic::edition_differs("Visuals", false) && !logic::edition_differs("Visuals", true) && !logic::edition_differs("", true));
        CHECK(!logic::needs_install("2.3", "2.3", true, false) && !logic::needs_install("2.2", "2.3", false, false) && !logic::needs_install("", "2.3", false, true));
        CHECK(!logic::needs_install("2.4 b1", "2.3", true, true) && logic::needs_install("2.4 b1", "2.3", true, false) && logic::needs_install("2.3", "2.4 b1", true, true));
        std::filesystem::path launch_dir = std::filesystem::temp_directory_path() / "fatrainer-launch-test";
        std::error_code launch_ec;
        std::filesystem::create_directories(launch_dir, launch_ec);
        CHECK(logic::write_launch_file(launch_dir, 1234));
        std::string launch_text;
        std::ifstream(launch_dir / LAUNCH_FILE) >> launch_text;
        CHECK(launch_text == "1234");
        CHECK(!logic::write_launch_file(launch_dir / "missing" / "deeper", 1234));
        std::filesystem::remove_all(launch_dir, launch_ec);
        CHECK(same_version("2.2 b1", "2.2 b3") && same_version("2.2", "2.2 b1") && !same_version("2.2 b1", "2.3") && !same_version("2.2.1", "2.2"));
        std::string ini = "accent=1,1,1\r\nallow_older=0\ncheat=god\n";
        CHECK(logic::ini_value(ini, "allow_older") == "0" && logic::ini_value(ini, "missing").empty());
        std::string changed = logic::with_ini_value(ini, "allow_older", "1");
        CHECK(changed == "accent=1,1,1\nallow_older=1\ncheat=god\n" && logic::with_ini_value("", "allow_older", "1") == "allow_older=1\n");
        auto news = logic::parse_announcement("id=7\r\ntitle=Servers down\r\n\r\nFirst line.\r\n\r\nSecond line.\r\n");
        CHECK(news.id == "7" && news.title == "Servers down" && news.text == "First line.\n\nSecond line.");
        CHECK(logic::parse_announcement("").id.empty() && logic::parse_announcement("title=No id\n\nText").title.empty() && logic::parse_announcement("404: Not Found").id.empty());
    }
    auto log = logic::parse_changelog("# Changelog\n\n## 2.0\n\nInstaller Update\n\n- One\n  more\n- Two\n\n## 1.9\n\n- Three\n");
    CHECK(log.size() == 2 && log[0].version == "2.0" && log[0].name == "Installer Update" && log[1].name.empty() && log[0].lines.size() == 2 && log[0].lines[0] == "One more" && log[1].lines[0] == "Three");
    HMODULE m = LoadLibraryExA(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    CHECK(m);
    if (!m) return 1;
    CHECK(game::resolve_classes((uintptr_t)m));
    CHECK(game::g.vt_manager && game::g.vt_inv[1] && game::g.vt_inv[2]);
    {
        const uint8_t remove_one_start[] = {0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C, 0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57};
        const uint8_t remove_stack_start[] = {0x40, 0x55, 0x57, 0x41, 0x55, 0x41, 0x56, 0x48, 0x83, 0xEC, 0x28, 0x45, 0x8B, 0xE9};
        for (int i = 0; i < 3; i++) {
            uintptr_t vt = game::g.vt_inv[i];
            CHECK(vt && !memcmp((const void*)game::rdv<uintptr_t>(vt + game::SLOT_REMOVE_ONE * 8), remove_one_start, sizeof remove_one_start));
            CHECK(vt && !memcmp((const void*)game::rdv<uintptr_t>(vt + game::SLOT_REMOVE_STACK * 8), remove_stack_start, sizeof remove_stack_start));
        }
    }
    const uint8_t* f16 = *(const uint8_t**)(game::g.vt_money + 16 * 8);
    CHECK(memcmp(f16 + 7, "\x8b\x41\x40", 3) == 0);

    w.build();
    game::refresh();
    {
        uint64_t to[2] = {7, 7};
        static uint64_t from[2] = {0x1111, 0x2222};
        CHECK(game::safe_copy(to, (uintptr_t)from, sizeof from) && to[0] == 0x1111 && to[1] == 0x2222);
        void* gone = VirtualAlloc(nullptr, 0x2000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        VirtualFree((char*)gone + 0x1000, 0x1000, MEM_DECOMMIT);
        static uint64_t big[0x2000 / 8];
        CHECK(!game::safe_copy(big, (uintptr_t)gone, 0x2000));
        CHECK(game::safe_copy(to, (uintptr_t)from, sizeof from));
        VirtualFree(gone, 0, MEM_RELEASE);
    }
    printf("status: %s\nstats: %s\n", game::g.status.c_str(), game::stat_report().c_str());
    CHECK(game::g.wallets.size() == 1 && game::money(game::g.wallets[0]) == 15855);
    auto* bp = game::find_inventory(game::K_BACKPACK);
    auto* st = game::find_inventory(game::K_STASH);
    auto* mat = game::find_inventory(game::K_MATERIALS);
    CHECK(bp && bp->items.size() == 6 && bp->capacity == 14);
    CHECK(st && st->items.size() == 3);
    CHECK(std::count_if(game::g.invs.begin(), game::g.invs.end(), [](auto& i) { return i.kind == game::K_STASH; }) == 2);
    CHECK(mat && mat->items.size() == 5 && mat->items[0].info && !strcmp(mat->items[0].info->id, "Craft_Gauze"));
    CHECK(game::g.descs.size() == (size_t)ITEM_COUNT);
    CHECK(game::g.stats[ST_Damage].off == fake::DAMAGE_OFF && game::g.stats[ST_Damage].is_float);
    CHECK(game::g.stats[ST_Condition].off == fake::CONDITION_OFF && !game::g.stats[ST_Condition].is_float);
    CHECK(game::g.stats[ST_Color].off == fake::COLOR_OFF);
    CHECK(game::g.stats[ST_Price].off < 0);

    uintptr_t machete = w.desc["Melee_MacheteAGen"];
    CHECK(game::get_stat(machete, ST_Damage) == 184.0f);
    CHECK(game::set_stat(machete, ST_Damage, 999.5f) && game::get_stat(machete, ST_Damage) == 999.5f);
    CHECK(game::set_stat(machete, ST_Condition, 500) && game::rdv<int>(machete + fake::CONDITION_OFF) == 500);
    uintptr_t gen = w.item("Melee_MacheteAGen", 1), plain = w.item("Melee_MacheteAGen", 1);
    game::wr<uint32_t>(gen + game::ITEM_CONTEXT, (3 << 3) | 2);
    CHECK(game::rarity(gen, machete) == 2 && game::rarity(plain, machete) == 0);
    CHECK(game::set_rarity(gen, machete, 4) && game::rdv<uint32_t>(gen + game::ITEM_CONTEXT) == ((3 << 3) | 4));
    CHECK(game::set_rarity(gen, machete, 0) && game::rarity(gen, machete) == 0 && game::rdv<uint32_t>(gen + game::ITEM_CONTEXT) >> 3 == 3);
    CHECK(game::get_stat(machete, ST_Color) == 0);
    CHECK(game::set_rarity(plain, machete, 3) && game::get_stat(machete, ST_Color) == 3 && !game::rdv<uint32_t>(plain + game::ITEM_CONTEXT));
    uintptr_t potion = w.item("Melee_MacheteAGen", 1);
    game::wr<uint32_t>(potion + game::ITEM_CONTEXT, 0x62f69000);
    CHECK(game::rarity(potion, machete) == 3);
    game::set_stat(machete, ST_Color, 0);

    CHECK(game::money(game::g.wallets[0]) == 15855 && game::set_money(game::g.wallets[0], 30000) &&
          game::money(game::g.wallets[0]) == 30000);
    CHECK(game::set_count(mat->items[0], 999) && game::count(mat->items[0]) == 999);

    uintptr_t vt[8] = {};
    vt[6] = (uintptr_t)&fake_add;
    uintptr_t inv = w.alloc(0x80), arr = w.alloc(8 * 16);
    w.put<uintptr_t>(inv, (uintptr_t)vt);
    w.put<uintptr_t>(inv + 0x40, arr);
    uintptr_t tmpl = mat->items[1].addr;
    uintptr_t medkit = game::g.descs["Medkit_HealthPackLarge"];
    CHECK(game::pick_template(mat) == mat->items[0].addr);
    CHECK(game::give(inv, tmpl, medkit, 5));
    uintptr_t added = game::rdv<uintptr_t>(arr);
    CHECK(game::rdv<int>(added + 0x40) == 5 && game::rdv<uintptr_t>(added + 0x60) == medkit);
    {
        uintptr_t remove_vt[11] = {};
        remove_vt[9] = (uintptr_t)&fake_remove_one;
        remove_vt[10] = (uintptr_t)&fake_remove_stack;
        uintptr_t bag = w.alloc(0x80), slots = w.alloc(8 * 4), stack = w.alloc(0x100), sword = w.alloc(0x100);
        w.put<uintptr_t>(bag, (uintptr_t)remove_vt);
        w.put<uintptr_t>(bag + 0x40, slots);
        w.put<uintptr_t>(slots, stack);
        w.put<uintptr_t>(slots + 8, sword);
        w.put<uint32_t>(bag + 0x48, 2);
        w.put<int>(stack + 0x40, 7);
        w.put<int>(sword + 0x40, 1);
        CHECK(game::remove_item(bag, stack, game::K_MATERIALS) && game::rdv<uint32_t>(bag + 0x48) == 1 && game::rdv<uintptr_t>(slots) == sword);
        CHECK(game::remove_item(bag, sword, game::K_BACKPACK) && game::rdv<uint32_t>(bag + 0x48) == 0);
        CHECK(!game::remove_item(bag, sword, game::K_BACKPACK));
        w.put<uintptr_t>(slots, stack);
        w.put<uint32_t>(bag + 0x48, 1);
        w.put<int>(stack + 0x40, 3);
        CHECK(!game::remove_item(bag, stack, game::K_AMMO) && game::rdv<int>(stack + 0x40) == 2);
    }
    CHECK(game::default_target("Craft_Gauze") == game::K_MATERIALS && game::default_target("Melee_MacheteAGen") == game::K_BACKPACK);
    CHECK(game::default_target("Ammo_PistolBig") == game::K_AMMO);
    {
        auto order = game::give_order(game::K_BACKPACK);
        CHECK(std::find(order.begin(), order.end(), game::K_AMMO) == order.end());
        CHECK(game::give_order(game::K_AMMO)[0] == game::K_AMMO);
    }

    CHECK(game::g.vt_player && game::g.vt_human && game::g.vt_health[0]);
    CHECK(!memcmp((const void*)game::rdv<uintptr_t>(game::g.vt_player + cheats::SLOT_PHYSICS_POSITION * 8), cheats::PHYSICS_POSITION_START, sizeof cheats::PHYSICS_POSITION_START));
    CHECK(game::g.players.size() == 1);
    CHECK(game::class_name(w.player) == ".?AVPlayerDI@@" && game::class_name(0x1234) == "?");
    game::set_stat(machete, ST_Damage, 1234);
    game::write_stat(machete, ST_Damage, 5);
    cheats::locate((uintptr_t)m);
    printf("params: %zu names\n", cheats::param_ids.size());
    CHECK(cheats::param_ids["MaxStamina"] == 495 && cheats::param_ids["InfiniteStamina"] == 969);
    CHECK(cheats::param_ids["GrapplingHookCooldown"] == 541 && cheats::param_ids["TDCooldown"] == 20);
    CHECK(cheats::param_ids.count("FlashlightDrainMul") && cheats::param_ids.count("ZombieSpitToxicEnabled"));
    for (auto& c : cheats::CHEATS) {
        for (auto& n : c.numbers) CHECK(cheats::param_ids.count(n.first));
        for (auto* n : c.switches) CHECK(cheats::param_ids.count(n));
    }
    for (auto& [name, index] : cheats::param_ids) game::wr<uintptr_t>(w.params + index * 16, cheats::vt_param_float);
    for (auto& c : cheats::CHEATS)
        for (auto* n : c.switches) game::wr<uintptr_t>(w.params + cheats::param_ids[n] * 16, cheats::vt_param_bool);
    {
        uintptr_t hook = w.params + cheats::param_ids["CanUseHook"] * 16 + 8, regen = w.params + cheats::param_ids["RopeEnergyRegenTime"] * 16 + 8;
        CHECK(cheats::param_kind_ok(hook, true) && !cheats::param_kind_ok(hook, false) && cheats::param_kind_ok(regen, false) && !cheats::param_kind_ok(regen, true));
        uintptr_t stray = w.alloc(16);
        CHECK(!cheats::param_kind_ok(stray + 8, false) && !cheats::param_kind_ok(0, false));
    }
    CHECK(cheats::local_player_root && cheats::params_root && cheats::unlimited_ammo_flag && cheats::var_root);
    {
        std::string engine_dir = argv[1];
        engine_dir = engine_dir.substr(0, engine_dir.find_last_of("\\/") + 1) + "engine_x64_rwdi.dll";
        HMODULE engine = LoadLibraryExA(engine_dir.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
        CHECK(visuals::locate_engine(engine));
        visuals::locate_texts(game::g.base, engine);
        CHECK(visuals::tools_global == cheats::var_root && visuals::texts_api.find && (uintptr_t)visuals::texts_api.find - (uintptr_t)engine == 0x634e30);
        CHECK(visuals::messages_api.post && (uintptr_t)visuals::messages_api.post - game::g.base == 0x57bdc0 && visuals::messages_api.game_global - game::g.base == 0x1c16358);
        auto english = std::make_shared<const visuals::TextLibrary>(visuals::load_language(visuals::game_folder(), "En"));
        auto polish = visuals::load_language(visuals::game_folder(), "Pl");
        size_t count = 0;
        const visuals::GameText* killer = nullptr;
        for (auto& f : *english)
            for (auto& t : f.texts) count++, killer = t.key == "HUDMessage_BTZDeath_Killer" ? &t : killer;
        CHECK(english->size() > 20 && count > 20000 && killer && killer->text == L"You killed %s" && killer->kind == visuals::TEXT_HUD);
        CHECK(polish.size() == english->size() && visuals::load_language(visuals::game_folder(), "Zz").empty());
        std::map<std::string, std::string> english_texts;
        for (auto& f : *english)
            for (auto& t : f.texts) english_texts.emplace(t.key, visuals::narrow(t.text.c_str()));
        for (auto& c : visuals::CALLOUTS) {
            auto it = english_texts.find(c.key);
            bool fits = it != english_texts.end() && visuals::safe_format(visuals::callout_text(c, true), it->second);
            if (!fits) printf("callout %s does not fit the game's text\n", c.key);
            CHECK(fits);
        }
        CHECK(visuals::console_text_file("data/maps/common_texts_xbo.bin") && visuals::console_text_file("data/maps/testmap_shop_texts_all.bin"));
        CHECK(!visuals::console_text_file("data/maps/slums_texts_all.bin") && !visuals::console_text_file("common_texts_all_DLC1.bin"));
        visuals::libraries["En"] = english;
        CHECK(visuals::text_kind("Shopkeeper_GoingOutside_02DLG01") == visuals::TEXT_SPOKEN && visuals::text_kind("Melee_Fists_N") == visuals::TEXT_ITEMS);
        CHECK(visuals::text_kind("Skill_Agility_1") == visuals::TEXT_SKILLS && visuals::text_kind("Q_Explosives_OBJ_TurnOfAlarm") == visuals::TEXT_QUESTS);
        CHECK(visuals::text_kind("Tut_CoopInLegend") == visuals::TEXT_TUTORIALS && visuals::text_kind("HUDMessage_BTZDeath_Victim") == visuals::TEXT_HUD);
        CHECK(visuals::text_kind("MMenuMisisionEnd_Demo_BtnBuyNow") == visuals::TEXT_MENUS && visuals::text_kind("ChatterNeutral_01") == visuals::TEXT_SPOKEN);
        CHECK(visuals::text_kind("PlayerBar_LevelFighterLabel") != visuals::TEXT_SPOKEN && visuals::text_kind("EncCounter_SurvivorsRescued") != visuals::TEXT_SPOKEN);
        CHECK(visuals::text_kind("PLAYERMAN1_NAME") != visuals::TEXT_SPOKEN && visuals::text_kind("MenuOptionsGame_LabelSelectSpeechTip") != visuals::TEXT_SPOKEN);
        CHECK(visuals::text_kind("EncEscortD_10CHT01_Babar") == visuals::TEXT_SPOKEN && visuals::text_kind("PlayerM1_BTZAllyDying_02") == visuals::TEXT_SPOKEN);
        CHECK(visuals::text_kind("Player_SafePlace_Bed01") == visuals::TEXT_SPOKEN && visuals::text_kind("Akram_Cofee_01CHT01") == visuals::TEXT_SPOKEN);
    }
    {
        static uintptr_t inner_seen;
        static std::string id_seen;
        static bool force_seen;
        auto inner = [](uintptr_t inner_game, const char* id, bool force) -> bool { return inner_seen = inner_game, id_seen = id, force_seen = force, true; };
        auto* code = (uint8_t*)VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        memset(code, 0xCC, 64);
        memcpy(code, visuals::SPEECH_EXPORT, sizeof visuals::SPEECH_EXPORT);
        *(int32_t*)(code + sizeof visuals::SPEECH_EXPORT) = 32 - (int32_t)visuals::SPEECH_EXPORT_SPAN;
        game::write_jump(code + 32, (uintptr_t)(bool (*)(uintptr_t, const char*, bool))inner);
        auto saved_original = visuals::speech_original;
        auto saved_voice = visuals::forced_voice;
        visuals::speech_original = visuals::hook_speech_export(code);
        CHECK(visuals::speech_original == (visuals::SpeechFn)(code + 32));
        visuals::forced_voice = "Pl";
        uintptr_t fake_game[2] = {0, 0x4242};
        CHECK(((bool (*)(uintptr_t, const char*, bool))code)((uintptr_t)fake_game, "En", true));
        CHECK(inner_seen == 0x4242 && id_seen == "Pl" && force_seen && visuals::language_report().find("the game asked for En") != std::string::npos);
        uint8_t plain[16] = {0x48, 0x8B, 0x49, 0x08, 0xE9, 0, 0, 0, 0, 0x90};
        CHECK(!visuals::hook_speech_export(plain));
        auto* locale_code = (uint8_t*)VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        memset(locale_code, 0xCC, 64);
        memcpy(locale_code, visuals::VOICE_LOCALE_EXPORT, sizeof visuals::VOICE_LOCALE_EXPORT);
        *(int32_t*)(locale_code + visuals::VOICE_LOCALE_JE + 2) = 0x20 - (int32_t)visuals::VOICE_LOCALE_RET;
        locale_code[visuals::VOICE_LOCALE_RET] = 0xC3;
        static uint8_t inner_game[0x500] = {};
        memcpy(inner_game + visuals::ENGINE_LOCALE, "en", 3);
        uintptr_t locale_game[2] = {0, (uintptr_t)inner_game};
        CHECK(visuals::hook_voice_locale(locale_code) && visuals::locale_fallback == (visuals::LocaleFallbackFn)(locale_code + 0x20));
        CHECK(!strcmp(((const char* (*)(uintptr_t))locale_code)((uintptr_t)locale_game), "pl"));
        visuals::forced_voice = visuals::FOLLOW_GAME;
        CHECK(!strcmp(((const char* (*)(uintptr_t))locale_code)((uintptr_t)locale_game), "en"));
        visuals::forced_voice = "Pl";
        VirtualFree(locale_code, 0, MEM_RELEASE);
        visuals::locale_fallback = nullptr, visuals::voice_locale[0] = 0;
        auto saved_steam = visuals::steam_language, saved_game = visuals::startup_game_setting;
        visuals::steam_language = "Pl", visuals::startup_game_setting = "En", visuals::forced_voice = "Pl";
        CHECK(visuals::engine_language() == "Pl" && visuals::game_wanted() == "En");
        visuals::forced_voice = visuals::FOLLOW_GAME;
        CHECK(visuals::engine_language() == "En");
        visuals::startup_game_setting.clear(), visuals::forced_voice = config::STEAM_LANGUAGE;
        CHECK(visuals::engine_language().empty());
        visuals::startup_game_setting = "Cs", visuals::forced_voice = visuals::FOLLOW_GAME;
        CHECK(visuals::engine_language() == "Cs");
        visuals::forced_voice = "De";
        CHECK(visuals::engine_language() == "De");
        visuals::steam_language = saved_steam, visuals::startup_game_setting = saved_game;
        visuals::speech_original = saved_original, visuals::forced_voice = saved_voice, visuals::started_voice.clear(), visuals::speech_asked.clear();
        VirtualFree(code, 0, MEM_RELEASE);
    }
    visuals::locate(game::g.base);
    CHECK(game::g.vt_hud && visuals::hud_element_offset >= 0);
    CHECK(cheats::vt_throwable[0] && cheats::vt_throwable[1] && cheats::throwable_control[0] > 0 && cheats::throwable_control[1] > 0);
    printf("throwables: control +%x +%x\n", cheats::throwable_control[0], cheats::throwable_control[1]);
    {
        cheats::Vec3 eye{0, 1.7f, 0}, still{};
        auto hit = cheats::closest_approach({0, 2.0f, 10}, {0, 0, -20}, eye, still);
        CHECK(hit.miss < 0.05f && std::fabs(hit.when - 0.5f) < 0.02f);
        auto wide = cheats::closest_approach({3, 2.0f, 10}, {0, 0, -20}, eye, still);
        CHECK(std::fabs(wide.miss - 3) < 0.05f && wide.away.x < -2.9f);
        auto high = cheats::closest_approach({0, 9.0f, 10}, {0, 0, -20}, eye, still);
        CHECK(high.miss > cheats::DODGE_HIT_RADIUS);
        float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
        CHECK(cheats::dodge_key(m, {-1, 0, 0}, {0, 0, -20}) == cheats::KEY_LEFT);
        CHECK(cheats::dodge_key(m, {1, 0, 0.2f}, {0, 0, -20}) == cheats::KEY_RIGHT);
        CHECK(cheats::dodge_key(m, {0, 0, -1}, {20, 0, 0}) == cheats::KEY_BACK);
        BYTE side = cheats::dodge_key(m, {}, {0, 0, -20});
        CHECK(side == cheats::KEY_LEFT || side == cheats::KEY_RIGHT);
    }
    CHECK(cheats::var_root > (uintptr_t)m && cheats::var_root < (uintptr_t)m + 0x4000000);
    CHECK(cheats::level_from_xp_fn && cheats::cache_get_fn);
    uintptr_t lockpick_code = cheats::lockpick_patch;
    CHECK(cheats::profile_root > (uintptr_t)m && cheats::profile_root < (uintptr_t)m + 0x4000000);
    CHECK(cheats::locations_root == (uintptr_t)m + 0x1d0f900 && cheats::vt_quest_goals == (uintptr_t)m + 0x14c4818 && cheats::vt_quest == (uintptr_t)m + 0x1737680);
    {
        const uintptr_t POINT_LOCATION_VTABLE = 0x15771e8;
        const int POINT_POSITION = 0x98;
        static uint64_t places[6][0xa8 / 8];
        auto place = [&](int i, uintptr_t vt, uint8_t flags, int type, int state, cheats::Vec3 at) {
            uint8_t* p = (uint8_t*)places[i];
            *(uintptr_t*)p = vt;
            p[teleport::LOCATION_FLAGS] = flags;
            memcpy(p + teleport::LOCATION_TYPE, &type, 4);
            memcpy(p + teleport::LOCATION_STATE, &state, 4);
            memcpy(p + POINT_POSITION, &at, sizeof at);
            return (uintptr_t)p;
        };
        uintptr_t point = (uintptr_t)m + POINT_LOCATION_VTABLE;
        CHECK(game::class_name((uintptr_t)&point) == ".?AVPointLocation@Locations@@");
        static uintptr_t list[7];
        list[0] = place(0, point, 1, teleport::TYPE_WAYPOINT, teleport::STATE_TRACKED, {1, 0, 1});
        list[1] = place(1, point, 1, 0xf, 0, {2, 0, 2});
        list[2] = place(2, point, 0, 0xf, teleport::STATE_TRACKED, {3, 0, 3});
        list[3] = place(3, cheats::vt_quest, 1, 0xf, teleport::STATE_TRACKED, {4, 0, 4});
        list[4] = place(4, point, 0x11, 0xf, teleport::STATE_TRACKED, {500, 7, 0});
        list[5] = place(5, point, 1, 0x9, teleport::STATE_TRACKED, {-40, 9, 30});
        list[6] = 0x10;
        static cheats::EngineVector locations{list, 7, 7};
        uintptr_t real_locations = cheats::locations_root;
        cheats::locations_root = (uintptr_t)&locations;
        cheats::Vec3 found{};
        CHECK(teleport::tracked_location({0, 0, 0}, found) && found.x == -40 && found.y == 9 && found.z == 30);
        CHECK(teleport::tracked_location({450, 0, 0}, found) && found.x == 500 && found.y == 7);
        places[4][0] = places[5][0] = 0;
        CHECK(!teleport::tracked_location({0, 0, 0}, found) && !teleport::tracked_location({NAN, 0, 0}, found));
        locations.size = 0;
        CHECK(!teleport::tracked_location({0, 0, 0}, found));
        cheats::locations_root = real_locations;
    }
    CHECK(cheats::forced_damage_jump == (uintptr_t)m + 0xbae6c3);
    CHECK(cheats::pound_exposure_check == (uintptr_t)m + 0xd654e7);
    uintptr_t pound_code = cheats::pound_exposure_check;
    {
        static uint8_t fake_jump[6] = {0x0F, 0x84, 1, 2, 3, 4};
        uintptr_t real = cheats::forced_damage_jump;
        cheats::forced_damage_jump = (uintptr_t)fake_jump;
        cheats::block_forced_damage(true);
        CHECK(fake_jump[0] == 0x90 && fake_jump[1] == 0xE9 && fake_jump[2] == 1 && cheats::forced_damage_blocked());
        cheats::block_forced_damage(false);
        CHECK(fake_jump[0] == 0x0F && fake_jump[1] == 0x84 && !cheats::forced_damage_blocked());
        cheats::forced_damage_jump = real;
    }
    CHECK(lockpick_code == (uintptr_t)m + 0x7817ae);
    CHECK(!memcmp((const void*)cheats::cache_get_fn, "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18", 15));
    fake_cache_base = w.cache;
    cheats::cache_get_original = fake_cache_get;
    cheats::level_from_xp_fn = (uintptr_t)&fake_level_from_xp;
    fake_var_vtable[1 + cheats::SLOT_VAR_FLOAT] = (uintptr_t)&fake_var_float;
    fake_var_vtable[1 + cheats::SLOT_VAR_VEC3] = (uintptr_t)&fake_var_vec3;
    cheats::var_root = (uintptr_t)&fake_var_holder;
    cheats::local_player_root = cheats::params_root = 0;
    for (auto& p : cheats::PRESETS) {
        for (auto* k : p.cheats) CHECK(cheats::find(k));
        for (auto& [k, v] : p.tweaks) CHECK(cheats::find_tweak(k) && v > 1.0f && v <= cheats::find_tweak(k)->max);
        cheats::apply_profile(cheats::preset_profile(p));
        CHECK(cheats::preset_active(p));
        for (auto& other : cheats::PRESETS) CHECK(&other == &p || !cheats::preset_active(other));
    }
    cheats::all_off();
    uintptr_t flag = cheats::unlimited_ammo_flag;
    static uint8_t fake_rules[4];
    cheats::unlimited_ammo_flag = (uintptr_t)fake_rules;
    for (auto* k : {"god", "stamina", "supplies", "one_hit", "hook", "uv", "ammo", "z_spits", "no_fall", "durability"}) cheats::find(k)->on = true;
    for (auto& t : cheats::TWEAKS)
        for (auto* n : t.params) CHECK(cheats::param_ids.count(n));
    game::wr<float>(w.params + cheats::param_ids["RopeEnergyRegenTime"] * 16 + 8, 12.5f);
    game::wr<float>(w.params + cheats::param_ids["AirKickRangeMul"] * 16 + 8, 1.0f);
    cheats::find("no_reload")->on = true;
    cheats::find_tweak("h_dropkick")->factor = 3.0f;
    for (auto [n, v] : {std::pair{"RangeMeleeMul", 1.0f}, {"BestTargetMeleeRange", 3.0f}, {"MeeleAttackSpeedMul", 0.0f}, {"BluntAttackSpeedMul", 0.25f}})
        game::wr<float>(w.params + cheats::param_ids[n] * 16 + 8, v);
    cheats::find_tweak("h_melee")->factor = 2.0f;
    cheats::find_tweak("melee_speed")->factor = 2.0f;
    static uint8_t survivor_logical[0x800] = {};
    *(uintptr_t*)survivor_logical = game::g.vt_logical_player;
    uintptr_t no_logical = game::rdv<uintptr_t>(w.player + cheats::LOGICAL_PLAYER);
    game::wr<uintptr_t>(w.player + cheats::LOGICAL_PLAYER, (uintptr_t)survivor_logical);
    cheats::scan_enemies();
    game::set_count(mat->items[0], 999);
    cheats::tick();
    game::set_count(mat->items[0], 3);
    cheats::tick();
    CHECK(game::get_stat(machete, ST_Damage) == 1234);
    CHECK(cheats::player == w.player && cheats::health() == 87);
    CHECK(game::rdv<uintptr_t>(w.player + 0x8f8) == (uintptr_t)&cheats::immortal_vtable[1]);
    CHECK(((bool(__fastcall*)(uintptr_t))cheats::slot(w.player + 0x8f8, cheats::SLOT_IS_IMMORTAL))(0));
    CHECK(game::rdv<float>(w.stamina + 0x10) == 100);
    CHECK(game::rdv<float>(w.enemy_health + 0x78) == 1);
    CHECK(game::count(mat->items[0]) == 999);
    auto param = [&](const char* n) { return w.params + cheats::param_ids[n] * 16 + 8; };
    auto cached = [&](const char* n) { return w.cache + (cheats::param_ids[n] + 1) * 40; };
    uintptr_t my_provider = game::rdv<uintptr_t>(w.player + 0x9c0);
    for (auto* n : {"RopeEnergyRegenTime", "CanUseHook", "AirKickRangeMul"}) cheats::cache_get_hook(my_provider, cheats::param_ids[n] + 1);
    CHECK(cheats::reads_of("RopeEnergyRegenTime") == 1);
    {
        int id = cheats::param_ids["RopeEnergyRegenTime"] + 1;
        game::wr<float>(cached("RopeEnergyRegenTime") + 0x10, 7.0f);
        game::wr<uint64_t>(cached("RopeEnergyRegenTime"), 5);
        cheats::cache_get_hook(0x5000, id);
        CHECK(game::rdv<float>(cached("RopeEnergyRegenTime") + 0x10) == 7.0f && cheats::reads_of("RopeEnergyRegenTime") == 1);
        CHECK(game::rdv<uint64_t>(cached("RopeEnergyRegenTime")) == ~0ull && cheats::overridden_providers[id].count(0x5000));
        cheats::cache_get_hook(my_provider, id);
        CHECK(game::rdv<float>(cached("RopeEnergyRegenTime") + 0x10) == 0.01f && cheats::reads_of("RopeEnergyRegenTime") == 2);
    }
    {
        int id = cheats::param_ids["RopeEnergyRegenTime"] + 1;
        uintptr_t tick_provider = cheats::player_provider.exchange(0), saved_root = cheats::local_player_root;
        static uint8_t local_slot[0x800] = {};
        static uintptr_t local_root = (uintptr_t)local_slot;
        *(uintptr_t*)(local_slot + cheats::LOCAL_PLAYER) = w.player;
        cheats::local_player_root = (uintptr_t)&local_root;
        CHECK(cheats::local_provider() == my_provider);
        game::wr<float>(cached("RopeEnergyRegenTime") + 0x10, 7.0f);
        cheats::cache_get_hook(my_provider, id);
        CHECK(game::rdv<float>(cached("RopeEnergyRegenTime") + 0x10) == 0.01f);
        cheats::local_player_root = saved_root;
        cheats::player_provider = tick_provider;
    }
    {
        int id = cheats::param_ids["RopeEnergyRegenTime"] + 1;
        auto real_root = cheats::profile_root;
        static uint8_t hunter_logical[0x800] = {}, profile[0x100] = {}, profile_player[0x1000] = {}, game_object[0x600] = {};
        *(uintptr_t*)hunter_logical = game::g.vt_logical_player;
        *(int*)(hunter_logical + cheats::PLAYER_ROLE) = cheats::ROLE_HUNTER;
        *(uintptr_t*)profile_player = game::g.vt_player;
        *(uintptr_t*)(profile_player + cheats::LOGICAL_PLAYER) = (uintptr_t)hunter_logical;
        *(uintptr_t*)(profile_player + cheats::PARAM_PROVIDER) = 0x7000;
        *(uintptr_t*)(profile + cheats::PROFILE_PLAYER) = (uintptr_t)profile_player;
        *(uintptr_t*)(game_object + cheats::GAME_PROFILE) = (uintptr_t)profile;
        static uintptr_t root_value = (uintptr_t)game_object;
        cheats::profile_root = (uintptr_t)&root_value;
        CHECK(cheats::own_provider(0x7000) && !cheats::own_provider(0x5000));
        game::wr<float>(cached("RopeEnergyRegenTime") + 0x10, 7.0f);
        cheats::cache_get_hook(0x7000, id);
        CHECK(game::rdv<float>(cached("RopeEnergyRegenTime") + 0x10) == 0.01f);
        cheats::tick();
        CHECK(cheats::side == cheats::SIDE_HUNTER);
        cheats::profile_root = real_root;
        cheats::tick();
        CHECK(cheats::side == cheats::SIDE_SURVIVOR);
    }
    {
        uintptr_t own = game::rdv<uintptr_t>(w.player + cheats::PARAM_PROVIDER), logical = game::rdv<uintptr_t>(w.player + cheats::LOGICAL_PLAYER);
        DWORD settle = cheats::side_settle_ms;
        game::wr<uintptr_t>(w.player + cheats::PARAM_PROVIDER, 0);
        game::wr<uintptr_t>(w.player + cheats::LOGICAL_PLAYER, 0);
        cheats::side_settle_ms = 0;
        cheats::tick();
        cheats::tick();
        CHECK(cheats::side == cheats::SIDE_UNKNOWN);
        cheats::side_settle_ms = settle;
        game::wr<uintptr_t>(w.player + cheats::PARAM_PROVIDER, own);
        game::wr<uintptr_t>(w.player + cheats::LOGICAL_PLAYER, logical);
        cheats::tick();
        CHECK(cheats::side == cheats::SIDE_SURVIVOR);
    }
    CHECK(game::rdv<float>(cached("RopeEnergyRegenTime") + 0x10) == 0.01f && (game::rdv<uint8_t>(cached("RopeEnergyRegenTime") + 0x20) & 1));
    CHECK(game::rdv<uintptr_t>(cached("RopeEnergyRegenTime") + 8) == cheats::vt_param_float && cheats::vt_param_float);
    CHECK(game::rdv<uint8_t>(cached("CanUseHook") + 0x10) == 1 && game::rdv<uintptr_t>(cached("CanUseHook") + 8) == cheats::vt_param_bool);
    CHECK(game::rdv<float>(param("AirKickRangeMul")) == 3.0f && game::rdv<float>(cached("AirKickRangeMul") + 0x10) == 3.0f);
    CHECK(game::rdv<float>(param("RangeMeleeMul")) == 1.0f && game::rdv<float>(param("BestTargetMeleeRange")) == 3.0f);
    CHECK(game::rdv<float>(param("MeeleAttackSpeedMul")) == 0.0f && game::rdv<float>(param("BluntAttackSpeedMul")) == 0.25f);
    CHECK(game::g.stats[ST_SwingSpeed].off == fake::SWING_OFF && game::g.stats[ST_DamageRange].off == fake::MELEE_RANGE_OFF);
    CHECK(game::rdv<float>(w.desc["Melee_Fists"] + fake::SWING_OFF) == 0.9f && game::rdv<float>(w.desc["Melee_Fists"] + fake::MELEE_RANGE_OFF) == 1.15f);
    CHECK(game::rdv<float>(w.desc["Throwable_LightDisable_Spit"] + fake::MELEE_RANGE_OFF) == 5.0f);
    CHECK(game::g.stats[ST_AmmoCount].off == fake::AMMO_OFF && game::g.stats[ST_DepletionTime].off == fake::DEPLETION_OFF);
    CHECK(game::rdv<int>(w.desc["Firearm_PistolAGen"] + fake::AMMO_OFF) == 999 && game::rdv<float>(w.desc["Firearm_PistolAGen"] + fake::RELOAD_OFF) == 0.05f);
    CHECK(game::rdv<int>(w.desc["Ammo_PistolBig"] + fake::AMMO_OFF) == (int)game::lookup("Ammo_PistolBig")->st[ST_AmmoCount] && game::rdv<int>(w.desc["Crossbow_SHU"] + fake::AMMO_OFF) == 999);
    CHECK(game::rdv<float>(w.desc["Flashlight_Superlight"] + fake::DEPLETION_OFF) == 1e7f);
    CHECK(cheats::tree_level(2) == 7 && cheats::tree_max(2) == 25 && cheats::tree_max(5) == 0);
    game::wr<uint32_t>(w.trees + 2 * 0x20 + 0xc, 1000);
    game::wr<uint32_t>(w.trees + 2 * 0x20 + 0x10, 500);
    cheats::level_up_with_xp(2);
    CHECK(game::rdv<uint32_t>(w.trees + 2 * 0x20 + 8) == 1500 && fake_level_calls == 1);
    {
        auto xp_of = [&](int type) { return w.trees + type * 0x20 + 8; };
        for (int type : {1, 3}) game::wr<uint16_t>(w.trees + type * 0x20 + 0x16, 20);
        cheats::find_tweak("xp")->factor = 5.0f;
        cheats::tick();
        game::wr<uint32_t>(xp_of(2), 1600);
        cheats::tick();
        CHECK(game::rdv<uint32_t>(xp_of(2)) == 2000 && fake_level_calls == 2);
        cheats::tick();
        CHECK(game::rdv<uint32_t>(xp_of(2)) == 2000);
        for (int type : {1, 2, 3}) game::wr<uint32_t>(xp_of(type), 9000);
        cheats::tick();
        CHECK(game::rdv<uint32_t>(xp_of(1)) == 9000 && game::rdv<uint32_t>(xp_of(2)) == 9000 && fake_level_calls == 2);
        cheats::find_tweak("xp")->factor = 1.0f;
        game::wr<uint32_t>(xp_of(2), 9100);
        cheats::tick();
        CHECK(game::rdv<uint32_t>(xp_of(2)) == 9100);
        for (int type : {1, 2, 3}) game::wr<uint32_t>(xp_of(type), 0);
        for (int type : {1, 3}) game::wr<uint16_t>(w.trees + type * 0x20 + 0x16, 0);
    }
    {
        CHECK(cheats::xp_award_site);
        const uint8_t harness[] = {0x41, 0x57, 0x41, 0x56, 0x56, 0x53, 0x41, 0x89, 0xCF, 0x41, 0x89, 0xD6, 0x4C, 0x89, 0xC6,
                                   0x45, 0x85, 0xFF, 0x0F, 0x8E, 0x0C, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x9E, 0xE8, 0x13, 0x00, 0x00,
                                   0x44, 0x89, 0xF8, 0xEB, 0x05, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0x5B, 0x5E, 0x41, 0x5E, 0x41, 0x5F, 0xC3};
        const int SITE = 15;
        auto* code = (uint8_t*)VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        memcpy(code, harness, sizeof harness);
        auto award = (int(*)(int, int, uintptr_t))code;
        static uint8_t award_owner[0x1400];
        CHECK(award(70, 2, (uintptr_t)award_owner) == 70 && award(0, 2, (uintptr_t)award_owner) == -1);
        uintptr_t real_site = cheats::xp_award_site;
        uint8_t* real_stub = cheats::xp_stub;
        cheats::xp_stub = nullptr;
        cheats::xp_award_site = (uintptr_t)code + SITE;
        cheats::set_xp_factor(2.5f);
        CHECK(cheats::xp_stub && code[SITE] == 0xFF && code[SITE + 1] == 0x25);
        CHECK(award(70, 2, (uintptr_t)award_owner) == 175 && award(0, 2, (uintptr_t)award_owner) == -1 && award(-5, 3, (uintptr_t)award_owner) == -1);
        CHECK(cheats::xp_awarded_by_hook(2) && !cheats::xp_awarded_by_hook(2) && cheats::xp_awarded_by_hook(3) && !cheats::xp_awarded_by_hook(1));
        cheats::set_xp_factor(1.0f);
        CHECK(award(70, 6, (uintptr_t)award_owner) == 70);
        cheats::xp_award_site = real_site;
        cheats::xp_stub = real_stub;
    }
    {
        static float set_to = 0;
        static uintptr_t fake_health_vtable[8] = {};
        fake_health_vtable[cheats::SLOT_IS_IMMORTAL] = (uintptr_t) + [](uintptr_t) -> bool { return false; };
        fake_health_vtable[cheats::SLOT_SET_HEALTH] = (uintptr_t) + [](uintptr_t, float v, bool) -> uintptr_t { set_to = v; return 0; };
        uintptr_t real_original = cheats::original_vtable;
        bool god_was = cheats::immortal;
        cheats::immortal = false;
        cheats::original_vtable = (uintptr_t)fake_health_vtable;
        uintptr_t health_object = w.player + cheats::HEALTH_OBJECT;
        float before = game::rdv<float>(health_object + cheats::HEALTH_VALUE);
        game::wr<float>(health_object + cheats::HEALTH_VALUE, 100.0f);
        cheats::damage_divisor = 4.0f;
        cheats::scaled_set_health(health_object, 60.0f, false);
        CHECK(set_to == 90.0f);
        cheats::scaled_set_health(health_object, 130.0f, false);
        CHECK(set_to == 130.0f && !cheats::immortal_check(health_object));
        cheats::damage_divisor = 1.0f;
        cheats::scaled_set_health(health_object, 60.0f, false);
        CHECK(set_to == 60.0f);
        cheats::immortal = true;
        CHECK(cheats::immortal_check(health_object));
        cheats::immortal = god_was;
        game::wr<float>(health_object + cheats::HEALTH_VALUE, before);
        cheats::original_vtable = real_original;
    }
    {
        static uintptr_t global_container;
        global_container = game::rdv<uintptr_t>(w.player + cheats::PARAM_CONTAINER);
        game::wr<uintptr_t>(w.player + cheats::PARAM_CONTAINER, 0);
        cheats::params_root = (uintptr_t)&global_container;
        CHECK(cheats::skill_container() == global_container && cheats::tree_level(2) == 7 && cheats::tree_max(2) == 25);
        game::wr<uintptr_t>(w.player + cheats::PARAM_CONTAINER, global_container);
        cheats::params_root = 0;
    }
    {
        auto saved_set_level = cheats::set_level_fn;
        cheats::set_level_fn = (uintptr_t)&fake_set_level;
        uintptr_t record = cheats::tree_record(2);
        game::wr<uint32_t>(record + cheats::TREE_XP, 20000);
        cheats::set_tree_level(2, 5);
        CHECK(cheats::tree_xp(2) == 5000 && cheats::tree_level(2) == 5);
        cheats::set_tree_level(2, cheats::tree_max(2));
        CHECK(cheats::tree_xp(2) == 25 * 1000);
        cheats::set_level_fn = saved_set_level;
    }
    game::wr<float>(w.enemy_health + 0x78, 300);
    fatrainer_module_update(w.enemy_health);
    CHECK(game::rdv<float>(w.enemy_health + 0x78) == 1);
    CHECK(cheats::vt_nest_logic);
    {
        static uintptr_t nest_logic[4] = {cheats::vt_nest_logic};
        game::wr<uintptr_t>(w.enemy + 0x88, (uintptr_t)nest_logic);
        cheats::nest_modules.clear();
        game::wr<float>(w.enemy_health + 0x78, 90);
        fatrainer_module_update(w.enemy_health);
        CHECK(game::rdv<float>(w.enemy_health + 0x78) == 90 && cheats::is_nest_module(w.enemy_health));
        game::wr<uintptr_t>(w.enemy + 0x88, 0);
        cheats::nest_modules.clear();
        fatrainer_module_update(w.enemy_health);
        CHECK(game::rdv<float>(w.enemy_health + 0x78) == 1 && !cheats::is_nest_module(w.enemy_health));
    }
    CHECK(game::rdv<float>(param("RopeEnergyRegenTime")) == 0.01f && game::rdv<uint8_t>(param("CanUseHook")) == 1);
    CHECK(game::rdv<uint8_t>(param("InfiniteStamina")) == 1 && game::rdv<float>(param("FlashlightRechargeSpeed")) == 1000);
    CHECK(game::rdv<uint8_t>(param("CanUseHook")) == 1 && game::rdv<float>(param("ZombieSpitLightDisableAmmoRegenTime3v1")) == 0.05f);
    CHECK(fake_rules[0] == 1);
    {
        static uint8_t bind[0x100] = {}, nodes[2][0x40] = {}, anims[2][0x40] = {}, item[0x80] = {}, desc[0x10] = {};
        static uintptr_t fake_bind;
        fake_bind = (uintptr_t)bind;
        auto saved_bind = cheats::anim_bind_of;
        auto saved_desc = cheats::vt_melee_desc;
        cheats::anim_bind_of = [](uintptr_t model) -> uintptr_t { return model == cheats::player ? fake_bind : 0; };
        cheats::vt_melee_desc = 0x7777;
        *(uintptr_t*)desc = 0x7777;
        *(uintptr_t*)(item + cheats::ITEM_DESC) = (uintptr_t)desc;
        *(uintptr_t*)(bind + cheats::ANIMS_PLAYING) = (uintptr_t)(nodes[0] + 0x10);
        *(uintptr_t*)(nodes[0] + 0x10) = (uintptr_t)(nodes[1] + 0x10);
        *(uintptr_t*)(nodes[0] + 0x08) = (uintptr_t)anims[0];
        *(uintptr_t*)(nodes[1] + 0x08) = (uintptr_t)anims[1];
        *(float*)(anims[0] + cheats::ANIM_SPEED) = 1.0f;
        *(float*)(anims[1] + cheats::ANIM_SPEED) = 0.5f;
        uintptr_t hand = cheats::player + cheats::PLAYER_HANDS + cheats::HAND_SIZE;
        game::wr<uintptr_t>(hand, 1);
        game::wr<uintptr_t>(hand + cheats::HAND_ITEM, (uintptr_t)item);
        static uintptr_t controller_vtable[4] = {};
        static uint8_t controller[0x500] = {};
        *(uintptr_t*)controller = (uintptr_t)&controller_vtable[1];
        *(int*)(controller + cheats::MELEE_ATTACK) = 2;
        float* duration = (float*)(controller + cheats::MELEE_DURATIONS + 2 * 4);
        *duration = 0.8f;
        float* phase_times = (float*)(controller + cheats::MELEE_PHASE_TIMES + cheats::MELEE_TABLE_PHASE_FIRST * 4);
        phase_times[0] = 1.6f, phase_times[2] = 0.5f;
        auto saved_controller_vt = cheats::vt_melee_controller;
        cheats::vt_melee_controller = (uintptr_t)&controller_vtable[1];
        uintptr_t saved_controller = game::rdv<uintptr_t>(cheats::player + cheats::MELEE_CONTROLLER);
        game::wr<uintptr_t>(cheats::player + cheats::MELEE_CONTROLLER, (uintptr_t)controller);
        auto speed = [](int i) { return *(float*)(anims[i] + cheats::ANIM_SPEED); };
        cheats::find_tweak("melee_speed")->factor = 2.0f;
        cheats::speed_melee_anims();
        cheats::speed_melee_anims();
        CHECK(speed(0) == 2.0f && speed(1) == 1.0f);
        CHECK(*duration == 0.4f && cheats::melee_durations.size() == 1);
        CHECK(phase_times[0] == 0.8f && phase_times[1] == 0 && phase_times[2] == 0.25f);
        phase_times[2] = 0.75f;
        cheats::speed_melee_anims();
        CHECK(phase_times[0] == 0.8f && phase_times[2] == 0.375f);
        *(float*)(anims[1] + cheats::ANIM_SPEED) = 0.75f;
        cheats::speed_melee_anims();
        CHECK(speed(1) == 1.5f && *duration == 0.4f);
        *(int*)(controller + cheats::MELEE_PHASE) = -1;
        *(int*)(controller + cheats::MELEE_ATTACK) = -1;
        cheats::speed_melee_anims();
        CHECK(speed(0) == 1.0f && speed(1) == 0.75f && cheats::anim_speeds.empty());
        *(int*)(controller + cheats::MELEE_ATTACK) = 2;
        cheats::speed_melee_anims();
        CHECK(speed(0) == 2.0f && speed(1) == 1.5f);
        game::wr<uintptr_t>(hand, 0);
        cheats::speed_melee_anims();
        CHECK(speed(0) == 1.0f && speed(1) == 0.75f && cheats::anim_speeds.empty());
        CHECK(*duration == 0.8f && cheats::melee_durations.empty());
        CHECK(phase_times[0] == 1.6f && phase_times[2] == 0.75f);
        game::wr<uintptr_t>(cheats::player + cheats::MELEE_CONTROLLER, saved_controller);
        cheats::vt_melee_controller = saved_controller_vt;
        cheats::anim_bind_of = saved_bind, cheats::vt_melee_desc = saved_desc;
    }
    for (auto& c : cheats::CHEATS) c.on = false;
    cheats::find_tweak("h_dropkick")->factor = 1.0f;
    cheats::find_tweak("h_melee")->factor = 1.0f;
    cheats::find_tweak("melee_speed")->factor = 1.0f;
    cheats::tick();
    CHECK(game::rdv<float>(param("AirKickRangeMul")) == 1.0f && game::rdv<uint64_t>(cached("AirKickRangeMul")) != ~0ull);
    for (auto* n : {"RopeEnergyRegenTime", "CanUseHook", "AirKickRangeMul"}) cheats::cache_get_hook(my_provider, cheats::param_ids[n] + 1);
    CHECK(game::rdv<uint64_t>(cached("AirKickRangeMul")) == ~0ull);
    CHECK(game::rdv<uint64_t>(cached("RopeEnergyRegenTime")) == ~0ull && game::rdv<uint64_t>(cached("CanUseHook")) == ~0ull);
    CHECK(cheats::side == cheats::SIDE_SURVIVOR);
    game::wr<uintptr_t>(w.player + cheats::LOGICAL_PLAYER, no_logical);
    game::wr<uint64_t>(cached("CanUseHook"), 5);
    cheats::cache_get_hook(my_provider, cheats::param_ids["CanUseHook"] + 1);
    CHECK(game::rdv<uint64_t>(cached("CanUseHook")) == 5 && cheats::overridden_providers[cheats::param_ids["CanUseHook"] + 1].empty());
    CHECK(game::rdv<int>(w.desc["Firearm_PistolAGen"] + fake::AMMO_OFF) == 8 && game::rdv<float>(w.desc["Flashlight_Superlight"] + fake::DEPLETION_OFF) == 10.0f);
    CHECK(game::rdv<float>(w.desc["Melee_Fists"] + fake::SWING_OFF) == 0.9f && game::rdv<float>(w.desc["Melee_Fists"] + fake::MELEE_RANGE_OFF) == 1.15f);
    CHECK(game::rdv<float>(param("RopeEnergyRegenTime")) == 12.5f && game::rdv<uint8_t>(param("CanUseHook")) == 0 && fake_rules[0] == 0);
    (void)flag;
    uintptr_t blade = 0;
    for (auto& it : game::find_inventory(game::K_BACKPACK)->items)
        if (it.info && !strcmp(it.info->id, "Melee_MacheteAGen")) blade = it.addr;
    CHECK(blade);
    game::wr<float>(blade + cheats::ITEM_CONDITION, 30.0f);
    cheats::find("durability")->on = true;
    cheats::tick();
    game::wr<float>(blade + cheats::ITEM_CONDITION, 12.5f);
    cheats::tick();
    CHECK(game::rdv<float>(blade + cheats::ITEM_CONDITION) == 30.0f);
    uintptr_t picked_up = w.item("Melee_MacheteAGen", 1), bag = game::rdv<uintptr_t>(w.backpack + 0x40);
    uint32_t bag_count = game::rdv<uint32_t>(w.backpack + 0x48);
    game::wr<uintptr_t>(bag + bag_count * 8, picked_up);
    game::wr<uint32_t>(w.backpack + 0x48, bag_count + 1);
    game::wr<float>(picked_up + cheats::ITEM_CONDITION, 40.0f);
    cheats::tick();
    game::wr<float>(picked_up + cheats::ITEM_CONDITION, 20.0f);
    cheats::tick();
    CHECK(game::rdv<float>(picked_up + cheats::ITEM_CONDITION) == 40.0f);
    game::wr<uint32_t>(w.backpack + 0x48, bag_count);
    cheats::tick();
    CHECK(!cheats::condition_floor.count(picked_up));
    {
        uintptr_t fighter = param("SkillPointsFighter");
        game::wr<uintptr_t>(fighter - 8, cheats::vt_param_float);
        game::wr<float>(fighter, 4.0f);
        CHECK(cheats::skill_points(2) == 4.0f);
        cheats::extra_skill_points[2] = 3;
        cheats::tick();
        CHECK(cheats::skill_points(2) == 7.0f);
        cheats::extra_skill_points[2] = -10;
        cheats::tick();
        CHECK(cheats::skill_points(2) == 0.0f);
        cheats::extra_skill_points[2] = 0;
        cheats::tick();
        CHECK(cheats::skill_points(2) == 4.0f);
        CHECK(cheats::vt_player_float);
        game::wr<uintptr_t>(fighter - 8, cheats::vt_player_float);
        cheats::extra_skill_points[2] = 2;
        cheats::tick();
        CHECK(cheats::skill_points(2) == 6.0f);
        cheats::extra_skill_points[2] = 0;
        cheats::tick();
        CHECK(cheats::skill_points(2) == 4.0f);
        game::wr<uintptr_t>(fighter - 8, cheats::vt_param_float);
    }
    {
        const char* ids[] = {"StunAttacks", "Stunning", "Multithrow", "Finisher", "LegendSkill_MaxHealth"};
        const int GAME_SKILLS = 5;
        uintptr_t manager = w.alloc(0x40), list = w.alloc(8 * GAME_SKILLS);
        for (int i = 0; i < GAME_SKILLS; i++) {
            uintptr_t skill = w.alloc(0x80);
            w.put<uintptr_t>(skill + 8, (uintptr_t)ids[i]);
            w.put<uint16_t>(skill + 0x1c, i == 4 ? 25 : 1);
            w.put<uintptr_t>(list + i * 8, skill);
        }
        w.put<uintptr_t>(manager + 0x30, list);
        w.put<int>(manager + 0x38, GAME_SKILLS);
        uintptr_t saved_manager = cheats::skill_manager, saved_add = cheats::add_skill_fn, saved_remove = cheats::remove_skill_fn;
        CHECK(cheats::skill_manager && cheats::add_skill_fn && cheats::remove_skill_fn);
        cheats::skill_manager = manager, cheats::add_skill_fn = (uintptr_t)&fake_add_skill, cheats::remove_skill_fn = (uintptr_t)&fake_remove_skill;
        cheats::skill_slots.clear();
        uintptr_t container = cheats::skill_container();
        CHECK(container);
        fake_learned.clear();
        fake_store_learned(container);
        cheats::tick();
        int stun = cheats::skill_index("StunAttacks"), kick = cheats::skill_index("Stunning"), multi = cheats::skill_index("Multithrow"),
            finisher = cheats::skill_index("Finisher"), legend = cheats::skill_index("LegendSkill_MaxHealth"), other = cheats::skill_index("Dodge");
        CHECK(stun >= 0 && kick >= 0 && multi >= 0 && finisher >= 0 && legend >= 0 && other >= 0);
        CHECK(cheats::skill_slots[stun] == 0 && cheats::skill_slots[finisher] == 3 && cheats::skill_slots[other] == -1);
        CHECK(cheats::skill_levels_now()[stun] == 0 && cheats::skill_levels_now()[other] == -1);
        cheats::learn_skill(finisher);
        cheats::learn_skill(multi);
        cheats::read_skill_levels();
        auto levels = cheats::skill_levels_now();
        CHECK(levels[stun] == 1 && levels[kick] == 1 && levels[finisher] == 1 && levels[multi] == 1 && fake_learned.size() == 4);
        CHECK(cheats::spent_points(2, levels) == 4);
        cheats::forget_skill(kick);
        CHECK(fake_learned.size() == 2 && fake_learned.count(0) && fake_learned.count(2));
        cheats::set_skill_level(legend, 7);
        CHECK(fake_learned[4] == 7);
        cheats::set_skill_level(legend, 3);
        CHECK(fake_learned[4] == 3);
        cheats::set_skill_level(legend, 99);
        CHECK(fake_learned[4] == 25);
        cheats::set_whole_tree(2, false);
        CHECK(!fake_learned.count(0) && !fake_learned.count(2) && fake_learned.count(4));
        int calls = fake_skill_calls;
        cheats::set_skill_level(other, 1);
        CHECK(fake_skill_calls == calls);
        fake_learned.clear();
        fake_store_learned(container);
        cheats::skill_manager = saved_manager, cheats::add_skill_fn = saved_add, cheats::remove_skill_fn = saved_remove;
        cheats::skill_slots.clear();
    }
    cheats::find("durability")->on = false;
    cheats::tick();
    game::wr<float>(blade + cheats::ITEM_CONDITION, 12.5f);
    cheats::tick();
    CHECK(game::rdv<float>(blade + cheats::ITEM_CONDITION) == 12.5f && cheats::condition_floor.empty());
    CHECK(game::rdv<uintptr_t>(w.player + 0x8f8) == game::g.vt_human);
    printf("cheats: %s\n", cheats::describe().c_str());
    CHECK(cheats::grab_filter_fn && cheats::grab_filter_fn == game::g.base + 0xd61b50);
    {
        static uintptr_t fake_vt = 0;
        alignas(16) static uint8_t thrower[0x100] = {};
        uintptr_t saved_vt = cheats::vt_throw_controller;
        cheats::vt_throw_controller = (uintptr_t)&fake_vt;
        *(uintptr_t*)thrower = (uintptr_t)&fake_vt;
        float times[cheats::THROW_PHASE_COUNT] = {0.5f, 0.2f, 0.4f, 0.1f, 0.3f, 0.6f};
        memcpy(thrower + cheats::THROW_PHASE_TIMES, times, sizeof times);
        CHECK(!cheats::throw_busy((uintptr_t)thrower));
        *(int*)(thrower + cheats::THROW_STATE) = 3;
        CHECK(cheats::throw_busy((uintptr_t)thrower));
        cheats::keep_throw_phase_times((uintptr_t)thrower, 2.0f);
        CHECK(*(float*)(thrower + cheats::THROW_PHASE_TIMES) == 0.25f && *(float*)(thrower + cheats::THROW_PHASE_TIMES + 20) == 0.3f);
        cheats::keep_throw_phase_times((uintptr_t)thrower, 2.0f);
        CHECK(*(float*)(thrower + cheats::THROW_PHASE_TIMES) == 0.25f);
        cheats::keep_throw_phase_times((uintptr_t)thrower, 1.0f);
        CHECK(!memcmp(thrower + cheats::THROW_PHASE_TIMES, times, sizeof times));
        cheats::vt_throw_controller = saved_vt;
        CHECK(cheats::uv_flare_id("Throwable_Btz_Flare") && cheats::uv_flare_id("Throwable_ZaidFlare") && !cheats::uv_flare_id("Throwable_Smoke_Flare") && !cheats::uv_flare_id(nullptr));
    }
    CHECK(cheats::vt_throw_controller && cheats::melee_reach_fn == game::g.base + 0xdbec70 && cheats::survivor_raise_check == game::g.base + 0x4ae135);
    {
        CHECK(game::g.context_ctor == game::g.base + 0x745130 && game::g.elemental_root == game::g.base + 0x1c16348);
        CHECK(game::g.vt_upgrade_desc && game::g.vt_blueprint_desc);
        uintptr_t pistol = w.desc["Firearm_PistolAGen"], gauze = w.desc["Craft_Gauze"];
        CHECK(game::has_upgrade_slots(pistol) && game::upgrade_slots(pistol) == 0 && game::upgrade_slots(gauze) == -1);
        CHECK(game::set_upgrade_slots(pistol, 3) && game::upgrade_slots(pistol) == 3);
        CHECK(game::set_upgrade_slots(pistol, 9) && game::upgrade_slots(pistol) == 4 && !game::set_upgrade_slots(gauze, 2));
        game::restore_upgrade_slots(pistol);
        CHECK(game::upgrade_slots(pistol) == 0 && game::slot_originals.empty());

        uintptr_t king = w.desc["Craft_Upgrade_DamL2DurL2BalL2"], dam = w.desc["Craft_Upgrade_Dam"], mod = w.desc["Craftplan_ElectricMelee"];
        CHECK(game::upgrade_desc(king) && game::upgrade_desc(dam) && !game::upgrade_desc(gauze) && game::weapon_blueprint(mod) && !game::weapon_blueprint(king));
        CHECK(!game::weapon_blueprint(w.desc["Craftplan_Molotov"]) && game::upgrade_slots(w.desc["Melee_MacheteAGen"]) == 1);

        uintptr_t saved_ctor = game::g.context_ctor;
        game::g.context_ctor = (uintptr_t)&fake_context_ctor;
        uintptr_t item = w.item("Melee_MacheteAGen", 1), ctx = item + game::ITEM_CONTEXT_OBJECT, recipe = 0x1234;
        w.put<uint32_t>(ctx + game::CONTEXT_WORD, 0x5a);
        w.put<uintptr_t>(ctx + game::CONTEXT_BLUEPRINT, recipe);
        CHECK(game::item_mods(item).mod == 0 && game::item_mods(item).parts.empty());
        CHECK(game::rebuild_item(item, mod, {0, king}));
        auto mods = game::item_mods(item);
        CHECK(mods.mod == mod && mods.parts.size() == 2 && mods.parts[0] == 0 && mods.parts[1] == king);
        CHECK(game::rdv<uintptr_t>(ctx) == game::g.base + 0x1000 && game::rdv<uintptr_t>(ctx + game::CONTEXT_DESC) == w.desc["Melee_MacheteAGen"]);
        CHECK(game::rdv<uint32_t>(ctx + game::CONTEXT_WORD) == 0x5a && game::rdv<uintptr_t>(ctx + game::CONTEXT_BLUEPRINT) == recipe && game::rdv<int>(ctx + 0x14) == 7);
        CHECK(game::rdv<uintptr_t>(ctx + 0x48) == (uintptr_t)fake_cache_object);
        CHECK(!game::rebuild_item(item, king, {}) && !game::rebuild_item(item, 0, {0x1234}) && !game::rebuild_item(item, 0, {dam, dam, dam, dam, dam}));
        CHECK(game::item_mods(item).mod == mod);
        CHECK(game::rebuild_item(item, mod, {mod, gauze, w.desc["Melee_MacheteAGen"]}));
        mods = game::item_mods(item);
        CHECK(mods.mod == mod && mods.parts == std::vector<uintptr_t>({mod, gauze, w.desc["Melee_MacheteAGen"]}));
        CHECK(game::rebuild_item(item, 0, {dam, dam, dam, dam}) && game::item_mods(item).parts.size() == 4 && !game::item_mods(item).mod);
        fake_random_mod = mod;
        CHECK(!game::rebuild_item(item, 0, {}) && game::item_mods(item).parts.size() == 4);
        fake_random_mod = 0;
        game::g.context_ctor = 0;
        CHECK(!game::rebuild_item(item, mod, {}));
        game::g.context_ctor = saved_ctor;

        uintptr_t shock_levels = fake_tree_node(game::LEVEL_NODE_KEY, 2, fake_tree_node(game::LEVEL_NODE_KEY, 1, 0, 0),
                                                fake_tree_node(game::LEVEL_NODE_KEY, 3, 0, fake_tree_node(game::LEVEL_NODE_KEY, 101, 0, 0)));
        uintptr_t burn_levels = fake_tree_node(game::LEVEL_NODE_KEY, 1, 0, fake_tree_node(game::LEVEL_NODE_KEY, 4, fake_tree_node(game::LEVEL_NODE_KEY, 2, 0, 0), 0));
        uintptr_t shock = fake_tree_node(game::ELEMENTAL_NODE_KEY, 1, 0, 0), burn = fake_tree_node(game::ELEMENTAL_NODE_KEY, 4, 0, 0);
        uintptr_t elementals = fake_tree_node(game::ELEMENTAL_NODE_KEY, 3, shock, burn);
        w.put<uintptr_t>(shock - game::ELEMENTAL_NODE_ENTRY + game::LEVEL_TREE, shock_levels);
        w.put<uintptr_t>(burn - game::ELEMENTAL_NODE_ENTRY + game::LEVEL_TREE, burn_levels);
        w.put<uint64_t>(shock - game::ELEMENTAL_NODE_ENTRY + game::ELEMENTAL_MASKS, 0x2);
        w.put<uint64_t>(burn - game::ELEMENTAL_NODE_ENTRY + game::ELEMENTAL_MASKS, 0x10);
        w.put<uint64_t>(burn - game::ELEMENTAL_NODE_ENTRY + game::ELEMENTAL_MASKS + 8, 0x4);
        uintptr_t manager = w.alloc(0x40), holder = w.alloc(0x1000), root = w.alloc(8);
        w.put<uintptr_t>(manager + game::ELEMENTAL_TREE, elementals);
        w.put<uintptr_t>(holder + game::ELEMENTAL_MANAGER, manager);
        w.put<uintptr_t>(root, holder);
        uintptr_t saved_root = game::g.elemental_root;
        game::g.elemental_root = root;
        CHECK(game::elemental_entry(1) == shock - game::ELEMENTAL_NODE_ENTRY && game::elemental_entry(3) && !game::elemental_entry(2) && !game::elemental_entry(9));
        CHECK(game::elemental_levels(1) == std::vector<int>({1, 2, 3, 101}) && game::elemental_levels(4) == std::vector<int>({1, 2, 4}) && game::elemental_levels(2).empty());
        CHECK(game::level_number(game::elemental_level(1, 3)) == 3 && game::level_number(game::elemental_level(4, 2)) == 2);
        CHECK(!game::elemental_level(1, 4) && !game::elemental_level(4, 3) && !game::elemental_level(1, 0));

        w.put<uintptr_t>(mod + game::BLUEPRINT_ELEMENTALS + 8, game::elemental_level(1, 1));
        w.put<uint64_t>(mod + game::BLUEPRINT_MASKS, 0x102);
        CHECK(game::mod_effect_level(mod, 1) == 1 && game::mod_effect_level(mod, 4) == 0 && game::mod_effect_level(mod, 40) == 0);
        CHECK(game::set_mod_effect(mod, 4, 2) && game::mod_effect_level(mod, 4) == 2);
        CHECK(game::rdv<uint64_t>(mod + game::BLUEPRINT_MASKS) == 0x112 && game::rdv<uint64_t>(mod + game::BLUEPRINT_MASKS + 8) == 0x4);
        CHECK(game::set_mod_effect(mod, 1, 0) && game::mod_effect_level(mod, 1) == 0 && game::rdv<uint64_t>(mod + game::BLUEPRINT_MASKS) == 0x110);
        CHECK(!game::set_mod_effect(mod, 1, 4) && !game::set_mod_effect(mod, 17, 1) && !game::set_mod_effect(king, 1, 1));
        CHECK(game::set_mod_effect(mod, 1, 101) && game::mod_effect_level(mod, 1) == 101);
        game::restore_mod(mod);
        CHECK(game::mod_effect_level(mod, 1) == 1 && game::mod_effect_level(mod, 4) == 0 && game::rdv<uint64_t>(mod + game::BLUEPRINT_MASKS) == 0x102);
        CHECK(game::rdv<uint64_t>(mod + game::BLUEPRINT_MASKS + 8) == 0 && game::mod_originals.empty());

        CHECK(cheats::effect_type(cheats::effect_key(11)) == 11 && cheats::effect_type("Effect") == -1 && cheats::effect_type("Damage") == -1);
        cheats::set_weapon_edits({{"Firearm_PistolAGen", {{cheats::SLOTS_KEY, 2}}}, {"Craftplan_ElectricMelee", {{cheats::effect_key(4), 4}, {cheats::effect_key(1), 0}}}});
        uintptr_t machete_ctx = game::rdv<uintptr_t>(game::rdv<uintptr_t>(w.backpack + 0x40) + 4 * 8) + game::ITEM_CONTEXT_OBJECT;
        CHECK(game::rdv<uintptr_t>(machete_ctx + game::CONTEXT_MOD) == mod && game::rdv<int>(machete_ctx + 0x14) == 0);
        game::g.context_ctor = (uintptr_t)&fake_context_ctor;
        cheats::apply_weapon_edits();
        CHECK(game::upgrade_slots(pistol) == 2 && game::mod_effect_level(mod, 4) == 4 && game::mod_effect_level(mod, 1) == 0);
        CHECK(game::rdv<int>(machete_ctx + 0x14) == 7 && game::rdv<uintptr_t>(machete_ctx + game::CONTEXT_MOD) == mod);
        CHECK(game::rebuild_items_with_mod(mod) == 1 && game::rebuild_items_with_mod(king) == 0 && game::rebuild_items_with_mod(0) == 0);
        game::g.context_ctor = saved_ctor;
        cheats::set_weapon_edits({});
        game::restore_mod(mod);
        game::restore_upgrade_slots(pistol);
        CHECK(game::upgrade_slots(pistol) == 0 && game::mod_effect_level(mod, 1) == 1);
        game::g.elemental_root = saved_root;
    }
    {
        const uintptr_t FN = 0x4ae0c0, SIZE = 0x164;
        auto* copy = (uint8_t*)VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        memcpy(copy, (const void*)(game::g.base + FN), SIZE);
        const uintptr_t ADD_FN = 0x1677f0, ADD_SIZE = 0x25b, OVERFLOW_IMM = 0x16799c, TREE_ADD_FN = 0xbb2f70, TREE_ADD_SIZE = 0x40e, FLAG_IMM = 0xbb3153;
        auto* add_copy = copy + 0x200;
        auto* tree_copy = (uint8_t*)VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        memcpy(tree_copy, (const void*)(game::g.base + TREE_ADD_FN), TREE_ADD_SIZE);
        memcpy(add_copy, (const void*)(game::g.base + ADD_FN), ADD_SIZE);
        uintptr_t real_check = cheats::survivor_raise_check, real_overflow = cheats::survivor_overflow_tree, real_flag = cheats::survivor_tree_flag;
        CHECK(real_overflow == game::g.base + OVERFLOW_IMM && game::rdv<uint8_t>(real_overflow) == 5);
        cheats::survivor_raise_check = (uintptr_t)copy + 0x4ae135 - FN;
        cheats::survivor_overflow_tree = (uintptr_t)add_copy + OVERFLOW_IMM - ADD_FN;
        cheats::survivor_tree_flag = (uintptr_t)tree_copy + FLAG_IMM - TREE_ADD_FN;
        CHECK(real_flag == game::g.base + FLAG_IMM && game::rdv<uint8_t>(real_flag) == 0);
        CHECK(cheats::keep_lowered_survivor() && cheats::keep_lowered_survivor());
        CHECK(copy[0x86] == 0xE9 && (uintptr_t)copy + 0x8b + *(int32_t*)(copy + 0x87) == (uintptr_t)copy + 0x155 && copy[0x8b] == 0x90 && copy[0x8c] == 0x90);
        CHECK(add_copy[OVERFLOW_IMM - ADD_FN] == 3 && add_copy[OVERFLOW_IMM - ADD_FN - 2] == 0x41 && add_copy[OVERFLOW_IMM - ADD_FN + 1] == 0);
        cheats::survivor_raise_check = real_check;
        CHECK(tree_copy[FLAG_IMM - TREE_ADD_FN] == 1);
        cheats::survivor_overflow_tree = real_overflow;
        cheats::survivor_tree_flag = real_flag;
    }
    {
        static uintptr_t target_vt[0x900 / 8] = {};
        for (auto& f : target_vt) f = (uintptr_t)&fake_no;
        target_vt[0x6e0 / 8] = (uintptr_t)&fake_yes;
        static uint8_t target[0x1000] = {}, grab[0x300] = {}, state[0x20] = {};
        *(uintptr_t*)target = (uintptr_t)target_vt;
        *(uintptr_t*)(target + 0xc88) = (uintptr_t)grab;
        *(uintptr_t*)(target + 0xbf8) = (uintptr_t)state;
        *(int*)(grab + 0x288) = -1;
        state[0x10] = 1;
        CHECK(cheats::grab_report((uintptr_t)target) == "0x878 no, 0x120 no, 0x6e0 yes, +600 0, grab +288 -1 +290 0 +298 0 +10 0, +bf8 1");
        const size_t FILTER_SIZE = 0xb5;
        auto* copy = (uint8_t*)VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        memcpy(copy, (const void*)cheats::grab_filter_fn, FILTER_SIZE);
        uintptr_t real_filter = cheats::grab_filter_fn;
        cheats::grab_filter_fn = (uintptr_t)copy;
        CHECK(cheats::install_grab_filter_hook() && copy[0] == 0xFF && copy[1] == 0x25);
        auto filter = (cheats::GrabFilterFn)copy;
        CHECK(!filter(0, (uintptr_t)target));
        target_vt[0x6e0 / 8] = (uintptr_t)&fake_no;
        state[0x10] = 0;
        CHECK(filter(0, (uintptr_t)target) && !filter(0, 0));
        int was_side = cheats::side;
        cheats::find_tweak("h_dropkick_angle")->factor = 3.0f;
        cheats::side = cheats::SIDE_SURVIVOR;
        CHECK(!filter(0, (uintptr_t)target));
        cheats::side = cheats::SIDE_HUNTER;
        CHECK(filter(0, (uintptr_t)target));
        cheats::side = cheats::SIDE_SURVIVOR;
        cheats::find_tweak("h_dropkick_angle")->factor = 1.0f;
        CHECK(filter(0, (uintptr_t)target));
        cheats::side = was_side;
        static uint8_t hunter[0x1000] = {}, hunter_logical[0x800] = {};
        *(uintptr_t*)hunter = game::g.vt_player;
        *(uintptr_t*)hunter_logical = game::g.vt_logical_player;
        *(uintptr_t*)(hunter + cheats::LOGICAL_PLAYER) = (uintptr_t)hunter_logical;
        CHECK(!cheats::is_hunter((uintptr_t)hunter));
        *(int*)(hunter_logical + cheats::PLAYER_ROLE) = cheats::ROLE_HUNTER;
        CHECK(cheats::is_hunter((uintptr_t)hunter) && !cheats::is_hunter((uintptr_t)target));
        cheats::hunter_in_match = true;
        cheats::update_side(w.player, false);
        CHECK(cheats::side == cheats::SIDE_SURVIVOR);
        cheats::hunter_in_match = false;
        cheats::side = was_side;
        cheats::grab_filter_fn = real_filter;
    }

    CHECK(read_var("f_btz_zombie_grab_range") == 10);
    cheats::find_tweak("z_pounce")->factor = 5.5f;
    cheats::find_tweak("h_dropkick")->factor = 10.0f;
    cheats::tick();
    CHECK(fake_var_object == (uintptr_t)&cheats::var_vtable[1]);
    CHECK(cheats::side == cheats::SIDE_PENDING && read_var("f_btz_zombie_grab_range") == 10 && read_var("f_btz_wrestling_kick_angle_max") == 22);
    cheats::side = cheats::SIDE_UNKNOWN;
    CHECK(read_var("f_btz_zombie_grab_range") == 25 && read_var("f_btz_wrestling_kick_angle_max") == 22);
    cheats::find_tweak("h_dropkick_angle")->factor = 10.0f;
    CHECK(read_var("f_btz_wrestling_kick_angle_max") == 180 && cheats::human_targeting_widened());
    cheats::find_tweak("h_dropkick_angle")->factor = 1.0f;
    CHECK(read_var("f_btz_pvp_grab_below_angle_threshold") == -50 && read_var("f_btz_zombie_grab_angle_max") == 7);
    CHECK(read_var("f_btz_other") == 7 && read_var("i_other") == 7);
    cheats::find_tweak("h_dropkick_height")->factor = 10.0f;
    CHECK(read_var("f_btz_pvp_grab_below_angle_threshold") == -90 && read_var("f_btz_pvp_grab_above_angle_threshold") == 90);
    cheats::find_tweak("h_dropkick_height")->factor = 1.0f;
    CHECK(cheats::active_count() == 2);
    cheats::find_tweak("h_dfa")->factor = 10.0f;
    CHECK(read_var("f_btz_jump_attack_range") == 12 && read_var("f_btz_jump_attack_angle_max") == 7);
    cheats::find_tweak("h_dfa_pull")->factor = 10.0f;
    CHECK(read_var("f_btz_jump_attack_angle_max") == 180 && read_var("f_btz_jump_attack_range") == 12);
    CHECK(read_var("f_btz_pvp_grab_below_angle_threshold") == -90 && read_var("f_btz_zombie_grab_range") == 25);
    int side = cheats::side;
    cheats::side = cheats::SIDE_HUNTER;
    CHECK(read_var("f_btz_pvp_grab_below_angle_threshold") == -50 && read_var("f_btz_jump_attack_angle_max") == 7 && read_var("f_btz_zombie_grab_range") == 25);
    cheats::side = cheats::SIDE_SURVIVOR;
    CHECK(read_var("f_btz_zombie_grab_range") == 10 && read_var("f_btz_jump_attack_angle_max") == 180);
    cheats::side = side;
    CHECK(read_var("f_btz_jump_attack_angle_max") == 180 && read_var("f_btz_zombie_grab_range") == 25);
    {
        auto& tackle = *cheats::find_tweak("z_tackle");
        float saved = tackle.factor;
        tackle.factor = 2.5f;
        CHECK(cheats::toggle_max(tackle) == tackle.max && cheats::tweak_text(tackle, tackle.factor) == "Max");
        CHECK(cheats::toggle_max(tackle) == 2.5f && cheats::tweak_text(tackle, 2.5f) == "x2.5" && cheats::tweak_text(tackle, 1.0f) == "Normal");
        CHECK(cheats::tweak_text(*cheats::find_tweak("h_recharge"), 5.5f) == "50%");
        tackle.factor = saved;
    }
    CHECK(read_var("f_flare_throw_recharge_time") == 7);
    cheats::find_tweak("h_recharge")->factor = 10.0f;
    CHECK(std::fabs(read_var("f_flare_throw_recharge_time") - cheats::RECHARGE_AT_MAX) < 1e-4f);
    CHECK(std::fabs(read_var("f_injector_throw_recharge_time_drained_btz_3_human") - cheats::RECHARGE_AT_MAX) < 1e-4f);
    CHECK(read_var("f_flare_throw_use_amount") == 0 && read_var("f_mine_throw_recharge_delay_drained") == 0 && read_var("f_flare_throw_min_charge_on_drained") == 7);
    cheats::find_tweak("h_recharge")->factor = 5.5f;
    CHECK(std::fabs(read_var("f_grenade_throw_recharge_time") - (7 + (cheats::RECHARGE_AT_MAX - 7) * 0.5f)) < 1e-4f);
    cheats::find_tweak("h_recharge")->factor = 1.0f;
    cheats::all_off();
    CHECK(read_var("f_btz_jump_attack_range") == 7);
    CHECK(read_var("f_btz_zombie_grab_range") == 10 && read_var("f_btz_wrestling_kick_angle_max") == 22 && cheats::active_count() == 0);
    {
        auto& glow = config::cfg.hunter_glow;
        CHECK(read_vec3("v3_btz_zombie_veins_draining_color") == (std::vector<float>{255, 50, 0}));
        glow.on = true;
        glow.color[0] = 0, glow.color[1] = 0.5f, glow.color[2] = 0.25f, glow.glow = 2;
        CHECK(read_vec3("v3_btz_zombie_veins_draining_color") == (std::vector<float>{0, 510, 255}));
        CHECK(read_vec3("v3_btz_zombie_veins_not_drained_color") == (std::vector<float>{1, 2, 3}));
        CHECK(read_vec3("v3_btz_zombie_uv_block_color") == (std::vector<float>{1, 2, 3}));
        glow.on = false;
        CHECK(read_vec3("v3_btz_zombie_veins_draining_color") == (std::vector<float>{255, 50, 0}));
        auto& horde = config::cfg.survivor_glow["horde"];
        horde = {true, {0, 1, 0}};
        CHECK(read_vec3("v3_btz_human_glow_color_cth") == (std::vector<float>{0, 3, 0}));
        CHECK(read_vec3("v3_btz_human_glow_color_ld") == (std::vector<float>{1, 2, 3}));
        config::cfg.survivor_glow["sense"] = {true, {0, 0, 1}};
        CHECK(read_vec3("v3_btz_human_glow_color_camo_spit") == (std::vector<float>{0, 0, 3}));
        horde.on = false;
        CHECK(read_vec3("v3_btz_human_glow_color_cth") == (std::vector<float>{1, 2, 3}));
        config::cfg.survivor_glow.clear();
        config::cfg.btz_colors["uv_block"] = {true, {0, 0, 1}};
        config::cfg.btz_colors["toxic"] = {true, {1, 0, 0}};
        CHECK(read_vec3("v3_btz_zombie_uv_block_color") == (std::vector<float>{0, 0, 3}) && read_vec3("v3_btz_zombie_uv_block_color_fpp") == (std::vector<float>{0, 0, 3}));
        CHECK(read_vec3("f_btz_zombie_fill_light_color") == (std::vector<float>{1, 2, 3}) && visuals::var_colors_on());
        config::cfg.btz_colors["uv_block"].on = false;
        CHECK(read_vec3("v3_btz_zombie_uv_block_color") == (std::vector<float>{1, 2, 3}) && !visuals::var_colors_on());
        config::cfg.btz_colors.clear();
    }

    {
        using visuals::fx;
        auto kept_fx = fx;
        fx = visuals::FxState();
        std::string engine_path = argv[1];
        engine_path = engine_path.substr(0, engine_path.find_last_of("/\\") + 1) + "engine_x64_rwdi.dll";
        HMODULE engine = LoadLibraryExA(engine_path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
        CHECK(engine && visuals::fx_find_manager(quiet) && fx.global == (uintptr_t)engine + 0xa3ef28);
        CHECK(std::all_of(std::begin(fx.vtables), std::end(fx.vtables), [](uintptr_t v) { return v != 0; }));
        fx = visuals::FxState();
        fx.resolved = true;
        CHECK(visuals::fx_base("Data\\FX/Toxic_Spit_Trail_Sequence.FX") == "toxic_spit_trail_sequence" && visuals::fx_base("camo") == "camo");
        uintptr_t vt_manager = w.alloc(8), vt_particle = w.alloc(8), vt_sequence = w.alloc(8);
        fx.manager_vtable = vt_manager, fx.vtables[0] = vt_particle, fx.vtables[2] = vt_sequence;
        uintptr_t manager = w.alloc(0x200), global = w.alloc(8);
        w.put<uintptr_t>(global, manager);
        w.put<uintptr_t>(manager, vt_manager);
        fx.global = global;
        auto named_def = [&](uintptr_t vt, const char* name) {
            uintptr_t def = w.alloc(0x200), text = w.alloc(64);
            w.put<uintptr_t>(def, vt);
            memcpy((void*)text, name, strlen(name) + 1);
            w.put<uintptr_t>(def + visuals::FX_DEF_NAME, text);
            return def;
        };
        auto in_map = [&](int map, std::vector<uintptr_t> defs) {
            uintptr_t first = 0;
            for (size_t i = defs.size(); i-- > 0;) {
                uintptr_t node = w.alloc(0x30) + 0x10;
                w.put<uintptr_t>(node - visuals::FX_NODE_DEF, defs[i]);
                w.put<uintptr_t>(node + game::TREE_MORE, first);
                first = node;
            }
            w.put<uintptr_t>(manager + map, first);
            w.put<uint32_t>(manager + map + visuals::FX_MAP_COUNT, (uint32_t)defs.size());
        };
        auto with_keys = [&](uintptr_t def, int at, std::vector<float> colors) {
            uintptr_t keys = w.alloc(colors.size() / 8 * visuals::FX_KEY_SIZE);
            for (size_t k = 0; k < colors.size() / 8; k++) {
                memcpy((void*)(keys + k * visuals::FX_KEY_SIZE + visuals::FX_KEY_MIN), &colors[k * 8], 16);
                memcpy((void*)(keys + k * visuals::FX_KEY_SIZE + visuals::FX_KEY_MAX), &colors[k * 8 + 4], 16);
            }
            w.put<uintptr_t>(def + at, keys);
            w.put<uint32_t>(def + at + visuals::FX_TRACK_COUNT, (uint32_t)(colors.size() / 8));
            return keys;
        };
        auto key = [](uintptr_t keys, int k, int at) {
            float* c = (float*)(keys + k * visuals::FX_KEY_SIZE + at);
            return std::vector<float>{c[0], c[1], c[2], c[3]};
        };
        uintptr_t spit_particle = named_def(vt_particle, "toxic_spit_trail_particle"), other = named_def(vt_particle, "fire_sparks");
        uintptr_t spit_keys = with_keys(spit_particle, 0xc0, {0.5f, 1, 0.5f, 1, 0.25f, 0.5f, 0.25f, 0.5f, 0, 2, 0, 1, 0, 1, 0, 1});
        uintptr_t other_keys = with_keys(other, 0xc0, {1, 0.5f, 0, 1, 1, 0.5f, 0, 1});
        uintptr_t sequence = named_def(vt_sequence, "Toxic_Spit_Trail_Sequence.fx"), element = w.alloc(0xd8);
        w.put<uintptr_t>(element + visuals::FX_ELEMENT_DEFS[0], spit_particle);
        w.put<uintptr_t>(element + visuals::FX_ELEMENT_DEFS[1], sequence);
        w.put<uintptr_t>(sequence + visuals::FX_ELEMENTS, element + visuals::FX_ELEMENT_NODE);
        in_map(0x48, {spit_particle, other});
        in_map(0x78, {sequence});
        visuals::apply_spit_colors(quiet);
        CHECK(fx.tracks.empty() && fx.keys.empty());
        config::cfg.btz_colors["toxic"] = {true, {1, 0, 0}};
        visuals::apply_spit_colors(quiet);
        CHECK(fx.tracks.size() == 1 && fx.tracks[0].def == spit_particle);
        CHECK(key(spit_keys, 0, visuals::FX_KEY_MIN) == (std::vector<float>{1, 0, 0, 1}) && key(spit_keys, 0, visuals::FX_KEY_MAX) == (std::vector<float>{0.5f, 0, 0, 0.5f}));
        CHECK(key(spit_keys, 1, visuals::FX_KEY_MIN) == (std::vector<float>{2, 0, 0, 1}) && key(other_keys, 0, visuals::FX_KEY_MIN) == (std::vector<float>{1, 0.5f, 0, 1}));
        config::cfg.btz_colors["toxic"].color[0] = 0, config::cfg.btz_colors["toxic"].color[2] = 1;
        visuals::apply_spit_colors(quiet);
        CHECK(key(spit_keys, 0, visuals::FX_KEY_MIN) == (std::vector<float>{0, 0, 1, 1}));
        config::cfg.btz_colors["toxic"].on = false;
        visuals::apply_spit_colors(quiet);
        CHECK(key(spit_keys, 0, visuals::FX_KEY_MIN) == (std::vector<float>{0.5f, 1, 0.5f, 1}) && key(spit_keys, 1, visuals::FX_KEY_MAX) == (std::vector<float>{0, 1, 0, 1}));
        config::cfg.btz_colors["toxic"].on = true;
        w.put<uintptr_t>(spit_particle, vt_sequence);
        visuals::apply_spit_colors(quiet);
        CHECK(key(spit_keys, 0, visuals::FX_KEY_MIN) == (std::vector<float>{0.5f, 1, 0.5f, 1}));
        config::cfg.btz_colors.clear();
        fx = kept_fx;
    }

    {
        uintptr_t equipment = w.alloc(0x80), prison = w.alloc(0x80);
        w.put<uintptr_t>(equipment, game::g.vt_equipment);
        w.put<float>(equipment + 0x50, 0.3f);
        w.put<uint8_t>(equipment + 0x55, 1);
        w.put<uintptr_t>(prison, game::g.vt_prison_data);
        w.put<float>(prison + 0x44, 100.0f);
        w.put<float>(prison + 0x48, 160.0f);
        uintptr_t rope = w.alloc(0x80);
        CHECK(game::g.vt_rope);
        w.put<uintptr_t>(rope, game::g.vt_rope);
        w.put<float>(rope + 0x40, 0.2f);
        w.put<uint8_t>(rope + 0x44, 1);
        game::wr<float>(w.params + cheats::param_ids["RopeMaxEnergy"] * 16 + 8, 1.0f);
        game::g.ropes = {rope};
        cheats::request_ownership = fake_request_ownership;
        cheats::find("hook")->on = true;
        game::g.equipment = {equipment};
        game::g.prison_data = {prison};
        CHECK(game::g.vt_equipment && game::g.vt_prison_data && game::g.vt_prison_sensor);
        CHECK(dlc::mode() == dlc::MODE_PRISON);
        {
            auto live = game::g.prison_data;
            game::g.prison_data = {equipment};
            CHECK(dlc::mode() == dlc::MODE_NONE);
            game::g.prison_data.clear();
            CHECK(dlc::mode() == dlc::MODE_NONE);
            game::g.prison_data = live;
        }
        {
            static uint8_t stub[14] = {0xFF, 0x25, 0, 0, 0, 0};
            uintptr_t target = 0x123456789;
            memcpy(stub + 6, &target, 8);
            CHECK(cheats::follow_jump((uintptr_t)stub) == target && cheats::follow_jump((uintptr_t)&fake_request_ownership) == (uintptr_t)&fake_request_ownership);
        }
        CHECK(game::g.human_control == 0x18);
        CHECK(!memcmp((const void*)(game::rdv<uintptr_t>(game::g.vt_human + cheats::SLOT_KILL * 8) + 6), "\x48\x8B\x81\xE8\x0C\x00\x00", 7));
        CHECK(game::g.player_control == 0x18 && game::g.sensor_control == 0 && game::g.sensor_rtti == 0x10);
        cheats::find("uv")->on = true;
        cheats::find("prison_pause")->on = true;
        cheats::tick();
        Sleep(300);
        cheats::tick();
        CHECK(game::rdv<float>(equipment + 0x50) == 1.0f && game::rdv<uint8_t>(equipment + 0x55) == 0 && !cheats::objects_missing);
        float start = game::rdv<float>(prison + 0x44), end = game::rdv<float>(prison + 0x48);
        CHECK(start > 100.2f && start < 101.0f && std::fabs(end - start - 60.0f) < 0.01f);
        CHECK(ownership_requests == 1 && game::rdv<uint8_t>(prison + 0x28) == 1);
        CHECK(game::rdv<float>(rope + 0x40) == 1.0f && game::rdv<uint8_t>(rope + 0x44) == 0);
        game::g.ropes.clear();
        cheats::tick();
        CHECK(cheats::objects_missing);
        cheats::find("hook")->on = false;
        cheats::find("uv")->on = false;
        cheats::find("prison_pause")->on = false;
        cheats::tick();
    }

    {
        CHECK(lockpick_code);
        static uint8_t fake_check[4];
        memcpy(fake_check, cheats::SPOT_DISTANCE_CLAMP, 4);
        cheats::lockpick_patch = (uintptr_t)fake_check;
        cheats::find("lockpick")->on = true;
        cheats::tick();
        CHECK(!memcmp(fake_check, cheats::SPOT_DISTANCE_ZERO, 4) && cheats::lockpick_patched());
        cheats::find("lockpick")->on = false;
        cheats::tick();
        CHECK(!memcmp(fake_check, cheats::SPOT_DISTANCE_CLAMP, 4) && !cheats::lockpick_patched());
        static uint8_t fake_pound[4];
        memcpy(fake_pound, (const void*)pound_code, 4);
        CHECK(!memcmp(fake_pound, cheats::POUND_EXPOSURE_TEST, 4));
        cheats::pound_exposure_check = (uintptr_t)fake_pound;
        cheats::find("z_pound_hits")->on = true;
        cheats::tick();
        CHECK(cheats::pound_always_hits());
        cheats::find("z_pound_hits")->on = false;
        cheats::tick();
        CHECK(!memcmp(fake_pound, cheats::POUND_EXPOSURE_TEST, 4) && !cheats::pound_always_hits());
    }

    {
        uintptr_t equipment = w.alloc(0x80);
        w.put<uintptr_t>(equipment, game::g.vt_equipment);
        w.put<float>(equipment + 0x50, 0.8f);
        game::g.equipment = {equipment};
        cheats::find_tweak("uv_slow")->factor = 10.0f;
        cheats::tick();
        w.put<float>(equipment + 0x50, 0.7f);
        cheats::tick();
        CHECK(std::fabs(game::rdv<float>(equipment + 0x50) - 0.79f) < 1e-4f);
        w.put<float>(equipment + 0x50, 0.9f);
        cheats::tick();
        CHECK(game::rdv<float>(equipment + 0x50) == 0.9f);
        cheats::find_tweak("uv_slow")->factor = 1.0f;
        cheats::tick();
        game::g.equipment.clear();
    }

    {
        uintptr_t profile = w.alloc(0x3000), mine = w.alloc(0x800), other = w.alloc(0x800), game = w.alloc(0x600);
        w.put<uintptr_t>(game + 0x540, profile);
        auto real_root = cheats::profile_root;
        static uintptr_t root_value;
        root_value = game;
        cheats::profile_root = (uintptr_t)&root_value;
        w.put<int>(profile + 0x2d98, 4);
        w.put<int>(profile + 0x2dd0, 7);
        for (uintptr_t lp : {mine, other}) w.put<uintptr_t>(lp, game::g.vt_logical_player);
        w.put<int>(mine + 0x74c, 4);
        w.put<int>(other + 0x74c, 9);
        w.put<uint8_t>(mine + 0x28, 1);
        w.put<uint8_t>(other + 0x28, 1);
        game::g.logical_players = {mine, other};
        CHECK(game::g.vt_logical_player && cheats::pvp_rank(false) == 4 && cheats::pvp_rank(true) == 7);
        cheats::set_pvp_rank(false, 14);
        CHECK(cheats::pvp_rank(false) == 14 && game::rdv<int>(mine + 0x74c) == 14 && game::rdv<int>(other + 0x74c) == 9);
        cheats::set_pvp_rank(true, -3);
        CHECK(cheats::pvp_rank(true) == 0);
        CHECK(cheats::rank_title(0) == 0 && cheats::rank_title(299) == 0 && cheats::rank_title(300) == 1 && cheats::rank_title(44890) == 11);
        CHECK(cheats::rank_start(1) == 300 && cheats::rank_start(2) == 690 && cheats::rank_title(cheats::rank_start(10)) == 10);
        cheats::set_pvp_title(true, 11);
        CHECK(cheats::rank_title(cheats::pvp_rank(true)) == 11);
        cheats::set_pvp_title(true, 3);
        CHECK(cheats::pvp_rank(true) == cheats::rank_start(3));
        cheats::profile_root = real_root;
        game::g.logical_players.clear();
    }

    {
        std::vector<std::pair<uintptr_t, const ItemInfo*>> packed;
        for (int i = 0; i < ITEM_COUNT && packed.size() < 60; i++) {
            float color = ITEMS[i].st[ST_Color];
            if (std::isnan(color) || color < 0 || color > 5) continue;
            uintptr_t d = w.alloc(0x600);
            w.put<uintptr_t>(d, game::g.base + 0x1000);
            w.put<uintptr_t>(d + 0x18, (uintptr_t)ITEMS[i].id);
            w.put<uint32_t>(d + 0x78, ((uint32_t)color << 24) | 0x00AB0013 | ((uint32_t)(i & 3) << 28));
            packed.push_back({d, &ITEMS[i]});
        }
        game::StatField fields[ST_COUNT];
        game::calibrate_stats(packed, fields);
        CHECK(packed.size() >= 20 && fields[ST_Color].off == 0x78 && fields[ST_Color].shift == 24 && !fields[ST_Color].is_float);
        auto saved = game::g.stats[ST_Color];
        game::g.stats[ST_Color] = fields[ST_Color];
        uintptr_t d = packed[0].first;
        uint32_t before = game::rdv<uint32_t>(d + 0x78);
        CHECK(game::write_stat(d, ST_Color, 5) && game::get_stat(d, ST_Color) == 5);
        CHECK((game::rdv<uint32_t>(d + 0x78) & ~(0xFu << 24)) == (before & ~(0xFu << 24)));
        game::g.stats[ST_Color] = saved;
    }
    {
        uintptr_t firearm_vt = game::g.base + 0x2000, throwable_vt = game::g.base + 0x3000;
        std::vector<std::pair<uintptr_t, const ItemInfo*>> mixed;
        int firearms = 0, throwables = 0;
        for (int i = 0; i < ITEM_COUNT; i++) {
            float ammo = ITEMS[i].st[ST_AmmoCount];
            if (std::isnan(ammo)) continue;
            bool firearm = !strncmp(ITEMS[i].id, "Firearm", 7);
            if (firearm ? firearms >= 30 : throwables >= 30) continue;
            uintptr_t d = w.alloc(0x600);
            w.put<uintptr_t>(d, (uintptr_t)(firearm ? firearm_vt : throwable_vt));
            w.put<uintptr_t>(d + 0x18, (uintptr_t)ITEMS[i].id);
            w.put<int>(d + (firearm ? 0x160 : 0x200), (int)ammo);
            mixed.push_back({d, &ITEMS[i]});
            (firearm ? firearms : throwables)++;
        }
        game::StatField global[ST_COUNT];
        game::calibrate_stats(mixed, global);
        auto classes = game::calibrate_classes(mixed, global);
        CHECK(firearms == 30 && throwables == 30 && global[ST_AmmoCount].off < 0);
        CHECK(classes[ST_AmmoCount].size() == 2 && classes[ST_AmmoCount][(uintptr_t)firearm_vt].off == 0x160 && classes[ST_AmmoCount][(uintptr_t)throwable_vt].off == 0x200);
        auto saved_global = game::g.stats[ST_AmmoCount];
        auto saved_classes = game::g.class_stats[ST_AmmoCount];
        game::g.stats[ST_AmmoCount] = global[ST_AmmoCount], game::g.class_stats[ST_AmmoCount] = classes[ST_AmmoCount];
        uintptr_t gun = 0;
        for (auto& [d, info] : mixed) gun = !gun && game::rdv<uintptr_t>(d) == (uintptr_t)firearm_vt ? d : gun;
        CHECK(game::has_stat(ST_AmmoCount) && game::write_stat(gun, ST_AmmoCount, 999) && game::rdv<int>(gun + 0x160) == 999 && game::get_stat(gun, ST_AmmoCount) == 999);
        uintptr_t freed = w.alloc(0x600);
        w.put<uintptr_t>(freed, 0x3d4ccccd);
        CHECK(!game::desc_alive(freed) && !game::write_stat(freed, ST_AmmoCount, 999) && game::rdv<int>(freed + 0x160) == 0);
        CHECK(game::desc_alive(gun));
        game::g.stats[ST_AmmoCount] = saved_global, game::g.class_stats[ST_AmmoCount] = saved_classes;
    }

    {
        static uintptr_t refusing_vt[8] = {}, accepting_vt[8] = {};
        refusing_vt[6] = (uintptr_t)&refuse_add;
        accepting_vt[6] = (uintptr_t)&fake_add;
        uintptr_t refusing = w.alloc(0x80), accepting = w.alloc(0x80);
        w.put<uintptr_t>(refusing, (uintptr_t)refusing_vt);
        w.put<uintptr_t>(accepting, (uintptr_t)accepting_vt);
        uintptr_t refusing_items = w.alloc(8 * 16);
        w.put<uintptr_t>(refusing + 0x40, refusing_items);
        w.put<uintptr_t>(accepting + 0x40, w.alloc(8 * 16));
        w.put<uintptr_t>(refusing_items, mat->items[0].addr);
        w.put<uint32_t>(refusing + 0x48, 1);
        uintptr_t saved_vts[2] = {game::g.vt_inv[2], game::g.vt_inv[3]};
        game::g.vt_inv[2] = (uintptr_t)refusing_vt;
        game::g.vt_inv[3] = (uintptr_t)accepting_vt;
        auto saved = game::g.invs;
        game::Item gauze = mat->items[0], alcohol = mat->items[1];
        game::g.invs = {{refusing, game::K_MATERIALS, -1, {gauze}}, {accepting, game::K_TOOLS, -1, {alcohol}}};
        uintptr_t king = game::g.descs["Craft_Upgrade_DamL2DurL2BalL2"];
        CHECK(king && game::give_anywhere(game::K_MATERIALS, king, 3) == game::K_TOOLS);
        uintptr_t given = game::rdv<uintptr_t>(game::rdv<uintptr_t>(accepting + 0x40));
        CHECK(game::rdv<uintptr_t>(given + 0x60) == king && game::rdv<int>(given + 0x40) == 3);
        game::g.invs = {{refusing, game::K_MATERIALS, -1, {gauze}}};
        CHECK(game::give_anywhere(game::K_MATERIALS, king, 3) == -1);
        game::g.invs = saved;
        game::g.vt_inv[2] = saved_vts[0];
        game::g.vt_inv[3] = saved_vts[1];
    }
    {
        static uintptr_t pack_vt[8] = {}, stash_vt[8] = {};
        pack_vt[6] = stash_vt[6] = (uintptr_t)&fake_add;
        uintptr_t pack = w.alloc(0x80), stash = w.alloc(0x80);
        w.put<uintptr_t>(pack, (uintptr_t)pack_vt), w.put<uintptr_t>(stash, (uintptr_t)stash_vt);
        w.put<uintptr_t>(pack + 0x40, w.alloc(8 * 16)), w.put<uintptr_t>(stash + 0x40, w.alloc(8 * 16));
        uintptr_t axe = w.alloc(0x100), knife = w.alloc(0x100);
        auto make = [&](uintptr_t desc, int count) {
            uintptr_t it = w.alloc(0x100);
            w.put<int>(it + 0x40, count), w.put<float>(it + 0x44, 50.0f), w.put<uintptr_t>(it + 0x60, desc), w.put<uintptr_t>(it + 0xA0, 0x1234);
            return it;
        };
        uintptr_t weapon = make(axe, 1), stack = make(knife, 5), other = make(knife, 1);
        auto set_items = [&](uintptr_t inv, std::vector<uintptr_t> items) {
            for (size_t i = 0; i < items.size(); i++) w.put<uintptr_t>(game::rdv<uintptr_t>(inv + 0x40) + i * 8, items[i]);
            w.put<uint32_t>(inv + 0x48, (uint32_t)items.size());
        };
        static std::vector<uintptr_t> ground;
        ground.clear();
        auto drop = [&](uintptr_t desc) {
            uintptr_t obj = w.alloc(0x800);
            w.put<uintptr_t>(obj + 0x120, desc);
            ground.push_back(obj);
        };
        cheats::dropped_near = [] { return ground; };
        set_items(pack, {weapon, stack, other});
        uintptr_t saved_vts[2] = {game::g.vt_inv[3], game::g.vt_inv[4]};
        game::g.vt_inv[3] = (uintptr_t)pack_vt, game::g.vt_inv[4] = (uintptr_t)stash_vt;
        auto saved_invs = game::g.invs;
        auto saved_wallets = game::g.wallets;
        game::g.invs = {{pack, game::K_BACKPACK, 22, {}}, {stash, game::K_STASH, -1, {}}};
        game::g.wallets.clear();
        cheats::dupe_drops(true);
        set_items(pack, {stack, other});
        cheats::dupe_drops(true);
        CHECK(game::rdv<uint32_t>(pack + 0x48) == 2 && cheats::dupes == 0 && cheats::pending_drops.size() == 1);
        drop(axe);
        cheats::dupe_drops(true);
        uint32_t n = game::rdv<uint32_t>(pack + 0x48);
        uintptr_t copy = game::rdv<uintptr_t>(game::rdv<uintptr_t>(pack + 0x40) + 2 * 8);
        CHECK(n == 3 && game::rdv<uintptr_t>(copy + 0x60) == axe && cheats::dupes == 1 && cheats::pending_drops.empty());
        set_items(pack, {stack, copy}), set_items(stash, {other});
        drop(knife);
        cheats::dupe_drops(true);
        CHECK(game::rdv<uint32_t>(pack + 0x48) == 2 && cheats::dupes == 1);
        w.put<int>(stack + 0x40, 3);
        cheats::dupe_drops(true);
        CHECK(game::rdv<int>(stack + 0x40) == 3);
        drop(knife);
        cheats::dupe_drops(true);
        CHECK(game::rdv<int>(stack + 0x40) == 5 && cheats::dupes == 2);
        set_items(pack, {stack});
        cheats::dupe_drops(true);
        cheats::dupe_drops(true);
        CHECK(game::rdv<uint32_t>(pack + 0x48) == 1 && cheats::dupes == 2);
        cheats::dupe_drops(false);
        CHECK(cheats::held_items.empty() && cheats::pending_drops.empty());
        cheats::dropped_near = nullptr;
        game::g.invs = saved_invs, game::g.wallets = saved_wallets;
        game::g.vt_inv[3] = saved_vts[0], game::g.vt_inv[4] = saved_vts[1];
        cheats::dupes = 0;
    }

    config::cfg.accent[0] = 0.25f;
    config::cfg.pages = {{"combat", false}, {"player", true}};
    config::cfg.cheats_on = {"god", "uv"};
    config::cfg.tweaks = {{"h_dfa", 4.5f}};
    config::cfg.cheat_keys = {{"god", 'G'}, {"hook", VK_F5}, {REFILL_KEY, 'R'}};
    config::cfg.hidden_configs = {"hunter_rage"};
    char tmp[MAX_PATH];
    GetTempPathA(MAX_PATH, tmp);
    std::string path = std::string(tmp) + "fatrainer_selftest.ini";
    CHECK(config::save(path));
    config::cfg = config::Config();
    config::load(path);
    config::load(path);
    CHECK(config::cfg.cheat_keys.size() == 3 && config::cfg.cheat_keys["god"] == 'G' && config::cfg.cheat_keys["hook"] == VK_F5 && config::cfg.cheat_keys[REFILL_KEY] == 'R');
    CHECK(config::cfg.hidden_configs.size() == 1);
    config::normalize({"player", "combat", "settings"});
    CHECK(config::cfg.accent[0] == 0.25f && config::cfg.cheats_on.size() == 2 && config::cfg.cheats_on[1] == "uv");
    CHECK(config::cfg.pages.size() == 3 && config::cfg.pages[0].id == "combat" && !config::cfg.pages[0].visible);
    CHECK(config::cfg.pages[2].id == "settings" && config::cfg.pages[2].visible);
    CHECK(config::cfg.tweaks.size() == 1 && config::cfg.tweaks[0].second == 4.5f);
    config::normalize({"player", "combat", "skills", "settings"});
    CHECK(config::cfg.pages[1].id == "skills" && config::cfg.pages[3].id == "settings");
    config::cfg.pages = {{"player", true}, {"combat", true}, {"give", true}, {"settings", true}};
    config::normalize({"player", "combat", "prison", "give", "settings"});
    CHECK(config::cfg.pages[2].id == "prison" && config::cfg.pages[3].id == "give");
    CHECK(config::migrate_page_id("pvp") == "survivor" && config::migrate_page_id("prison") == "dlc" && config::migrate_page_id("player") == "player");
    config::cfg.pages = {{"prison", false}, {"pvp", true}, {"survivor", true}, {"player", true}};
    config::normalize({"player", "survivor", "dlc", "settings"});
    CHECK(config::cfg.pages.size() == 4 && config::cfg.pages[0].id == "dlc" && !config::cfg.pages[0].visible && config::cfg.pages[1].id == "survivor" && config::cfg.pages[2].id == "player" && config::cfg.pages[3].id == "settings");
    {
        float rgb[3] = {1.0f, 0.5f, 0.0f};
        CHECK(color::to_hex(rgb) == "#FF8000");
        CHECK(color::to_byte(0.5f) == 128 && color::to_byte(-1) == 0 && color::to_byte(2) == 255);
        float parsed[3] = {0, 0, 0};
        CHECK(color::from_hex("#3293c5", parsed) && color::to_byte(parsed[0]) == 0x32 && color::to_byte(parsed[2]) == 0xc5);
        CHECK(color::from_hex("FF0000", parsed) && parsed[0] == 1.0f && parsed[1] == 0.0f);
        float kept[3] = {0.1f, 0.2f, 0.3f};
        CHECK(!color::from_hex("#12", kept) && !color::from_hex("zzzzzz", kept) && !color::from_hex("#1234567", kept) && kept[0] == 0.1f);
        float spun[3] = {0.5f, 0.2f, 0.1f};
        color::rainbow(spun, 0.0);
        CHECK(spun[0] == 0.5f && spun[1] == 0 && spun[2] == 0);
        color::rainbow(spun, 1.0 / 3);
        CHECK(spun[0] < 1e-5f && fabsf(spun[1] - 0.5f) < 1e-5f && spun[2] < 1e-5f);
        color::rainbow(spun, 2.5);
        CHECK(spun[0] < 1e-5f && fabsf(spun[1] - 0.5f) < 1e-5f && fabsf(spun[2] - 0.5f) < 1e-5f);
        float black[3] = {0, 0, 0};
        color::rainbow(black, 0);
        CHECK(black[0] == 1.0f && black[1] == 0 && black[2] == 0);
    }
    {
        auto kept = config::cfg;
        config::cfg = config::Config();
        config::cfg.hud_tint = {true, {0.5f, 0.25f, 1.0f}};
        config::cfg.hud_parts["stamina"] = {true, {1.0f, 0.0f, 0.0f}};
        config::cfg.hud_parts["radar"] = {true, {0.0f, 1.0f, 0.0f}, true};
        config::cfg.rainbow["hud.radar"] = 4.5f;
        config::cfg.survivor_glow["uv"] = {true, {1.0f, 0.0f, 0.0f}};
        config::cfg.btz_colors["horde_spit"] = {true, {0.0f, 0.5f, 1.0f}};
        config::cfg.crosshair = {true, false, config::CIRCLE, 12, 3, 5, 0.8f, {0.1f, 0.2f, 0.3f}};
        config::cfg.texts["btz_pounce_interrupted"] = "Nope, line\none, a=b \\ done";
        config::cfg.texts["HUDMessage_BTZDeath_Killer"] = "Zabi\xc5\x82" "e\xc5\x9b %s";
        CHECK(config::save(path));
        config::cfg = config::Config();
        config::load(path);
        CHECK(config::cfg.hud_tint.on && config::cfg.hud_tint.color[1] == 0.25f);
        CHECK(config::cfg.hud_parts.size() == 2 && config::cfg.hud_parts["stamina"].on && config::cfg.hud_parts["stamina"].color[0] == 1.0f);
        CHECK(config::cfg.hud_parts["radar"].hidden && !config::cfg.hud_parts["stamina"].hidden && config::cfg.rainbow["hud.radar"] == 4.5f);
        CHECK(config::cfg.survivor_glow["uv"].on && config::cfg.survivor_glow["uv"].color[0] == 1.0f);
        CHECK(config::cfg.btz_colors.size() == 1 && config::cfg.btz_colors["horde_spit"].on && config::cfg.btz_colors["horde_spit"].color[1] == 0.5f);
        CHECK(config::color_of("btz.horde_spit") == config::cfg.btz_colors["horde_spit"].color && !config::color_of("btz.missing"));
        const auto& x = config::cfg.crosshair;
        CHECK(x.on && !x.outline && x.style == config::CIRCLE && x.size == 12 && x.gap == 5 && x.opacity == 0.8f && x.color[2] == 0.3f);
        CHECK(config::color_of("hud.radar") == config::cfg.hud_parts["radar"].color && config::color_of("glow.uv") == config::cfg.survivor_glow["uv"].color);
        CHECK(!config::color_of("hud.missing") && !config::color_of("nothing") && config::color_of("accent") == config::cfg.accent);
        config::cfg.hud_tint.color[0] = 1, config::cfg.hud_tint.color[1] = 0, config::cfg.hud_tint.color[2] = 0;
        config::cfg.rainbow["hud_tint"] = 4;
        visuals::spin_rainbows(1.0);
        CHECK(fabsf(config::cfg.hud_tint.color[0] - 0.5f) < 1e-5f && config::cfg.hud_tint.color[1] == 1.0f && config::cfg.hud_tint.color[2] == 0);
        CHECK(config::cfg.texts["btz_pounce_interrupted"] == "Nope, line\none, a=b \\ done");
        CHECK(config::cfg.texts["HUDMessage_BTZDeath_Killer"] == "Zabi\xc5\x82" "e\xc5\x9b %s");
        FILE* old = fopen(path.c_str(), "w");
        fprintf(old, "hud=map,1,0.200,0.400,0.600\nhud=stamina,1,1.000,0.000,0.000\n");
        fclose(old);
        config::load(path);
        auto& parts = config::cfg.hud_parts;
        CHECK(parts.size() == 4 && parts["radar"].on && parts["objectives"].color[1] == 0.4f && parts["marker"].on && parts["stamina"].on && !parts.count("map"));
        config::cfg = kept;
    }
    {
        const float cyan[3] = {0.0f, 1.0f, 0.89f}, white[3] = {1, 1, 1};
        float hive[3] = {0.84f, 0.84f, 0.84f}, fill[3] = {0.84f, 0.17f, 0.26f};
        visuals::tint_track(hive, cyan);
        CHECK(fabsf(hive[0] - 0.84f) < 1e-4f && fabsf(hive[1] - 0.84f) < 1e-4f && fabsf(hive[2] - 0.84f) < 1e-4f);
        visuals::tint_track(fill, cyan);
        CHECK(fabsf(fill[1] - 0.84f) < 1e-3f && fabsf(fill[0] - 0.17f) < 1e-3f && fill[2] < fill[1] && fill[2] > fill[0]);
        float grey[3] = {0.84f, 0.84f, 0.84f};
        visuals::tint_track(grey, white);
        CHECK(fabsf(grey[0] - 0.336f) < 1e-4f && grey[0] == grey[1] && grey[1] == grey[2]);
        float black[3] = {0, 0, 0}, red[3] = {0.84f, 0.17f, 0.26f};
        const float none[3] = {0, 0, 0};
        visuals::tint_track(black, cyan);
        visuals::tint_track(red, none);
        CHECK(black[0] == 0 && black[1] == 0 && black[2] == 0 && !std::isnan(red[0]) && !std::isnan(red[1]) && !std::isnan(red[2]));
        CHECK(visuals::track_element(visuals::part_named("HudZombieHiveCounter"), "I_Hive"));
        CHECK(!visuals::track_element(visuals::part_named("HudZombieHiveCounter"), "I_Progress") && !visuals::track_element(visuals::part_named("HudStamina"), "I_Hive"));
    }
    {
        FakeElement health_bar{1, "I_HealthBar", visuals::UI_VIEW}, stamina_bar{1, "I_StaminaBar", visuals::UI_VIEW}, label{1, "T_Level", visuals::UI_TEXT};
        stamina_bar.color[0] = stamina_bar.original[0] = 0.75f;
        FakeElement stamina_bg{1, "StaminaBarBg", visuals::UI_VIEW};
        for (int c = 0; c < 3; c++) stamina_bg.color[c] = stamina_bg.original[c] = 0.15f;
        label.alpha = 0.3f;
        FakeElement health{1, "health_wrap", visuals::UI_NONE}, stamina{1, "HudStamina", visuals::UI_NONE}, root{1, "HUD_DI", visuals::UI_NONE};
        FakeElement radar{1, "HudRadar", visuals::UI_NONE}, radar_icon{1, "I_Icon", visuals::UI_VIEW};
        FakeElement nests{1, "HudZombieHiveCounter", visuals::UI_NONE}, hive{1, "I_Hive", visuals::UI_VIEW}, fill{1, "I_Progress", visuals::UI_VIEW};
        for (int c = 0; c < 3; c++) hive.color[c] = hive.original[c] = 0.84f;
        fill.color[0] = fill.original[0] = 0.84f, fill.color[1] = fill.original[1] = 0.17f, fill.color[2] = fill.original[2] = 0.26f;
        nests.children = {&hive, &fill};
        health.children = {&health_bar, &label};
        stamina.children = {&stamina_bg, &stamina_bar};
        radar.children = {&radar_icon};
        root.children = {&health, &stamina, &radar, &nests};
        auto saved_ui = visuals::ui;
        auto saved_cfg = config::cfg;
        visuals::ui = {fake_children_count, fake_child, fake_get_color, fake_set_color, fake_get_color, fake_set_text_color, fake_name_of, fake_kind_of,
                       fake_vtable_of, fake_text_alpha, fake_get_opacity, fake_set_opacity, fake_shown};
        CHECK(!visuals::hud_found());
        visuals::find_hud((uintptr_t)&root);
        CHECK(visuals::hud_found() && visuals::hud_elements() == 7 && !visuals::game_crosshair_shown());
        config::cfg.hud_tint = {true, {0.5f, 1.0f, 1.0f}};
        config::cfg.hud_parts["stamina"] = {true, {0.0f, 1.0f, 0.0f}};
        config::cfg.hud_parts["nests"] = {true, {0.0f, 1.0f, 0.89f}};
        visuals::apply_hud();
        CHECK(fabsf(hive.color[0] - 0.84f) < 1e-4f && fabsf(hive.color[1] - 0.84f) < 1e-4f);
        CHECK(fill.color[0] < 0.01f && fabsf(fill.color[1] - 0.84f) < 1e-4f);
        CHECK(health_bar.color[0] == 0.5f && health_bar.color[1] == 1.0f && label.color[0] == 0.5f && label.written_alpha == 0.3f);
        CHECK(stamina_bar.color[0] == 0.0f && stamina_bar.color[1] == 1.0f);
        CHECK(stamina_bg.color[0] == 0.0f && fabsf(stamina_bg.color[1] - 0.15f) < 1e-6f);
        stamina_bar.color[0] = 0.9f, stamina_bar.color[1] = 0.2f;
        visuals::apply_hud();
        CHECK(stamina_bar.color[0] == 0.0f && fabsf(stamina_bar.color[1] - 0.9f) < 1e-6f);
        {
            static uint64_t view_engine[0x100] = {0x77}, text_engine[0x100] = {0x78};
            static float seen[4];
            size_t bar = visuals::hud_known[(uintptr_t)&stamina_bar], text = visuals::hud_known[(uintptr_t)&label];
            visuals::hud_nodes[bar].engine = (uintptr_t)view_engine, visuals::hud_nodes[bar].engine_vtable = 0x77;
            visuals::hud_nodes[text].engine = (uintptr_t)text_engine, visuals::hud_nodes[text].engine_vtable = 0x78;
            visuals::hud_engines[(uintptr_t)view_engine] = bar, visuals::hud_engines[(uintptr_t)text_engine] = text;
            auto saved_view = visuals::view_color_original;
            auto saved_text = visuals::text_color_original;
            visuals::view_color_original = [](uintptr_t, float r, float g, float b) { seen[0] = r, seen[1] = g, seen[2] = b; };
            visuals::text_color_original = [](uintptr_t, const float* c) { memcpy(seen, c, sizeof seen); };
            visuals::view_color_hook((uintptr_t)view_engine, 0.8f, 0.2f, 0.0f);
            CHECK(seen[0] == 0.0f && fabsf(seen[1] - 0.8f) < 1e-6f && visuals::hud_nodes[bar].original[0] == 0.8f);
            visuals::trainer_writing = true;
            visuals::view_color_hook((uintptr_t)view_engine, 0.3f, 0.0f, 0.0f);
            visuals::trainer_writing = false;
            CHECK(seen[0] == 0.3f && visuals::hud_nodes[bar].original[0] == 0.8f);
            visuals::view_color_hook((uintptr_t)view_engine + 8, 0.3f, 0.0f, 0.0f);
            CHECK(seen[0] == 0.3f);
            float* own = (float*)((uint8_t*)text_engine + visuals::TEXT_COLOR);
            own[0] = 0.6f, own[1] = 0.0f, own[2] = 0.0f, own[3] = 0.5f;
            visuals::text_color_hook((uintptr_t)text_engine, own);
            CHECK(fabsf(seen[0] - 0.3f) < 1e-6f && fabsf(seen[1] - 0.6f) < 1e-6f && seen[3] == 0.5f && fabsf(own[1] - 0.6f) < 1e-6f);
            CHECK(visuals::hud_nodes[text].original[0] == 0.6f);
            const float other[4] = {0.0f, 0.0f, 0.9f, 1.0f};
            visuals::text_color_hook((uintptr_t)text_engine, other);
            CHECK(seen[2] == 0.9f && seen[0] == 0.0f && visuals::hud_nodes[text].original[0] == 0.6f);
            visuals::view_color_original = saved_view, visuals::text_color_original = saved_text;
            visuals::hud_nodes[bar].engine = visuals::hud_nodes[text].engine = 0;
            visuals::hud_nodes[bar].engine_vtable = visuals::hud_nodes[text].engine_vtable = 0;
            visuals::hud_engines.clear();
            stamina_bar.color[0] = 0.9f, stamina_bar.color[1] = 0.2f, stamina_bar.color[2] = 0.0f;
            visuals::apply_hud();
        }
        config::cfg.hud_tint.on = false, config::cfg.hud_parts.clear();
        visuals::apply_hud();
        CHECK(health_bar.color[0] == 1.0f && stamina_bar.color[0] == 0.9f && stamina_bar.color[1] == 0.2f);
        visuals::apply_hud();
        CHECK(stamina_bar.color[0] == 0.9f && stamina_bar.color[1] == 0.2f);
        FakeElement late_icon{1, "I_Late", visuals::UI_VIEW};
        radar.children.push_back(&late_icon);
        visuals::walk_hud((uintptr_t)&root, -1, 0);
        CHECK(visuals::hud_elements() == 8);
        visuals::walk_hud((uintptr_t)&root, -1, 0);
        CHECK(visuals::hud_elements() == 8);
        config::cfg.hud_parts["radar"] = {true, {0.0f, 0.0f, 1.0f}};
        visuals::apply_hud();
        CHECK(late_icon.color[2] == 1.0f && late_icon.color[0] == 0.0f);
        config::cfg.hud_parts.clear();
        visuals::apply_hud();
        CHECK(late_icon.color[0] == 1.0f);
        radar.children.pop_back();
        late_icon.vt = 2;
        health_bar.vt = 2;
        config::cfg.hud_tint.on = true;
        visuals::apply_hud();
        CHECK(dead_writes == 0 && fabsf(stamina_bar.color[0] - 0.45f) < 1e-6f);
        auto saved_opacity = visuals::opacity_original;
        visuals::opacity_original = fake_engine_opacity;
        radar.engine = (uintptr_t)&radar;
        CHECK(offsetof(FakeElement, engine) == visuals::UI_ENGINE_OBJECT);
        config::cfg.hud_parts["radar"].hidden = true;
        visuals::apply_hud();
        CHECK(radar.opacity == 0 && radar_icon.opacity == 1);
        fake_set_opacity((uintptr_t)&radar, 0.7f);
        visuals::apply_hud();
        CHECK(radar.opacity == 0);
        config::cfg.hud_parts["radar"].hidden = false;
        visuals::apply_hud();
        CHECK(radar.opacity == 0.7f);
        config::cfg.hud_parts["radar"].hidden = true;
        visuals::apply_hud();
        CHECK(radar.opacity == 0);
        visuals::forget_hud();
        CHECK(!visuals::hud_found() && stamina_bar.color[0] == 0.9f && dead_writes == 0 && radar.opacity == 0.7f);
        visuals::opacity_original = saved_opacity;

        config::cfg.hud_parts.erase("radar");
        CHECK(visuals::looks_missing());
        config::cfg.hud_tint.on = false;
        CHECK(!visuals::looks_missing());
        visuals::ui = saved_ui;
        config::cfg = saved_cfg;
    }
    {
        auto saved_api = visuals::texts_api;
        auto saved_cfg = config::cfg;
        uintptr_t saved_global = std::exchange(visuals::tools_global, 0);
        visuals::texts_api = {fake_find_text, fake_add_text, 0x1000};
        fake_load_text("btz_pounce_interrupted", L"Pounce interrupted by UV light!");
        auto shown = [](const char* key) { return std::wstring(fake_text_entries[key].text); };
        config::cfg.texts["btz_pounce_interrupted"] = "No pounce \xc5\x82\nfor you";
        config::cfg.texts["missing_key"] = "Nothing";
        visuals::keep_texts();
        CHECK(shown("btz_pounce_interrupted") == L"No pounce \x0142\nfor you" && !fake_text_entries.count("missing_key"));
        fake_load_text("btz_pounce_interrupted", L"Pounce interrupted by UV light!");
        visuals::keep_texts();
        CHECK(shown("btz_pounce_interrupted") == L"No pounce \x0142\nfor you");
        config::cfg.texts.clear();
        visuals::keep_texts();
        CHECK(shown("btz_pounce_interrupted") == L"Pounce interrupted by UV light!");
        CHECK(visuals::safe_format("Got you %s", "You killed %s") && visuals::safe_format("100%% sure %s", "You killed %s"));
        CHECK(!visuals::safe_format("%s got 100% rekt", "You killed %s") && !visuals::safe_format("%s and %s", "You killed %s"));
        CHECK(!visuals::safe_format("trailing %", "Pounce interrupted by UV light!") && visuals::safe_format("No pounce", "You interrupted %s pouncing %s"));
        CHECK(visuals::safe_format("%COLOR(FF4D6D)Pounced:%COLOR(reset) %s", "You pounced %s") && !visuals::safe_format("%COLOR(FF4D6D %s", "You pounced %s"));
        CHECK(visuals::colored("Kicked:", "#5AC8FA") == "%COLOR(5AC8FA)Kicked:%COLOR(reset)" && visuals::without_colors("%s %COLOR(5AC8FA)Kicked:%COLOR(reset) You") == "%s Kicked: You");
        CHECK(config::game_colors("%s <font color=\"#38E1A0\">Failed Pounce:</font> You") == "%s %COLOR(38E1A0)Failed Pounce:%COLOR(reset) You");
        CHECK(config::game_colors("<font color=\"red\">x</font>") == "x%COLOR(reset)");
        fake_load_text("HUDMessage_BTZDeath_Killer", L"You killed %s");
        config::cfg.texts["HUDMessage_BTZDeath_Killer"] = "%s got 100% rekt";
        visuals::keep_texts();
        CHECK(shown("HUDMessage_BTZDeath_Killer") == L"You killed %s");
        config::cfg.texts.clear();
        const std::wstring fists_pl = L"Pi\x0119\x015b" L"ci", hello_pl = L"Cze\x015b\x0107";
        fake_load_text("Melee_Fists_N", fists_pl.c_str());
        fake_load_text("Shopkeeper_X_DLG01", hello_pl.c_str());
        visuals::libraries["Xx"] = std::make_shared<const visuals::TextLibrary>(visuals::TextLibrary{
            {"common", {{"Melee_Fists_N", L"Fists", visuals::TEXT_ITEMS}, {"Shopkeeper_X_DLG01", L"Hello", visuals::TEXT_SPOKEN}}}});
        std::string saved_started = std::exchange(visuals::started_language, "Pl"), saved_steam = std::exchange(visuals::steam_language, "Pl");
        config::cfg.subtitle_language = "Xx";
        visuals::keep_texts();
        CHECK(shown("Melee_Fists_N") == fists_pl && shown("Shopkeeper_X_DLG01") == L"Hello");
        fake_load_text("Melee_Fists_N", fists_pl.c_str());
        fake_load_text("Shopkeeper_X_DLG01", hello_pl.c_str());
        visuals::keep_texts();
        CHECK(shown("Shopkeeper_X_DLG01") == L"Hello");
        config::cfg.texts["Shopkeeper_X_DLG01"] = "Yo";
        visuals::keep_texts();
        CHECK(shown("Shopkeeper_X_DLG01") == L"Yo");
        config::cfg.texts.erase("Shopkeeper_X_DLG01");
        visuals::keep_texts();
        CHECK(shown("Shopkeeper_X_DLG01") == L"Hello");
        visuals::rows();
        visuals::keep_text_rows();
        auto table = visuals::rows();
        CHECK(table && table->index.count("Melee_Fists_N") && table->rows[table->index.at("Melee_Fists_N")].shown == visuals::utf8(fists_pl.c_str()));
        CHECK(table->rows[table->index.at("HUDMessage_BTZDeath_Killer")].english == "You killed %s");
        config::cfg.subtitle_language.clear();
        visuals::keep_texts();
        CHECK(shown("Melee_Fists_N") == fists_pl && shown("Shopkeeper_X_DLG01") == hello_pl && visuals::text_originals.empty());
        config::cfg.kind_languages["items"] = "Xx";
        visuals::keep_texts();
        CHECK(shown("Melee_Fists_N") == L"Fists" && shown("Shopkeeper_X_DLG01") == hello_pl && visuals::kind_code(visuals::TEXT_HUD).empty());
        config::cfg.kind_languages.clear();
        visuals::keep_texts();
        CHECK(shown("Melee_Fists_N") == fists_pl && visuals::text_originals.empty());
        visuals::steam_language = "Xx";
        config::cfg.subtitle_language = config::STEAM_LANGUAGE;
        visuals::keep_texts();
        CHECK(shown("Shopkeeper_X_DLG01") == L"Hello");
        visuals::started_language = "Xx";
        visuals::keep_texts();
        CHECK(shown("Shopkeeper_X_DLG01") == hello_pl && visuals::subtitle_code().empty());
        config::cfg.subtitle_language.clear();
        visuals::started_language = saved_started, visuals::steam_language = saved_steam;
        {
            static uint64_t buckets[visuals::TEXT_BUCKETS * 3] = {};
            uintptr_t saved_table = std::exchange(visuals::texts_api.table, (uintptr_t)buckets);
            DWORD t = GetTickCount();
            visuals::texts_settled(t);
            CHECK(visuals::texts_settled(t + visuals::TEXTS_SETTLE_MS * 2));
            buckets[7] = 0x1234;
            DWORD later = t + visuals::TEXTS_SETTLE_MS * 2 + 1;
            CHECK(!visuals::texts_settled(later) && !visuals::texts_settled(later + visuals::TEXTS_SETTLE_MS - 1));
            CHECK(visuals::texts_settled(later + visuals::TEXTS_SETTLE_MS));
            buckets[7] = 0x5678;
            visuals::texts_written();
            CHECK(visuals::texts_settled(later + visuals::TEXTS_SETTLE_MS));
            visuals::texts_api.table = saved_table;
            visuals::texts_since = 0;
        }
        visuals::libraries.erase("Xx");
        visuals::texts_api = saved_api;
        visuals::tools_global = saved_global;
        config::cfg = saved_cfg;
    }
    if (FILE* f = fopen(path.c_str(), "w")) {
        fputs("hidden_config=hunter_rage\nhidden_config=survivor_legit\nhidden_config=hunter_rage\n", f);
        fclose(f);
    }
    config::load(path);
    CHECK(config::cfg.hidden_configs.size() == 2 && config::cfg.hidden_configs[1] == "survivor_legit");
    if (FILE* f = fopen(path.c_str(), "w")) {
        fputs("text_language=En\ntext_keep=items,spoken\nfov=9\n", f);
        fclose(f);
    }
    config::load(path);
    CHECK(config::cfg.game_language == "En" && config::cfg.subtitle_language == config::STEAM_LANGUAGE && config::cfg.voice_language.empty());
    CHECK(config::cfg.fov == config::FOV_MAX && config::peek(path, "text_keep") == "items,spoken" && config::peek(path, "missing").empty());
    config::cfg.voice_language = "game", config::cfg.fov = 1.25f;
    config::save(path);
    config::load(path);
    CHECK(config::cfg.game_language == "En" && config::cfg.voice_language == "game" && config::cfg.subtitle_language == "steam" && config::cfg.fov == 1.25f);
    CHECK(config::peek(path, "game_language") == "En" && config::peek(path, "text_language").empty());
    config::cfg.kind_languages = {{"hud", "En"}, {"items", "De"}};
    config::save(path);
    config::cfg.kind_languages.clear();
    config::load(path);
    CHECK(config::cfg.kind_languages.size() == 2 && config::cfg.kind_languages["hud"] == "En" && config::cfg.kind_languages["items"] == "De" && config::cfg.game_language == "En");
    config::cfg.kind_languages.clear();
    {
        std::string log = "--- FaTrainer | Dying Light 2.5 b10, Fatum Version loaded (x)\nold\n--- FaTrainer | Dying Light 2.5 b11, Fatum Version loaded (x) at 1000\nplayed\n"
                          "--- FaTrainer | Dying Light 2.5 b11, Fatum Version loaded (x) at 2000\nnow\n";
        std::string last = crash::previous_session(log);
        CHECK(last.find("played") != std::string::npos && last.find("now") == std::string::npos && crash::session_time(last) == 1000);
        CHECK(crash::crashed_after(last, 1500) && !crash::crashed_after(last, 900) && !crash::crashed_after(last + crash::SHOWN_MARK, 1500));
        CHECK(!crash::crashed_after(crash::previous_session("--- FaTrainer old\nx\n--- FaTrainer new at 5\n"), 9) && crash::previous_session("--- FaTrainer one\n").empty());
        CHECK(crash::tail("aaa\nbbb\nccc", 5) == "ccc" && crash::widen("a\nb") == L"a\r\nb");
    }
    CHECK(visuals::language_code("pl") == "Pl" && visuals::styled_like("en", "pl") == "en" && visuals::styled_like("en", "PL") == "EN" && visuals::styled_like("pl", "En") == "Pl");
    config::cfg = config::Config{};
    DeleteFileA(path.c_str());

    static int position_calls = 0;
    auto saved_get = cheats::get_position;
    cheats::get_position = [](uintptr_t, cheats::Vec3* out) { position_calls++; *out = {1, 2, 3}; return out; };
    uint8_t node[0x40] = {}, entity[0x100] = {}, control[0x10] = {};
    *(uintptr_t*)(entity + cheats::ENTITY_NODE) = (uintptr_t)node;
    *(uintptr_t*)(control + cheats::CONTROL_ENTITY) = (uintptr_t)entity;
    cheats::Vec3 at;
    CHECK(cheats::position_of((uintptr_t)control, &at) && at.y == 2 && position_calls == 1);
    *(uintptr_t*)(entity + cheats::ENTITY_NODE) = 0x10;
    CHECK(!cheats::position_of((uintptr_t)control, &at) && std::isnan(at.x) && position_calls == 1);
    *(uintptr_t*)(control + cheats::CONTROL_ENTITY) = 0xdead0000;
    CHECK(!cheats::position_of((uintptr_t)control, &at) && position_calls == 1);
    cheats::get_position = saved_get;

    CHECK(cheats::dfa_fall_speed);
    {
        float fall = 12.0f;
        uintptr_t real = cheats::dfa_fall_speed;
        cheats::dfa_fall_speed = (uintptr_t)&fall;
        cheats::dfa_fall_original = NAN;
        auto* height = cheats::find_tweak("h_dfa_height");
        height->factor = height->max;
        cheats::apply_dfa_fall_speed();
        CHECK(fall == 12.0f && cheats::dfa_fall_original == 12.0f);
        int was_side = cheats::side;
        cheats::hunter_in_match = true;
        cheats::side = cheats::SIDE_SURVIVOR;
        cheats::apply_dfa_fall_speed();
        CHECK(fall == cheats::DFA_FALL_SPEED_AT_MAX);
        cheats::hunter_in_match = false;
        cheats::side = was_side;
        height->factor = 1.0f;
        cheats::apply_dfa_fall_speed();
        CHECK(fall == 12.0f);
        cheats::dfa_fall_speed = real;
    }

    {
        float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        float sx, sy;
        CHECK(cheats::to_screen(m, {0, 0, 5}, 1000, 500, sx, sy) && sx == 500 && sy == 250);
        CHECK(cheats::to_screen(m, {1, 1, 5}, 1000, 500, sx, sy) && sx == 1000 && sy == 0);
        m[15] = -1;
        CHECK(!cheats::to_screen(m, {0, 0, 5}, 1000, 500, sx, sy));

        static float camera_matrix[0xb0 / 4 + 16];
        for (int i = 0; i < 16; i++) camera_matrix[0xb0 / 4 + i] = (float)i;
        static uintptr_t engine_camera_holder[2] = {0, (uintptr_t)camera_matrix};
        static uintptr_t game_object = 0x1234;
        auto real_root = cheats::profile_root;
        auto real_level = cheats::active_level, real_camera = cheats::view_camera;
        cheats::profile_root = (uintptr_t)&game_object;
        cheats::active_level = [](uintptr_t game) -> uintptr_t { return game == 0x1234 ? 0x5678 : 0; };
        cheats::view_camera = [](uintptr_t level) -> uintptr_t { return level == 0x5678 ? (uintptr_t)engine_camera_holder : 0; };
        float got[16];
        CHECK(cheats::view_matrix(got) && got[0] == 0 && got[15] == 15);
        cheats::profile_root = real_root, cheats::active_level = real_level, cheats::view_camera = real_camera;
        std::string engine_path = argv[1];
        engine_path = engine_path.substr(0, engine_path.find_last_of("\\/") + 1) + "engine_x64_rwdi.dll";
        HMODULE engine = LoadLibraryExA(engine_path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
        auto get = cheats::get_position;
        auto set = cheats::set_position;
        auto fint = cheats::field_int, fenum = cheats::field_enum;
        auto fbool = cheats::field_bool;
        auto owner = cheats::request_ownership;
        auto editor = cheats::float_field_editor;
        auto vec3 = cheats::field_vec3;
        auto ray = cheats::raytrace;
        auto terrain = cheats::terrain_height;
        cheats::locate_engine(engine);
        CHECK(engine && cheats::active_level && cheats::view_camera && cheats::get_position);
        CHECK(cheats::field_vec3 && cheats::raytrace && cheats::terrain_height);
        uintptr_t view_internal = game::tail_jump((uintptr_t)GetProcAddress(engine, "?SetColor@IUIElementView@@QEAAXAEBVvec3@@@Z"), visuals::EXPORT_SPAN);
        uintptr_t text_internal = game::tail_jump((uintptr_t)GetProcAddress(engine, "?SetTextColor@IUIText@@QEAAXAEBVvec4@@@Z"), visuals::EXPORT_SPAN);
        CHECK(view_internal && !memcmp((const void*)view_internal, visuals::VIEW_COLOR_PROLOGUE, sizeof visuals::VIEW_COLOR_PROLOGUE));
        CHECK(text_internal && !memcmp((const void*)text_internal, visuals::TEXT_COLOR_PROLOGUE, sizeof visuals::TEXT_COLOR_PROLOGUE));
        auto* speech = (const uint8_t*)GetProcAddress(engine, "?SetSpeechID@IGame@@QEBA_NPEBD_N@Z");
        CHECK(speech && !memcmp(speech, visuals::SPEECH_EXPORT, sizeof visuals::SPEECH_EXPORT));
        for (size_t i = visuals::SPEECH_EXPORT_SPAN; speech && i < visuals::SPEECH_PATCH; i++) CHECK(speech[i] == 0xCC);
        auto* voice_locale = (const uint8_t*)GetProcAddress(engine, "?GetVoiceLocaleID@IGame@@UEAAPEBDXZ");
        CHECK(voice_locale && !memcmp(voice_locale, visuals::VOICE_LOCALE_EXPORT, sizeof visuals::VOICE_LOCALE_EXPORT) && voice_locale[visuals::VOICE_LOCALE_RET] == 0xC3);
        cheats::active_level = real_level, cheats::view_camera = real_camera, cheats::get_position = get, cheats::set_position = set;
        cheats::field_int = fint, cheats::field_enum = fenum, cheats::field_bool = fbool, cheats::request_ownership = owner;
        cheats::float_field_editor = editor;
        cheats::field_vec3 = vec3, cheats::raytrace = ray, cheats::terrain_height = terrain;
    }
    {
        static uint8_t logical[0x800] = {};
        *(uintptr_t*)logical = game::g.vt_logical_player;
        *(int*)(logical + cheats::PLAYER_ROLE) = cheats::ROLE_HUNTER;
        uintptr_t saved = game::rdv<uintptr_t>(w.player + cheats::LOGICAL_PLAYER);
        game::wr<uintptr_t>(w.player + cheats::LOGICAL_PLAYER, (uintptr_t)logical);
        CHECK(cheats::logical_player(w.player) == (uintptr_t)logical);
        *(uintptr_t*)logical = 0;
        CHECK(!cheats::logical_player(w.player));
        game::wr<uintptr_t>(w.player + cheats::LOGICAL_PLAYER, saved);
    }
    {
        uintptr_t uv = w.desc["Flashlight_Superlight"];
        float full[3] = {50, 0, 255}, beam[3] = {32, 0, 64}, intensity = 3.5f;
        game::wr_bytes(uv + 0x180, full, sizeof full);
        game::wr_bytes(uv + 0x18c, full, sizeof full);
        game::wr_bytes(uv + 0x198, beam, sizeof beam);
        game::wr<float>(uv + 0x1bc, intensity);
        float red[3] = {1, 0, 0};
        cheats::apply_uv_light(true, red, 2.0f);
        CHECK(game::rdv<float>(uv + 0x180) == 255 && game::rdv<float>(uv + 0x188) == 0 && game::rdv<float>(uv + 0x198) == 128);
        CHECK(game::rdv<float>(uv + 0x1bc) == 7.0f && cheats::uv_originals.count(uv));
        cheats::apply_uv_light(true, red, 2.0f);
        CHECK(game::rdv<float>(uv + 0x1bc) == 7.0f);
        cheats::apply_uv_light(false, red, 2.0f);
        CHECK(game::rdv<float>(uv + 0x180) == 50 && game::rdv<float>(uv + 0x188) == 255 && game::rdv<float>(uv + 0x1bc) == 3.5f);
    }
    {
        input::press(0x04, 60);
        BYTE keys[256] = {};
        input::add_injected_state(keys);
        CHECK(keys[0x04] == 0x80 && keys[0x05] == 0);
        DIDEVICEOBJECTDATA events[4] = {};
        DWORD count = 1;
        input::add_injected_events((BYTE*)events, sizeof events[0], &count, 4);
        CHECK(count == 2 && events[1].dwOfs == 0x04 && events[1].dwData == 0x80);
        Sleep(80);
        count = 0;
        input::add_injected_events((BYTE*)events, sizeof events[0], &count, 4);
        CHECK(count == 1 && events[0].dwOfs == 0x04 && events[0].dwData == 0 && input::injected.empty());
        memset(keys, 0, sizeof keys);
        input::add_injected_state(keys);
        CHECK(keys[0x04] == 0);
    }
    {
        std::string script = cheat_engine::ATTACH_SCRIPT;
        CHECK(script.find("getAutoAttachList().add(\"DyingLightGame.exe\")") != std::string::npos && script.find("closeCE()") != std::string::npos);
        CHECK(script.find("--") == std::string::npos);
        std::wstring args = cheat_engine::installer_arguments(L"C:\\Users\\a\\AppData\\Local\\FaTrainer\\Cheat Engine");
        CHECK(args == L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /ZBDIST /DIR=\"C:\\Users\\a\\AppData\\Local\\FaTrainer\\Cheat Engine\"");
    }
    {
        auto mode = [](UINT w, UINT h, UINT hz) { DXGI_MODE_DESC m{}; m.Width = w, m.Height = h, m.RefreshRate = {hz, 1}, m.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; return m; };
        auto count = [](const std::vector<DXGI_MODE_DESC>& v, UINT w, UINT h, UINT hz) { return std::count_if(v.begin(), v.end(), [&](const DXGI_MODE_DESC& m) { return m.Width == w && m.Height == h && (!hz || m.RefreshRate.Numerator == hz); }); };
        std::vector<DXGI_MODE_DESC> native = {mode(1920, 1080, 60), mode(2560, 1440, 60), mode(2560, 1440, 144)};
        auto merged = visuals::merged_modes(native);
        CHECK(count(merged, 3840, 2160, 60) == 1 && count(merged, 3840, 2160, 144) == 1 && count(merged, 7680, 4320, 144) == 1);
        CHECK(count(merged, 1920, 1080, 0) == 1 && count(merged, 2560, 1440, 0) == 2);
        CHECK(std::is_sorted(merged.begin(), merged.end(), [](const DXGI_MODE_DESC& a, const DXGI_MODE_DESC& b) { return std::tie(a.Width, a.Height, a.RefreshRate.Numerator) < std::tie(b.Width, b.Height, b.RefreshRate.Numerator); }));
        CHECK(merged[0].Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        CHECK(visuals::merged_modes({}).empty());
        UINT n = 0;
        CHECK(visuals::copy_modes(merged, &n, nullptr) == S_OK && n == merged.size());
        std::vector<DXGI_MODE_DESC> small(4);
        n = 3;
        CHECK(visuals::copy_modes(merged, &n, small.data()) == DXGI_ERROR_MORE_DATA && n == 3 && small[3].Width == 0);
        std::vector<DXGI_MODE_DESC> all(merged.size());
        n = (UINT)all.size();
        CHECK(visuals::copy_modes(merged, &n, all.data()) == S_OK && n == merged.size() && all.back().Width == merged.back().Width);
    }
    {
        auto saved_cfg = config::cfg;
        config::cfg.spit_keys[2] = 'G';
        config::cfg.esp.on = true, config::cfg.esp.box = false, config::cfg.esp.max_distance = 450, config::cfg.esp.hunter[1] = 0.5f;
        config::cfg.uv.on = true, config::cfg.uv.glow = 2.5f, config::cfg.uv.color[0] = 0.25f;
        config::cfg.hidden_configs = {"hunter_rage", "survivor_legit"}, config::cfg.allow_older = true, config::cfg.all_resolutions = true, config::cfg.cheat_engine = true;
        std::string ini = config::profile_dir() + "_test.ini";
        CHECK(config::save(ini));
        config::cfg = config::Config();
        config::load(ini);
        CHECK(config::cfg.spit_keys[2] == 'G' && config::cfg.spit_keys[0] == 0);
        CHECK(config::cfg.hidden_configs.size() == 2 && config::cfg.hidden_configs[1] == "survivor_legit" && config::cfg.allow_older);
        CHECK(config::cfg.esp.on && !config::cfg.esp.box && config::cfg.esp.max_distance == 450 && config::cfg.esp.hunter[1] == 0.5f);
        CHECK(config::cfg.uv.on && config::cfg.uv.glow == 2.5f && config::cfg.uv.color[0] == 0.25f);
        CHECK(config::cfg.all_resolutions && config::peek(ini, "all_resolutions") == "1");
        CHECK(config::cfg.cheat_engine && config::peek(ini, "cheat_engine") == "1");
        DeleteFileA(ini.c_str());
        config::cfg = saved_cfg;
    }

    {
        const int RANGE_OFF = 0x300;
        auto saved_range = game::g.stats[ST_DamageRange];
        game::g.stats[ST_DamageRange].off = RANGE_OFF, game::g.stats[ST_DamageRange].is_float = true;
        uintptr_t camo = game::g.descs["ZZZZZ_Throwable_Camo_Spit"], toxic = game::g.descs["ZZZZZ_Throwable_Toxic_Spit"];
        uintptr_t grenade = game::g.descs["Throwable_SpitGrenade"];
        w.put<float>(camo + RANGE_OFF, 5.0f), w.put<float>(grenade + RANGE_OFF, 5.0f);
        w.put<float>(toxic + cheats::TOXIC_SPLASH_MIN, 1.0f), w.put<float>(toxic + cheats::TOXIC_SPLASH_MAX, 4.0f);
        cheats::find_tweak("z_spit")->factor = 3.0f;
        cheats::tick();
        CHECK(game::rdv<float>(camo + RANGE_OFF) == 5.0f && !cheats::playing_hunter());
        static uint8_t hunter_logical[0x800] = {};
        *(uintptr_t*)hunter_logical = game::g.vt_logical_player;
        *(int*)(hunter_logical + cheats::PLAYER_ROLE) = cheats::ROLE_HUNTER;
        uintptr_t saved_logical = game::rdv<uintptr_t>(w.player + cheats::LOGICAL_PLAYER);
        game::wr<uintptr_t>(w.player + cheats::LOGICAL_PLAYER, (uintptr_t)hunter_logical);
        cheats::tick();
        CHECK(cheats::playing_hunter() && game::rdv<float>(camo + RANGE_OFF) == 15.0f && game::rdv<float>(grenade + RANGE_OFF) == 5.0f);
        CHECK(game::rdv<float>(toxic + cheats::TOXIC_SPLASH_MIN) == 3.0f && game::rdv<float>(toxic + cheats::TOXIC_SPLASH_MAX) == 12.0f);
        cheats::tick();
        CHECK(game::rdv<float>(camo + RANGE_OFF) == 15.0f && game::rdv<float>(toxic + cheats::TOXIC_SPLASH_MAX) == 12.0f);
        cheats::find_tweak("z_spit")->factor = 50.0f;
        cheats::tick();
        CHECK(game::rdv<float>(camo + RANGE_OFF) == 30.0f);
        cheats::all_off();
        cheats::tick();
        CHECK(game::rdv<float>(camo + RANGE_OFF) == 5.0f && game::rdv<float>(toxic + cheats::TOXIC_SPLASH_MIN) == 1.0f &&
              game::rdv<float>(toxic + cheats::TOXIC_SPLASH_MAX) == 4.0f);
        game::wr<uintptr_t>(w.player + cheats::LOGICAL_PLAYER, saved_logical);
        game::g.stats[ST_DamageRange] = saved_range;
        cheats::spit_radius_originals.clear();
    }

    {
        static uint8_t controller[0x400] = {};
        auto saved_controller_vt = std::exchange(cheats::vt_melee_controller, (uintptr_t)&fake_yes);
        *(uintptr_t*)controller = cheats::vt_melee_controller;
        uintptr_t saved_controller = game::rdv<uintptr_t>(cheats::player + cheats::MELEE_CONTROLLER);
        game::wr<uintptr_t>(cheats::player + cheats::MELEE_CONTROLLER, (uintptr_t)controller);
        auto saved_reach = cheats::melee_reach_original;
        cheats::melee_reach_original = [](uintptr_t, uintptr_t) { return 1.7f; };
        DWORD settle = cheats::side_settle_ms;
        cheats::side_settle_ms = 0;
        cheats::find_tweak("h_melee")->factor = 10.0f;
        cheats::tick();
        CHECK(cheats::melee_reach_hook((uintptr_t)controller, 0) == 17.0f && cheats::melee_reach_hook(1, 0) == 1.7f);
        cheats::find_tweak("h_melee")->factor = 1.0f;
        cheats::tick();
        CHECK(cheats::melee_reach_hook((uintptr_t)controller, 0) == 1.7f);
        game::wr<uintptr_t>(cheats::player + cheats::MELEE_CONTROLLER, saved_controller);
        cheats::melee_reach_original = saved_reach;
        cheats::vt_melee_controller = saved_controller_vt;
        cheats::side_settle_ms = settle;
    }

    {
        teleport::Spot s{"Tower roof", {1.5f, -2.25f, 300}}, back;
        std::string level;
        CHECK(teleport::format_spot("Harran", s) == "spot=Harran|Tower roof|1.50,-2.25,300.00");
        CHECK(teleport::parse_spot("spot=Harran|Tower roof|1.50,-2.25,300.00", level, back) && level == "Harran" && back.name == "Tower roof" && back.pos.z == 300.0f);
        CHECK(!teleport::parse_spot("spot=Harran|x|1,2", level, back) && !teleport::parse_spot("spot=Harran||1,2,3", level, back));
        CHECK(teleport::parse_spot("spot=|x|1,2,3", level, back) && level.empty() && back.name == "x");
        CHECK(teleport::format_spot("Harran", {"a|b", {0, 0, 0}}) == "spot=Harran|ab|0.00,0.00,0.00");
        CHECK(!teleport::parse_spot("spot=Harran|x|1,2,1e30", level, back) && !teleport::parse_spot("garbage", level, back));
        CHECK(teleport::next_name({{"Position 1", {}}, {"Position 3", {}}}) == "Position 2" && teleport::next_name({}) == "Position 1");
        CHECK(teleport::distance_text({0, 0, 0}, {0, 0, 840}) == "840 m" && teleport::distance_text({0, 0, 0}, {1400, 0, 0}) == "1.4 km");
        cheats::Vec3 N = teleport::NORTH, E = teleport::EAST;
        CHECK(teleport::compass({0, 0, 0}, {N.x * 10, 0, N.z * 10}) == "north" && teleport::compass({0, 0, 0}, {-N.x * 10, 0, -N.z * 10}) == "south");
        CHECK(teleport::compass({0, 0, 0}, {E.x * 10, 5, E.z * 10}) == "east" && teleport::compass({0, 0, 0}, {-E.x * 10, 0, -E.z * 10}) == "west");
        CHECK(teleport::compass({5, 0, 5}, {5 + (N.x + E.x) * 7, 0, 5 + (N.z + E.z) * 7}) == "north-east" && teleport::compass({0, 0, 0}, {-(N.x + E.x), 0, -(N.z + E.z)}) == "south-west");
        CHECK(teleport::compass({0, 0, 0}, {N.x * 10 - E.x * 3, 0, N.z * 10 - E.z * 3}) == "north" && teleport::compass({0, 0, 0}, {N.x - E.x * 10, 0, N.z - E.z * 10}) == "west");
        std::ofstream("selftest_positions.ini") << "spot=Harran|A|1,2,3\nbroken line\nspot=Harran|B|4,5,6\nspot=Following|C|7,8,9\n";
        CHECK(teleport::load_spots("selftest_positions.ini") && teleport::spots["Harran"].size() == 2 && teleport::spots["Following"].size() == 1);
        CHECK(teleport::save_spots("selftest_positions.ini") && teleport::load_spots("selftest_positions.ini") && teleport::spots["Harran"][1].name == "B");
        remove("selftest_positions.ini");
        char tmp[MAX_PATH];
        GetTempPathA(MAX_PATH, tmp);
        std::string old = std::string(tmp) + "fatrainer_selftest_keys.ini";
        auto kept = config::cfg;
        std::ofstream(old) << "cheat_key=go_saved_position,117\ncheat_key=save_position,116\n";
        config::load(old);
        CHECK(config::cfg.cheat_keys.size() == 2 && config::cfg.cheat_keys["go_last_position"] == 117 && config::cfg.cheat_keys["save_position"] == 116);
        remove(old.c_str());
        config::cfg = kept;
    }

    CHECK(config::profile_name("  PvP: hunter/../x  ") == "PvP hunterx");
    config::delete_profile("selftest profile");
    config::Profile saved{{"god", "uv"}, {{"h_dfa", 7.5f}}};
    CHECK(config::save_profile("selftest profile", saved));
    auto listed = config::list_profiles();
    CHECK(std::find(listed.begin(), listed.end(), "selftest profile") != listed.end());
    config::Profile back;
    CHECK(config::load_profile("selftest profile", back) && back.cheats == saved.cheats && back.tweaks.size() == 1 && back.tweaks[0].second == 7.5f);
    cheats::find("stamina")->on = true;
    cheats::apply_profile(back);
    CHECK(cheats::is_on("god") && cheats::is_on("uv") && !cheats::is_on("stamina") && cheats::find_tweak("h_dfa")->factor == 7.5f);
    CHECK(cheats::current_profile().cheats.size() == 2 && cheats::current_profile().tweaks.size() == 1);
    cheats::all_off();
    CHECK(config::delete_profile("selftest profile") && !config::load_profile("selftest profile", back));
    RemoveDirectoryA(config::profile_dir().c_str());

    {
        auto kept = config::cfg;
        config::cfg = config::Config();
        config::cfg.fov = 1.3f, config::cfg.crosshair.on = true, config::cfg.cheats_on = {"god"};
        config::cfg.rainbow["accent"] = 5, config::cfg.rainbow["uv_light"] = 4;
        CHECK(config::save_visual_config("selftest visuals"));
        config::cfg.fov = 1.0f, config::cfg.crosshair.on = false, config::cfg.rainbow.erase("uv_light");
        CHECK(config::load_visual_config("selftest visuals"));
        CHECK(config::cfg.fov == 1.3f && config::cfg.crosshair.on && config::cfg.rainbow.count("uv_light") && config::cfg.rainbow["accent"] == 5);
        CHECK(config::cfg.cheats_on == std::vector<std::string>{"god"});
        CHECK(!config::load_visual_config("selftest missing") && config::cfg.fov == 1.3f);
        CHECK(config::visual_lines().find("accent") == std::string::npos && config::visual_lines().find("cheat") == std::string::npos);
        config::cfg.rainbow.erase("uv_light"), config::cfg.fov = 1.0f, config::cfg.cheats_on = {"uv"};
        CHECK(config::save_visual_config("selftest second") && config::load_visual_config("selftest visuals") && config::cfg.cheats_on == std::vector<std::string>{"uv"});
        CHECK(!config::rename_visual_config("selftest visuals", "selftest second") && !config::rename_visual_config("selftest absent", "selftest other"));
        CHECK(!config::rename_visual_config("selftest visuals", "  /\\ ") && !config::save_visual_config(" ?? ") && !config::delete_visual_config(" ?? "));
        auto listed_visual = [](const char* name) {
            auto all = config::list_visual_configs();
            return std::find(all.begin(), all.end(), name) != all.end();
        };
        CHECK(listed_visual("selftest second") && listed_visual("selftest visuals") && !listed_visual("selftest second.ini"));
        CHECK(config::rename_visual_config("selftest visuals", "selftest renamed") && !config::load_visual_config("selftest visuals"));
        std::ofstream(config::visual_config_path("selftest hand")) << "rainbow=accent,9\nfov=99\nbroken\nhud=,1,1,1,1\ncrosshair=1\n";
        CHECK(config::load_visual_config("selftest hand") && config::cfg.rainbow["accent"] == 5 && config::cfg.fov == config::FOV_MAX && !config::cfg.crosshair.on);
        CHECK(config::delete_visual_config("selftest hand"));
        std::string ini = config::visual_config_dir() + "_test.ini";
        config::cfg.rainbow["accent"] = 7, config::cfg.rainbow["uv_light"] = 3;
        CHECK(config::save(ini));
        config::cfg.rainbow.clear();
        config::load(ini);
        CHECK(config::cfg.rainbow["accent"] == 7 && config::cfg.rainbow["uv_light"] == 3);
        DeleteFileA(ini.c_str());
        CHECK(config::delete_visual_config("selftest renamed") && config::delete_visual_config("selftest second") && !config::delete_visual_config("selftest second"));
        CHECK(!listed_visual("selftest second") && !listed_visual("selftest visuals") && !listed_visual("selftest renamed"));
        RemoveDirectoryA(config::visual_config_dir().c_str());
        config::cfg = kept;
    }

    {
        auto stock = visuals::callout_set("Colored callouts");
        CHECK(!stock.empty() && stock == visuals::stock_callouts(1) && stock.begin()->second.find("%COLOR(") != std::string::npos);
        auto names = visuals::callout_set_names();
        CHECK(names.size() == 2 && names[0] == "Callouts");
        config::CalloutTexts mine = visuals::callout_set("Callouts");
        mine["HUDMessage_BTZDeath_Killer_Hunter_Highrage"] = "Got %s\\back\nnow";
        CHECK(config::save_callout_set("Callouts", mine));
        CHECK(visuals::callout_set("Callouts") == mine && visuals::callout_set_names().size() == 2);
        CHECK(config::delete_callout_set("Callouts") && visuals::callout_set("Callouts") == visuals::stock_callouts(0));
        CHECK(config::save_callout_set("Selftest set", mine) && config::rename_callout_set("Selftest set", "Selftest two"));
        names = visuals::callout_set_names();
        CHECK(names.size() == 3 && names[2] == "Selftest two" && visuals::callout_set("Selftest two") == mine);
        CHECK(visuals::callouts_used(mine, mine) == (int)mine.size() && visuals::callouts_used({}, mine) == 0);
        CHECK(config::delete_callout_set("Selftest two"));
        RemoveDirectoryA(config::callout_dir().c_str());
    }

    {
        auto file = [](const char* path) {
            std::ifstream in(path, std::ios::binary);
            return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        };
        std::string full = file("xinput1_3.dll"), cosmetic = file("xinput1_3-visuals.dll"), lowered = cosmetic;
        for (char& ch : lowered) ch = (char)tolower((unsigned char)ch);
        CHECK(full.find("FaTrainer-edition:Fatum") != std::string::npos);
        CHECK(!cosmetic.empty() && cosmetic.find("FaTrainer-edition:Visuals") != std::string::npos);
        CHECK(lowered.find("winhttp") == std::string::npos);
        for (const char* cheat : {"one_hit", "Infinite ammo", "God mode", "Show players", "Dodge spit", "getAutoAttachList", "cheat engine:"}) CHECK(cosmetic.find(cheat) == std::string::npos);
        CHECK(full.find("getAutoAttachList") != std::string::npos);
    }
    printf(fails ? "SELFTEST FAILED (%d)\n" : "SELFTEST OK\n", fails);
    return fails != 0;
}
