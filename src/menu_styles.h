#pragma once

inline void page_icon(ImDrawList* dl, const char* id, ImVec2 c, float s, ImU32 col) {
    const float PI = 3.14159265f;
    float u = s / 2, t = std::max(1.2f, s * 0.09f);
    auto P = [&](float x, float y) { return ImVec2{c.x + x * u, c.y + y * u}; };
    auto line = [&](float x0, float y0, float x1, float y1) { dl->AddLine(P(x0, y0), P(x1, y1), col, t); };
    auto ring = [&](float x, float y, float r) { dl->AddCircle(P(x, y), r * u, col, 0, t); };
    auto disc = [&](float x, float y, float r) { dl->AddCircleFilled(P(x, y), r * u, col); };
    auto box = [&](float x0, float y0, float x1, float y1, float r) { dl->AddRect(P(x0, y0), P(x1, y1), col, r * u, 0, t); };
    auto arc = [&](float x, float y, float r, float a0, float a1) {
        dl->PathArcTo(P(x, y), r * u, a0, a1);
        dl->PathStroke(col, 0, t);
    };
    auto poly = [&](std::initializer_list<ImVec2> points, bool closed) {
        for (ImVec2 p : points) dl->PathLineTo(P(p.x, p.y));
        dl->PathStroke(col, closed ? ImDrawFlags_Closed : 0, t);
    };
    if (!strcmp(id, "player")) {
        ring(0, -0.42f, 0.36f);
        arc(0, 0.95f, 0.78f, PI, 2 * PI);
    } else if (!strcmp(id, "combat")) {
        ring(0, 0, 0.6f), disc(0, 0, 0.13f);
        line(0, -0.95f, 0, -0.35f), line(0, 0.35f, 0, 0.95f), line(-0.95f, 0, -0.35f, 0), line(0.35f, 0, 0.95f, 0);
    } else if (!strcmp(id, "skills")) {
        poly({{-0.7f, 0.05f}, {0, -0.6f}, {0.7f, 0.05f}}, false);
        poly({{-0.7f, 0.7f}, {0, 0.05f}, {0.7f, 0.7f}}, false);
    } else if (!strcmp(id, "teleport")) {
        dl->PathArcTo(P(0, -0.3f), 0.55f * u, 2.66f, 6.77f);
        dl->PathLineTo(P(0, 0.92f));
        dl->PathStroke(col, ImDrawFlags_Closed, t);
        disc(0, -0.3f, 0.16f);
    } else if (!strcmp(id, "survivor")) {
        poly({{-0.72f, -0.68f}, {0, -0.95f}, {0.72f, -0.68f}, {0.72f, 0.08f}, {0, 0.95f}, {-0.72f, 0.08f}}, true);
    } else if (!strcmp(id, "zombie")) {
        dl->AddBezierQuadratic(P(-0.8f, -0.45f), P(-0.55f, 0.3f), P(-0.15f, 0.85f), col, t * 1.3f);
        dl->AddBezierQuadratic(P(-0.3f, -0.9f), P(-0.02f, -0.05f), P(0.35f, 0.65f), col, t * 1.3f);
        dl->AddBezierQuadratic(P(0.25f, -0.85f), P(0.5f, -0.25f), P(0.8f, 0.25f), col, t * 1.3f);
    } else if (!strcmp(id, "dlc")) {
        line(-0.6f, -0.92f, -0.6f, 0.95f);
        poly({{-0.6f, -0.85f}, {0.78f, -0.5f}, {-0.6f, -0.12f}}, true);
    } else if (!strcmp(id, "visuals")) {
        dl->AddBezierQuadratic(P(-0.98f, 0), P(0, -1.0f), P(0.98f, 0), col, t);
        dl->AddBezierQuadratic(P(-0.98f, 0), P(0, 1.0f), P(0.98f, 0), col, t);
        ring(0, 0, 0.3f), disc(0, 0, 0.12f);
    } else if (!strcmp(id, "texts")) {
        box(-0.88f, -0.78f, 0.88f, 0.42f, 0.25f);
        poly({{-0.42f, 0.42f}, {-0.58f, 0.92f}, {0.05f, 0.42f}}, false);
        line(-0.5f, -0.35f, 0.5f, -0.35f), line(-0.5f, 0.02f, 0.18f, 0.02f);
    } else if (!strcmp(id, "cash")) {
        box(-0.95f, -0.55f, 0.95f, 0.55f, 0.12f);
        ring(0, 0, 0.27f), disc(-0.62f, 0, 0.08f), disc(0.62f, 0, 0.08f);
    } else if (!strcmp(id, "backpack")) {
        box(-0.68f, -0.45f, 0.68f, 0.95f, 0.3f);
        arc(0, -0.45f, 0.34f, PI, 2 * PI);
        box(-0.38f, 0.28f, 0.38f, 0.68f, 0.08f);
    } else if (!strcmp(id, "stash")) {
        box(-0.9f, -0.55f, 0.9f, 0.85f, 0.12f);
        line(-0.9f, -0.08f, 0.9f, -0.08f);
        dl->AddRectFilled(P(-0.15f, -0.25f), P(0.15f, 0.15f), col, 0.06f * u);
    } else if (!strcmp(id, "materials")) {
        for (int k = 0; k < 6; k++) dl->PathLineTo(P(0.9f * cosf(PI / 6 + k * PI / 3), 0.9f * sinf(PI / 6 + k * PI / 3)));
        dl->PathStroke(col, ImDrawFlags_Closed, t);
        ring(0, 0, 0.34f);
    } else if (!strcmp(id, "tools")) {
        dl->AddLine(P(-0.8f, 0.8f), P(0.16f, -0.16f), col, t * 1.6f);
        arc(0.45f, -0.45f, 0.4f, -PI / 4 + 0.85f, -PI / 4 + 2 * PI - 0.85f);
    } else if (!strcmp(id, "give")) {
        box(-0.85f, -0.85f, 0.85f, 0.85f, 0.3f);
        line(0, -0.45f, 0, 0.45f), line(-0.45f, 0, 0.45f, 0);
    } else if (!strcmp(id, "configs")) {
        const float rows[][2] = {{-0.6f, -0.35f}, {0, 0.4f}, {0.6f, -0.1f}};
        for (auto& r : rows) line(-0.9f, r[0], 0.9f, r[0]), disc(r[1], r[0], 0.2f);
    } else if (!strcmp(id, "keys")) {
        box(-0.95f, -0.6f, 0.95f, 0.6f, 0.22f);
        disc(-0.5f, -0.18f, 0.09f), disc(0, -0.18f, 0.09f), disc(0.5f, -0.18f, 0.09f);
        line(-0.45f, 0.25f, 0.45f, 0.25f);
    } else if (!strcmp(id, "settings")) {
        for (int k = 0; k < 8; k++) dl->AddLine(P(0.5f * cosf(k * PI / 4), 0.5f * sinf(k * PI / 4)), P(0.92f * cosf(k * PI / 4), 0.92f * sinf(k * PI / 4)), col, t * 2.2f);
        ring(0, 0, 0.56f), ring(0, 0, 0.2f);
    } else if (!strcmp(id, "all_off")) {
        arc(0, 0.08f, 0.75f, -PI / 2 + 0.75f, 3 * PI / 2 - 0.75f);
        line(0, -0.95f, 0, -0.12f);
    } else if (!strcmp(id, "refresh")) {
        float a1 = 3 * PI / 2 - 0.35f, r = 0.68f;
        arc(0, 0, r, -PI / 2 + 0.6f, a1);
        ImVec2 e = P(r * cosf(a1), r * sinf(a1)), dir{-sinf(a1), cosf(a1)}, n{cosf(a1), sinf(a1)};
        dl->AddTriangleFilled({e.x + dir.x * 0.34f * u, e.y + dir.y * 0.34f * u}, {e.x + n.x * 0.26f * u, e.y + n.y * 0.26f * u},
                              {e.x - n.x * 0.26f * u, e.y - n.y * 0.26f * u}, col);
    }
}

