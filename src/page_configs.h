#pragma once

inline std::string summary_list(const std::vector<std::string>& parts, const char* none) {
    if (parts.empty()) return none;
    const size_t SHOWN = 4;
    std::string out;
    for (size_t i = 0; i < parts.size() && i < SHOWN; i++) out += (i ? ", " : "") + parts[i];
    if (parts.size() > SHOWN) out += " and " + std::to_string(parts.size() - SHOWN) + " more";
    return out + ".";
}

inline void tile_frame(ImVec2 a, ImVec2 b, bool on, bool dashed = false) {
    float hover = animate(ImGui::GetID("hover"), ImGui::IsMouseHoveringRect(a, b) ? 1.0f : 0.0f);
    float lit = animate(ImGui::GetID("on"), on ? 1.0f : 0.0f);
    auto* dl = ImGui::GetWindowDrawList();
    ImU32 line = brand::mix(brand::mix(T.line_soft, T.line, hover), T.accent, lit);
    if (dashed) {
        line = brand::mix(T.line, T.accent, hover * 0.7f);
        dl->AddRectFilled(a, b, C(T.frame, 0.4f + 0.6f * hover), R(10));
        for (float x = a.x + S(12); x < b.x - S(12); x += S(10)) {
            dl->AddLine({x, a.y}, {std::min(x + S(5), b.x - S(12)), a.y}, C(line));
            dl->AddLine({x, b.y}, {std::min(x + S(5), b.x - S(12)), b.y}, C(line));
        }
        for (float y = a.y + S(12); y < b.y - S(12); y += S(10)) {
            dl->AddLine({a.x, y}, {a.x, std::min(y + S(5), b.y - S(12))}, C(line));
            dl->AddLine({b.x, y}, {b.x, std::min(y + S(5), b.y - S(12))}, C(line));
        }
        return;
    }
    dl->AddRectFilled(a, b, C(brand::mix(T.frame, T.hover, hover)), R(10));
    if (lit > 0.01f) dl->AddRectFilled(a, b, C(T.accent, 0.08f * lit), R(10));
    dl->AddRect(a, b, C(line), R(10), 0, 1.0f + lit);
}

#if !FATRAINER_VISUALS
inline void apply_preset(const cheats::Preset& p) {
    cheats::apply_profile(cheats::preset_profile(p));
    auto& e = config::cfg.esp;
    bool rage = p.esp == cheats::ESP_RAGE;
    e.on = e.role = e.health_bar = e.distance = true;
    e.box = e.health_text = e.rank = e.rage = e.snaplines = e.allies = rage;
    e.max_distance = rage ? 1000.0f : 300.0f;
    save_config();
    toast(std::string("Loaded config ") + p.role + " " + p.style);
}

inline void preset_tile(const cheats::Preset& p, float w, float h) {
    ImGui::PushID(p.key);
    ImGui::BeginGroup();
    ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
    bool on = cheats::preset_active(p);
    bool rage = p.esp == cheats::ESP_RAGE;
    float hover = animate(ImGui::GetID("hover"), ImGui::IsMouseHoveringRect(a, b) ? 1.0f : 0.0f);
    float lit = animate(ImGui::GetID("on"), on ? 1.0f : 0.0f);
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, b, C(brand::mix(T.frame, T.hover, hover)), R(10));
    if (lit > 0.01f) dl->AddRectFilled(a, b, C(T.accent, 0.08f * lit), R(10));
    dl->AddRect(a, b, C(brand::mix(brand::mix(T.line_soft, T.line, hover), T.accent, lit)), R(10), 0, 1.0f + lit);
    float pad = S(16), fs = font_size(f_label);
    std::string role = p.role;
    std::transform(role.begin(), role.end(), role.begin(), ::toupper);
    brand::spaced_caps(dl, f_label, fs, {a.x + pad, a.y + pad}, role.c_str(), C(T.muted), S(1.4f));
    float big = font_size(f_tile);
    dl->AddText(f_tile, big, {a.x + pad, a.y + pad + fs + S(8)}, C(rage ? T.accent : T.text), p.style);
    float small = font_size(f_small);
    dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + big + S(14)}, C(T.soft), p.summary, nullptr, w - pad * 2);
    float bh = ImGui::GetFrameHeight();
    ImGui::Dummy({w, h});
    ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
    if (on) {
        if (ImGui::Button("Turn off", {S(110), 0})) cheats::all_off(), save_config();
        ImGui::SameLine(0, S(12));
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(V(T.accent), "Active");
    } else if (accent_button("Load", {S(96), 0})) {
        apply_preset(p);
    }
    static std::string confirm_hide;
    bool sure = confirm_hide == p.key;
    ImGui::SameLine(0, 0);
    ImGui::SetCursorScreenPos({b.x - pad - S(62), b.y - pad - bh});
    if (ImGui::Button(sure ? "Sure?" : "Delete", {S(62), 0})) {
        if (sure) {
            config::cfg.hidden_configs.push_back(p.key);
            confirm_hide.clear();
            save_config();
            toast(std::string("Deleted config ") + p.role + " " + p.style);
        } else {
            confirm_hide = p.key;
        }
    }
    ImGui::SetCursorScreenPos({a.x, b.y});
    ImGui::EndGroup();
    ImGui::PopID();
}

