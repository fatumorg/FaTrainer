# FaTrainer launcher

## Goal

The FaTrainer Installer becomes a launcher named FaTrainer. Pressing Play starts Dying Light with the trainer. Starting the game normally through Steam gives a clean game without the trainer, so people can play with or without it.

## Trainer DLL: armed proxy

- `xinput1_3.dll` stays in the game folder, but it starts the trainer only when the launcher armed it.
- Arming: the launcher writes `fatrainer.launch` next to `DyingLightGame.exe` right before it starts the game. The file content is the Unix time it was written.
- In `DllMain` (process attach) the DLL reads `fatrainer.launch` from its own folder. The file counts when it exists and its time is at most 180 s before now and at most 30 s after now (clock skew). The DLL then deletes it and starts `main_thread` as today.
- Without a valid file the DLL only forwards the xinput exports to the system `xinput1_3.dll`: no hooks, no update check, no network, no menu. It writes one log line `launch: not started from FaTrainer, trainer off`.
- A stale file is deleted too.
- `FATRAINER_ARMED=1` in the environment also arms it, for the preview and the self test.
- The freshness rule is a pure function `launch_armed(written, now)` in `src/version.h` (shared by the DLL, launcher and self test) and checked by the self test.
- Arming starts with trainer 2.3 (`FIRST_ARMED_VERSION`). For an older installed trainer Play starts the game without waiting for the file.

## Launcher (the installer reworked)

- Name: `FaTrainer` in window titles, the Windows class name, the VERSIONINFO description, the Linux program name and every message. The edition (`Fatum Version`) stays in the sidebar like today.
- Main button Play. One job:
  1. If the DLL in the game folder is not the newest version on the channel (latest release, or the beta when Beta builds is on and newer) download it, check size and SHA-256 and install it with the existing install code, which also backs up a foreign `xinput1_3.dll`.
  2. Write `fatrainer.launch`.
  3. Start Steam with `steam://rungameid/239140` (existing `launch_game`).
  4. Watch the file for up to 180 s. When the DLL deletes it, the status says "Running. Press Insert or F8 in game." When it is still there after 180 s, delete it and say the game did not load the trainer. On Linux the message adds that the Steam launch option is needed.
- Play is disabled while the game folder is not found, while the release check runs or failed with nothing installed, and while the launcher updates itself. When offline and an arming-capable DLL is installed, Play still works with the installed one.
- Kept: game folder detection and picker, Beta builds checkbox, changelog tab, announcements, mandatory self update, the Linux Steam launch option card, the stats row (installed, latest, runs on).
- Remove from game (formerly Uninstall) stays as a quiet button: removes the DLL and restores `xinput1_3.dll.before-fatrainer`.
- Removed: the version picker (`installable_releases` list in the UI; older trainers would run on every Steam start), the Use older versions checkbox (the trainer's own Settings switch stays), the separate Install and Launch Dying Light buttons.
- Shortcut card on both platforms:
  - Linux: `fatrainer.desktop` with `Name=FaTrainer`, icon `fatrainer.png`. The old `fatrainer-installer.desktop` and its icon are removed when the shortcut is written.
  - Windows: `FaTrainer.lnk` on the Desktop and in the Start menu Programs folder through `IShellLinkW`, pointing at the running exe.

## Release and migration

- `INSTALLER_VERSION` becomes `2.0`. The release tag stays `installer` and the version files stay `installer-windows.txt` / `installer-linux.txt`, so installers 1.1 to 1.5 find the update.
- Assets: `FaTrainer.exe`, `FaTrainer` (Linux), `FaTrainer-Linux.tar.gz`, plus the same binaries under the old names `FaTrainer-Fatum-Version-Installer.exe`, `FaTrainer-Fatum-Version-Installer` and `FaTrainer-Fatum-Version-Installer-Linux.tar.gz`. Old installers download the old name and replace their own file, which keeps its old file name. The new launcher's `INSTALLER_ASSET` is the new name.
- `build_installer.sh` writes `dist/FaTrainer.exe` and `dist/FaTrainer`.
- Trainer 2.3 final (renamed Launcher Update) and launcher 2.0 are published together.

## Website and docs

- `site/dying-light/index.html`, `docs.html`: download links to the new asset names, install steps become "download FaTrainer, open it, press Play", the installer description becomes the launcher, a note that a normal Steam start has no trainer.
- `legal.html`, `prawne.html`: rename the installer to the FaTrainer launcher where it is named; the network requests stay the same. The trainer no longer contacts GitHub when started without the launcher.
- `CLAUDE.md`, `installer/CHANGELOG.md`, `CHANGELOG.md`, `PROGRESS.md`.

## Testing

- Self test: `launch_armed` boundaries (fresh, 180 s, older, future within and beyond 30 s).
- Build the DLL and both launchers; run the self test.
- Preview the launcher (Linux build under Xvfb) for the new main view.
- Live test in the user's game only after asking, with the installed DLL backed up and restored.