inline bool icon_button(const char* id, const char* tip, ImVec2 size, bool active, bool marked, ImU32 tint = 0) {
    ImGui::PushID(id);
    ImVec2 at = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton("icon", size);
    bool hovered = ImGui::IsItemHovered();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand), ImGui::SetTooltip("%s", tip);
    float hover = animate(ImGui::GetID("h"), hovered && !active ? 1.0f : 0.0f), on = animate(ImGui::GetID("a"), active ? 1.0f : 0.0f);
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 end{at.x + size.x, at.y + size.y};
    if (hover > 0.01f) dl->AddRectFilled(at, end, C(IM_COL32(255, 255, 255, 255), 0.05f * hover), R(9));
    if (on > 0.01f) {
        dl->AddRectFilled(at, end, C(T.accent, 0.12f * on), R(9));
        dl->AddRectFilled({at.x, at.y + size.y * 0.26f}, {at.x + S(3), at.y + size.y * 0.74f}, C(T.accent, on), S(2));
    }
    ImU32 col = ImGui::GetItemFlags() & ImGuiItemFlags_Disabled ? T.muted : tint ? tint : brand::mix(brand::mix(T.muted, T.soft, hover), T.text, on);
    page_icon(dl, id, {at.x + size.x / 2, at.y + size.y / 2}, S(18), C(col));
    if (marked) dl->AddCircleFilled({end.x - S(8), at.y + S(8)}, S(3), C(T.accent));
    ImGui::PopID();
    return clicked;
}

