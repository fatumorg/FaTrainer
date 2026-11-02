#pragma once

inline uintptr_t g_lobby_pick = 0;

inline void lobby_actions(const lobby::Row& r) {
    begin_card("lobby_actions", "ACTIONS");
    ImGui::TextUnformatted(r.who.name.c_str());
    float w = std::min(S(200), ImGui::GetContentRegionAvail().x);
    if (accent_button("Kill", {w, S(36)})) {
        uintptr_t p = r.who.p;
        on_game_thread([p] { pvp::impale(p); });
        toast("Throwing " + r.who.name + " onto the nearest spikes");
    }
    same_line_if_room(w, S(8));
    if (ImGui::Button("Go to", {w, S(36)})) {
        cheats::Vec3 at{};
        uintptr_t p = r.who.p;
        if (game::g.player_control >= 0 && cheats::position_of(p + game::g.player_control, &at))
            on_game_thread([at] { std::lock_guard<std::mutex> l(game::mx); cheats::teleport({at.x, at.y + 1.0f, at.z}); }), toast("Teleported to " + r.who.name);
        else toast("Cannot see where " + r.who.name + " is");
    }
    same_line_if_room(w, S(8));
    bool picked = pvp::picked == r.who.p;
    if (ImGui::Button(picked ? "Picked for Auto-Kill" : "Pick for Auto-Kill", {w, S(36)})) pvp::picked = picked ? 0 : r.who.p;
    note("Kill throws the player onto the nearest spikes, up to 250 m from him. Unspikeable on his side stops it.");
    end_card();
}

inline void session_members_card() {
    auto members = announce::members();
    if (members.empty()) return;
    begin_card("lobby_session", "IN YOUR SESSION");
    int n = 1;
    for (auto& m : members) {
        ImGui::PushID(m.user);
        std::string name = m.you ? "You" : "Player " + std::to_string(n++);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name.c_str());
        if (m.owner) ImGui::SameLine(0, S(8)), ImGui::TextDisabled("host");
        if (m.fatrainer) caps(announce::badge(m.peer).c_str(), T.accent);
        ImGui::PopID();
    }
    end_card();
}

inline void lobby_page() {
    cheats::kill_tracking = true;
    session_members_card();
    auto list = lobby::rows();
    begin_card("lobby_players", "PLAYERS");
    if (list.empty()) {
        note("Play with other players to see them here.");
        end_card();
        return;
    }
    if (ImGui::BeginTable("players", 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("PLAYER", ImGuiTableColumnFlags_WidthStretch, 3);
        ImGui::TableSetupColumn("RANK", ImGuiTableColumnFlags_WidthStretch, 1);
        ImGui::TableSetupColumn("HEALTH", ImGuiTableColumnFlags_WidthStretch, 1);
        ImGui::TableSetupColumn("DISTANCE", ImGuiTableColumnFlags_WidthStretch, 1);
        ImGui::TableHeadersRow();
        for (auto& r : list) {
            ImGui::PushID((void*)r.who.p);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Selectable(r.who.name.c_str(), g_lobby_pick == r.who.p, ImGuiSelectableFlags_SpanAllColumns)) g_lobby_pick = r.who.p;
            caps(lobby::role(r.who).c_str());
            if (r.fatrainer) caps(announce::badge(r.peer).c_str(), T.accent);
            if (!r.findings.empty()) {
                caps("PROBABLY CHEATING", IM_COL32(235, 87, 87, 255));
                for (auto& f : r.findings) note(detect::describe(f).c_str());
            }
            ImGui::TableNextColumn();
            if (r.rank >= 0) ImGui::Text("%d", r.rank);
            ImGui::TableNextColumn();
            ImGui::Text("%.0f / %.0f", r.health, r.max_health);
            ImGui::TableNextColumn();
            ImGui::Text("%.0f m", r.distance);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    end_card();
    for (auto& r : list)
        if (r.who.p == g_lobby_pick) lobby_actions(r);
}
