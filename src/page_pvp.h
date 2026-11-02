#pragma once

inline void spit_keys_card() {
    begin_card("spit_keys", "SPIT KEYS");
    note("Your own key for each spit type, while the menu is closed.");
    for (int i = 0; i < config::SPIT_KEYS; i++) {
        ImGui::PushID(i);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(SPIT_NAMES[i]);
        beside_label(S(220), S(248));
        int& vk = config::cfg.spit_keys[i];
        if (key_button("spit", vk)) {
            if (vk) take_key_from_others(vk, "spit");
            save_config();
        }
        ImGui::SameLine(0, S(12));
        label((std::string("game key ") + SPIT_GAME_KEY_NAMES[i]).c_str());
        ImGui::PopID();
    }
    capture_note();
    end_card();
}

inline const char* SURVIVOR_RANKS[] = {"Prey", "Casualty", "Endangered", "Underdog", "Runner", "Contender",
                                       "Challenger", "Fighter", "Dominant", "Ruthless", "Indomitable", "Ultimate Survivor"};
inline const char* HUNTER_RANKS[] = {"Walker", "Runner", "Biter", "Bolter", "Stalker", "Beast",
                                     "Mauler", "Juggernaut", "Widow Maker", "Carnivore", "Hunter", "Apex Predator"};

inline void rank_row(const char* label, bool zombie, const char* const* names, bool stacked) {
    int points = cheats::pvp_rank(zombie);
    if (points < 0) return;
    int title = cheats::rank_title(points);
    ImGui::PushID(label);
    ImGui::TableNextRow(0, S(44));
    ImGui::BeginDisabled(zombie ? cheats::hunter_is_someone_else() : cheats::playing_hunter());
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    if (!stacked) ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(names[title]);
    ImGui::SameLine(0, S(6));
    ImGui::TextDisabled("%s pts", thousands(points).c_str());
    if (!stacked) ImGui::TableNextColumn();
    struct Step { const char* text; int target; };
    bool first = true;
    for (Step st : {Step{"Lowest", 0}, Step{"-1", title - 1}, Step{"+1", title + 1}, Step{"Highest", cheats::RANK_TITLES}}) {
        float w = S(st.text[0] == '-' || st.text[0] == '+' ? 56 : 84);
        if (!first) same_line_if_room(w, S(6));
        first = false;
        if (ImGui::Button(st.text, {w, 0})) {
            int target = st.target;
            on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_pvp_title(zombie, target); });
        }
    }
    ImGui::EndDisabled();
    ImGui::PopID();
}

inline void rank_card(bool zombie) {
    if (cheats::pvp_rank(zombie) < 0) return;
    begin_card(zombie ? "rank_hunter" : "rank_survivor", "RANK");
    note(zombie ? "Your Be The Zombie rank as the Night Hunter. Grayed out while another player is the Night Hunter. The game may change it after your next Be The Zombie match."
                : "Your Be The Zombie rank as a survivor. Grayed out while you play the Night Hunter. The game may change it after your next Be The Zombie match.");
    ImGui::Dummy({0, S(4)});
    bool stacked = ImGui::GetContentRegionAvail().x < S(130 + 210 + 310);
    if (ImGui::BeginTable("ranks", stacked ? 1 : 3, ImGuiTableFlags_SizingFixedFit)) {
        if (stacked) ImGui::TableSetupColumn("all", ImGuiTableColumnFlags_WidthStretch);
        else {
            ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, S(130));
            ImGui::TableSetupColumn("rank", ImGuiTableColumnFlags_WidthFixed, S(210));
            ImGui::TableSetupColumn("buttons", ImGuiTableColumnFlags_WidthStretch);
        }
        if (zombie) rank_row("Night Hunter", true, HUNTER_RANKS, stacked);
        else rank_row("Survivor", false, SURVIVOR_RANKS, stacked);
        ImGui::EndTable();
    }
    end_card();
}

inline const char* EVADE_DODGE_LOCK[] = {"Dodge", "Lock"};
inline const char* EVADE_UV_LOCK[] = {"Aim + UV", "Lock"};
inline const char* REACTION_TIMES[] = {"Human", "Instant"};

inline void evade_row(const char* cheat, int attack) {
    cheat_switch(cheat);
    int& method = config::cfg.pvp.evade[attack];
    bool pounce = attack == config::EVADE_POUNCE;
    int shown = method == config::EVADE_LOCK ? 1 : 0;
    ImGui::PushID(cheat);
    if (segmented("method", pounce ? EVADE_UV_LOCK : EVADE_DODGE_LOCK, 2, shown, std::min(S(300), ImGui::GetContentRegionAvail().x))) {
        method = shown ? config::EVADE_LOCK : pounce ? config::EVADE_AIM_UV : config::EVADE_DODGE;
        save_config();
    }
    ImGui::PopID();
    ImGui::Dummy({0, S(8)});
}