inline int status_buttons_count() {
#if FATRAINER_VISUALS
    return 2;
#else
    return cheats::active_count() ? 3 : 2;
#endif
}

inline void status_buttons(ImVec2 size, bool row) {
    auto next = [&] {
        if (row) ImGui::SameLine(0, S(4));
    };
#if !FATRAINER_VISUALS
    if (int active = cheats::active_count()) {
        std::string tip = "Turn all off (" + std::to_string(active) + ")";
        if (icon_button("all_off", tip.c_str(), size, false, false, T.accent)) cheats::all_off(), save_config();
        next();
    }
#endif
    ImU32 sc;
    const char* st = game_status(sc);
    ImVec2 at = ImGui::GetCursorScreenPos(), dot{at.x + size.x / 2, at.y + size.y / 2};
    ImGui::Dummy(size);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", st);
    float period = game::g.scanning ? 0.9f : 2.2f, pulse = motion() > 0 ? fmodf(now(), period) / period : 1;
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddCircle(dot, S(4) + S(6) * pulse, C(sc, 0.45f * (1 - pulse)), 0, S(1.2f));
    dl->AddCircleFilled(dot, S(4), C(sc));
    next();
    ImGui::BeginDisabled(game::g.scanning);
    if (icon_button("refresh", "Refresh", size, false, false)) request_refresh();
    ImGui::EndDisabled();
}

