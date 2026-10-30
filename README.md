<div align="center">

# FaTrainer | Dying Light

**In-game trainer for Dying Light 1 on Windows and Linux**

![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-64C864)
![game](https://img.shields.io/badge/game-Dying%20Light%201-red)
![license](https://img.shields.io/badge/license-MIT-blue)

</div>

---

### Features

#### Cheats

- **Player**: god mode, infinite stamina, take less damage, draggable health and stamina bars, infinite grappling hook, infinite UV flashlight, instant lockpicking, no fall damage, movement speed, jump height, instant refill
- **Combat**: one hit kill, infinite ammo, no reload, infinite consumables, unbreakable weapons
- **Skills**: XP multiplier for every skill tree (the game shows the boosted XP), draggable XP bars, +1 point through real XP, -10, -1, +10 and Max for any skill tree, extra skill points without changing the level
- **Teleport**: go to your map marker or the objective of the quest you track, with how far away it is and in which direction; saved positions for every map, with go, rename and delete

#### Be The Zombie

- **Survivor**: Dodge spit steps you aside from spit that would hit you, with a human reaction time; Auto UV flare throws a UV flare when a UV Suppressor turns your UV light off (it presses the middle mouse button, the game's throw key); One hit kill on the Night Hunter drops him to 1 health in your game (not in a real match); Unspikeable keeps your game from starting a spike death on you, and Spike range lets your kicks find spikes farther from where the target flies (both sides); one reach slider per attack (death from above range, pull and height, dropkick range, angle and height, other kicks and ground pound, melee), throwable recharge and your survivor rank. At Max the dropkick and death from above also hit targets that are not in front of you
- **Night Hunter**: hunter god mode, infinite energy, take less damage, less UV damage, sure-hit ground pound, no ability cooldowns, infinite spits, long camouflage, one reach slider per attack (pounce, ground pound, tackle, claws, spit hit radius), spit keys and your hunter rank

#### Modes

- **DLC Modes**: pause the Harran Prison timers and kill all enemies nearby in Harran Prison

#### Look

- **Visuals**: player ESP for Be The Zombie and co-op, custom UV light color and glow, custom Night Hunter glow color, HUD colors, your own crosshair, survivor glow, Be The Zombie colors, field of view
- **Texts**: replace any text the game shows, game language, voices and subtitles, Be The Zombie callouts

#### Items

- **Cash / Backpack / Stash / Materials**: set your money and any stack count
- **Give items**: spawn any weapon, upgrade, consumable, material or blueprint; each item goes into the inventory the game accepts it in
- **Weapon editor**: damage, durability, crits, knockback, stamina, reach, magazine, reload time, repairs, rarity, four upgrade slots with any upgrade, any mod on any weapon, mod effects with levels, upgrade slots per weapon type; crossbows, shields and every other weapon too

#### Menu

- **Configs**: cheat configs (four built-in ones plus your own) and visual configs that keep your Player ESP, colors, glow, HUD, crosshair, field of view and screen settings; create, load, save over, rename and delete
- **Keybinds**: the menu key and a key for every cheat, slider, action and config. A slider key sets Max or your own On and Off levels; a config key loads the config and puts back what you had on the next press
- **Quick wheel** (Fatum Version): hold a key, point and let go to switch one of up to 8 pinned cheats, sliders and configs
- **Settings**: accent and background colors, window opacity, corner roundness, animations (Full, Subtle, Off), Harran skyline and accent glow, interface size, background dim, menu key, sidebar order and visibility; window style (Classic, Compact rail, Top tabs, Side dock), density, font, text size, card style, switches or checkboxes, descriptions, where the menu opens and sidebar width; in the Fatum Version, the Active list of what is on in a screen corner while the menu is closed; all saved to `fatrainer.ini`. About shows your version and whether it is the Fatum Version or the Visuals Version

### Install

Download FaTrainer, the launcher (Fatum Version), from the [launcher release](https://github.com/fatumorg/FaTrainer/releases/tag/installer):

- **Windows**: `FaTrainer.exe`
- **Linux**: `FaTrainer-Linux.tar.gz`, unpack it and run `FaTrainer` (works on any distribution with glibc 2.27 or newer and `curl` or `wget`). On NixOS start it with `steam-run ./FaTrainer`

On Linux, Dying Light has to run through Proton (Properties → Compatibility → force a Proton version); the trainer does not load in the native Linux version.

FaTrainer finds Dying Light in your Steam libraries (or lets you choose the folder). Press Play: it downloads `xinput1_3.dll` from the release when yours is older, checks it against the release checksum, puts it next to `DyingLightGame.exe` and starts the game through Steam with the trainer on. Start Dying Light from Steam instead and you play the normal game: the file stays, but the trainer does nothing. The version list lets you play an older version. Remove from game deletes it. Add shortcut puts FaTrainer on the desktop and in the Start menu, or in the Linux app menu. On Linux it also shows the Steam launch option Proton needs:

```
WINEDLLOVERRIDES="xinput1_3=n,b" %command%
```

FaTrainer has its own version and updates itself as soon as it opens when a newer one is out. Beta builds, off unless you tick them in FaTrainer or in Settings, give you the version being worked on right now; they may have bugs. It also shows announcements once when it opens (`announcement.txt` in this repository, posted with `./announce.sh`). Versions can have build numbers, like 2.3 b1; a newer build of your version installs itself the next time you start the game, and the last build of a version is just the version, like 2.3. Without internet, Play starts the game with the trainer you have.

The **Nexus Version** on [Nexus Mods](https://www.nexusmods.com/dyinglight/mods/1724) is frozen at 1.9 and never updated: just `xinput1_3.dll`, with no installer, no internet connection and no update check, and with the multiplayer features (Night Hunter and PvP pages, player ESP, spit keys, PvP ranks, Night Hunter level) grayed out under a notice that the newest version is on fatrainer.fatum.cc and GitHub. Every cheat turns off while you play the Night Hunter. Its source is the `nexus-1.9` branch.

Press **Insert** or **F8** in game to open the menu. While it is open the game ignores your mouse and keyboard. A dot in the sidebar marks pages with something turned on, and **Turn all off** resets every cheat and slider at once.

See [CHANGELOG.md](CHANGELOG.md) for what changed in each version.

### Building

`./build.sh` builds the trainer, `./build_installer.sh` the launcher, `./release.sh` the trainer release files, and `./release_installer.sh` the launcher release files (all through nix; `--publish` uploads them).

### Notes

- Made for single player. Back up your saves first.
- Log file: `fatrainer.log` next to `DyingLightGame.exe`.

<div align="center"><sub>MIT License</sub></div>

Questions and ideas go to the [forum](https://github.com/fatumorg/FaTrainer/discussions); its rules are in section 4 of [Privacy, contact and forum rules](https://fatrainer.fatum.cc/legal.html#forum).
