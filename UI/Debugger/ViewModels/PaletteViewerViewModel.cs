using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for the palette viewer window.
/// Displays and allows editing of color palettes used by the emulated system's PPU.
/// </summary>
public sealed class PaletteViewerViewModel : DisposableViewModel, ICpuTypeModel {
	/// <summary>
	/// Gets or sets the CPU type for this palette viewer instance.
	/// </summary>
	public CpuType CpuType { get; set; }

	/// <summary>
	/// Gets the configuration settings for the palette viewer.
	/// </summary>
	public PaletteViewerConfig Config { get; }

	/// <summary>
	/// Gets the view model controlling refresh timing behavior.
	/// </summary>
	public RefreshTimingViewModel RefreshTiming { get; }

	/// <summary>
	/// Gets or sets the array of RGB palette colors for display.
	/// </summary>
	[Reactive] public UInt32[] PaletteColors { get; set; } = [];

	/// <summary>
	/// Gets or sets the raw palette values (for indexed palette formats).
	/// Null for non-indexed formats like RGB555.
	/// </summary>
	[Reactive] public UInt32[]? PaletteValues { get; set; } = null;

	/// <summary>
	/// Gets the number of columns to display in the palette grid (colors per row/palette).
	/// </summary>
	[Reactive] public int PaletteColumnCount { get; private set; } = 16;

	/// <summary>
	/// Gets or sets the tooltip panel showing selected color details.
	/// </summary>
	[Reactive] public DynamicTooltip? PreviewPanel { get; private set; }

	/// <summary>
	/// Gets or sets the index of the currently selected palette entry.
	/// </summary>
	[Reactive] public int SelectedPalette { get; set; } = 0;

	/// <summary>
	/// Gets or sets the display block size for each palette entry in pixels.
	/// </summary>
	[Reactive] public int BlockSize { get; set; } = 8;

	/// <summary>
	/// Gets or sets the tooltip shown when hovering over the palette viewer.
	/// </summary>
	[Reactive] public DynamicTooltip? ViewerTooltip { get; set; }

	/// <summary>
	/// Gets or sets the palette index currently under the mouse pointer, or -1 if none.
	/// </summary>
	[Reactive] public int ViewerMouseOverPalette { get; set; } = -1;

	/// <summary>
	/// Gets or sets the menu actions for the File menu.
	/// </summary>
	[Reactive] public List<object> FileMenuActions { get; private set; } = new();

	/// <summary>
	/// Gets or sets the menu actions for the View menu.
	/// </summary>
	[Reactive] public List<object> ViewMenuActions { get; private set; } = new();

	/// <summary>
	/// Cached palette information from the emulator core.
	/// </summary>
	private RefStruct<DebugPaletteInfo>? _palette = null;

	/// <summary>
	/// Designer-only constructor. Do not use in production code.
	/// </summary>
	[Obsolete("For designer only")]
	public PaletteViewerViewModel() : this(CpuType.Snes) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="PaletteViewerViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type whose palette to view.</param>
	public PaletteViewerViewModel(CpuType cpuType) {
		Config = ConfigManager.Config.Debug.PaletteViewer.Clone();
		CpuType = cpuType;
		RefreshTiming = new RefreshTimingViewModel(Config.RefreshTiming, cpuType);

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(this.WhenAnyValue(x => x.Config.Zoom).Subscribe(x => BlockSize = Math.Max(16, 16 + ((x - 1) * 4))));
		AddDisposable(this.WhenAnyValue(x => x.SelectedPalette).Subscribe(x => UpdatePreviewPanel()));
	}

