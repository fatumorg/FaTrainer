# Changelog

## 3.0 b10

Lobby Update

- New: Auto-Destroy Nests on the Survivor page. Destroy all nests destroys the Night Hunter's nests one by one, and the loop switch keeps doing it until none is left. The card shows how many nests are left.
- New: the Teleport page lists the living nests during an invasion, nearest first, with Go for each and Nearest nest.
- This is a beta build. It has not been tried in a real match yet. Nests are destroyed with the game's own kill, the way Kill all enemies nearby kills; if a nest stays, or the match does not count it, send the log (fatrainer.log).

## 3.0 b9

Lobby Update

- New: Unspikeable on the Survivor and Night Hunter pages. When a kick, tackle or ground pound would throw you onto spikes, your game does not start the spike death.
- New: Spike range on the Survivor and Night Hunter pages. Your kicks, tackles and ground pounds also find spikes farther from the path the target flies; at Max, spikes up to 6 m off it. The spike search runs in the attacker's game, so this works as the host and as a joined player.
- This is a beta build. Neither has been seen to work in a real match yet: if a spike kill still happens with Unspikeable on, or Spike range does nothing, send the log (fatrainer.log).

## 3.0 b8

Lobby Update

- Fixed: Show descriptions off still showed the grey line under the language rows on the Texts page.
- Fixed: in the Top tabs style, the last card of a page with an odd number of cards took only half the width.
- Fixed: with a bigger Text size or the Mono font, the Edit, Set 99 and Delete buttons, the hex color field and the language lists cut off their text.
- This is a beta build. These changes have not been tried in the game yet: if something looks wrong, send a screenshot and the log (fatrainer.log).

## 3.0 b7

Lobby Update

- Fixed: the frame rate dropped every 3 seconds while One hit kill was on, for example with the Hunter Rage config. FaTrainer searched the whole game memory for enemies each time, although One hit kill already reaches every enemy as the game updates it; the search now only runs when that path is not working.
- This is a beta build. It has not been tried in the game yet: if something goes wrong, send the log (fatrainer.log).

## 3.0 b6

Lobby Update

- New: Auto UV flare on the Survivor page. When a UV Suppressor turns your UV light off, it throws a UV flare by pressing the middle mouse button, the game's throw key.
- Fixed: with UV flare throw speed on, the flare stayed in your hand for a second or two after the throw.
- Fixed: the frame rate dropped every 10 seconds while a survivor reach slider, the Player ESP or One hit kill on the Night Hunter was on. FaTrainer searched the whole game memory for the players each time; it now asks the game for them once a second and searches the memory only once a minute to check that no player is missing.
- This is a beta build. None of these changes has been tried in the game yet: if something goes wrong, send the log (fatrainer.log).

## 3.0 b5

Lobby Update

- Fixed: Go to objective on the Teleport page showed a text code such as "&Museum_Name&" for some quests instead of the quest name.
- Fixed: dashes could disappear from wrapped text when the menu scale is below 100%.
- Fixed: in narrow windows such as the Dock style, the Go, Rename and Delete buttons of a saved position covered its name. They move under the name now.
- This is a beta build. The Quick wheel, the Active list, every window style and Teleport were tried in the game; picking from the Quick wheel with its key was not. If something goes wrong, send the log (fatrainer.log).

## 3.0 b4

Lobby Update

- Fixed: the game could crash with the menu open while the HUD was being rebuilt, for example when a match or map was loading, with HUD colors or your own texts on.
- This is a beta build. Quick wheel, Active list and the window styles other than Classic have not been tried in the game yet: if something goes wrong, send the log (fatrainer.log).

## 3.0 b3

Lobby Update