inline void evade_card() {
    begin_card("evade", "AUTO-EVADE");
    note("With Lock the hunter's game still shows the attack landing.");
    evade_row("evade_tackle", config::EVADE_TACKLE);
    evade_row("evade_pounce", config::EVADE_POUNCE);
    evade_row("evade_pound", config::EVADE_POUND);
    evade_row("evade_spit", config::EVADE_SPIT);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Reaction time");
    ImGui::SameLine(0, S(16));
    int instant = config::cfg.pvp.instant ? 1 : 0;
    if (segmented("reaction", REACTION_TIMES, 2, instant, std::min(S(240), ImGui::GetContentRegionAvail().x))) config::cfg.pvp.instant = instant, save_config();
    end_card();
}

inline const char* KILL_TARGETS[] = {"All", "Nearest", "Picked"};

inline void kill_card(bool hunter) {
    begin_card("autokill", hunter ? "AUTO-KILL SURVIVORS" : "AUTO-KILL THE NIGHT HUNTER");
    note("Throws the target onto the nearest spikes, up to 250 m from it. Needs spikes near the target.");
    if (accent_button("Kill once", {ImGui::GetContentRegionAvail().x, S(38)})) kill_once_now();
    pin_menu("kill_once");
    cheat_switch("auto_kill");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Delay after respawn");
    ImGui::SameLine(0, S(16));
    ImGui::SetNextItemWidth(std::min(S(260), ImGui::GetContentRegionAvail().x));
    float& delay = config::cfg.pvp.kill_delay;
    if (ImGui::SliderFloat("##killdelay", &delay, 0, 20, delay <= 0 ? "Instant" : "%.1f s")) delay = std::clamp(delay, 0.0f, 20.0f);
    if (ImGui::IsItemDeactivatedAfterEdit()) save_config();
    if (hunter) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Target");
        ImGui::SameLine(0, S(16));
        if (segmented("killtarget", KILL_TARGETS, 3, config::cfg.pvp.kill_target, std::min(S(300), ImGui::GetContentRegionAvail().x))) save_config();
        if (config::cfg.pvp.kill_target == config::TARGET_PICKED && !pvp::picked) note("Pick a player on the Lobby page.");
    }
    end_card();
}

inline void zombie_page() {
    begin_card("hunter", "HUNTER");
    cheat_switch("god", "Hunter god mode");
    cheat_switch("z_energy");
    cheat_switch("anti_afk");
    ImGui::Dummy({0, S(4)});
    tweak_slider(*cheats::find_tweak("damage_taken"));
    tweak_sliders(cheats::G_HUNTER);
    end_card();
    begin_card("abilities", "ABILITIES");
    cheat_switch("z_cooldowns");
    cheat_switch("z_spits");
    cheat_switch("z_pound_hits");
    cheat_switch("z_camo");
    end_card();
    begin_card("reach", "ATTACK REACH");
    tweak_sliders(cheats::G_ZOMBIE);
    end_card();
    kill_card(true);
    begin_card("defense", "DEFENSE");
    cheat_switch("undropkickable");
    cheat_switch("no_dfa");
    cheat_switch("unspikeable");
    end_card();
    begin_card("spikes", "SPIKES");
    tweak_sliders(cheats::G_SPIKES);
    end_card();
    spit_keys_card();
    rank_card(true);
}

inline void survivor_page() {
    begin_card("auto", "AUTO CHEATS");
    cheat_switch("auto_flare");
    cheat_switch("one_hit_hunter");
    cheat_switch("anti_afk");
    end_card();
    kill_card(false);
    size_t left = cheats::living_nests().size();
    begin_card("nests", left ? ("NESTS - " + std::to_string(left) + " LEFT").c_str() : "NESTS");
    if (accent_button("Destroy all nests", {ImGui::GetContentRegionAvail().x, S(38)})) destroy_nests_now();
    pin_menu("nests_once");
    cheat_switch("nests_loop");
    end_card();
    begin_card("reach", "REACH AND THROWABLES");
    tweak_sliders(cheats::G_HUMAN);
    end_card();
    begin_card("spikes", "SPIKES");
    cheat_switch("unspikeable");
    tweak_sliders(cheats::G_SPIKES);
    end_card();
    evade_card();
    rank_card(false);
}
