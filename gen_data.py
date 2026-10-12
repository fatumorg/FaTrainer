#!/usr/bin/env python3
import math
import os
import re
import struct
import sys
import xml.etree.ElementTree as ET
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
STEAM = os.path.expanduser("~/.local/share/Steam")
STATS = ["Damage", "Condition", "CriticalProb", "CriticalDamage", "Force", "StaminaUsage", "DamageRange",
         "UpgradeLevel", "AllowedRepairs", "MaxStackCount", "Price", "Color", "AmmoCount", "ReloadTime", "DepletionTime", "SwingSpeed"]
COLORS = {"Color_White": 0, "Color_Green": 1, "Color_Blue": 2, "Color_Violet": 3, "Color_Orange": 4, "Color_Platinum": 5}


def game_dw():
    vdf = os.path.join(STEAM, "steamapps/libraryfolders.vdf")
    for lib in re.findall(r'"path"\s+"([^"]+)"', open(vdf).read()):
        p = os.path.join(lib, "steamapps/common/Dying Light/DW")
        if os.path.exists(os.path.join(p, "Data0.pak")):
            return p
    sys.exit("Dying Light install not found")


def read_texts(t, every=False):
    out, p = {}, 8
    for _ in range(struct.unpack_from("<I", t, 4)[0]):
        n = struct.unpack_from("<H", t, p)[0]
        key = t[p + 2:p + 2 + n].decode("latin1")
        p += 2 + n
        n = struct.unpack_from("<H", t, p)[0]
        text = t[p + 2:p + 2 + 2 * n].decode("utf-16-le", "replace")
        p += 2 + 2 * n
        if every:
            out[key.lower()] = text
        elif key.endswith("_N"):
            out[key[:-2]] = text
    return out


def number(arg):
    arg = arg.strip()
    if arg in COLORS:
        return float(COLORS[arg])
    if re.fullmatch(r"-?[\d.]+(\s*[*/+-]\s*-?[\d.]+)*", arg):
        try:
            return float(eval(arg, {"__builtins__": {}}))
        except (SyntaxError, ZeroDivisionError):
            return None
    return None


def item_stats(z):
    stats, cur = {}, None
    for n in sorted(z.namelist()):
        if not (n.startswith("data/scripts/inventory/") and n.endswith(".scr")):
            continue
        for line in z.read(n).decode("latin1").splitlines():
            m = re.match(r'\s*Item\(\s*"([^"]+)"\s*(?:,\s*(\w+))?', line)
            if m:
                cur = stats.setdefault(m.group(1), {})
                cur.setdefault("melee", m.group(2) == "CategoryType_Melee")
                continue
            m = re.match(r"\s*(\w+)\((.*)\);", line)
            if m and cur is not None and m.group(1) in STATS and m.group(1) not in cur:
                v = number(m.group(2))
                if v is not None:
                    cur[m.group(1)] = v
    return stats


def c_str(s):
    out = []
    for ch in s.encode("utf-8"):
        c = chr(ch)
        out.append(c if 32 <= ch < 127 and c not in '"\\?' else f"\\{ch:03o}")
    return '"' + "".join(out) + '"'


def c_float(v):
    return "NAN" if v is None or math.isnan(v) else repr(float(v)) + "f"


SKILL_TREES = {"runner": 1, "fighter": 2, "status": 3, "legend": 5, "driver": 6, "zombie": 0}
SKILL_GROUPS = {"status": "G_StatusSkills", "fighter": "G_FighterSkills", "runner": "G_RunnerSkills", "driver": "G_DriverSkills",
                "legend": "G_LegendSkills"}