struct OwnConfigs { std::vector<std::string> names; std::map<std::string, config::Profile> data; bool loaded = false; };

inline OwnConfigs& own_configs(bool reload = false) {
    static OwnConfigs c;
    if (c.loaded && !reload) return c;
    c.names = config::list_profiles();
    c.data.clear();
    for (auto& n : c.names) config::load_profile(n, c.data[n]);
    c.loaded = true;
    return c;
}

inline std::string profile_summary(const config::Profile& p) {
    std::vector<std::string> parts;
    for (auto& k : p.cheats)
        if (auto* c = cheats::find(k)) parts.push_back(c->label);
    for (auto& [k, v] : p.tweaks)
        if (auto* t = cheats::find_tweak(k)) {
            char b[96];
            snprintf(b, sizeof b, "%s x%.1f", t->label, v);
            parts.push_back(b);
        }
    return summary_list(parts, "Everything off.");
}

inline void own_config_tile(const std::string& name, const config::Profile& p, float w, float h) {
    static std::string renaming, confirm_delete;
    static char rename_to[48] = "";
    ImGui::PushID(name.c_str());
    ImGui::BeginGroup();
    ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
    bool on = cheats::profile_active(p);
    tile_frame(a, b, on);
    auto* dl = ImGui::GetWindowDrawList();
    float pad = S(16), fs = font_size(f_label), big = font_size(f_tile), small = font_size(f_small), bh = ImGui::GetFrameHeight();
    brand::spaced_caps(dl, f_label, fs, {a.x + pad, a.y + pad}, "YOUR CONFIG", C(T.muted), S(1.4f));
    ImGui::Dummy({w, h});
    if (renaming == name) {
        ImGui::SetCursorScreenPos({a.x + pad, a.y + pad + fs + S(8)});
        ImGui::SetNextItemWidth(w - pad * 2);
        if (ImGui::IsWindowAppearing() || !ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere();
        bool enter = ImGui::InputText("##rename", rename_to, sizeof rename_to, ImGuiInputTextFlags_EnterReturnsTrue);
        std::string clean = config::profile_name(rename_to);
        bool taken = clean != name && own_configs().data.count(clean);
        dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + bh + S(16)}, C(taken ? T.bad : T.soft),
                    taken ? "A config with this name already exists." : "Letters, numbers, spaces, - and _.", nullptr, w - pad * 2);
        ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
        ImGui::BeginDisabled(clean.empty() || taken);
        if (accent_button("Rename", {S(110), 0}) || (enter && !clean.empty() && !taken)) {
            if (clean == name || config::rename_profile(name, clean)) toast("Renamed config to " + clean), own_configs(true);
            else toast("Could not rename the config");
            renaming.clear();
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, S(8));
        if (ImGui::Button("Cancel", {S(90), 0})) renaming.clear();
    } else {
        dl->AddText(f_tile, big, {a.x + pad, a.y + pad + fs + S(8)}, C(on ? T.accent : T.text), name.c_str());
        dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + big + S(14)}, C(T.soft), profile_summary(p).c_str(), nullptr, w - pad * 2);
        ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
        if (on) {
            if (ImGui::Button("Turn off", {S(96), 0})) cheats::all_off(), save_config();
        } else if (accent_button("Load", {S(96), 0})) {
            cheats::apply_profile(p);
            save_config();
            toast("Loaded config " + name);
        }
        float gap = S(6), right = b.x - pad;
        bool sure = confirm_delete == name;
        ImGui::SameLine(0, 0);
        ImGui::SetCursorScreenPos({right - S(62) - S(72) - S(56) - gap * 2, b.y - pad - bh});
        if (ImGui::Button("Save", {S(56), 0})) {
            bool ok = config::save_profile(name, cheats::current_profile());
            toast(ok ? "Saved what is on into " + name : std::string("Could not save the config"));
            own_configs(true);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Replace this config with the cheats and sliders that are on");
        ImGui::SameLine(0, gap);
        if (ImGui::Button("Rename", {S(72), 0})) renaming = name, snprintf(rename_to, sizeof rename_to, "%s", name.c_str()), confirm_delete.clear();
        ImGui::SameLine(0, gap);
        if (ImGui::Button(sure ? "Sure?" : "Delete", {S(62), 0})) {
            if (sure) {
                config::delete_profile(name);
                confirm_delete.clear();
                toast("Deleted config " + name);
                own_configs(true);
            } else {
                confirm_delete = name;
            }
        }
    }
    ImGui::SetCursorScreenPos({a.x, b.y});
    ImGui::EndGroup();
    ImGui::PopID();
}

