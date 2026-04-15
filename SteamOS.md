# Nexen on SteamOS / Steam Deck

This guide covers running Nexen on SteamOS in both Desktop Mode and Game Mode.

## Download

Use official Nexen release assets from:

- https://github.com/TheAnsarya/Nexen/releases

For Steam Deck, use one of these Linux assets:

- `Nexen-Linux-x64-vX.Y.Z.AppImage`
- `Nexen-Linux-ARM64-vX.Y.Z.AppImage` (if using ARM64 Linux targets)

## Setup in Desktop Mode

1. Download the AppImage from the Nexen releases page.
2. Mark it executable:
	- File Manager -> Right click AppImage -> Properties -> Permissions -> Is executable.
3. Start Nexen once in Desktop Mode.
4. Complete the initial controller/input setup in Nexen.

## Add Nexen to Steam

1. In Steam (Desktop Mode), choose Add a Non-Steam Game.
2. Select the Nexen AppImage.
3. Rename the shortcut to "Nexen" and set artwork/icon if desired.

## Game Mode Notes

Gamescope can affect popup-heavy UI flows for Avalonia applications.
If UI dialogs are hard to access in Game Mode, use one of these approaches:

- Configure settings in Desktop Mode first.
- Bind fallback keys to back buttons (L4/R4/L5/R5) through Steam Input.

Recommended Steam Input bindings:

- `Ctrl+O`: Open ROM file picker.
- `Esc`: Pause/resume menu flow.
- `F11`: Toggle fullscreen.

## Audio Troubleshooting

If you get no audio output:

1. Open Nexen audio settings.
2. Confirm an output device is selected.
3. Re-test in both Desktop Mode and Game Mode.

## Per-Game Steam Shortcuts

To create game-specific entries:

1. Duplicate the Nexen non-Steam shortcut.
2. Open shortcut Properties -> Launch Options.
3. Pass ROM path first, then optional flags.

Example:

```text
"/home/deck/roms/nes/SuperMarioBros.nes" --fullscreen
```

## Related Docs

- [Compiling](COMPILING.md)
- [Release Guide](docs/RELEASE.md)
