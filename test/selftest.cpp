#include <cstdio>
#include "cheats.h"
#include "config.h"
#include "fake.h"
#include "input.h"

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

static float fake_var_float(uintptr_t, uintptr_t name, uintptr_t, uintptr_t) {
    const char* s = *(const char**)name;
    if (!strcmp(s, "f_btz_zombie_grab_range")) return 10;
    if (!strcmp(s, "f_btz_wrestling_kick_angle_max")) return 22;
    if (!strcmp(s, "f_btz_pvp_grab_below_angle_threshold")) return -50;
    return 7;
}
static uintptr_t fake_var_vtable[1100];
static uintptr_t fake_var_object = (uintptr_t)&fake_var_vtable[1];
static uintptr_t fake_var_holder = (uintptr_t)&fake_var_object;

static float read_var(const char* name) {
    auto fn = (cheats::VarFloatFn)cheats::slot((uintptr_t)&fake_var_object, cheats::SLOT_VAR_FLOAT);
    return fn((uintptr_t)&fake_var_object, (uintptr_t)&name, 0, 0);
}

static void __fastcall refuse_add(uintptr_t, void*, int, bool) {}
static uintptr_t fake_cache_base;
static uintptr_t fake_cache_get(uintptr_t, int id) { return fake_cache_base + id * 40 + 8; }
static int fake_level_calls = 0;
static int ownership_requests = 0;
static void fake_request_ownership(uintptr_t repl) { ownership_requests++; *(uint8_t*)(repl + 0x28) = 1; }
static void __fastcall fake_level_from_xp(uintptr_t, int) { fake_level_calls++; }

