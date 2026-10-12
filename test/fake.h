#pragma once
#include <cstring>
#include "game.h"

namespace fake {

const int DAMAGE_OFF = 0x120, CONDITION_OFF = 0x140, COLOR_OFF = 0x150, AMMO_OFF = 0x160, RELOAD_OFF = 0x164, DEPLETION_OFF = 0x168, SWING_OFF = 0x16c, MELEE_RANGE_OFF = 0x170;

struct World {
    uint8_t* mem;
    size_t used = 0;
    uintptr_t wallet, backpack, stash, materials, manager, player, stamina, enemy, enemy_health, params, cache, trees;
    std::map<std::string, uintptr_t> desc;

    uintptr_t alloc(size_t n) {
        uintptr_t p = (uintptr_t)mem + used;
        used += (n + 15) & ~(size_t)15;
        return p;
    }
    template <class T> void put(uintptr_t a, T v) { memcpy((void*)a, &v, sizeof v); }

    uintptr_t item(const char* id, int count) {
        uintptr_t it = alloc(0x100);
        put<int>(it + 0x40, count);
        put<uintptr_t>(it + 0x60, desc[id]);
        put<uintptr_t>(it + 0x58, game::g.base + 0x1000);
        return it;
    }
    uintptr_t inventory(uintptr_t vt, int cap, std::vector<std::pair<const char*, int>> items) {
        uintptr_t inv = alloc(0x80), arr = alloc(8 * 64);
        put<uintptr_t>(inv, vt);
        put<uintptr_t>(inv + 0x40, arr);
        put<uint32_t>(inv + 0x48, (uint32_t)items.size());
        put<int>(inv + 0x58, cap);
        for (size_t i = 0; i < items.size(); i++) put<uintptr_t>(arr + i * 8, item(items[i].first, items[i].second));
        return inv;
    }

    void build() {
        mem = (uint8_t*)VirtualAlloc(nullptr, 64 << 20, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        uintptr_t names = alloc(ITEM_COUNT * 72), arr = alloc(ITEM_COUNT * 8);
        for (int i = 0; i < ITEM_COUNT; i++) {
            uintptr_t d = alloc(0x600), s = names + i * 72;
            strcpy((char*)s, ITEMS[i].id);
            put<uintptr_t>(d + 0x18, s);
            if (!std::isnan(ITEMS[i].st[ST_Damage])) put<float>(d + DAMAGE_OFF, ITEMS[i].st[ST_Damage]);
            if (!std::isnan(ITEMS[i].st[ST_Condition])) put<int>(d + CONDITION_OFF, (int)ITEMS[i].st[ST_Condition]);
            if (!std::isnan(ITEMS[i].st[ST_Color])) put<int>(d + COLOR_OFF, (int)ITEMS[i].st[ST_Color]);
            if (!std::isnan(ITEMS[i].st[ST_AmmoCount])) put<int>(d + AMMO_OFF, (int)ITEMS[i].st[ST_AmmoCount]);
            if (!std::isnan(ITEMS[i].st[ST_ReloadTime])) put<float>(d + RELOAD_OFF, ITEMS[i].st[ST_ReloadTime]);
            if (!std::isnan(ITEMS[i].st[ST_DepletionTime])) put<float>(d + DEPLETION_OFF, ITEMS[i].st[ST_DepletionTime]);
            if (!std::isnan(ITEMS[i].st[ST_SwingSpeed])) put<float>(d + SWING_OFF, ITEMS[i].st[ST_SwingSpeed]);
            if (!std::isnan(ITEMS[i].st[ST_DamageRange])) put<float>(d + MELEE_RANGE_OFF, ITEMS[i].st[ST_DamageRange]);
            put<uintptr_t>(arr + i * 8, d);
            desc[ITEMS[i].id] = d;
        }
        manager = alloc(0x80);
        put<uintptr_t>(manager, game::g.vt_manager);
        put<uintptr_t>(manager + 0x50, arr);
        put<uint32_t>(manager + 0x58, ITEM_COUNT);
        wallet = alloc(0x80);
        put<uintptr_t>(wallet, game::g.vt_money);
        put<int>(wallet + 0x40, 15855);
        backpack = inventory(game::g.vt_inv[0], 14, {{"Throwable_ZaidFlare", 9}, {"Firearm_PistolAGen", 1},
                                                     {"Throwable_ThrowingAxeAGen", 30}, {"Throwable_Molotov", 7},
                                                     {"Melee_MacheteAGen", 1}, {"Special_Hook", 1}});
        stash = inventory(game::g.vt_inv[0], -1, {{"Melee_WrenchARusty", 1}, {"Melee_KnifeDGen", 1}, {"Throwable_Btz_Flare", 8}});
        inventory(game::g.vt_inv[0], -1, {});
        player = alloc(0x3000);
        put<uintptr_t>(player, game::g.vt_player);
        put<uintptr_t>(player + 0x8f8, game::g.vt_human);
        put<float>(player + 0x125c, 87);
        uintptr_t container = alloc(0x100), holder = alloc(0x10), table = alloc(8 * 1100);
        params = alloc(16 * 1100);
        put<uintptr_t>(player + 0xe58, container);
        put<uintptr_t>(container + 0xd0, holder);
        put<uintptr_t>(holder, table);
        for (int i = 0; i < 1100; i++) put<uintptr_t>(table + (i + 1) * 8, params + i * 16);
        uintptr_t provider = alloc(0x40);
        cache = alloc(40 * 1100);
        put<uintptr_t>(player + 0x9c0, provider);
        put<uintptr_t>(provider + 0x28, cache);
        trees = alloc(0x20 * 8);
        put<uintptr_t>(container + 0x40, trees);
        put<uint16_t>(trees + 2 * 0x20 + 0x14, 7);
        put<uint16_t>(trees + 2 * 0x20 + 0x16, 25);
        stamina = alloc(0x40);
        put<float>(stamina + 0x10, 40);
        put<float>(stamina + 0x14, 100);
        put<uintptr_t>(player + 0x1340, stamina);
        enemy = alloc(0x100);
        put<uintptr_t>(enemy, game::g.vt_human);
        enemy_health = alloc(0x100);
        put<uintptr_t>(enemy_health, game::g.vt_health[0]);
        put<uintptr_t>(enemy_health + 0x40, enemy);
        put<float>(enemy_health + 0x78, 250);
        materials = inventory(game::g.vt_inv[1], -1, {{"Craft_Gauze", 48}, {"Craft_Alcohol", 21}, {"Craft_MetalScrap", 81},
                                                      {"Medkit_HealthPackLarge", 20}, {"LockpickItem", 28}});
    }
};

}
