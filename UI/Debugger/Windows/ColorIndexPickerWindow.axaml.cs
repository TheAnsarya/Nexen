using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Controls;
using Nexen.Interop;
using Nexen.Utilities;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.Windows;
public class ColorIndexPickerWindow : NexenWindow {
	public UInt32[] Palette { get; set; } = [];
	public int SelectedPalette { get; set; }
	public int BlockSize { get; set; } = 24;
	public int ColumnCount { get; set; } = 16;

	[Obsolete("For designer only")]
	public ColorIndexPickerWindow() : this(CpuType.Nes, 0) { }

	public ColorIndexPickerWindow(CpuType cpuType, int selectedPalette) {
		SelectedPalette = selectedPalette;
		switch (cpuType) {
			case CpuType.Nes:
				Palette = ConfigManager.Config.Nes.UserPalette;
				break;

			case CpuType.Pce:
				Palette = ConfigManager.Config.PcEngine.Palette;
				break;

			case CpuType.Gameboy:
				Palette = selectedPalette < 4
					? ConfigManager.Config.Gameboy.BgColors
					: selectedPalette < 8 ? ConfigManager.Config.Gameboy.Obj0Colors : ConfigManager.Config.Gameboy.Obj1Colors;

				ColumnCount = 4;
				SelectedPalette = selectedPalette % 4;
				break;

			case CpuType.Sms:
				Palette = GenerateSmsPalette();
				ColumnCount = 8;
				break;

			case CpuType.Ws:
				Palette = GenerateWsPalette();
				ColumnCount = 8;
				break;

			case CpuType.Lynx:
				Palette = GenerateLynxPalette();
				ColumnCount = 16;
				break;

			case CpuType.Atari2600:
				Palette = GenerateAtari2600Palette();
				ColumnCount = 16;
				break;

			default:
				throw new NotImplementedException();
		}

		InitializeComponent();
#if DEBUG
		this.AttachDevTools();
#endif
	}

	private static UInt32[] GenerateWsPalette() {
		UInt32[] pal = new UInt32[8];
		WsState state = DebugApi.GetConsoleState<WsState>(ConsoleType.Ws);
		for (int i = 0; i < 8; i++) {
			int b = state.Ppu.BwShades[i] ^ 0x0F;
			pal[i] = 0xFF000000 | (UInt32)(b | (b << 4) | (b << 8) | (b << 12) | (b << 16) | (b << 20) | (b << 24));
		}

		return pal;
	}

	private static UInt32[] GenerateSmsPalette() {
		UInt32[] pal = new UInt32[0x40];
		for (int i = 0; i < 0x40; i++) {
			pal[i] = Rgb222ToArgb((byte)i);
		}

		return pal;
	}

	private static UInt32[] GenerateLynxPalette() {
		// Lynx has 16 palette entries, read from current state
		LynxState state = DebugApi.GetConsoleState<LynxState>(ConsoleType.Lynx);
		UInt32[] pal = new UInt32[16];
		for (int i = 0; i < 16; i++) {
			pal[i] = state.Mikey.Palette[i] | 0xFF000000;
		}

		return pal;
	}

	private static UInt32[] GenerateAtari2600Palette() {
		// Standard Stella NTSC TIA palette: 128 colors (16 hues × 8 luminance)
		// Source: Stella emulator standard NTSC palette
		ReadOnlySpan<uint> ntsc = [
			// Hue 0  — Gray
			0x000000, 0x404040, 0x6c6c6c, 0x909090, 0xb0b0b0, 0xc8c8c8, 0xdcdcdc, 0xf4f4f4,
			// Hue 1  — Gold
			0x444400, 0x646410, 0x848424, 0xa0a034, 0xb8b840, 0xd0d050, 0xe8e85c, 0xfcfc68,
			// Hue 2  — Orange
			0x702800, 0x844414, 0x985c28, 0xac783c, 0xbc8c4c, 0xcca05c, 0xdcb468, 0xecc878,
			// Hue 3  — Red-Orange
			0x841800, 0x983418, 0xac5030, 0xc06848, 0xd0805c, 0xe09470, 0xeca880, 0xfcbc94,
			// Hue 4  — Red
			0x880000, 0x9c2020, 0xb03c3c, 0xc05858, 0xd07070, 0xe08888, 0xeca0a0, 0xfcb4b4,
			// Hue 5  — Purple-Red
			0x78005c, 0x8c2074, 0xa03c88, 0xb0589c, 0xc070b0, 0xd084c0, 0xdc9cd0, 0xecb0e0,
			// Hue 6  — Purple
			0x480078, 0x602090, 0x783ca4, 0x8c58b8, 0xa070cc, 0xb484dc, 0xc49cec, 0xd4b0fc,
			// Hue 7  — Blue-Purple
			0x140084, 0x302098, 0x4c3cac, 0x6858c0, 0x7c70d0, 0x9488e0, 0xa8a0ec, 0xbcb4fc,
			// Hue 8  — Blue
			0x000088, 0x1c209c, 0x3840b0, 0x505cc0, 0x6874d0, 0x7c8ce0, 0x90a4ec, 0xa4b8fc,
			// Hue 9  — Light Blue
			0x00187c, 0x1c3890, 0x3854a8, 0x5070bc, 0x6888cc, 0x7c9cdc, 0x90b4ec, 0xa4c8fc,
			// Hue 10 — Cyan
			0x002c5c, 0x1c4c78, 0x386890, 0x5084ac, 0x689cc0, 0x7cb4d4, 0x90cce8, 0xa4e0fc,
			// Hue 11 — Teal
			0x003c2c, 0x1c5c48, 0x387c64, 0x509c80, 0x68b494, 0x7cd0ac, 0x90e4c0, 0xa4fcd4,
			// Hue 12 — Green
			0x003c00, 0x205c20, 0x407c40, 0x5c9c5c, 0x74b474, 0x8cd08c, 0xa4e4a4, 0xb8fcb8,
			// Hue 13 — Yellow-Green
			0x143800, 0x345c1c, 0x507c38, 0x6c9850, 0x84b468, 0x9ccc7c, 0xb4e490, 0xc8fca4,
			// Hue 14 — Yellow-Green (warm)
			0x2c3000, 0x4c501c, 0x687034, 0x848c4c, 0x9ca864, 0xb4c078, 0xccd488, 0xe0ec9c,
			// Hue 15 — Dark Yellow
			0x442800, 0x644818, 0x846830, 0xa08444, 0xb89c58, 0xd0b46c, 0xe8cc7c, 0xfce08c,
		];

		UInt32[] pal = new UInt32[128];
		for (int i = 0; i < 128; i++) {
			pal[i] = 0xff000000 | ntsc[i];
		}

		return pal;
	}

	private static byte Rgb222To8Bit(byte color) {
		return (byte)((byte)(color << 6) | (byte)(color << 4) | (byte)(color << 2) | color);
	}

	private static UInt32 Rgb222ToArgb(byte rgb222) {
		byte b = Rgb222To8Bit((byte)(rgb222 >> 4));
		byte g = Rgb222To8Bit((byte)((rgb222 >> 2) & 0x3));
		byte r = Rgb222To8Bit((byte)(rgb222 & 0x3));

		return 0xFF000000 | (UInt32)(r << 16) | (UInt32)(g << 8) | b;
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);
		if (Owner != null) {
			WindowExtensions.CenterWindow(this, Owner);
		}
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		Close(null!);
	}

	private void PaletteColor_OnClick(object sender, PaletteSelector.ColorClickEventArgs e) {
		Close(e.ColorIndex);
	}
}