- Changed: the menu is green (#64C864) by default, and the Harran skyline windows take your accent color. An accent you picked before stays; Reset appearance or the green swatch gives you the new one.
- New: Background color in Settings. The menu background is darker by default and stays dark enough for the text to read.
- Fixed: the game could crash after loading another save or map with weapon edits on. Weapon edits are now only applied while you are in the game.
- Fixed: the map name in Saved positions on the Teleport page flickered.
- Fixed: right-click did nothing on the big buttons, such as Kill all enemies nearby. It pins them to the Quick wheel now.
- Fixed: holding the Quick wheel key could keep keyboard presses from reaching the game.
- Changed: Sidebar width is only shown for the Classic window style, the only one with a sidebar.
- This is a beta build. Quick wheel, Active list and the window styles other than Classic have not been tried in the game yet: if something goes wrong, send the log (fatrainer.log).

## 3.0 b2

Lobby Update

- New: Menu style in Settings. Density (Comfortable or Compact), Font (Default, Wide or Mono), Text size from 80% to 130%, Card style (Filled, Outlined or Flat), Switches look like switches or checkboxes, Show descriptions for the grey line under each cheat and option, Opens at (Center, Where I left it, Left or Right) and Sidebar width from 190 to 320 px.
- New: four window styles, picked with Style in Settings. Classic is the menu with its sidebar. Compact rail is a smaller window with a column of page icons. Top tabs puts the page groups and pages in tabs on top of a wider window, with the cards in two columns when there is room. Side dock is a panel the full height of the screen on the right edge, or on the left when Opens at is Left.
- The Menu style options and the window styles are in the Visuals Version too.
- New: Quick wheel, in the Fatum Version only. Hold its key while the menu is closed, point with the mouse and let go to switch the pin you point at; the camera stays still while you point. Pin up to 8 cheats, sliders and configs: right-click a cheat or slider, or press Pin next to a config on the Keybinds page, where the Quick wheel card has the key. A slider pin works like the slider's key and a config pin like the config's key. The wheel does not open while the game's chat is open.
- New: Active list in Settings, in the Fatum Version only. While the menu is closed, the screen corner you pick lists the cheats that are on and the sliders you changed, with their level.
- Fixed: Window opacity, Corner roundness, Animations, Harran skyline and Accent glow went back to their defaults every time the game started.
- Fixed: a key for a config that was deleted or renamed could not be cleared or changed. The Keybinds page lists such keys with Config not found.
- Fixed: a fatrainer_positions.ini edited by hand lost positions: a line with spaces at the end, the first line of a file saved with a BOM, and a position without a name. A position without a name gets the next free Position N.
- Fixed: Delete on a saved position asks Sure? first.
- Fixed: Go to last saved position said Back at the position's name also when it could not take you there.
- This is a beta build. Quick wheel, Active list and the new window styles have not been tried in the game yet: if something goes wrong, send the log (fatrainer.log).

## 3.0 b1

Lobby Update

- 3.0 b1 comes after 2.5.1 b1, and everything from 2.5.1 b1 is part of 3.0: upgrade slots that take blueprints and any other item, and Be The Zombie colors.
- New: the sidebar groups the pages into Cheats, Be The Zombie, Modes, Look, Items and Menu. The PvP page is now the Survivor page under Be The Zombie, with Dodge spit, One hit kill, the survivor reach sliders and the survivor rank. The Night Hunter page has the Night Hunter reach sliders, the spit keys and the hunter rank. Your sidebar order carries over.
- New: Teleport page. Go to waypoint takes you to the marker you placed on the map, and Go to objective to the quest you track; under each button you see how far away it is and in which direction. Save here keeps where you stand under a name, and every map has its own list of saved positions to go back to, rename or delete. The Keybinds page has keys for Go to waypoint, Go to objective, Save position and Go to last saved position.
- Changed: the Go to saved position key is Go to last saved position. It takes you to the newest position saved on the map you are on, and your key carries over.
- New: DLC Modes page with Pause prison timers and Kill all enemies nearby for Harran Prison. It says when you are in Harran Prison.
- Removed: Find sections, the teleports to every prison section and the route recorder from the Prison page.
- New: Configs page with two tabs, Cheat configs and Visual configs. A visual config keeps your Player ESP, colors, glow, HUD, crosshair, field of view and screen settings: create, load, save over, rename and delete them. Loading a visual config leaves your cheats alone, and loading one of your own cheat configs leaves your visuals alone.
- Fixed: New config saved over a config whose name differed only in upper and lower case. That name is refused now, like any name that is taken.
- New: My levels for slider keys on the Keybinds page. Pick To Max or My levels next to a slider's key; with My levels one press sets your On level and the next press your Off level.
- New: keys for configs on the Keybinds page. One press loads a cheat config or visual config, the next press puts back what you had, and a message at the bottom of the screen says which. A key whose config was deleted or renamed only says Config not found.
- Visual configs and their keys are in the Visuals Version too, with the Configs page and a Keybinds page for the menu key and visual config keys.
- Changed: the menu texts are shorter and only say what each control does.
- New: Show beta builds on the Changelog tab of the FaTrainer launcher 3.0. Unticked, the changelog lists only the final versions, and each final from 2.2 to 2.5 lists everything new and every fix from its beta builds.
- This is a beta build. Teleport, the DLC Modes page, visual configs, My levels and the config keys have not been tried in the game yet: if one of them goes wrong, send the log (fatrainer.log).

## 2.5.1 b1

Night Colors Update

- New: upgrade slots hold more than upgrades. Each slot of the weapon editor lists every upgrade and every blueprint (weapon mod), and its search finds any other item of the game, also ones the game never allows there. A blueprint in a slot adds its effects to the weapon on top of the weapon's own mod; other items sit in the slot without an effect.
- New: Be The Zombie colors (Visuals). Your own color for the Toxic, UV Suppressor, Sense Suppressor and Horde Summoner spits (flight trail, splash, splats and motion trail), the Night Hunter's veins in UV light and the hunter's fill light. Every color keeps the game's brightness, has Rainbow, and is in the Visuals Version too.
- This is a beta build. Neither change has been tried in a match yet: if a spit keeps its color or an item in a slot causes trouble, send the log (fatrainer.log).

## 2.5

Survivor Update

- 2.5 is the last build of 2.5: it comes after 2.5 b26, which updates to it by itself.
- New: upgrades in the weapon editor. Every weapon has four upgrade slots, each empty or any upgrade the game has (Enforcer up to King and Clicker), also on weapons the game would not upgrade. The game keeps them in your save.
- New: mods in the weapon editor. Put any of the game's weapon mods on any weapon, also mods the weapon would not take, or remove one. The game keeps the mod in your save.
- New: effects of a mod. Shock, Impact, Burning, Poison, Bleeding, Freezing, Stun, Explosive and Acid, each off or any level the game has (up to 4, Shock and Impact also 101). Every weapon with that mod gets them, and they come back every time you start the game.
- New: upgrade slots in the game's own upgrade screen, 0 to 4, for every weapon of a type.
- New: crossbows, shields and every other weapon open the weapon editor too: stats, rarity, upgrades and mods, with No reload for crossbows.
- New: the weapon editor is redesigned: big editable values in a two column grid, the game's value next to every stat you changed, Undo per stat and for everything, rarity as colored buttons, and extras that fit the item (no reload for guns and crossbows, infinite ammo, unbreakable melee weapons, infinite UV for the UV flashlight, infinite grappling hook). Flashlights and the grappling hook can be edited too.
- New: weapon edits are saved and come back every time you start the game.
- New: the Visuals Version, a second trainer with only the looks: UV light and Night Hunter glow colors, HUD colors, texts and every other visual change of 2.5. No cheats, no ESP, no internet connection, and it runs whether you start the game with Play or from Steam. Tick Visuals only in the FaTrainer launcher 2.3 to install it.
- New: HUD colors on the Visuals page. Tint the whole HUD in one color, or give each of its 35 pieces, in seven groups, its own color, so the radar, the quest list and the objective markers each have their own. Every color keeps the piece's own brightness. Hide next to every piece makes it invisible until you show it again. Only you see them.
- New: Crosshair draws your own crosshair (cross, dot, circle or cross and dot) with size, thickness, gap, opacity, outline and color, whenever the game shows its own. It can hide the game's crosshair.
- New: Survivor glow, for Be The Zombie: your own colors for how survivors glow in the Night Hunter's sense (normal, hit by UV Suppressor, marked by Horde Summoner, hit by Sense Suppressor).
- New: Rainbow next to every color. The color cycles through the rainbow and keeps its brightness; pick how many seconds one cycle takes.
- Every color in the menu (accent, ESP colors, UV light, Night Hunter glow) has a color picker, R, G and B number fields from 0 to 255 and a hex field like #E8973A.
- New: Texts page. Search every text the game shows and write your own, for example "Pounce interrupted by UV light!" or the Be The Zombie killfeed lines like "You killed %s". Every result shows the text the way you see it in the game, the English text under it when they differ, its key and its group. Filter by group (spoken lines, weapons and items, skills, quests, tutorials, HUD, menus) or search. Reset brings back the game's text. Only you see your texts, and they win over the language you pick.
- New: click a text to edit it right where it is. Color all of it or just its label, and preview it. Preview on a text and Show in the killfeed post the line into the game's real killfeed when you are in a game.
- Texts refuses a replacement that uses a % the game's text does not have, since the killfeed lines are filled in with player names and a stray % could crash the game. Write a percent sign as %%.
- New: Game language, Voices and Subtitles are three separate settings on the Texts page: English, Polish, German, French, Spanish, Italian, Portuguese (Brazil), Czech, Dutch or Turkish. The game language is the game's own language switch, so menus, the Night Hunter info and everything else change natively; voices and subtitles can stay in your language. The game language and voices change at the next start.
- New: a language for every text category on the Texts page. Next to Subtitles, Weapons and items, Skills, Quests and objectives, Tutorials and hints, HUD and messages and Menus can each have their own language, changing right away.
- New: Be The Zombie callouts: short killfeed lines in one click (Pounced:, Failed Pounce:, Tackled:, Failed Tackle:, Dropkicked, From Above, Headshot, Nest Down, Hunter Down, every way to kill like sliced, shot or burned, self destruct, bled out and more), with optional colored labels.
- New: callout sets on the Texts page. Make your own sets of Be The Zombie callouts, rename and delete them, and edit every line. The two built-in sets can be edited too and restored with one click.
- New: Field of view on the Visuals page, live. Sprinting and aiming still zoom like before.
- New: Unlock every resolution (Visuals, Screen). The game's video settings list every common resolution, also sizes your monitor does not report, from the next start.
- New: Melee speed on the Combat page: how fast you swing every melee weapon, two-handed ones too, up to x5. Heavy and charged attacks and the rest after every attack are shortened by the same speed; climbing, the grappling hook and picking up items keep their speed. Works in every mode.
- Changed: the survivor Melee slider is Melee reach. It changes the reach the game uses for every swing (targets and hit lines), for every weapon. Only your own swings are changed.
- New: Dropkick angle and Dropkick height sliders on the PvP page. At Max of the angle the hunter can be behind you; the height lets you kick a hunter below you, like when you jump at him after his tackle missed. Dropkick is now Dropkick range and only changes the range.
- New: Throwable recharge on the PvP page: UV flares, injectors, grenades, molotovs, throwing stars and mines come back faster in Be The Zombie, at Max right away.
- New: UV flare throw speed (Combat). Throws the UV flare up to x5 faster, both of the game's UV flare items; other throwables are not changed.
- New: Always 1 bullet for Last Wish (Combat). The Last Wish revolver always has one round loaded, so every shot is its explosive last round.
- New: Item Dupe on the Combat page: weapons and other single items you drop stay in your backpack too. It waits until the dropped item lies next to you, and the copy is exact, with rarity and upgrades; part of a stack you drop comes back too.
- New: keys for sliders on the Keybinds page. One press sets the slider to Max, the next press puts back what you had, and a message says which.
- New: keys for more actions on the Keybinds page: Kill all enemies nearby, Save position, Go to saved position and Player ESP on / off, next to Refill health & stamina.
- New: if the game crashes, the trainer opens a window with its log and a Copy log button. If the game closed too fast for that, the window opens at the next start.
- New: Start Cheat Engine in the FaTrainer launcher 2.5. The trainer installs Cheat Engine the first time the game starts and opens it with the game, in the game's own prefix, attached to Dying Light.
- Fixed: no survivor or Night Hunter slider on the PvP page did anything, also Melee and every dropkick and death from above slider, because the trainer never found out which side you play. It now reads your role from the level, the way the game does. In story mode, where there are no sides, the sliders apply after 5 seconds again, and the dropkick and death from above sliders never reach zombies further than a normal kick would, so E keeps kicking.
- Fixed: No reload never changed magazine sizes and still needed reloads. It changes the real magazine of every kind of item, crossbows included, and the weapon in your hands always stays fully loaded, so R switches bolts right away.
- Fixed: Infinite ammo skipped crossbow bolts.
- Fixed: a crash while loading into a game with No reload on. The trainer wrote settings while the game was still building them; it now waits until you are in the game and checks every object before it writes.
- Fixed: settings the game keeps in its own cache (movement speed, jump height and others) only changed after a respawn or a level load when you moved their slider in game. They now change right away.
- Fixed: levels you lowered on the Skills page (for example Survivor from 25 to 21) went back to the maximum with the next XP you earned or after loading the game again, and the extra XP landed in Legend. Setting a level also sets the XP to the start of that level, new Survivor XP goes to Survivor, and your level can climb again.
- Fixed: Dodge spit and the player ESP could lose the game world. The game's active level is empty in some sessions; the trainer now takes the level your character is in.
- Fixed: scrolling in the menu stopped as soon as the mouse was over a card or the page title. Cards took the wheel for themselves; now the whole page scrolls wherever the mouse is.

## 2.5 b26

Survivor Update

- Fixed: with HUD colors on, the Be The Zombie nest counter was one solid color. The grey nest icon and its colored fill had the same brightness, so both got the same color; the icon now stays grey and the fill drains over it again.
- Fixed: UV flare throw speed only knew one of the game's two UV flare items. It now covers both, and the log records how each throw runs so the rest can be fixed.
- New: Unlock every resolution (Visuals, Screen). The game's video settings list every common resolution, also sizes your monitor does not report, from the next start.
- New: Start Cheat Engine in the FaTrainer launcher 2.5. The trainer installs Cheat Engine the first time the game starts and opens it with the game, in the game's own prefix, attached to Dying Light.

## 2.5 b25

Survivor Update

- Fixed: Survivor XP went to Legend only after you lowered your Survivor level. The game keeps a separate Survivor XP record that stays at your old highest value, so new Survivor XP counted as overflow and went to Legend. It now goes to Survivor, and your level can climb again.
- New: UV flare throw speed (Combat). Throws the UV flare up to x5 faster; other throwables are not changed.

## 2.5 b24

Survivor Update

- Fixed: the game crashed when you quit to the menu after changing a Survivor level (2.5 b23). The trainer no longer writes into the profile; it makes the game's Survivor XP check on load keep your lowered level instead.
- Changed: Melee reach now changes the reach the game uses for every swing (targets and hit lines), for every weapon, instead of the weapon's damage range. Only your own swings are changed.

## 2.5 b23

Survivor Update

- Fixed: a Survivor level you lowered went back to 25 after leaving and loading the game again. The game keeps a separate Survivor XP record and raises your Survivor XP back to it on every load; lowering the level or the XP bar now lowers that record too.

## 2.5 b22

Survivor Update

- Fixed: levels you lowered in the Skills page (for example Survivor from 25 to 21) went back to the maximum with the next XP you earned, and the extra XP landed in Legend. Setting a level now also sets the XP to the start of that level.
- Fixed: Melee speed also sped up climbing, the grappling hook and picking up items. Animations are now only sped up while your melee weapon is swinging.
- Fixed: Melee reach did nothing for some weapons, like the Egyptian Khopesh and DLC weapons, because the game used a copy of the weapon the trainer had not changed. It now changes the weapon in your hand.
- New: a language for every text category on the Texts page. Next to Subtitles, Weapons and items, Skills, Quests and objectives, Tutorials and hints, HUD and messages and Menus can each have their own language, changing right away.

## 2.5 b21

Survivor Update

- Fixed: Melee speed still left a wait before or after some swings. Heavy and charged attacks (the long recovery after a held swing) ignored the speed; they now take their normal time divided by your melee speed, at every speed above x1.
- Fixed: No reload could stop after one crossbow bolt when a Night Hunter was in the session. The weapon in your hand now always gets the endless magazine, also when the game uses a copy of the weapon the trainer had not patched. The log now explains it when a weapon still has a small magazine.

## 2.5 b20

Survivor Update

- Fixed: Melee speed still left a short wait after every swing. The game times each swing phase on its own clock, and that clock now runs at your melee speed too, so at x5 you swing about twice as often as before.
- Fixed: some menu texts like Cash, Level, Power level and Survivors rescued stayed in the voice language. They were counted as spoken lines; now they follow the game language. Most texts already on screen switch right away; the first main menu after starting the game can still show some in the voice language.
- New: Always 1 bullet for Last Wish (Combat). The Last Wish revolver always has one round loaded, so every shot is its explosive last round.
- New: keys for more actions on the Keybinds page: Kill all enemies nearby, Save position, Go to saved position and Player ESP on / off, next to Refill health & stamina.
- New: callout sets on the Texts page. Make your own sets of Be The Zombie callouts, rename and delete them, and edit every line. The two built-in sets can be edited too and restored with one click.

## 2.5 b19

Survivor Update

- Fixed: Melee speed still lagged at the end of each swing. The game makes you rest for a moment after every attack; that rest is now shortened by the same speed. Melee speed goes up to x5 now.
- Fixed: Item Dupe never found the dropped item, and Dodge spit and the player ESP could lose the game world. The game's active level is empty in some sessions; the trainer now takes the level your character is in.
- Fixed: scrolling in the menu stopped as soon as the mouse was over a card or the page title. Cards took the wheel for themselves; now the whole page scrolls wherever the mouse is.
- Fixed: a crash when the HUD was rebuilt while loading into a game with HUD colors or colored callouts on.
- Removed: Show next to HUD pieces.

## 2.5 b18

Survivor Update

- Fixed: No reload kept showing [R] Reload although the bolt was loaded, and in invasions the crossbow needed R before the next shot. The weapon in your hands now always stays fully loaded, so R switches bolts right away.
- Fixed: Melee speed made some hits miss, more often the faster it was set. Only the end of each attack is sped up now, the rest of the game's attack timing is left alone.
- Fixed: the voices stayed English with Polish voices and English as the game language. The game loads its voice files in the language it starts in, so it now starts in your voice language and the trainer shows every text in your game language.
- Fixed: Hide on HUD pieces did nothing, because the game sets the opacity of most pieces again every frame. Hidden pieces now stay hidden.
- Changed: Show no longer forces a piece visible (that made the health bar flicker and drew every crosshair at once). The piece flashes in your accent color with a frame around where it sits, also when the game hides it at that moment.
- Fixed: the Health bar color also colored the medkit counter; it belongs to Medkits now.
- Fixed: callout colors showed as %COLOR(...) text in XP popups like Hunter Down +240XP. Every HUD text now understands the color codes.
- New: more Be The Zombie callouts: every way to kill in the killfeed (sliced, shot, burned, bludgeoned, blown up, run over and more), self destruct and bled out.
- Item Dupe writes what it sees to the log when an item leaves your backpack, to find out why drops are not copied yet.

## 2.5 b17

Survivor Update

- Fixed: a crash while loading into a game with No reload on. The trainer wrote settings while the game was still building them; it now waits until you are in the game and checks every object before it writes.
- Fixed: No reload still needed reloads and turned ammo packs into 999 rounds. Firearms and crossbows keep their magazine somewhere the trainer did not look; it now changes the real magazine and leaves ammo packs alone.
- Fixed: Melee speed made the weapon hang for a moment after each swing. The game's attack timing now runs at the same speed as the swing.
- Fixed: the voices stayed English with Polish voices picked and English as the game language. The game picks its voice files by the game language; the trainer now gives it your voice language there too.
- Changed: Item Dupe only copies what you really drop: it waits until the dropped item lies next to you. The copy is exact, with rarity and upgrades, and part of a stack you drop comes back too.

## 2.5 b16

Survivor Update

- New: keys for sliders on the Keybinds page. One press sets the slider to Max, the next press puts back what you had, and a message says which.

## 2.5 b15

Survivor Update

- New: the weapon editor is redesigned: big editable values in a two column grid, the game's value next to every stat you changed, Undo per stat and for everything, rarity as colored buttons, and extras that fit the item (no reload for guns and crossbows, infinite ammo, unbreakable melee weapons, infinite UV for the UV flashlight, infinite grappling hook). Flashlights and the grappling hook can be edited too.
- New: weapon edits are saved and come back every time you start the game.
- New: Item Dupe on the Combat page: weapons and other single items you drop stay in your backpack too.
- New: Throwable recharge on the PvP page: UV flares, injectors, grenades, molotovs, throwing stars and mines come back faster in Be The Zombie, at Max right away.
- Fixed: No reload never changed magazine sizes, because guns and throwables keep the magazine in different places; the trainer now finds it for every kind of item, crossbows included.
- Fixed: Infinite ammo skipped crossbow bolts.

## 2.5 b14

Survivor Update

- Fixed: Melee speed did nothing. The weapon swing value the trainer changed is not used by the game; Melee speed now speeds up your animations while you hold a melee weapon.
- Fixed: the voices stayed English with Polish picked. The game also sets the voices from its own settings; the trainer now catches that call too.
- Fixed: colored callouts showed raw tags in the killfeed. Colors now use the killfeed's own color codes, and texts you colored before are converted.
- New: more callouts: Dropkicked, From Above, Drop Attack, Clawed, Pounce Blast, Horde Hit, Horde Kill, UV Off, Toxic, Sense Off, Tripped, Headshot, Downed, Finished, Spiked, Nest Down and Hunter Down.
- New: Preview on a text and Show in the killfeed post the line into the game's real killfeed when you are in a game.
- Fixed: Show on a HUD piece now shows the whole piece, also the parts the game keeps hidden until something happens.

## 2.5 b13

Survivor Update

- Fixed: with a game language picked, the voices still followed it. The trainer now asks Steam for your language, so "Your Steam language" means the language Steam starts the game in.

## 2.5 b12

Survivor Update

- Fixed: with a game language picked, the voices switched to that language too instead of staying in your Steam language.
- Fixed: the first start after updating from 2.5 b10 did not pick up the text language you had chosen before.
- Fixed: the trainer can now tell whether you play a survivor or the Night Hunter: it reads your role from the level, the way the game does.
- The Be The Zombie callouts line under the preview was cut off.

## 2.5 b11

Survivor Update

- Fixed: Melee speed and Melee reach switched themselves off again a few seconds after a load. The trainer lost track of the weapon stats it had just changed; it now keeps them.
- Fixed: no dropkick or death from above setting did anything. The trainer never recognized the Night Hunter, so the sliders stayed off. They now always work, and they never reach zombies further than a normal kick would, so E keeps kicking in story mode.
- Fixed: some HUD colors stayed green (radar, objective distance) until you rejoined, and colors kept flickering back to the game's. The trainer now colors the HUD at the moment the game sets a color, including its own animations, and picks up HUD pieces the game creates later.
- New: Game language, Voices and Subtitles are three separate settings on the Texts page. The game language is the game's own language switch, so menus, the Night Hunter info and everything else change natively; voices and subtitles can stay in your language. The game language and voices change at the next start.
- New: Be The Zombie callouts: short killfeed lines in one click (Pounced:, Failed Pounce:, Tackled:, Failed Tackle: and more), with optional colored labels.
- New: click a text to edit it right where it is. Color all of it or just its label, and preview it.
- New: Show next to every HUD piece hides the menu for a few seconds and shows that piece, so you can see your colors in the game.
- New: Field of view on the Visuals page, live. Sprinting and aiming still zoom like before.
- New: if the game crashes, the trainer opens a window with its log and a Copy log button. If the game closed too fast for that, the window opens at the next start.
- Fixed: a crash while the game loaded its texts with a language picked: texts are now only changed once the game has finished loading them.
- All visual changes are in the Visuals Version too.

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
- New: a key to refill health and stamina. Set it under Actions on the Keybinds page and press it while the menu is closed; a message at the bottom of the screen confirms it.
- Fixed: `fatrainer.ini` kept every deleted built-in config several times when an older version had written it that way. The trainer now reads each one once.
- The log line `cheats:` now says which side the trainer thinks you play and whether it found your player's settings, to find out why survivor sliders do nothing in some games.

## 2.4 b1

Refill Update

- New: a key to refill health and stamina. Set it under Actions on the Keybinds page and press it while the menu is closed; a message at the bottom of the screen confirms it.

## 2.3

Launcher Update

- 2.3 is the last build of 2.3: it comes after 2.3 b1, b2 and b3, and those builds update to it by themselves.
- The FaTrainer Installer is now the FaTrainer launcher. Press Play: it installs or updates the trainer and starts Dying Light with it. Starting Dying Light from Steam plays the normal game without the trainer, so you can play with or without it.
- New: Keybinds page. Give any cheat its own key and press it while the menu is closed to turn the cheat on or off; a message at the bottom of the screen says which. You can change the menu key there too. A key can only belong to one thing, so picking a key takes it away from whatever had it before.
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
- Skills page: each skill tree is a tile with its level, an XP bar you can drag to set the XP, -10, -1, +1 point, +10 and Max, and the free skill points the game shows, with - and + that change them without changing the level.
- XP gain now multiplies experience in every skill tree, Survivor and Night Hunter included, and the game's XP popup shows the boosted amount. Before it only changed Agility, Power and Driver.
- Health and stamina bars on the Player page can be dragged to set them.
- New: Take less damage, as a survivor and as the Night Hunter.
- New: Less UV damage for the Night Hunter.
- New: One hit kill on the Night Hunter, on the PvP page.
- New: Sure-hit ground pound for the Night Hunter. The game only counts a survivor as hit when its blast trace reaches his body, so a step or uneven ground could make the ground pound miss even right next to him. Both Night Hunter presets turn it on.
- The Ground pound slider also raises how far above or below you a survivor can still be hit (normally 2 m).
- Presets are now Configs, on the PvP page and in Settings. Built-in configs can be deleted too, and Restore built-in configs brings them back: the four built-in ones plus your own. New config saves what is on now, and every config of yours can be loaded, saved over, renamed and deleted. Your old configs carry over.
- The Night Hunter page moved to Cheats, and the sidebar no longer shows a section twice after you reorder pages.
- The menu opens bigger, and it remembers the size you drag it to.
- Versions now have build numbers, like 2.2 b1. A newer build of your version downloads itself and starts the next time you open the game; until then the build you have keeps working. A newer version still needs the FaTrainer Installer, and an older version stays off unless you tick Use older versions.
- New: Beta builds in Settings and in the FaTrainer Installer. Off unless you turn it on. With it on, the trainer and the installer also use the beta build, the version being worked on right now, when it is newer than the latest release. Beta builds may have bugs.
- The FaTrainer Installer now has its own version (Installer 1.0) and keeps itself up to date.
- New: the FaTrainer Installer lets you pick which version to install, and its Use older versions checkbox lets an older version keep working in game after a newer one is out. The update notice still shows; without the checkbox an older version stays turned off, as before.
- The installer lists each version once and always installs its newest build.
- The trainer no longer touches your items, enemies or the Night Hunter until you are in a game, so nothing is written while the game loads from the main menu.
- When the game crashes, `fatrainer.log` now also records heap corruption and fail-fast crashes, with the chain of game functions that led there. Please attach it if the game closes on its own.
- Fixed: as the Night Hunter your melee, tackle and ground pound often did nothing, especially after starting the game with the Pounce slider already on. Pounce also widened the pounce aim to all around you. The game uses the attack for a pounce whenever a survivor counts as a pounce target, so a survivor beside or behind you took over your swipes, and the pounce then failed. Pounce now only lengthens the range, and you pounce the survivor you aim at.
- Fixed: the Night Hunter presets made the tackle miss. They widen the pounce aim, which the game shares with the tackle, so the tackle locked onto survivors beside you while charging straight ahead. The tackle now always keeps its normal aim.
- Fixed: the survivor sliders (Death from above, Dropkick, Melee and the other kicks) also changed the Night Hunter's attacks while you played the hunter, and the Night Hunter sliders changed a survivor's game. Each side's sliders now only work while you play that side.
- Fixed: Spit hit radius made a Night Hunter's spits hit you from anywhere while you played a survivor. The game that hosts the match decides spit hits, so the slider now only works while you play the Night Hunter, and only when you host.
- Fixed: Claws made your melee miss survivors next to you. It widened the range the game uses to pick a melee target, so the swipe turned toward survivors far away. Claws now only lengthens the reach.
- Fixed: the Night Hunter rank showed a number like 44890 instead of its name. Ranks now show the title and change one title at a time. In a Be The Zombie match the side you are not playing is grayed out.
- Fixed: Unbreakable weapons did not protect weapons you picked up after turning it on, until you pressed Refresh.
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
