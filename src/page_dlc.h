#pragma once

inline void dlc_page() {
    const float DIMMED = 0.55f;
    dlc::Mode mode = dlc::mode();
    if (mode == dlc::MODE_NONE) note("Load Harran Prison or The Bozak Horde.");
    else {
        ImGui::PushFont(f_small);
        ImGui::TextColored(V(T.accent), "You are in %s", mode == dlc::MODE_PRISON ? "Harran Prison" : "The Bozak Horde");
        ImGui::PopFont();
    }
    ImGui::Dummy({0, S(6)});
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * (mode == dlc::MODE_PRISON ? 1.0f : DIMMED));
    begin_card("prison", "HARRAN PRISON");
    cheat_switch("prison_pause");
    ImGui::Dummy({0, S(4)});
    kill_all_button();
    end_card();
    ImGui::PopStyleVar();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * (mode == dlc::MODE_BOZAK ? 1.0f : DIMMED));
    begin_card("bozak", "THE BOZAK HORDE");
    note("Load The Bozak Horde.");
    end_card();
    ImGui::PopStyleVar();
}
