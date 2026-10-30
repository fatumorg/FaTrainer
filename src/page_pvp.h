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

inline void zombie_page() {
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
    tweak_sliders(cheats::G_ZOMBIE);
    end_card();
    begin_card("spikes", "SPIKES");
    cheat_switch("unspikeable");
    tweak_sliders(cheats::G_SPIKES);
    end_card();
    spit_keys_card();
    rank_card(true);
}

inline void survivor_page() {
    begin_card("auto", "AUTO CHEATS");
    cheat_switch("dodge_spit");
    cheat_switch("auto_flare");
    cheat_switch("one_hit_hunter");
    end_card();
    begin_card("reach", "REACH AND THROWABLES");
    tweak_sliders(cheats::G_HUMAN);
    end_card();
    begin_card("spikes", "SPIKES");
    cheat_switch("unspikeable");
    tweak_sliders(cheats::G_SPIKES);
    end_card();
    rank_card(false);
}