inline void new_config_tile(float w, float h) {
    static bool naming = false;
    static char name[48] = "";
    ImGui::PushID("new_config");
    ImGui::BeginGroup();
    ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
    tile_frame(a, b, naming, !naming);
    auto* dl = ImGui::GetWindowDrawList();
    float pad = S(16), fs = font_size(f_label), big = font_size(f_tile), small = font_size(f_small), bh = ImGui::GetFrameHeight();
    if (!naming) {
        if (ImGui::InvisibleButton("start", {w, h})) naming = true, name[0] = 0;
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        float tw = f_tile->CalcTextSizeA(big, FLT_MAX, 0, "+ New config").x;
        if (tw > w - pad * 2) big *= (w - pad * 2) / tw, tw = w - pad * 2;
        dl->AddText(f_tile, big, {(a.x + b.x - tw) / 2, (a.y + b.y) / 2 - big}, C(T.accent), "+ New config");
        const char* hint = "Saves the cheats and sliders that are on.";
        float hw = f_small->CalcTextSizeA(small, FLT_MAX, 0, hint).x;
        dl->AddText(f_small, small, {(a.x + b.x - hw) / 2, (a.y + b.y) / 2 + S(8)}, C(T.soft), hint);
    } else {
        ImGui::Dummy({w, h});
        brand::spaced_caps(dl, f_label, fs, {a.x + pad, a.y + pad}, "NEW CONFIG", C(T.muted), S(1.4f));
        ImGui::SetCursorScreenPos({a.x + pad, a.y + pad + fs + S(8)});
        ImGui::SetNextItemWidth(w - pad * 2);
        if (!ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere();
        bool enter = ImGui::InputTextWithHint("##name", "Name, e.g. Hunter sweaty", name, sizeof name, ImGuiInputTextFlags_EnterReturnsTrue);
        std::string clean = config::profile_name(name);
        bool taken = config::path_exists(config::profile_path(clean));
        std::string what = taken ? "A config with this name already exists." : profile_summary(cheats::current_profile());
        dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + bh + S(16)}, C(taken ? T.bad : T.soft), what.c_str(), nullptr, w - pad * 2);
        ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
        ImGui::BeginDisabled(clean.empty() || taken);
        if (accent_button("Save config", {S(130), 0}) || (enter && !clean.empty() && !taken)) {
            bool ok = config::save_profile(clean, cheats::current_profile());
            toast(ok ? "Saved config " + clean : std::string("Could not save the config"));
            own_configs(true);
            naming = false;
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, S(8));
        if (ImGui::Button("Cancel", {S(90), 0})) naming = false;
    }
    ImGui::SetCursorScreenPos({a.x, b.y});
    ImGui::EndGroup();
    ImGui::PopID();
}