inline void rail_nav() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, V(T.side, config::cfg.look.opacity));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {S(8), D(16)});
    ImGui::BeginChild("rail", {S(56), 0}, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    ImGui::GetWindowDrawList()->AddLine({wp.x + ws.x - 1, wp.y}, {wp.x + ws.x - 1, wp.y + ws.y}, C(T.line_soft));
    float h = D(36), gap = S(2);
    ImVec2 button{ImGui::GetContentRegionAvail().x, h};
    float footer = status_buttons_count() * (h + ImGui::GetStyle().ItemSpacing.y) + D(6);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0, 0, 0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, S(3));
    ImGui::BeginChild("rail_pages", {0, ImGui::GetContentRegionAvail().y - footer});
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, gap});
    const char* section = nullptr;
    for (const Page* pg : ordered_pages()) {
        if (section && strcmp(section, pg->section)) {
            ImVec2 s = ImGui::GetCursorScreenPos();
            float w = ImGui::GetContentRegionAvail().x;
            ImGui::GetWindowDrawList()->AddLine({s.x + w * 0.3f, s.y + D(4)}, {s.x + w * 0.7f, s.y + D(4)}, C(T.line), S(1));
            ImGui::Dummy({0, D(8) - gap});
        }
        section = pg->section;
        if (icon_button(pg->id, pg->title, {ImGui::GetContentRegionAvail().x, h}, g_page == pg->id, active_on(*pg) > 0)) open_page(pg->id);
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Dummy({0, D(6) - ImGui::GetStyle().ItemSpacing.y});
    status_buttons(button, false);
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

inline bool tab_item(const char* text, bool on, bool marked, bool category, bool first) {
    ImFont* font = category || on ? f_strong : f_body;
    float fs = category ? font_size(f_small) : ImGui::GetFontSize(), tw = font->CalcTextSizeA(fs, FLT_MAX, 0, text).x;
    ImVec2 pad{category ? S(14) : S(12), category ? D(7) : D(6)};
    ImVec2 size{tw + pad.x * 2 + (marked ? S(12) : 0), fs + pad.y * 2};
    if (!first) same_line_if_room(size.x, S(6));
    ImGui::PushID(text);
    ImVec2 at = ImGui::GetCursorScreenPos(), end{at.x + size.x, at.y + size.y};
    bool clicked = ImGui::InvisibleButton("tab", size);
    bool hovered = ImGui::IsItemHovered();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    float hover = animate(ImGui::GetID("h"), hovered && !on ? 1.0f : 0.0f), lit = animate(ImGui::GetID("a"), on ? 1.0f : 0.0f);
    auto* dl = ImGui::GetWindowDrawList();
    ImU32 text_col;
    if (category) {
        dl->AddRectFilled(at, end, C(brand::mix(brand::mix(T.raised, T.raised_hi, hover), T.accent, lit)), R(8));
        text_col = brand::mix(brand::mix(T.soft, T.text, hover), T.accent_ink, lit);
    } else {
        if (hover > 0.01f) dl->AddRectFilled(at, end, C(IM_COL32(255, 255, 255, 255), 0.04f * hover), R(8));
        if (lit > 0.01f) dl->AddRectFilled({at.x + pad.x, end.y - S(2)}, {at.x + pad.x + tw * lit, end.y}, C(T.accent), S(1));
        text_col = brand::mix(brand::mix(T.muted, T.soft, hover), T.text, lit);
    }
    dl->AddText(font, fs, {at.x + pad.x, at.y + pad.y}, C(text_col), text);
    if (marked) dl->AddCircleFilled({at.x + pad.x + tw + S(9), at.y + size.y / 2}, S(3), C(category && on ? T.accent_ink : T.accent));
    ImGui::PopID();
    return clicked;
}

inline void tab_nav() {
    std::vector<const Page*> pages = ordered_pages();
    const Page* current = page_by_id(g_page);
    std::vector<const char*> sections;
    for (const Page* pg : pages)
        if (sections.empty() || strcmp(sections.back(), pg->section)) sections.push_back(pg->section);
    const char* chosen = current ? current->section : sections.empty() ? "" : sections[0];
    auto marked = [&](const char* section) {
        for (const Page* pg : pages)
            if (!strcmp(pg->section, section) && active_on(*pg) > 0) return true;
        return false;
    };
    ImGui::PushStyleColor(ImGuiCol_ChildBg, V(T.side, config::cfg.look.opacity));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {D(24), D(14)});
    ImGui::BeginChild("tabs", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {S(6), D(8)});
    for (size_t i = 0; i < sections.size(); i++)
        if (tab_item(sections[i], !strcmp(sections[i], chosen), marked(sections[i]), true, i == 0))
            for (const Page* pg : pages)
                if (!strcmp(pg->section, sections[i])) {
                    open_page(pg->id);
                    break;
                }
    ImVec2 button{S(34), ImGui::GetItemRectSize().y};
    int n = status_buttons_count();
    float width = n * button.x + (n - 1) * S(4), right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    if (!sections.empty() && ImGui::GetItemRectMax().x + S(16) + width <= right) ImGui::SameLine(right - ImGui::GetWindowPos().x - width);
    status_buttons(button, true);
    bool first = true;
    for (const Page* pg : pages)
        if (!strcmp(pg->section, chosen)) {
            if (tab_item(pg->title, g_page == pg->id, active_on(*pg) > 0, false, first)) open_page(pg->id);
            first = false;
        }
    ImGui::PopStyleVar();
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    ImGui::GetWindowDrawList()->AddLine({wp.x, wp.y + ws.y - 1}, {wp.x + ws.x, wp.y + ws.y - 1}, C(T.line_soft));
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

inline void dock_window(float shown) {
    ImVec2 d = ImGui::GetIO().DisplaySize;
    float w = std::min(S(420), d.x), slide = (1 - shown) * S(22) * motion();
    float x = config::cfg.look.open_at == config::OPEN_LEFT ? -slide : d.x - w + slide;
    ImGui::SetNextWindowPos({x, 0});
    ImGui::SetNextWindowSize({w, d.y});
    if (config::cfg.dim > 0) ImGui::GetBackgroundDrawList()->AddRectFilled({x, 0}, {x + w, d.y}, IM_COL32(0, 0, 0, (int)(config::cfg.dim * 255 * shown)));
}

#if !FATRAINER_VISUALS
inline ImVec2 g_wheel_at;
inline float g_wheel_shown = 0;

inline void draw_wheel(bool held) {
    float step_in = ImGui::GetIO().DeltaTime * (motion() > 0 ? 8.0f : 1000.0f);
    g_wheel_shown = held ? std::min(1.0f, g_wheel_shown + step_in) : std::max(0.0f, g_wheel_shown - step_in * 1.4f);
    float a = g_wheel_shown;
    if (a <= 0) return;
    const float TURN = 6.28318531f, UP = -TURN / 4;
    std::vector<WheelSlot> slots = wheel_slots();
    int n = (int)slots.size(), pick = cheats::wheel_pick(g_wheel_at.x / config::cfg.scale, g_wheel_at.y / config::cfg.scale, n);
    ImVec2 d = ImGui::GetIO().DisplaySize, c{d.x / 2, d.y / 2};
    float r = S(170) * (0.9f + 0.1f * brand::ease_out(a)), ring = S(64), step = n ? TURN / n : TURN;
    auto* dl = ImGui::GetForegroundDrawList();
    dl->AddCircle(c, r, C(T.ground, 0.88f * a), 96, ring);
    if (pick >= 0) {
        float mid = UP + pick * step;
        dl->PathArcTo(c, r, mid - step / 2, mid + step / 2, 32);
        dl->PathStroke(C(T.accent, 0.28f * a), 0, ring);
    }
    for (int i = 0; i < n; i++) {
        ImVec2 at{c.x + cosf(UP + i * step) * r, c.y + sinf(UP + i * step) * r};
        bool hot = i == pick;
        ImFont* font = hot ? f_strong : f_body;
        float fs = font_size(font), dot = slots[i].on ? S(14) : 0, pad = S(12);
        std::string text = fit_text(font, fs, slots[i].label, S(200));
        ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0, text.c_str());
        ImVec2 p0{at.x - (ts.x + dot) / 2 - pad, at.y - ts.y / 2 - S(7)}, p1{at.x + (ts.x + dot) / 2 + pad, at.y + ts.y / 2 + S(7)};
        dl->AddRectFilled(p0, p1, C(hot ? T.accent : T.raised, a), (p1.y - p0.y) / 2);
        if (!hot) dl->AddRect(p0, p1, C(T.line, a), (p1.y - p0.y) / 2);
        if (slots[i].on) dl->AddCircleFilled({p0.x + pad + S(4), at.y}, S(4), C(hot ? T.accent_ink : T.accent, a));
        dl->AddText(font, fs, {p0.x + pad + dot, at.y - ts.y / 2}, C(hot ? T.accent_ink : T.text, a), text.c_str());
    }
    ImVec2 tip{c.x + g_wheel_at.x, c.y + g_wheel_at.y};
    if (pick >= 0) dl->AddLine(c, tip, C(T.accent, 0.7f * a), S(2));
    dl->AddCircleFilled(c, S(9), C(T.accent, a));
    if (pick >= 0) dl->AddCircleFilled(tip, S(4), C(T.accent, a));
    if (!n) {
        float fs = font_size(f_small);
        ImVec2 ts = f_small->CalcTextSizeA(fs, FLT_MAX, 0, "No pins");
        dl->AddText(f_small, fs, {c.x - ts.x / 2, c.y + S(18)}, C(T.soft, a), "No pins");
    }
}

