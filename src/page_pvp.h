#pragma once

inline void spit_keys_card() {
    begin_card("spit_keys", "SPIT KEYS");
    note("Pick your own key for each spit type. The trainer presses the game's key for it (1 to 4) when you press yours while the menu is closed.");
    for (int i = 0; i < config::SPIT_KEYS; i++) {
        ImGui::PushID(i);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(SPIT_NAMES[i]);
        ImGui::SameLine(S(220));
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

inline void rank_row(const char* label, bool zombie, const char* const* names) {
    int points = cheats::pvp_rank(zombie);
    if (points < 0) return;
    int title = cheats::rank_title(points);
    ImGui::PushID(label);
    ImGui::TableNextRow(0, S(44));
    ImGui::BeginDisabled(zombie ? cheats::hunter_is_someone_else() : cheats::playing_hunter());
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(names[title]);
    ImGui::SameLine(0, S(6));
    ImGui::TextDisabled("%s pts", thousands(points).c_str());
    ImGui::TableNextColumn();
    struct Step { const char* text; int target; };
    for (Step st : {Step{"Lowest", 0}, Step{"-1", title - 1}, Step{"+1", title + 1}, Step{"Highest", cheats::RANK_TITLES}}) {
        if (ImGui::Button(st.text, {S(st.text[0] == '-' || st.text[0] == '+' ? 56 : 84), 0})) {
            int target = st.target;
            on_game_thread([=] { std::lock_guard<std::mutex> l(game::mx); cheats::set_pvp_title(zombie, target); });
        }
        ImGui::SameLine(0, S(6));
    }
    ImGui::NewLine();
    ImGui::EndDisabled();
    ImGui::PopID();
}

inline void rank_card(bool zombie) {
    if (cheats::pvp_rank(zombie) < 0) return;
    begin_card(zombie ? "rank_hunter" : "rank_survivor", "RANK");
    note(zombie ? "Your Be The Zombie rank as the Night Hunter. Grayed out while another player is the Night Hunter. The game may adjust it again after your next Be The Zombie match."
                : "Your Be The Zombie rank as a survivor. Grayed out while you play the Night Hunter. The game may adjust it again after your next Be The Zombie match.");
    ImGui::Dummy({0, S(4)});
    if (ImGui::BeginTable("ranks", 3, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, S(130));
        ImGui::TableSetupColumn("rank", ImGuiTableColumnFlags_WidthFixed, S(210));
        ImGui::TableSetupColumn("buttons", ImGuiTableColumnFlags_WidthStretch);
        if (zombie) rank_row("Night Hunter", true, HUNTER_RANKS);
        else rank_row("Survivor", false, SURVIVOR_RANKS);
        ImGui::EndTable();
    }
    end_card();
}

inline void zombie_page() {
    begin_card("about");
    note("For Be The Zombie matches, where you play the Night Hunter and invade another player's game. Turn these on before or during a match.");
    end_card();
    begin_card("hunter", "HUNTER");
    cheat_switch("god", "Hunter god mode");
    cheat_switch("z_energy");
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
    note("Slide right to reach further. At Max the pounce also hits targets that are not in front of you. Each player's game decides its own attacks.");
    tweak_sliders(cheats::G_ZOMBIE);
    end_card();
    spit_keys_card();
    rank_card(true);
}

inline void survivor_page() {
    begin_card("auto", "AUTO CHEATS");
    cheat_switch("dodge_spit");
    cheat_switch("one_hit_hunter");
    end_card();
    begin_card("reach", "REACH AND THROWABLES");
    note("Slide right to reach further. At Max the dropkick and death from above also hit targets that are not in front of you. Each player's game decides its own attacks.");
    tweak_sliders(cheats::G_HUMAN);
    end_card();
    rank_card(false);
}