	/// <summary>
	/// Initializes menu actions and context menus for the palette viewer window.
	/// </summary>
	/// <param name="wnd">The parent window.</param>
	/// <param name="palSelector">The palette selector control.</param>
	/// <param name="selectorBorder">The border around the selector for context menu attachment.</param>
	public void InitActions(Window wnd, PaletteSelector palSelector, Border selectorBorder) {
		FileMenuActions = AddDisposables(new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Exit,
				OnClick = () => wnd?.Close()
			}
		});

		ViewMenuActions = AddDisposables(new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Refresh,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.Refresh),
				OnClick = () => RefreshData()
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.EnableAutoRefresh,
				IsSelected = () => Config.RefreshTiming.AutoRefresh,
				OnClick = () => Config.RefreshTiming.AutoRefresh = !Config.RefreshTiming.AutoRefresh
			},
			new ContextMenuAction() {
				ActionType = ActionType.RefreshOnBreakPause,
				IsSelected = () => Config.RefreshTiming.RefreshOnBreakPause,
				OnClick = () => Config.RefreshTiming.RefreshOnBreakPause = !Config.RefreshTiming.RefreshOnBreakPause
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.ShowSettingsPanel,
				Shortcut =  () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ToggleSettingsPanel),
				IsSelected = () => Config.ShowSettingsPanel,
				OnClick = () => Config.ShowSettingsPanel = !Config.ShowSettingsPanel
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.ZoomIn,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ZoomIn),
				OnClick = ZoomIn
			},
			new ContextMenuAction() {
				ActionType = ActionType.ZoomOut,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ZoomOut),
				OnClick = ZoomOut
			},
		});

		DebugShortcutManager.RegisterActions(wnd, FileMenuActions);
		DebugShortcutManager.RegisterActions(wnd, ViewMenuActions);

		AddDisposables(DebugShortcutManager.CreateContextMenu(palSelector, selectorBorder, new List<object> {
			new ContextMenuAction() {
				ActionType = ActionType.EditColor,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.PaletteViewer_EditColor),
				IsEnabled = () => SelectedPalette >= 0,
				OnClick = () => Dispatcher.UIThread.Post(() => EditColor(wnd, SelectedPalette))             },
			new ContextMenuSeparator() { IsVisible = () => _palette is not null && _palette.Get().HasMemType },
			new ContextMenuAction() {
				ActionType = ActionType.ViewInMemoryViewer,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.PaletteViewer_ViewInMemoryViewer),
				IsVisible = () => _palette is not null && _palette.Get().HasMemType,
				IsEnabled = () => SelectedPalette >= 0,
				OnClick = () => {
					if(_palette is not null) {
						DebugPaletteInfo pal = _palette.Get();
						int memSize = DebugApi.GetMemorySize(pal.PaletteMemType) - (int)pal.PaletteMemOffset;
						if(memSize > 0) {
							int bytesPerColor = memSize / (int)pal.ColorCount;
							MemoryToolsWindow.ShowInMemoryTools(pal.PaletteMemType, (int)pal.PaletteMemOffset + (SelectedPalette * bytesPerColor));
						}
					}
				}
			},
		}));
	}

	/// <summary>
	/// Opens a color editor dialog for the specified palette entry.
	/// </summary>
	/// <param name="wnd">The parent window for the dialog.</param>
	/// <param name="selectedPalette">The palette index to edit.</param>
	/// <remarks>
	/// For RGB formats (555, 444), shows a standard color picker.
	/// For indexed formats, shows a color index picker with the platform's color palette.
	/// </remarks>
	private async void EditColor(Window wnd, int selectedPalette) {
		if (_palette is null) {
			return;
		}

		DebugPaletteInfo palette = _palette.Get();
		if (selectedPalette < 0 || selectedPalette >= palette.ColorCount) {
			return;
		}

		if (palette.RawFormat is RawPaletteFormat.Rgb555 or RawPaletteFormat.Rgb444 or RawPaletteFormat.Bgr444) {
			ColorPickerViewModel model = new ColorPickerViewModel() { Color = Color.FromUInt32(palette.GetRgbPalette()[selectedPalette]) };
			ColorPickerWindow colorPicker = new ColorPickerWindow() { DataContext = model };
			bool success = await colorPicker.ShowCenteredDialog<bool>(wnd);
			if (success) {
				DebugApi.SetPaletteColor(CpuType, selectedPalette, model.Color.ToUInt32());
				RefreshData();
			}
		} else {
			//Show palette and let user pick a color
			ColorIndexPickerWindow colorPicker = new ColorIndexPickerWindow(CpuType, selectedPalette);
			int? colorIndex = await colorPicker.ShowCenteredDialog<int?>(wnd);
			if (colorIndex.HasValue) {
				DebugApi.SetPaletteColor(CpuType, selectedPalette, (uint)colorIndex.Value);
				RefreshData();
			}
		}
	}

	/// <summary>
	/// Decreases the zoom level of the palette display.
	/// </summary>
	public void ZoomOut() {
		Config.Zoom = Math.Min(20, Math.Max(1, Config.Zoom - 1));
	}

	/// <summary>
	/// Increases the zoom level of the palette display.
	/// </summary>
	public void ZoomIn() {
		Config.Zoom = Math.Min(20, Math.Max(1, Config.Zoom + 1));
	}

	/// <summary>
	/// Refreshes palette data from the emulator core and updates the display.
	/// </summary>
	public void RefreshData() {
		DebugPaletteInfo paletteInfo = DebugApi.GetPaletteInfo(CpuType);
		uint[] paletteColors = paletteInfo.GetRgbPalette();
		uint[]? paletteValues = paletteInfo.RawFormat == RawPaletteFormat.Indexed ? paletteInfo.GetRawPalette() : null;
		int paletteColumnCount = (int)paletteInfo.ColorsPerPalette;

		Dispatcher.UIThread.Post(() => {
			PaletteColors = paletteColors;
			PaletteValues = paletteValues;
			PaletteColumnCount = paletteColumnCount;
			_palette = new(paletteInfo);

			UpdatePreviewPanel();
		});
	}

	/// <summary>
	/// Updates the preview panel to reflect the currently selected or hovered palette entry.
	/// </summary>
	private void UpdatePreviewPanel() {
		PreviewPanel = GetPreviewPanel(SelectedPalette, PreviewPanel);

		if (ViewerTooltip is not null && ViewerMouseOverPalette >= 0) {
			GetPreviewPanel(ViewerMouseOverPalette, ViewerTooltip);
		}
	}

	/// <summary>
	/// Gets or updates a preview panel showing detailed information about a palette entry.
	/// </summary>
	/// <param name="index">The palette index to display information for.</param>
	/// <param name="tooltipToUpdate">An existing tooltip to update, or null to create a new one.</param>
	/// <returns>The updated or newly created tooltip, or null if palette data is unavailable.</returns>
	public DynamicTooltip? GetPreviewPanel(int index, DynamicTooltip? tooltipToUpdate) {
		if (_palette is null) {
			return null;
		}

		DebugPaletteInfo palette = _palette.Get();
		if (index >= palette.ColorCount) {
			return null;
		}

		UInt32[] rgbPalette = palette.GetRgbPalette();
		UInt32[] rawPalette = palette.GetRawPalette();

		return PaletteHelper.GetPreviewPanel(rgbPalette, rawPalette, palette.RawFormat, index, tooltipToUpdate);
	}

	/// <summary>
	/// Called when a game is loaded. Refreshes palette data for the new game.
	/// </summary>
	public void OnGameLoaded() {
		RefreshData();
	}
}
