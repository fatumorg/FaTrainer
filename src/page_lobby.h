#pragma once

inline uintptr_t g_lobby_pick = 0;
const ImU32 HUNTER_RED = IM_COL32(235, 87, 87, 255);

// The first letter of a name, upper case when it is ASCII.
inline std::string initial(const std::string& name) {
    if (name.empty()) return "?";
    unsigned int cp;
    int n = ImTextCharFromUtf8(&cp, name.c_str(), nullptr);
    std::string s = name.substr(0, n);
    if (n == 1) s[0] = (char)toupper((unsigned char)s[0]);
    return s;
}

inline void avatar(ImDrawList* dl, ImVec2 c, float r, const std::string& name, ImU32 color) {
    dl->AddCircleFilled(c, r, C(color, 0.16f));
    dl->AddCircle(c, r, C(color, 0.55f), 0, S(1.5f));
    std::string ch = initial(name);
    float fs = font_size(f_strong), w = f_strong->CalcTextSizeA(fs, FLT_MAX, 0, ch.c_str()).x;
    dl->AddText(f_strong, fs, {c.x - w / 2, c.y - fs / 2}, C(color), ch.c_str());
}

// A small rounded tag; returns its width.
inline float tag(ImDrawList* dl, ImVec2 at, const char* text, ImU32 color) {
    float fs = font_size(f_label), w = f_label->CalcTextSizeA(fs, FLT_MAX, 0, text).x + S(12), h = fs + S(6);
    dl->AddRectFilled(at, {at.x + w, at.y + h}, C(color, 0.14f), h / 2);
    dl->AddText(f_label, fs, {at.x + S(6), at.y + S(3)}, C(color), text);
    return w;
}

inline float tag_height() { return font_size(f_label) + S(6); }

// The name with its tags after it, drawn from at; tags are (text, color) pairs.
inline void name_line(ImDrawList* dl, ImVec2 at, const std::string& name, const std::vector<std::pair<std::string, ImU32>>& tags) {
    float fs = font_size(f_strong);
    dl->AddText(f_strong, fs, at, C(T.text), name.c_str());
    float x = at.x + f_strong->CalcTextSizeA(fs, FLT_MAX, 0, name.c_str()).x + S(10);
    for (auto& [text, color] : tags) x += tag(dl, {x, at.y + (fs - tag_height()) / 2}, text.c_str(), color) + S(6);
}

inline void lobby_actions(const lobby::Row& r) {
    ImGui::Indent(S(52));
    for (auto& f : r.findings) note(detect::describe(f).c_str());
    float w = std::min(S(170), (ImGui::GetContentRegionAvail().x - S(16)) / 3);
    if (accent_button("Kill", {w, S(34)})) {
        uintptr_t p = r.who.p;
        on_game_thread([p] { pvp::impale(p); });
        toast("Killing " + r.who.name);
    }
    same_line_if_room(w, S(8));
    if (ImGui::Button("Go to", {w, S(34)})) {
        cheats::Vec3 at{};
        uintptr_t p = r.who.p;
        if (game::g.player_control >= 0 && cheats::position_of(p + game::g.player_control, &at))
            on_game_thread([at] { std::lock_guard<std::mutex> l(game::mx); cheats::teleport({at.x, at.y + 1.0f, at.z}); }), toast("Teleported to " + r.who.name);
        else toast("Cannot see where " + r.who.name + " is");
    }
    same_line_if_room(w, S(8));
    bool picked = pvp::picked == r.who.p;
    if (ImGui::Button(picked ? "Picked for Auto-Kill" : "Pick for Auto-Kill", {w, S(34)})) pvp::picked = picked ? 0 : r.who.p;
    note("Kill impales the player where they stand. Unspikeable on their side stops it.");
    ImGui::Unindent(S(52));
    ImGui::Dummy({0, S(4)});
}