int main(int argc, char** argv) {
    HMODULE m = LoadLibraryExA(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    CHECK(m);
    if (!m) return 1;
    CHECK(game::resolve_classes((uintptr_t)m));
    CHECK(game::g.vt_manager && game::g.vt_inv[1] && game::g.vt_inv[2]);
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
    CHECK(cheats::var_root > (uintptr_t)m && cheats::var_root < (uintptr_t)m + 0x4000000);
    CHECK(cheats::level_from_xp_fn && cheats::cache_get_fn);
    uintptr_t lockpick_code = cheats::lockpick_patch;
    CHECK(cheats::profile_root > (uintptr_t)m && cheats::profile_root < (uintptr_t)m + 0x4000000);
    CHECK(cheats::forced_damage_jump == (uintptr_t)m + 0xbae6c3);
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
    cheats::var_root = (uintptr_t)&fake_var_holder;
    cheats::local_player_root = cheats::params_root = 0;
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
    CHECK(game::rdv<float>(cached("RopeEnergyRegenTime") + 0x10) == 0.01f && (game::rdv<uint8_t>(cached("RopeEnergyRegenTime") + 0x20) & 1));
    CHECK(game::rdv<uintptr_t>(cached("RopeEnergyRegenTime") + 8) == cheats::vt_param_float && cheats::vt_param_float);
    CHECK(game::rdv<uint8_t>(cached("CanUseHook") + 0x10) == 1 && game::rdv<uintptr_t>(cached("CanUseHook") + 8) == cheats::vt_param_bool);
    CHECK(game::rdv<float>(param("AirKickRangeMul")) == 1.0f);
    CHECK(game::g.stats[ST_AmmoCount].off == fake::AMMO_OFF && game::g.stats[ST_DepletionTime].off == fake::DEPLETION_OFF);
    CHECK(game::rdv<int>(w.desc["Firearm_PistolAGen"] + fake::AMMO_OFF) == 999 && game::rdv<float>(w.desc["Firearm_PistolAGen"] + fake::RELOAD_OFF) == 0.05f);
    CHECK(game::rdv<float>(w.desc["Flashlight_Superlight"] + fake::DEPLETION_OFF) == 1e7f);
    CHECK(cheats::tree_level(2) == 7 && cheats::tree_max(2) == 25 && cheats::tree_max(5) == 0);
    game::wr<uint32_t>(w.trees + 2 * 0x20 + 0xc, 1000);
    game::wr<uint32_t>(w.trees + 2 * 0x20 + 0x10, 500);
    cheats::level_up_with_xp(2);
    CHECK(game::rdv<uint32_t>(w.trees + 2 * 0x20 + 8) == 1500 && fake_level_calls == 1);
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
    CHECK(game::rdv<uint8_t>(param("CanUseHook")) == 1 && game::rdv<float>(param("ZombieSpitLightDisableAmmoRegenTime3v1")) != 0.05f);
    CHECK(fake_rules[0] == 1);
    for (auto& c : cheats::CHEATS) c.on = false;
    cheats::find_tweak("h_dropkick")->factor = 1.0f;
    cheats::tick();
    CHECK(game::rdv<float>(param("AirKickRangeMul")) == 1.0f && game::rdv<uint64_t>(cached("AirKickRangeMul")) != ~0ull);
    for (auto* n : {"RopeEnergyRegenTime", "CanUseHook", "AirKickRangeMul"}) cheats::cache_get_hook(my_provider, cheats::param_ids[n] + 1);
    CHECK(game::rdv<uint64_t>(cached("AirKickRangeMul")) != ~0ull);
    CHECK(game::rdv<uint64_t>(cached("RopeEnergyRegenTime")) == ~0ull && game::rdv<uint64_t>(cached("CanUseHook")) == ~0ull);
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
    cheats::find("durability")->on = false;
    cheats::tick();
    game::wr<float>(blade + cheats::ITEM_CONDITION, 12.5f);
    cheats::tick();
    CHECK(game::rdv<float>(blade + cheats::ITEM_CONDITION) == 12.5f && cheats::condition_floor.empty());
    CHECK(game::rdv<uintptr_t>(w.player + 0x8f8) == game::g.vt_human);
    printf("cheats: %s\n", cheats::describe().c_str());

    CHECK(read_var("f_btz_zombie_grab_range") == 10);
    cheats::find_tweak("z_pounce")->factor = 5.5f;
    cheats::find_tweak("h_dropkick")->factor = 10.0f;
    cheats::tick();
    CHECK(fake_var_object != (uintptr_t)&cheats::var_vtable[1]);
    CHECK(read_var("f_btz_zombie_grab_range") == 10 && read_var("f_btz_wrestling_kick_angle_max") == 22 && cheats::active_count() == 2);
    cheats::find_tweak("h_dfa")->factor = 10.0f;
    cheats::find_tweak("h_dfa_pull")->factor = 10.0f;
    CHECK(read_var("f_btz_jump_attack_range") == 7);
    cheats::all_off();
    CHECK(read_var("f_btz_jump_attack_range") == 7);
    CHECK(read_var("f_btz_zombie_grab_range") == 10 && read_var("f_btz_wrestling_kick_angle_max") == 22 && cheats::active_count() == 0);

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
    char tmp[MAX_PATH];
    GetTempPathA(MAX_PATH, tmp);
    std::string path = std::string(tmp) + "fatrainer_selftest.ini";
    CHECK(config::save(path));
    config::cfg = config::Config();
    config::load(path);
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
        std::string ini = config::profile_dir() + "_test.ini";
        CHECK(config::save(ini));
        config::cfg = config::Config();
        config::load(ini);
        CHECK(config::cfg.spit_keys[2] == 'G' && config::cfg.spit_keys[0] == 0);
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
        CHECK(game::rdv<float>(camo + RANGE_OFF) == 5.0f && game::rdv<float>(toxic + cheats::TOXIC_SPLASH_MAX) == 4.0f);
        cheats::all_off();
        cheats::tick();
        CHECK(game::rdv<float>(camo + RANGE_OFF) == 5.0f && game::rdv<float>(toxic + cheats::TOXIC_SPLASH_MIN) == 1.0f &&
              game::rdv<float>(toxic + cheats::TOXIC_SPLASH_MAX) == 4.0f);
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
    CHECK(cheats::is_on("god") && cheats::is_on("uv") && !cheats::is_on("stamina") && cheats::find_tweak("h_dfa")->factor == 1.0f);
    CHECK(cheats::current_profile().cheats.size() == 2 && cheats::current_profile().tweaks.empty());
    cheats::all_off();
    CHECK(config::delete_profile("selftest profile") && !config::load_profile("selftest profile", back));
    RemoveDirectoryA(config::profile_dir().c_str());

    printf(fails ? "SELFTEST FAILED (%d)\n" : "SELFTEST OK\n", fails);
    return fails != 0;
}
