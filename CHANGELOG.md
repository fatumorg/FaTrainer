# Changelog

## 2.5 b10

Survivor Update

- New: Texts has its own page, easier to read: every result shows the text the way you see it in the game, the English text under it when they differ, its key and its group. Filter by group (spoken lines, weapons and items, skills, quests, tutorials, HUD, menus) or search.
- New: Language on the Texts page shows the game's texts in another language while the voices stay as they are: English, Polish, German, French, Spanish, Italian, Portuguese (Brazil), Czech, Dutch or Turkish. Choose for each group whether it switches or keeps your game's language; spoken lines keep it unless you switch them.
- Your own texts win over the language you pick.

## 2.5 b9

Survivor Update

- New: Rainbow next to every color. The color cycles through the rainbow and keeps its brightness; pick how many seconds one cycle takes.
- New: every piece of the HUD has its own color now, 35 in seven groups, so the radar, the quest list and the objective markers no longer share one color. Your old part colors carry over to their pieces.
- New: Hide next to every HUD piece makes it invisible until you show it again.
- New: Crosshair draws your own crosshair (cross, dot, circle or cross and dot) with size, thickness, gap, opacity, outline and color, whenever the game shows its own. It can hide the game's crosshair.
- New: Survivor glow, for Be The Zombie: your own colors for how survivors glow in the Night Hunter's sense (normal, hit by UV Suppressor, marked by Horde Summoner, hit by Sense Suppressor).
- All of it is in the Visuals Version too.

## 2.5 b8

Survivor Update