HUNTER_SKILLS = {
    "ControlTheHordeSpitEnable": ("Horde Summoner Spit", "Spit on a survivor to send a horde of bombers after him."),
    "ControlTheHordeSpitAmmo": ("Extra Horde Summoner", "Carry one more Horde Summoner spit."),
    "ControlTheHordeSpitDuration": ("Longer Horde Summoner", "The horde hunts the marked survivor for longer."),
    "LightDisableEnable": ("UV Suppressor Spit", "Spit that turns off UV lights and flashlights where it lands."),
    "LightDisableAmmo": ("Extra UV Suppressor", "Carry one more UV Suppressor spit."),
    "LightDisableDuration": ("Longer UV Suppressor", "UV lights stay off for longer."),
    "DefensiveSmokeEnabled": ("UV Block", "A cloud that shields you from UV light."),
    "DefensiveSmokeAmmo": ("Extra UV Block", "Carry one more UV Block."),
    "DefensiveSmokeDuration": ("Longer UV Block", "The cloud lasts longer and keeps you safe from UV for 2 more seconds."),
    "ZombieGroundPoundMovement": ("Moving Ground Pound", "You can move while you ground pound."),
    "ZombieGroundPoundAirEnabled": ("Aerial Ground Pound", "Ground pound from the air."),
    "ZombieGroundPoundKnockback": ("Ground Pound Knockback", "The ground pound throws survivors further."),
    "ZombieChargeAttack": ("Tackle", "Charge into a survivor and knock him down."),
    "ZombieChargeAttackKnockback": ("Tackle Knockback", "The tackle throws survivors further."),
    "ZombieLeapfrog": ("Leapfrog", "Pounce from one survivor straight onto the next."),
    "ZombiePounceSlam": ("Pounce Slam", "Your pounce ends in a blast that hits survivors around the target."),
    "ZombieSprintUpgrade": ("Faster Sprint and Jump", "Run faster and jump higher."),
    "ZombieRopeLocoUpgrade": ("Longer Tendril", "Your tendril reaches further and pulls you faster."),
    "SpitChargingEnabled": ("Charged Spit", "Hold to throw your spits further."),
    "SpitGroundPoundEnabled": ("Spit Smash", "Ground pound into your own spit to spread it."),
}


def xui_props(e):
    p = e.find("Properties")
    return {c.tag: (c.text or "").strip() for c in p} if p is not None else {}


def skill_layout(xui, group, prefix):
    root = ET.fromstring(xui)
    g = next(e for e in root.iter() if xui_props(e).get("Id") == group)
    width, height = float(xui_props(g)["Width"]), float(xui_props(g)["Height"])
    spots = {}

    def walk(e, ox, oy):
        for c in e:
            if c.tag in ("Properties", "Timelines"):
                continue
            p = xui_props(c)
            x, y = (float(v) for v in p.get("Position", "0,0,0").split(",")[:2])
            if re.fullmatch(re.escape(prefix) + r"\d+", p.get("Id", "")):
                spots[int(p["Id"][len(prefix):])] = (ox + x + float(p.get("Width", 52)) / 2, oy + y + float(p.get("Height", 52)) / 2)
            walk(c, ox + x, oy + y)

    walk(g, 0, 0)
    return width, height, spots


def plain(text):
    text = re.sub(r"%KEY\(([^)]*)\)|&\w+&", "the key", text)
    return " ".join(text.replace("\n", " ").split())


def skills(dw, texts):
    with zipfile.ZipFile(os.path.join(dw, "Data0.pak")) as z:
        sources = re.findall(r'Script\("([^"]+)"\)', z.read("data/skills/skills_sources.scr").decode("latin1"))
        xml = [re.sub(r"<!--.*?-->", "", z.read("data/skills/" + s).decode("latin1"), flags=re.S) for s in sources]
        menu = z.read("data/menu/scr/menuskills.xui")
    with zipfile.ZipFile(os.path.join(os.path.dirname(dw), "DW_DLC1", "DataDLC1_0.pak")) as z:
        hunter_menu = z.read("data/menu/scr/menubtzskills.xui")
    layouts = {cat: skill_layout(menu, g, "G_") for cat, g in SKILL_GROUPS.items()}
    layouts["zombie"] = skill_layout(hunter_menu, "G_Skills", "B_Skill")
    out = []
    for x in xml:
        for m in re.finditer(r"<skill\s+([^>]*)>(.*?)</skill>", x, re.S):
            a = dict(re.findall(r'(\w+)\s*=\s*"([^"]*)"', m.group(1)))
            cat, body = a.get("cat"), m.group(2)
            if cat not in layouts:
                continue
            width, height, spots = layouts[cat]
            tier = int(a.get("tier", 0))
            spot = spots.get(tier - 1 if cat == "zombie" else tier)
            if not spot or spot[0] > width:
                continue
            sid = a["id"]
            name, desc = HUNTER_SKILLS.get(sid, (texts.get(("skill_" + sid).lower(), ""), plain(texts.get(("skilldesc_" + sid).lower(), ""))))
            if not name:
                continue
            level = re.findall(r'<level_req\s[^>]*value\s*=\s*"(\d+)"', body)
            prestige = re.findall(r'<prestige_level_req\s[^>]*value\s*=\s*"(\d+)"', body)
            needs = re.findall(r'<skill_req\s+id\s*=\s*"([^"]+)"', body)[:3]
            out.append(dict(id=sid, name=name, desc=desc, tree=SKILL_TREES[cat], max=int(a.get("max_level", 1)),
                            cost=int(a.get("skill_points") or 0), level=int(level[0]) if level else 0,
                            prestige=int(prestige[0]) if prestige else 0, node="NODE" in a.get("desc_params", "").split(";"),
                            x=spot[0] / width, y=spot[1] / height, needs=needs))
    canvas = {SKILL_TREES[cat]: (w, h) for cat, (w, h, _) in layouts.items()}
    return out, canvas