inline void player_row(const lobby::Row& r) {
    ImGui::PushID((void*)r.who.p);
    float w = ImGui::GetContentRegionAvail().x, h = D(62), pad = D(10);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool open = g_lobby_pick == r.who.p;
    if (ImGui::InvisibleButton("row", {w, h})) g_lobby_pick = open ? 0 : r.who.p;
    bool hovered = ImGui::IsItemHovered();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    auto* dl = ImGui::GetWindowDrawList();
    ImU32 role = r.who.hunter ? HUNTER_RED : T.accent;
    float hover = animate(ImGui::GetID("hover"), hovered || open ? 1.0f : 0.0f);
    if (hover > 0.01f) dl->AddRectFilled({p.x - S(8), p.y}, {p.x + w + S(8), p.y + h}, C(IM_COL32(255, 255, 255, 255), 0.035f * hover), R(10));
    if (open) dl->AddRectFilled({p.x - S(8), p.y + S(8)}, {p.x - S(5), p.y + h - S(8)}, C(role), S(2));

    float r_av = S(18);
    avatar(dl, {p.x + r_av + S(4), p.y + h / 2}, r_av, r.who.name, role);
    std::vector<std::pair<std::string, ImU32>> tags;
    if (r.who.host) tags.push_back({"HOST", T.soft});
    if (r.fatrainer) tags.push_back({"FATRAINER " + r.peer.version, T.accent});
    if (!r.findings.empty()) tags.push_back({"PROBABLY CHEATING", HUNTER_RED});
    float x = p.x + r_av * 2 + S(16), fs = font_size(f_strong), small = font_size(f_small);
    name_line(dl, {x, p.y + pad}, r.who.name, tags);
    std::string sub = r.who.hunter ? "Night Hunter" : "Survivor";
    if (r.rank >= 0) sub += "  \xC2\xB7  Rank " + std::to_string(r.rank);
    dl->AddText(f_small, small, {x, p.y + pad + fs + S(4)}, C(role, 0.85f), sub.c_str());

    float bar_w = std::min(S(160), w * 0.28f), right = p.x + w - S(4);
    char dist[32];
    snprintf(dist, sizeof dist, "%.0f m", r.distance);
    float dw = f_strong->CalcTextSizeA(fs, FLT_MAX, 0, dist).x;
    dl->AddText(f_strong, fs, {right - dw, p.y + pad}, C(T.text), dist);
    float t = r.max_health > 0 ? brand::clamp01(r.health / r.max_health) : 0;
    float shown = animate(ImGui::GetID("health"), t, 8);
    ImVec2 b0{right - bar_w, p.y + h - pad - S(6)}, b1{right, b0.y + S(6)};
    dl->AddRectFilled(b0, b1, C(T.press), S(3));
    if (shown > 0.005f) dl->AddRectFilled(b0, {b0.x + bar_w * shown, b1.y}, C(t < 0.3f ? HUNTER_RED : t < 0.6f ? IM_COL32(242, 178, 63, 255) : T.ok), S(3));
    char hp[48];
    snprintf(hp, sizeof hp, "%.0f / %.0f", r.health, r.max_health);
    float hw = f_small->CalcTextSizeA(small, FLT_MAX, 0, hp).x;
    dl->AddText(f_small, small, {b0.x - hw - S(8), b0.y + S(3) - small / 2}, C(T.muted), hp);

    if (open) lobby_actions(r);
    ImGui::PopID();
}

inline void session_members_card() {
    auto members = announce::members();
    if (members.empty()) return;
    begin_card("lobby_session", "IN YOUR SESSION");
    int n = 1;
    for (auto& m : members) {
        ImGui::PushID(m.user);
        std::string name = lobby::member_name(m);
        if (name.empty()) name = m.you ? "You" : "Player " + std::to_string(n);
        if (!m.you) n++;
        float w = ImGui::GetContentRegionAvail().x, h = D(44), r_av = S(14);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy({w, h});
        auto* dl = ImGui::GetWindowDrawList();
        avatar(dl, {p.x + r_av + S(4), p.y + h / 2}, r_av, name, m.you ? T.accent : T.soft);
        std::vector<std::pair<std::string, ImU32>> tags;
        if (m.you) tags.push_back({"YOU", T.accent});
        if (m.owner) tags.push_back({"HOST", T.soft});
        name_line(dl, {p.x + r_av * 2 + S(16), p.y + (h - font_size(f_strong)) / 2}, name, tags);
        std::string status = m.fatrainer ? announce::badge(m.peer) : "No FaTrainer";
        float small = font_size(f_small), sw = f_small->CalcTextSizeA(small, FLT_MAX, 0, status.c_str()).x;
        dl->AddText(f_small, small, {p.x + w - sw - S(4), p.y + (h - small) / 2}, C(m.fatrainer ? T.accent : T.muted), status.c_str());
        ImGui::PopID();
    }
    end_card();
}

inline void lobby_page() {
    cheats::kill_tracking = true;
    session_members_card();
    auto list = lobby::rows();
    begin_card("lobby_players", "PLAYERS");
    if (list.empty()) note("Play with other players to see them here.");
    for (size_t i = 0; i < list.size(); i++) {
        if (i) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddLine({p.x + S(52), p.y}, {p.x + ImGui::GetContentRegionAvail().x, p.y}, C(T.line_soft));
            ImGui::Dummy({0, S(1)});
        }
        player_row(list[i]);
    }
    end_card();
}