- Fixed: Melee speed did nothing. It now speeds up the swing of every melee weapon itself (the weapon's swing speed) instead of a game setting that only feeds the inventory numbers.
- Fixed: Melee did not reach further. Now called Melee reach, it makes every melee weapon reach further (the weapon's damage range).
- Fixed: E did not kick when the dropkick or death from above sliders were up. Those sliders widened the kick's targeting everywhere, so a jump kick in story mode locked onto zombies it could not reach. They now only work while a Night Hunter is in the match, and then the kick only aims at him.
- Fixed: settings the game keeps in its own cache (movement speed, jump height and others) only changed after a respawn or a level load when you moved their slider in game. They now change right away.
- Fixed: with HUD colors on, the health bar flickered while you lost health and the level bar turned white. The trainer now lets the game's own HUD animations play and colors each piece again once it settles, starting from the game's latest color.
- The trainer knows you are a survivor as soon as it sees a Night Hunter in the match.

## 2.5 b7

Survivor Update

- Fixed: a HUD part color painted every piece of that part in one flat color, so the stamina bar looked full all the time and dark backgrounds turned into bright blocks. Part colors and the whole HUD tint now keep each piece's own brightness.
- Fixed: with a HUD color on, texts the game fades out or hides could show up fully.
- Fixed: after a respawn or a level load the HUD tint could grow darker every time, and turning it off did not bring the game's colors back.
- Fixed: saved HUD colors waited until you opened the menu after loading a save; the trainer now finds the HUD by itself.
- Texts refuses a replacement that uses a % the game's text does not have, since the killfeed lines are filled in with player names and a stray % could crash the game. Write a percent sign as %%.

## 2.5 b6

Survivor Update

- New: the Visuals Version, a second trainer with only the looks: UV light and Night Hunter glow colors, HUD colors and texts. No cheats, no ESP, no internet connection, and it runs whether you start the game with Play or from Steam. Tick Visuals only in the FaTrainer launcher 2.3 to install it.

## 2.5 b5

Survivor Update

- New: Texts on the Visuals page. Search every text the game shows and write your own, for example "Pounce interrupted by UV light!" or the Be The Zombie killfeed lines like "You killed %s". Reset brings back the game's text. Only you see your texts.

## 2.5 b4

Survivor Update

- New: HUD colors on the Visuals page. Tint the whole HUD in one color, or give health, stamina, crosshair, weapons and ammo, XP, killfeed and messages, radar and objectives, enemy health and the Night Hunter HUD their own color. Only you see them.

## 2.5 b3

Survivor Update

- Every color in the menu (accent, ESP colors, UV light, Night Hunter glow) now has a color picker, R, G and B number fields from 0 to 255 and a hex field like #E8973A.

## 2.5 b2

Survivor Update

- Fixed: no survivor or Night Hunter slider on the PvP page did anything, also Melee reach and every dropkick and death from above slider. Since 2.3 b3 these sliders wait until the trainer knows your side, and it looked for your side in the wrong place, so it never found out. It now reads it the way the game itself does, and in story mode, where there are no sides, the sliders apply after 5 seconds again.
- Fixed: Melee speed did nothing. The game adds the attack speed bonus instead of multiplying it, and the bonus is 0 without skills; x2 now really swings twice as fast.

## 2.5 b1

Survivor Update

- New: Dropkick angle and Dropkick height sliders on the PvP page. At Max of the angle the hunter can be behind you; the height lets you kick a hunter below you, like when you jump at him after his tackle missed. Dropkick is now Dropkick range and only changes the range.
- New: Melee speed on the Combat page: how fast you swing every melee weapon, two-handed ones too. Works in every mode.
- Fixed: the survivor Melee slider also widened how far the game looks for a melee target, so your swings turned toward targets far away and missed the one in front of you. It now only changes reach and the up and down angle.
- When the game skips the hunter as a dropkick target, `fatrainer.log` now says why (lines start with `kick:`), to make kicks during his recovery after a missed tackle work in the next build.

## 2.4

Refill Update

- 2.4 is the last build of 2.4: it comes after 2.4 b1, which updates to it by itself.
- New: Delete next to every item on the Backpack, Stash, Materials and Tools pages removes it from the game. Lowering an amount to 0 removes the item too; before, it stayed as an empty item.
- Fixed: `fatrainer.ini` kept every deleted built-in config several times when an older version had written it that way. The trainer now reads each one once.
- The log line `cheats:` now says which side the trainer thinks you play and whether it found your player's settings, to find out why survivor sliders do nothing in some games.

## 2.4 b1

Refill Update

- New: a key to refill health and stamina. Set it under Actions on the Keybinds page and press it while the menu is closed; a message at the bottom of the screen confirms it.

## 2.3

Launcher Update

- 2.3 is the last build of 2.3: it comes after 2.3 b1, b2 and b3, and those builds update to it by themselves.
- The FaTrainer Installer is now the FaTrainer launcher. Press Play: it installs or updates the trainer and starts Dying Light with it. Starting Dying Light from Steam plays the normal game without the trainer, so you can play with or without it.
- New: Keybinds page. Give any cheat its own key and press it while the menu is closed to turn the cheat on or off; a message at the bottom of the screen says which. You can change the menu key there too.
- Every version now has a name after its most important change; the changelog and the launcher show it.
- The menu is darker, in the same warm near-black as the FaTrainer website and launcher.
- Fixed: after loading a save with a config or remembered sliders on, your weapons often hit nothing until you turned everything off and set it again. Night Hunter and survivor sliders now wait until the trainer knows which side you play.
- Fixed: `fatrainer.ini` listed every deleted built-in config several times.

## 2.3 b3

Launcher Update

- Every version now has a name after its most important change. 2.3 is the Launcher Update, and the changelog names the earlier ones too.
- New: Keybinds page. Give any cheat its own key and press it while the menu is closed to turn the cheat on or off; a message at the bottom of the screen says which. You can change the menu key there too. A key can only belong to one thing, so picking a key takes it away from whatever had it before.
- Fixed: after loading a save with a config or remembered sliders on, your weapons often hit nothing until you turned everything off and set it again. While the game loads, the trainer cannot tell yet whether you play a survivor or the Night Hunter, and it applied the Night Hunter sliders (Claws, Ground pound, Pounce, Tackle) to your survivor right while the game set up your character. Night Hunter and survivor sliders now wait until the trainer knows which side you play.
- Fixed: `fatrainer.ini` listed every deleted built-in config several times.

## 2.3 b2

Launcher Update

- `fatrainer.log` records the grappling hook energy and the UV charge while Infinite grappling hook, Infinite UV flashlight or Slow down UV flashlight is on, to find out why they stop working.

## 2.3 b1

Launcher Update

- The menu is darker, in the same warm near-black as the FaTrainer website and installer.

## 2.2

Skill Tree Update

- 2.2 is the last build of 2.2: it comes after 2.2 b1, b2 and b3, and those builds update to it by themselves.
- New: a skill tree editor on the Skills page. Every tree (Survivor, Agility, Power, Driver, Legend, and Night Hunter while you play the hunter) is laid out like the game's own skill menu. Click a skill to learn it or remove it; learning a skill also learns the skills it needs, removing one also removes the skills that need it. Legend skills go up and down a level at a time. Learn all and Remove all do a whole tree. Changes are saved with your game like skills you buy.
- Fixed: Free skill points - and + did nothing and their row never showed. The game keeps skill points in a different kind of value than the trainer looked for. The tiles now show the free points the game shows, and - and + change them.
- The menu opens bigger, and it remembers the size you drag it to.
- New: Beta builds in Settings and in the FaTrainer Installer. Off unless you turn it on. With it on, the trainer and the installer also use the beta build, the version being worked on right now, when it is newer than the latest release. Beta builds may have bugs.
- Fixed: a key or mouse button held while you opened the menu could press the switch under it.

## 2.2 b2

Skill Tree Update

- The trainer no longer touches your items, enemies or the Night Hunter until you are in a game, so nothing is written while the game loads from the main menu.
- When the game crashes, `fatrainer.log` now also records heap corruption and fail-fast crashes, with the chain of game functions that led there. Please attach it if the game closes on its own.
- Fixed: as the Night Hunter your melee, tackle and ground pound often did nothing, especially after starting the game with the Pounce slider already on. Pounce also widened the pounce aim to all around you. The game uses the attack for a pounce whenever a survivor counts as a pounce target, so a survivor beside or behind you took over your swipes, and the pounce then failed. Pounce now only lengthens the range, and you pounce the survivor you aim at.
- Fixed: the survivor sliders (Death from above, Dropkick, Melee and the other kicks) also changed the Night Hunter's attacks while you played the hunter, and the Night Hunter sliders changed a survivor's game. Each side's sliders now only work while you play that side.

## 2.2 b1

Skill Tree Update

- The FaTrainer Installer now has its own version (Installer 1.0) and keeps itself up to date.
- XP gain now multiplies experience in every skill tree, Survivor and Night Hunter included, and the game's XP popup shows the boosted amount. Before it only changed Agility, Power and Driver.
- Fixed: Unbreakable weapons did not protect weapons you picked up after turning it on, until you pressed Refresh.
- Skills page: each skill tree is a tile with its level, an XP bar you can drag to set the XP, -10, -1, +1 point, +10 and Max, and Skill points - and + that add points without changing the level.
- Health and stamina bars on the Player page can be dragged to set them.
- New: Take less damage, as a survivor and as the Night Hunter.
- New: Less UV damage for the Night Hunter.
- New: One hit kill on the Night Hunter, on the PvP page.
- New: Sure-hit ground pound for the Night Hunter. The game only counts a survivor as hit when its blast trace reaches his body, so a step or uneven ground could make the ground pound miss even right next to him. Both Night Hunter presets turn it on.
- Fixed: the Night Hunter presets made the tackle miss. They widen the pounce aim, which the game shares with the tackle, so the tackle locked onto survivors beside you while charging straight ahead. The tackle now always keeps its normal aim.
- Fixed: Spit hit radius made a Night Hunter's spits hit you from anywhere while you played a survivor. The game that hosts the match decides spit hits, so the slider now only works while you play the Night Hunter, and only when you host.
- Fixed: Claws made your melee miss survivors next to you. It widened the range the game uses to pick a melee target, so the swipe turned toward survivors far away. Claws now only lengthens the reach.
- The Ground pound slider also raises how far above or below you a survivor can still be hit (normally 2 m).
- Fixed: the Night Hunter rank showed a number like 44890 instead of its name. Ranks now show the title and change one title at a time. In a Be The Zombie match the side you are not playing is grayed out.
- Versions now have build numbers, like 2.2 b1. A newer build of your version downloads itself and starts the next time you open the game; until then the build you have keeps working. A newer version still needs the FaTrainer Installer, and an older version stays off unless you tick Use older versions.
- The installer lists each version once and always installs its newest build.
- New: the FaTrainer Installer lets you pick which version to install, and its Use older versions checkbox lets an older version keep working in game after a newer one is out. The update notice still shows; without the checkbox an older version stays turned off, as before.
- Presets are now Configs, on the PvP page and in Settings. Built-in configs can be deleted too, and Restore built-in configs brings them back: the four built-in ones plus your own. New config saves what is on now, and every config of yours can be loaded, saved over, renamed and deleted. Your old configs carry over.
- The Night Hunter page moved to Cheats, and the sidebar no longer shows a section twice after you reorder pages.

## 2.1

Redesign Update

- New: Dodge spit on the PvP page, under As a survivor. When a spit is about to hit you, you step aside on your own: after a human reaction time, for a short random moment, sideways when it can, using W A S D. It only reacts to spit that would really hit you, never presses against a key you hold and never touches your mouse. Also part of both Survivor presets.
- New: Night Hunter glow color on the Visuals page. The veins that light up when UV hits the Night Hunter glow in your own color and brightness, like the UV light color.
- Fixed: the Skills page showed no skill trees and could not change them. The trainer looked for them in the wrong place.
- New: a redesigned menu that matches the installer and the website, with the same fonts, colors and Harran skyline. It fades and slides in when you open it, page titles write themselves in, cards appear one after another, the active page marker slides between pages, and buttons, switches, sliders and messages animate. The city at the bottom of the sidebar lights its windows and follows your mouse.
- New: more ways to make the menu yours in Settings, Appearance: accent color, window opacity, corner roundness, background dim, animations (Full, Subtle or Off), and the skyline and accent glow on or off.
- New: four ready-made Be The Zombie presets on the PvP page: Survivor Legit, Survivor Rage, Night Hunter Legit and Night Hunter Rage. Legit stays believable to the other players, Rage holds nothing back. Each one also sets the player ESP to match.
- New: the menu shows whether you have the Fatum Version or the Nexus Version, under the FaTrainer name and in Settings, About, together with what that version does.
- Fixed: most cheats could stop working for the rest of the session. Sometimes, during a loading screen, the game replaced its window handler and the trainer no longer got the signal it applies the cheats on. The trainer now notices this and takes the window back.

## 2.0

Installer Update

- New: FaTrainer Installer for Windows and Linux. It finds Dying Light in your Steam libraries (or lets you choose the folder) and puts the trainer next to the game. It also updates and uninstalls, shows this changelog and, on Linux, the Steam launch option with a copy button. The Fatum Version (from the website and GitHub) fetches the trainer from the GitHub release and checks it against the release checksum; the Nexus Version (from Nexus Mods) has the trainer inside and never connects to the internet.
- New, Fatum Version only: update check. When the game starts, the trainer asks GitHub for the newest version number. If a newer FaTrainer is out, this version stays turned off and shows a short notice; open the installer and press Update. The game itself keeps working. Without internet the trainer works as before. The Nexus Version has no update check; track the mod on Nexus Mods to hear about new versions.
- The trainer file now carries its version in its Windows file properties, so the installer and you can see which version is installed.

## 1.9

Configs Update

- New: configs on the Settings page. Save the cheats and sliders you have on under a name, then load or delete them with one click. Each config is a small file in fatrainer_configs next to the game.
- New: Death from above height on the PvP page. The game only starts Death From Above once you are already falling fast (12 m/s, a long drop). At Max a normal jump is enough.
- Fixed: PvP sliders, Slow down UV flashlight and other cheats stopping after you die. After a respawn the trainer now recognises your new character straight away and finds the game objects again on its own.
- New: Visuals page with a player ESP for Be The Zombie and co-op: box, role (Night Hunter or survivor), health bar and number, distance, PvP rank, hunter rage, lines from the screen bottom, allies on or off, a range limit and separate colors for the Night Hunter and survivors. It keeps drawing with the menu closed.
- New: custom UV light color and glow on the Visuals page.
- New: spit keys on the Night Hunter page. Pick your own key for the Horde Summoner, UV Suppressor, Sense Suppressor and Toxic spit; the trainer presses the game's key for it.
- Fixed: one hit kill no longer drops Be The Zombie nests to 1 health, so their health bar is not stuck on red.
- New: Spit hit radius slider on the PvP page (replaces the old Spit slider, which only changed one spit type). Horde Summoner, UV Suppressor and Sense Suppressor spits affect survivors further from where they land (5 m normally, up to 30 m), and Toxic spit puddles grow with it.
- No ability cooldowns no longer sets the grab break cooldown, which the game never reads.
- Fixed: crashes and stutter while the ESP or one hit kill was on. The trainer searched all game memory every 3 seconds, twice, and could crash when the game freed memory during the search. It now does one search, every 10 seconds for players, at low priority with short pauses, and skips memory that disappears mid-read instead of crashing. When game objects for a cheat are missing (for example in menus), it searches less and less often instead of every 15 seconds.
- Fixed: the ESP flickering or not showing in Be The Zombie. It now draws from the game camera and holds each player for a moment when the game skips a position update, instead of depending on finding your own character first.

## 1.8.1

Crash Fix Update

- Fixed: the game could crash at start on Windows. The trainer's memory scan read game memory that the game was freeing at the same time; on Windows it now copies each block safely first.
- Fixed: a crash when the trainer read the position of a character that was being removed (Kill all enemies, saved positions, prison sections).
- Unbreakable weapons keeps each weapon's own durability full instead of relying on game settings, so it works for every melee weapon, including the Korek Machete.

## 1.8

Prison Run Update

- Fixed: zombies and NPCs standing still. Cheats that change game settings (for example the grappling hook) also changed a different setting of every zombie and NPC. They now only change your own settings.
- Fixed: giving an item could crash the game when the trainer tried it in the ammo inventory. Only ammo goes there now.
- Fixed a crash that could happen after turning a cheat off. The trainer wrote to game memory that the game had already freed, for example after a level load.
- Infinite UV flashlight: fixed. The trainer was filling the normal flashlight's battery instead of the UV charge.
- Instant lockpicking: fixed. The game now treats every pick position as the sweet spot, so the lock always turns all the way.
- Fixed a crash while loading into a save and when pressing Find sections: the trainer read prison section positions through the wrong part of the object. Positions are now looked up the way the game's own class layout describes, and only when you press Find sections.
- New: weapon rarity in the weapon editor (Gray, Green, Blue, Purple, Orange, Gold). Found and generated weapons keep their own rarity, so the editor changes that one weapon, and the game saves it with the weapon.
- New: Slow down UV flashlight slider next to Infinite UV flashlight. x10 makes the charge last ten times longer.
- New: Night Hunter level on the Skills page (-10, -1, +1, +10, Max), and a Be The Zombie ranks card to change your survivor and Night Hunter PvP rank.
- New: Kill all enemies nearby (Combat and Prison pages). Kills every zombie and human within 80 m through the game's own kill, so quest objectives like killing the bandits or a final wave count them.
- New: route recorder on the Prison page. Record your route once while playing normally; Replay teleports you along it in small steps so every quest trigger fires, and pauses where you stood still (fights, doors) until you press Continue. Saved in fatrainer_route.txt next to the game.
- Prison sections are recognised again (they all showed as unnamed sections before) and listed in run order: Start, every split checkpoint (Split 1, 2, ...), the reward room and the evacuation. A Next section button jumps to them one after another.
- Teleporting moves your physics body too, so the game no longer pulls you back to where you stood.
- God mode also works in Harran Prison: while it is on, the game's forced damage (which the prison uses) is blocked too.
- Infinite grappling hook keeps the rope energy full directly, so the hook is ready every time, also while you are still hanging on the rope.
- Pause prison timers: the trainer now takes ownership of the prison timer data like the game does, so the paused time is no longer overwritten.
- Cheats that need game objects (UV flashlight, grappling hook, prison timers) find them again on their own after a level load, even when the menu stays closed.
- Turning a cheat off restores a value only if the game has not changed it since.
- Ignores stray stash objects with garbage capacity.

## 1.7

Prison Update

- New: Prison page for Harran Prison. Pause the run timer and the reward room countdown, teleport to every prison section, and save and load your position anywhere.
- Infinite grappling hook: the hook's wait comes from rope energy, not from a cooldown, so the rope now recharges instantly.
- Infinite UV flashlight: the UV charge is held full directly on your equipment.
- Refill health and stamina writes full health and stamina itself if the game's refill leaves them low.
- Instant lockpicking also searches the game's own data for the lock difficulty settings.
- Fixed: giving items could crash the game when the item used as a template had just been removed. The trainer now takes it from your live inventory and checks it first.
- Fixed: the stash shows on the Stash page again (its capacity is unlimited, not -1).
- Fixed: memory scans under Proton skip Wine's own thread memory, which showed extra wallets and players.
- New pages are placed next to their group in the sidebar when you update.

## 1.6

Lockpick Update

- Infinite grappling hook and infinite UV flashlight: the trainer now hooks the function the game uses to read these settings, so the new values reach every place that uses them.
- New: instant lockpicking. Every pick position opens the lock and lockpicks never break.
- New: Level up button on the Skills page. It gives exactly the XP for the next level, so the game levels you up like normal play.
- Giving items tries every inventory until the game accepts the item, so upgrades like King and Clicker work, and ammo goes into the ammo inventory.
- Typing in the menu works under Proton: money amounts, search boxes, item counts and weapon stats.
- PvP: one slider per attack instead of separate range and angle sliders. At Max the pounce and dropkick also hit targets that are not in front of you.
- PvP: death from above has two sliders, range and pull. At Max pull the hunter can be beside or behind you and the attack pulls you onto him.
- Cash is one simple option: one amount, quick buttons and Set.
- The stash shows up even when it is empty, and only one stash is listed.
- Interface size changes only when you press Apply, so the slider no longer jumps while you drag it.
- Clearer labels on the Give items, inventory and PvP pages.
- Fixed: the memory scan could pick up stale copies from its own thread stack under Proton, which showed extra wallets.
- Better logs for bug reports: system, overlays and other mods, every overlay start step, a first frame watchdog, your inventories, failed gives and messages.

## 1.5

Sidebar Update

- New sidebar menu grouped into Cheats, Be The Zombie and Items. A dot marks pages with something turned on, and Turn all off resets everything.
- Settings saved to fatrainer.ini: accent color, interface size, background dim, menu key, page order and visibility, remembered cheats.
- The game ignores your mouse and keyboard while the menu is open, the background dims, and the mouse wheel scrolls the menu.
- Player: god mode, infinite stamina, infinite grappling hook, infinite UV flashlight, no fall damage, movement speed, jump height, refill health and stamina.
- Combat: one hit kill, infinite ammo, no reload, infinite consumables, unbreakable weapons.
- Skills: XP gain multiplier and skill tree levels with -10, -1, +1, +10 and Max.
- Night Hunter: hunter god mode, infinite energy, no ability cooldowns, infinite spits, long camouflage.
- PvP: range and aim angle sliders for pounce, ground pound, tackle, claws, spit, death from above, dropkick, kicks and melee.
- Weapon editor: magazine size and reload time, and edits stay applied while the game runs.
- Your own character is found correctly in co-op.

## 1.0.0

First Release

- First release: cash, backpack, stash, materials, give any item or blueprint, weapon editor.
- Works on Windows and on Linux with Proton.