def write_skills(rows, canvas):
    lines = []
    for s in rows:
        needs = ", ".join(c_str(n) for n in s["needs"])
        lines.append(f"  {{{c_str(s['id'])}, {c_str(s['name'])}, {c_str(s['desc'])}, {s['tree']}, {s['max']}, {s['cost']}, {s['level']}, "
                     f"{s['prestige']}, {str(s['node']).lower()}, {c_float(s['x'])}, {c_float(s['y'])}, {{{needs}}}}}")
    aspect = ", ".join(f"{{{t}, {c_float(w / h)}}}" for t, (w, h) in sorted(canvas.items()))
    with open(os.path.join(HERE, "src/skills.h"), "w") as f:
        f.write("#pragma once\n"
                "struct SkillInfo { const char* id; const char* name; const char* desc; int tree, max_level, cost, level_req, prestige_req; "
                "bool node; float x, y; const char* needs[3]; };\n"
                "struct SkillCanvas { int tree; float aspect; };\n"
                "static const SkillInfo SKILLS[] = {\n" + ",\n".join(lines) + "\n};\n"
                f"static const SkillCanvas SKILL_CANVASES[] = {{{aspect}}};\n")
    print(f"skills.h: {len(rows)} skills in {len(canvas)} trees")


def write_texts(t):
    rows, seen, p = [], set(), 8
    for _ in range(struct.unpack_from("<I", t, 4)[0]):
        n = struct.unpack_from("<H", t, p)[0]
        key = t[p + 2:p + 2 + n].decode("latin1")
        p += 2 + n
        n = struct.unpack_from("<H", t, p)[0]
        text = t[p + 2:p + 2 + 2 * n].decode("utf-16-le", "replace")
        p += 2 + 2 * n
        if key.lower() not in seen:
            seen.add(key.lower())
            rows.append(f"  {{{c_str(key)}, {c_str(text)}}}")
    with open(os.path.join(HERE, "src/texts.h"), "w") as f:
        f.write("#pragma once\n"
                "struct GameText { const char* key; const char* text; };\n"
                "static const GameText GAME_TEXTS[] = {\n" + ",\n".join(rows) + "\n};\n"
                f"static const int GAME_TEXT_COUNT = {len(rows)};\n")
    print(f"texts.h: {len(rows)} texts")


def main():
    dw = game_dw()
    with zipfile.ZipFile(os.path.join(dw, "Data0.pak")) as z:
        ids = sorted(set(re.findall(r'Item\(\d+,\s*"([^"]+)"\)', z.read("data/scripts/inventory/validitemids.scr").decode("latin1"))),
                     key=lambda s: s.encode())
        stats = item_stats(z)
    with zipfile.ZipFile(os.path.join(dw, "DataEn.pak")) as z:
        all_texts = z.read("data/maps/common_texts_all.bin")
    names = read_texts(all_texts)
    rows = []
    for i in ids:
        st = stats.get(i, {})
        vals = ", ".join(c_float(st.get(k)) for k in STATS)
        rows.append(f"  {{{c_str(i)}, {c_str(names.get(i, ''))}, {{{vals}}}, {'true' if st.get('melee') else 'false'}}}")
    enum = ", ".join(f"ST_{k}" for k in STATS)
    labels = ", ".join(c_str(k) for k in STATS)
    with open(os.path.join(HERE, "src/items.h"), "w") as f:
        f.write("#pragma once\n#include <cmath>\n"
                f"enum Stat {{ {enum}, ST_COUNT }};\n"
                f"static const char* STAT_KEYS[ST_COUNT] = {{{labels}}};\n"
                "struct ItemInfo { const char* id; const char* name; float st[ST_COUNT]; bool melee; };\n"
                "static const ItemInfo ITEMS[] = {\n" + ",\n".join(rows) + "\n};\n"
                f"static const int ITEM_COUNT = {len(ids)};\n")
    have = sum(1 for i in ids if any(k in STATS for k in stats.get(i, {})))
    print(f"items.h: {len(ids)} items, {sum(1 for i in ids if names.get(i))} named, {have} with stats")
    write_skills(*skills(dw, read_texts(all_texts, every=True)))
    write_texts(all_texts)


if __name__ == "__main__":
    main()
