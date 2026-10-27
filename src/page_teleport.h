#pragma once

inline void save_spot(const std::string& typed) {
    on_game_thread([typed] {
        std::lock_guard<std::mutex> l(game::mx);
        cheats::Vec3 at = cheats::player_position();
        if (!std::isfinite(at.x)) return toast("Your position could not be read");
        auto& list = teleport::spots[teleport::level_name()];
        teleport::Spot s{config::profile_name(typed), at};
        if (s.name.empty()) s.name = teleport::next_name(list);
        list.push_back(s);
        toast(teleport::save_spots(teleport::spots_path()) ? "Saved " + s.name : "fatrainer_positions.ini could not be written");
    });
}

inline void save_position() { save_spot(""); }

inline void go_last_position() {
    on_game_thread([] {
        std::lock_guard<std::mutex> l(game::mx);
        auto found = teleport::spots.find(teleport::level_name());
        if (found == teleport::spots.end() || found->second.empty()) return toast("No saved position on this map");
        cheats::teleport(found->second.back().pos);
        toast("Back at " + found->second.back().name);
    });
}

inline void teleport_page() {
    static char typed[41] = "", rename_to[41] = "";
    static int renaming = -1;
    static std::string last_level;
    std::string level = teleport::level_name();
    if (level != last_level) last_level = level, renaming = -1;
    cheats::Vec3 me = cheats::player_position();
    bool here = std::isfinite(me.x);
    std::string title = "SAVED POSITIONS";
    if (!level.empty()) title += " - ";
    for (char ch : level) title += ch == '_' ? ' ' : (char)toupper((unsigned char)ch);
    begin_card("spots", title.c_str());
    float button = S(110), gap = S(8);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - button - gap);
    bool enter = ImGui::InputTextWithHint("##spot_name", "Name this spot", typed, sizeof typed, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine(0, gap);
    ImGui::BeginDisabled(!here);
    if (accent_button("Save here", {button, 0}) || (enter && here)) save_spot(typed), typed[0] = 0;
    ImGui::EndDisabled();
    auto& list = teleport::spots[level];
    for (int i = 0; i < (int)list.size(); i++) {
        ImGui::PushID(i);
        ImGui::Dummy({0, S(4)});
        float avail = ImGui::GetContentRegionAvail().x, x = ImGui::GetCursorPosX(), y = ImGui::GetCursorPosY();
        if (renaming == i) {
            ImGui::SetNextItemWidth(avail - button * 2 - gap * 2);
            if (!ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere();
            bool done = ImGui::InputText("##rename", rename_to, sizeof rename_to, ImGuiInputTextFlags_EnterReturnsTrue);
            std::string clean = config::profile_name(rename_to);
            ImGui::SameLine(0, gap);
            ImGui::BeginDisabled(clean.empty());
            if (accent_button("Rename", {button, 0}) || (done && !clean.empty())) {
                list[i].name = clean;
                renaming = -1;
                if (!teleport::save_spots(teleport::spots_path())) toast("fatrainer_positions.ini could not be written");
            }
            ImGui::EndDisabled();
            ImGui::SameLine(0, gap);
            if (ImGui::Button("Cancel", {button, 0})) renaming = -1;
            ImGui::PopID();
            continue;
        }
        ImGui::BeginGroup();
        if (!here) ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(list[i].name.c_str());
        if (here) label(teleport::distance_text(me, list[i].pos).c_str());
        ImGui::EndGroup();
        float h = ImGui::GetItemRectSize().y, small = S(90);
        ImGui::SameLine();
        ImGui::SetCursorPos({x + avail - small * 3 - gap * 2, y + std::max(0.0f, (h - ImGui::GetFrameHeight()) / 2)});
        ImGui::BeginDisabled(!here);
        if (accent_button("Go", {small, 0})) go_to(list[i].pos);
        ImGui::EndDisabled();
        ImGui::SameLine(0, gap);
        if (ImGui::Button("Rename", {small, 0})) {
            renaming = i;
            snprintf(rename_to, sizeof rename_to, "%s", list[i].name.c_str());
        }
        ImGui::SameLine(0, gap);
        bool gone = ImGui::Button("Delete", {small, 0});
        ImGui::PopID();
        if (gone) {
            list.erase(list.begin() + i);
            renaming = -1;
            if (!teleport::save_spots(teleport::spots_path())) toast("fatrainer_positions.ini could not be written");
            break;
        }
    }
    ImGui::Dummy({0, S(4)});
    note("Each map keeps its own list. Names take letters, numbers, spaces, - and _.");
    end_card();
}