inline void configs_card() {
    begin_card("configs", "CONFIGS");
    note("Legit stays believable to the other players, Rage holds nothing back. Loading a config turns everything else off; "
         "the built-in ones also set the player ESP. Your configs are saved in fatrainer_configs next to the game.");
    ImGui::Dummy({0, S(4)});
    float gap = S(12), room = ImGui::GetContentRegionAvail().x, h = S(206);
    int cols = room >= S(520) ? 2 : 1, i = 0;
    float w = (room - gap * (cols - 1)) / cols;
    auto place = [&] {
        if (i % cols) ImGui::SameLine(0, gap);
        else if (i) ImGui::Dummy({0, S(2)});
        i++;
    };
    auto& hidden = config::cfg.hidden_configs;
    for (auto& p : cheats::PRESETS)
        if (std::find(hidden.begin(), hidden.end(), p.key) == hidden.end()) place(), preset_tile(p, w, h);
    OwnConfigs& own = own_configs();
    std::vector<std::string> names = own.names;
    for (auto& n : names)
        if (own_configs().data.count(n)) place(), own_config_tile(n, own_configs().data[n], w, h);
    place(), new_config_tile(w, h);
    if (!hidden.empty()) {
        ImGui::Dummy({0, S(6)});
        if (ImGui::Button(("Restore built-in configs (" + std::to_string(hidden.size()) + ")").c_str())) hidden.clear(), save_config();
    }
    end_card();
}

#endif

struct VisualConfigs { std::vector<std::pair<std::string, std::string>> items; bool loaded = false; };

inline std::string visual_summary(const std::string& name) {
    struct Part { const char* key; const char* label; bool keyed; };
    static const Part PARTS[] = {{"esp", "Player ESP", false}, {"uv_light", "UV light", false}, {"hunter_glow", "Night Hunter glow", false},
                                 {"hud_tint", "HUD tint", false}, {"hud", "HUD colors", true}, {"crosshair", "Crosshair", false},
                                 {"survivor_glow", "Survivor glow", true}, {"btz_color", "Night Hunter colors", true}};
    std::vector<std::string> parts;
    auto add = [&](const std::string& label) {
        if (std::find(parts.begin(), parts.end(), label) == parts.end()) parts.push_back(label);
    };
    FILE* f = fopen(config::visual_config_path(name).c_str(), "r");
    if (!f) return summary_list(parts, "The file could not be read.");
    char line[4096];
    while (fgets(line, sizeof line, f)) {
        std::string s = line;
        size_t eq = s.find('=');
        if (eq == std::string::npos) continue;
        std::string k = s.substr(0, eq), v = s.substr(eq + 1);
        for (auto& p : PARTS) {
            if (k != p.key) continue;
            config::HudColor h;
            size_t comma = v.find(',');
            if (p.keyed ? comma != std::string::npos && config::read_hud_color(v.c_str() + comma + 1, h) && (h.on || h.hidden) : atoi(v.c_str()) != 0) add(p.label);
        }
        if (k == "fov") {
            char b[32];
            snprintf(b, sizeof b, "FOV %g", atof(v.c_str()));
            add(b);
        } else if (k == "rainbow") add("Rainbow");
        else if (k == "all_resolutions" && v[0] == '1') add("All resolutions");
    }
    fclose(f);
    return summary_list(parts, "Everything at default.");
}

inline VisualConfigs& visual_configs(bool reload = false) {
    static VisualConfigs c;
    if (c.loaded && !reload) return c;
    c.items.clear();
    for (auto& n : config::list_visual_configs()) c.items.push_back({n, visual_summary(n)});
    c.loaded = true;
    return c;
}

inline bool visual_name_taken(const std::string& clean) {
    for (auto& item : visual_configs().items)
        if (item.first == clean) return true;
    return false;
}