inline void draw_active_list() {
    std::vector<std::string> lines = cheats::active_lines();
    if (lines.empty()) return;
    ImVec2 d = ImGui::GetIO().DisplaySize;
    float fs = font_size(f_small), pad = S(12), gap = S(5), bar = S(12), margin = S(24), w = 0;
    for (auto& line : lines) w = std::max(w, f_small->CalcTextSizeA(fs, FLT_MAX, 0, line.c_str()).x);
    ImVec2 size{w + bar + pad * 2, lines.size() * (fs + gap) - gap + pad * 2};
    int corner = config::cfg.active_corner;
    ImVec2 p0{corner & 1 ? d.x - margin - size.x : margin, corner & 2 ? d.y - margin - size.y : margin};
    auto* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled(p0, {p0.x + size.x, p0.y + size.y}, C(T.ground, 0.78f), R(8));
    for (size_t i = 0; i < lines.size(); i++) {
        float y = p0.y + pad + i * (fs + gap);
        dl->AddRectFilled({p0.x + pad, y + fs * 0.2f}, {p0.x + pad + S(3), y + fs * 0.85f}, C(T.accent), S(1.5f));
        dl->AddText(f_small, fs, {p0.x + pad + bar, y}, C(T.text), lines[i].c_str());
    }
}
#endif
