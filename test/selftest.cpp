#include <cstdio>
#include "cheats.h"
#include "color.h"
#include "config.h"
#include "fake.h"
#include "input.h"
#include "version.h"
#include "../installer/logic.h"

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
    CHECK(cheats::local_player_root && cheats::params_root && cheats::unlimited_ammo_flag && cheats::var_root);
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
        cheats::cache_get_hook(0x5000, id);
        CHECK(game::rdv<float>(cached("RopeEnergyRegenTime") + 0x10) == 7.0f && cheats::reads_of("RopeEnergyRegenTime") == 1);
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
    CHECK(game::rdv<float>(param("RangeMeleeMul")) == 2.0f && game::rdv<float>(param("BestTargetMeleeRange")) == 3.0f);
    CHECK(game::rdv<float>(param("MeeleAttackSpeedMul")) == 1.0f && game::rdv<float>(param("BluntAttackSpeedMul")) == 0.25f);
    CHECK(game::g.stats[ST_AmmoCount].off == fake::AMMO_OFF && game::g.stats[ST_DepletionTime].off == fake::DEPLETION_OFF);
    CHECK(game::rdv<int>(w.desc["Firearm_PistolAGen"] + fake::AMMO_OFF) == 999 && game::rdv<float>(w.desc["Firearm_PistolAGen"] + fake::RELOAD_OFF) == 0.05f);
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
    CHECK(read_var("f_btz_wrestling_kick_angle_max") == 180);
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
        {
            std::vector<cheats::Section> run = {{0, cheats::SENSOR_EVAC, 0}, {0, cheats::SENSOR_STAGE, 3}, {0, 0, -1}, {0, cheats::SENSOR_START, 0},
                                                {0, cheats::SENSOR_REWARD, 0}, {0, cheats::SENSOR_STAGE, 1}};
            std::sort(run.begin(), run.end(), [](auto& a, auto& b) { return cheats::run_order(a) < cheats::run_order(b); });
            CHECK(run[0].type == cheats::SENSOR_START && run[1].stage == 1 && run[2].stage == 3 && run[3].type == cheats::SENSOR_REWARD);
            CHECK(run[4].type == cheats::SENSOR_EVAC && run[5].type == 0);
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
        cheats::route.clear();
        cheats::record_route({0, 0, 0}, 1000);
        cheats::record_route({1, 0, 0}, 1100);
        cheats::record_route({3, 0, 0}, 1200);
        cheats::record_route({3.5f, 0, 0}, 3000);
        CHECK(cheats::route.size() == 2 && !cheats::route[1].stop);
        cheats::record_route({3.5f, 0, 0}, 5300);
        cheats::record_route({10, 0, 0}, 5400);
        CHECK(cheats::route.size() == 3 && cheats::route[1].stop && !cheats::route[2].stop);
        std::string file = "selftest_route.txt";
        CHECK(cheats::save_route(file));
        cheats::route.clear();
        CHECK(cheats::load_route(file) && cheats::route.size() == 3 && cheats::route[1].stop && cheats::route[2].pos.x == 10);
        DeleteFileA(file.c_str());
        cheats::route_at = 0;
        cheats::route_mode = cheats::ROUTE_REPLAYING;
        cheats::replay_route();
        cheats::replay_route();
        CHECK(cheats::route_mode == cheats::ROUTE_PAUSED && cheats::route_at == 2);
        cheats::route_mode = cheats::ROUTE_REPLAYING;
        cheats::replay_route();
        cheats::replay_route();
        CHECK(cheats::route_mode == cheats::ROUTE_IDLE && cheats::route_at == 0);
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
    {
        float rgb[3] = {1.0f, 0.5f, 0.0f};
        CHECK(color::to_hex(rgb) == "#FF8000");
        CHECK(color::to_byte(0.5f) == 128 && color::to_byte(-1) == 0 && color::to_byte(2) == 255);
        float parsed[3] = {0, 0, 0};
        CHECK(color::from_hex("#3293c5", parsed) && color::to_byte(parsed[0]) == 0x32 && color::to_byte(parsed[2]) == 0xc5);
        CHECK(color::from_hex("FF0000", parsed) && parsed[0] == 1.0f && parsed[1] == 0.0f);
        float kept[3] = {0.1f, 0.2f, 0.3f};
        CHECK(!color::from_hex("#12", kept) && !color::from_hex("zzzzzz", kept) && !color::from_hex("#1234567", kept) && kept[0] == 0.1f);
    }
    {
        auto kept = config::cfg;
        config::cfg = config::Config();
        config::cfg.hud_tint = {true, {0.5f, 0.25f, 1.0f}};
        config::cfg.hud_parts["stamina"] = {true, {1.0f, 0.0f, 0.0f}};
        config::cfg.texts["btz_pounce_interrupted"] = "Nope, line\none, a=b \\ done";
        config::cfg.texts["HUDMessage_BTZDeath_Killer"] = "Zabi\xc5\x82e\xc5\x9b %s";
        CHECK(config::save(path));
        config::cfg = config::Config();
        config::load(path);
        CHECK(config::cfg.hud_tint.on && config::cfg.hud_tint.color[1] == 0.25f);
        CHECK(config::cfg.hud_parts.size() == 1 && config::cfg.hud_parts["stamina"].on && config::cfg.hud_parts["stamina"].color[0] == 1.0f);
        CHECK(config::cfg.texts["btz_pounce_interrupted"] == "Nope, line\none, a=b \\ done");
        CHECK(config::cfg.texts["HUDMessage_BTZDeath_Killer"] == "Zabi\xc5\x82e\xc5\x9b %s");
        config::cfg = kept;
    }
    if (FILE* f = fopen(path.c_str(), "w")) {
        fputs("hidden_config=hunter_rage\nhidden_config=survivor_legit\nhidden_config=hunter_rage\n", f);
        fclose(f);
    }
    config::load(path);
    CHECK(config::cfg.hidden_configs.size() == 2 && config::cfg.hidden_configs[1] == "survivor_legit");
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
        CHECK(fall == cheats::DFA_FALL_SPEED_AT_MAX && cheats::dfa_fall_original == 12.0f);
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
        cheats::locate_engine(engine);
        CHECK(engine && cheats::active_level && cheats::view_camera && cheats::get_position);
        cheats::active_level = real_level, cheats::view_camera = real_camera, cheats::get_position = get, cheats::set_position = set;
        cheats::field_int = fint, cheats::field_enum = fenum, cheats::field_bool = fbool, cheats::request_ownership = owner;
        cheats::float_field_editor = editor;
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
        auto saved_cfg = config::cfg;
        config::cfg.spit_keys[2] = 'G';
        config::cfg.esp.on = true, config::cfg.esp.box = false, config::cfg.esp.max_distance = 450, config::cfg.esp.hunter[1] = 0.5f;
        config::cfg.uv.on = true, config::cfg.uv.glow = 2.5f, config::cfg.uv.color[0] = 0.25f;
        config::cfg.hidden_configs = {"hunter_rage", "survivor_legit"}, config::cfg.allow_older = true;
        std::string ini = config::profile_dir() + "_test.ini";
        CHECK(config::save(ini));
        config::cfg = config::Config();
        config::load(ini);
        CHECK(config::cfg.spit_keys[2] == 'G' && config::cfg.spit_keys[0] == 0);
        CHECK(config::cfg.hidden_configs.size() == 2 && config::cfg.hidden_configs[1] == "survivor_legit" && config::cfg.allow_older);
        CHECK(config::cfg.esp.on && !config::cfg.esp.box && config::cfg.esp.max_distance == 450 && config::cfg.esp.hunter[1] == 0.5f);
        CHECK(config::cfg.uv.on && config::cfg.uv.glow == 2.5f && config::cfg.uv.color[0] == 0.25f);
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

    printf(fails ? "SELFTEST FAILED (%d)\n" : "SELFTEST OK\n", fails);
    return fails != 0;
}