inline void visual_config_tile(const std::string& name, const std::string& summary, float w, float h) {
    static std::string renaming, confirm_save, confirm_delete;
    static char rename_to[48] = "";
    ImGui::PushID(name.c_str());
    ImGui::BeginGroup();
    ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
    tile_frame(a, b, false);
    auto* dl = ImGui::GetWindowDrawList();
    float pad = S(16), fs = font_size(f_label), big = font_size(f_tile), small = font_size(f_small), bh = ImGui::GetFrameHeight();
    brand::spaced_caps(dl, f_label, fs, {a.x + pad, a.y + pad}, "VISUAL CONFIG", C(T.muted), S(1.4f));
    ImGui::Dummy({w, h});
    if (renaming == name) {
        ImGui::SetCursorScreenPos({a.x + pad, a.y + pad + fs + S(8)});
        ImGui::SetNextItemWidth(w - pad * 2);
        if (ImGui::IsWindowAppearing() || !ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere();
        bool enter = ImGui::InputText("##rename", rename_to, sizeof rename_to, ImGuiInputTextFlags_EnterReturnsTrue);
        std::string clean = config::profile_name(rename_to);
        bool taken = clean != name && visual_name_taken(clean);
        dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + bh + S(16)}, C(taken ? T.bad : T.soft),
                    taken ? "A visual config with this name already exists." : "Letters, numbers, spaces, - and _.", nullptr, w - pad * 2);
        ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
        ImGui::BeginDisabled(clean.empty() || taken);
        if (accent_button("Rename", {S(110), 0}) || (enter && !clean.empty() && !taken)) {
            if (clean == name || config::rename_visual_config(name, clean)) toast("Renamed visual config to " + clean), visual_configs(true);
            else toast("Could not rename the visual config");
            renaming.clear();
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, S(8));
        if (ImGui::Button("Cancel", {S(90), 0})) renaming.clear();
    } else {
        dl->AddText(f_tile, big, {a.x + pad, a.y + pad + fs + S(8)}, C(T.text), name.c_str());
        dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + big + S(14)}, C(T.soft), summary.c_str(), nullptr, w - pad * 2);
        ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
        float gap = S(6), bw = (w - pad * 2 - gap * 3) / 4;
        if (accent_button("Load", {bw, 0})) {
            if (config::load_visual_config(name)) save_config(), toast("Loaded visual config " + name);
            else toast("Could not load the visual config"), visual_configs(true);
            confirm_save.clear(), confirm_delete.clear();
        }
        ImGui::SameLine(0, gap);
        bool sure_save = confirm_save == name;
        if (ImGui::Button(sure_save ? "Sure?" : "Save over", {bw, 0})) {
            confirm_delete.clear();
            if (sure_save) {
                toast(config::save_visual_config(name) ? "Saved the visuals into " + name : std::string("Could not save the visual config"));
                confirm_save.clear();
                visual_configs(true);
            } else {
                confirm_save = name;
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Replace this config with your visual settings");
        ImGui::SameLine(0, gap);
        if (ImGui::Button("Rename", {bw, 0})) {
            renaming = name, snprintf(rename_to, sizeof rename_to, "%s", name.c_str());
            confirm_save.clear(), confirm_delete.clear();
        }
        ImGui::SameLine(0, gap);
        bool sure_delete = confirm_delete == name;
        if (ImGui::Button(sure_delete ? "Sure?" : "Delete", {bw, 0})) {
            confirm_save.clear();
            if (sure_delete) {
                toast(config::delete_visual_config(name) ? "Deleted visual config " + name : std::string("Could not delete the visual config"));
                confirm_delete.clear();
                visual_configs(true);
            } else {
                confirm_delete = name;
            }
        }
    }
    ImGui::SetCursorScreenPos({a.x, b.y});
    ImGui::EndGroup();
    ImGui::PopID();
}

inline void new_visual_config_tile(float w, float h) {
    static bool naming = false;
    static char name[48] = "";
    ImGui::PushID("new_visual_config");
    ImGui::BeginGroup();
    ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
    tile_frame(a, b, naming, !naming);
    auto* dl = ImGui::GetWindowDrawList();
    float pad = S(16), fs = font_size(f_label), big = font_size(f_tile), small = font_size(f_small), bh = ImGui::GetFrameHeight();
    if (!naming) {
        if (ImGui::InvisibleButton("start", {w, h})) naming = true, name[0] = 0;
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        float tw = f_tile->CalcTextSizeA(big, FLT_MAX, 0, "+ New visual config").x;
        if (tw > w - pad * 2) big *= (w - pad * 2) / tw, tw = w - pad * 2;
        dl->AddText(f_tile, big, {(a.x + b.x - tw) / 2, (a.y + b.y) / 2 - big}, C(T.accent), "+ New visual config");
        const char* hint = "Saves the visual settings as they are.";
        float hw = f_small->CalcTextSizeA(small, FLT_MAX, 0, hint).x;
        dl->AddText(f_small, small, {(a.x + b.x - hw) / 2, (a.y + b.y) / 2 + S(8)}, C(T.soft), hint);
    } else {
        ImGui::Dummy({w, h});
        brand::spaced_caps(dl, f_label, fs, {a.x + pad, a.y + pad}, "NEW VISUAL CONFIG", C(T.muted), S(1.4f));
        ImGui::SetCursorScreenPos({a.x + pad, a.y + pad + fs + S(8)});
        ImGui::SetNextItemWidth(w - pad * 2);
        if (!ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere();
        bool enter = ImGui::InputTextWithHint("##name", "Name, e.g. Clean HUD", name, sizeof name, ImGuiInputTextFlags_EnterReturnsTrue);
        std::string clean = config::profile_name(name);
        bool taken = config::path_exists(config::visual_config_path(clean));
        dl->AddText(f_small, small, {a.x + pad, a.y + pad + fs + bh + S(16)}, C(taken ? T.bad : T.soft),
                    taken ? "A visual config with this name already exists." : "Letters, numbers, spaces, - and _.", nullptr, w - pad * 2);
        ImGui::SetCursorScreenPos({a.x + pad, b.y - pad - bh});
        ImGui::BeginDisabled(clean.empty() || taken);
        if (accent_button("Save config", {S(130), 0}) || (enter && !clean.empty() && !taken)) {
            toast(config::save_visual_config(clean) ? "Saved visual config " + clean : std::string("Could not save the visual config"));
            visual_configs(true);
            naming = false;
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, S(8));
        if (ImGui::Button("Cancel", {S(90), 0})) naming = false;
    }
    ImGui::SetCursorScreenPos({a.x, b.y});
    ImGui::EndGroup();
    ImGui::PopID();
}

inline void visual_configs_card() {
    begin_card("visual_configs", "VISUAL CONFIGS");
#if FATRAINER_VISUALS
    note("A visual config keeps your colors, glow, HUD, crosshair, field of view and screen settings. "
         "Your visual configs are saved in fatrainer_visual_configs next to the game.");
#else
    note("A visual config keeps your Player ESP, colors, glow, HUD, crosshair, field of view and screen settings. Loading one never changes your cheats. "
         "Your visual configs are saved in fatrainer_visual_configs next to the game.");
#endif
    ImGui::Dummy({0, S(4)});
    float gap = S(12), room = ImGui::GetContentRegionAvail().x, h = S(190);
    int cols = room >= S(520) ? 2 : 1, i = 0;
    float w = (room - gap * (cols - 1)) / cols;
    auto place = [&] {
        if (i % cols) ImGui::SameLine(0, gap);
        else if (i) ImGui::Dummy({0, S(2)});
        i++;
    };
    auto items = visual_configs().items;
    for (auto& [name, summary] : items) place(), visual_config_tile(name, summary, w, h);
    place(), new_visual_config_tile(w, h);
    end_card();
}

inline void configs_page() {
#if FATRAINER_VISUALS
    visual_configs_card();
#else
    static int tab = getenv("DLT_VISUAL_CONFIGS") != nullptr;
    const char* tabs[] = {"Cheat configs", "Visual configs"};
    segmented("configtabs", tabs, 2, tab, S(360));
    ImGui::Dummy({0, S(8)});
    if (tab) visual_configs_card();
    else configs_card();
#endif
}
