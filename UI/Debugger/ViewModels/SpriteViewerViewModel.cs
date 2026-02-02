using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics.CodeAnalysis;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Layout;
using Avalonia.Media;
using Avalonia.Platform;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for the sprite viewer window.
/// Displays all sprites currently in OAM (Object Attribute Memory) with selection,
/// preview panels, and support for editing tile graphics.
/// </summary>
public class SpriteViewerViewModel : DisposableViewModel, ICpuTypeModel, IMouseOverViewerModel {
	/// <summary>
	/// Gets the configuration settings for the sprite viewer.
	/// </summary>
	public SpriteViewerConfig Config { get; }

	/// <summary>
	/// Gets the view model controlling refresh timing behavior.
	/// </summary>
	public RefreshTimingViewModel RefreshTiming { get; }

	/// <summary>
	/// Gets or sets the CPU type for this sprite viewer instance.
	/// </summary>
	public CpuType CpuType { get; set; }

	/// <summary>
	/// Gets or sets the currently selected sprite preview model.
	/// </summary>
	[Reactive] public SpritePreviewModel? SelectedSprite { get; set; }

	/// <summary>
	/// Gets or sets the tooltip panel showing selected sprite details.
	/// </summary>
	[Reactive] public DynamicTooltip? SelectedPreviewPanel { get; set; }

	/// <summary>
	/// Gets or sets the tooltip for the preview panel area.
	/// </summary>
	[Reactive] public DynamicTooltip? PreviewPanelTooltip { get; set; }

	/// <summary>
	/// Gets or sets the sprite being displayed in the preview panel tooltip.
	/// </summary>
	[Reactive] public SpritePreviewModel? PreviewPanelSprite { get; set; }

	/// <summary>
	/// Gets or sets the tooltip shown when hovering over the sprite viewer.
	/// </summary>
	[Reactive] public DynamicTooltip? ViewerTooltip { get; set; }

	/// <summary>
	/// Gets or sets the mouse position over the viewer, or null if not hovering.
	/// </summary>
	[Reactive] public PixelPoint? ViewerMousePos { get; set; }

	/// <summary>
	/// Gets or sets the bitmap displaying all sprites.
	/// </summary>
	[Reactive] public DynamicBitmap ViewerBitmap { get; private set; }

	/// <summary>
	/// Gets or sets the selection rectangle for the currently selected sprite.
	/// </summary>
	[Reactive] public Rect SelectionRect { get; set; }

	/// <summary>
	/// Gets or sets the rectangle highlighting the sprite under the mouse, or null.
	/// </summary>
	[Reactive] public Rect? MouseOverRect { get; set; }

	/// <summary>
	/// Gets or sets the top clipping region size for off-screen sprite filtering.
	/// </summary>
	[Reactive] public int TopClipSize { get; set; }

	/// <summary>
	/// Gets or sets the bottom clipping region size for off-screen sprite filtering.
	/// </summary>
	[Reactive] public int BottomClipSize { get; set; }

	/// <summary>
	/// Gets or sets the left clipping region size for off-screen sprite filtering.
	/// </summary>
	[Reactive] public int LeftClipSize { get; set; }

	/// <summary>
	/// Gets or sets the right clipping region size for off-screen sprite filtering.
	/// </summary>
	[Reactive] public int RightClipSize { get; set; }

	/// <summary>
	/// Gets or sets the list of sprite preview models for all sprites in OAM.
	/// </summary>
	[Reactive] public List<SpritePreviewModel> SpritePreviews { get; set; } = new();

	/// <summary>
	/// Gets the view model for the sprite list panel.
	/// </summary>
	public SpriteViewerListViewModel ListView { get; }

	/// <summary>
	/// Gets or sets the maximum source offset for the sprite viewer.
	/// </summary>
	[Reactive] public int MaxSourceOffset { get; set; } = 0;

	/// <summary>
	/// Gets the menu actions for the File menu.
	/// </summary>
	public List<object> FileMenuActions { get; } = new();

	/// <summary>
	/// Gets the menu actions for the View menu.
	/// </summary>
	public List<object> ViewMenuActions { get; } = new();

	/// <summary>Grid control for displaying sprite thumbnails.</summary>
	private Grid _spriteGrid;

	/// <summary>Lock object for thread-safe data updates.</summary>
	private object _updateLock = new();

	/// <summary>Sprite viewer data for UI display.</summary>
	private SpriteViewerData _data = new();

	/// <summary>Sprite viewer data from the emulator core.</summary>
	private SpriteViewerData _coreData = new();

	/// <summary>Base VRAM data for sprite tiles.</summary>
	private byte[] _baseVram = [];

	/// <summary>Base sprite RAM (OAM) data.</summary>
	private byte[] _baseSpriteRam = [];

	/// <summary>Extended VRAM data (for systems with extended VRAM).</summary>
	private byte[] _extVram = [];

	/// <summary>Extended sprite RAM data.</summary>
	private byte[] _extSpriteRam = [];

	/// <summary>Array of sprite information from the emulator.</summary>
	private DebugSpriteInfo[] _spriteList = [];

	/// <summary>Pixel data for sprite preview images.</summary>
	private UInt32[] _spritePreviews = [];

	/// <summary>Flag indicating a refresh is pending.</summary>
	private bool _refreshPending;

	/// <summary>
	/// Designer-only constructor. Do not use in production code.
	/// </summary>
	[Obsolete("For designer only")]
	public SpriteViewerViewModel() : this(CpuType.Snes, new(), new(), new(), new(), null) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="SpriteViewerViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type whose sprites to view.</param>
	/// <param name="picViewer">The picture viewer control for sprite display.</param>
	/// <param name="scrollViewer">The scroll picture viewer for panning.</param>
	/// <param name="spriteGrid">The grid control for sprite thumbnails.</param>
	/// <param name="listView">The list view control.</param>
	/// <param name="wnd">The parent window for context menus and dialogs.</param>
	public SpriteViewerViewModel(CpuType cpuType, PictureViewer picViewer, ScrollPictureViewer scrollViewer, Grid spriteGrid, Control listView, Window? wnd) {
		Config = ConfigManager.Config.Debug.SpriteViewer.Clone();

		ListView = AddDisposable(new SpriteViewerListViewModel(this));

		CpuType = cpuType;
		RefreshTiming = new RefreshTimingViewModel(Config.RefreshTiming, cpuType);
		_spriteGrid = spriteGrid;
		_spriteGrid.PointerExited += SpriteGrid_PointerExited;

		InitBitmap(256, 256);

		FileMenuActions = AddDisposables(new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.ExportToPng,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.SaveAsPng),
				OnClick = () => picViewer.ExportToPng()
			},
			new ContextMenuSeparator(),
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
			new ContextMenuAction() {
				ActionType = ActionType.ShowListView,
				IsSelected = () => Config.ShowListView,
				OnClick = () => Config.ShowListView = !Config.ShowListView
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.ZoomIn,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ZoomIn),
				OnClick = () => picViewer.ZoomIn()
			},
			new ContextMenuAction() {
				ActionType = ActionType.ZoomOut,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ZoomOut),
				OnClick = () => picViewer.ZoomOut()
			},
		});

		if (Design.IsDesignMode || wnd == null) {
			return;
		}

		AddDisposables(DebugShortcutManager.CreateContextMenu(picViewer, scrollViewer, new List<object> {
			GetEditTileAction(wnd),
			GetViewInMemoryViewerAction(),
			GetViewInTileViewerAction(),
			GetCopyHdPackFormatActionSeparator(),
			GetCopyHdPackFormatAction()
		}));

		AddDisposables(DebugShortcutManager.CreateContextMenu(_spriteGrid, new List<object> {
			GetEditTileAction(wnd),
			GetViewInMemoryViewerAction(),
			GetViewInTileViewerAction(),
			GetCopyHdPackFormatActionSeparator(),
			GetCopyHdPackFormatAction()
		}));

		AddDisposables(DebugShortcutManager.CreateContextMenu(listView, new List<object> {
			GetEditTileAction(wnd),
			GetViewInMemoryViewerAction(),
			GetViewInTileViewerAction(),
			GetCopyHdPackFormatActionSeparator(),
			GetCopyHdPackFormatAction()
		}));

		AddDisposable(this.WhenAnyValue(x => x.SelectedSprite).Subscribe(x => {
			UpdateSelectionPreview();
			if (x != null) {
				ListView.SelectSprite(x.SpriteIndex);
			}
		}));

		AddDisposable(this.WhenAnyValue(x => x.ViewerMousePos, x => x.PreviewPanelSprite).Subscribe(x => UpdateMouseOverRect()));

		AddDisposable(this.WhenAnyValue(x => x.Config.Source, x => x.Config.SourceOffset).Subscribe(x => RefreshData()));

		AddDisposable(this.WhenAnyValue(x => x.Config.ShowOffscreenRegions).Subscribe(x => RefreshTab()));

		ListView.InitListViewObservers();

		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, Config_PropertyChanged));

		DebugShortcutManager.RegisterActions(wnd, FileMenuActions);
		DebugShortcutManager.RegisterActions(wnd, ViewMenuActions);
	}

	private ContextMenuAction GetEditTileAction(Window wnd) {
		return new ContextMenuAction() {
			ActionType = ActionType.EditSprite,
			Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.SpriteViewer_EditSprite),
			IsEnabled = () => GetSelectedSprite() != null,
			OnClick = () => {
				SpritePreviewModel? sprite = GetSelectedSprite();
				if (sprite?.TileAddress >= 0 && _data.Palette != null) {
					PixelSize size = sprite.Format.GetTileSize();
					DebugPaletteInfo pal = _data.Palette.Value;
					int paletteOffset = (int)(pal.SpritePaletteOffset / pal.ColorsPerPalette);
					TileEditorWindow.OpenAtTile(
						sprite.TileAddresses.Select(x => new AddressInfo() { Address = (int)x, Type = CpuType.GetVramMemoryType(sprite.UseExtendedVram) }).ToList(),
						sprite.RealWidth / size.Width,
						sprite.Format,
						sprite.Palette + paletteOffset,
						wnd,
						CpuType,
						RefreshTiming.Config.RefreshScanline,
						RefreshTiming.Config.RefreshCycle
					);
				}
			}
		};
	}

	private ContextMenuAction GetViewInMemoryViewerAction() {
		return new ContextMenuAction() {
			ActionType = ActionType.ViewInMemoryViewer,
			Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.SpriteViewer_ViewInMemoryViewer),
			IsEnabled = () => GetSelectedSprite() != null,
			OnClick = () => {
				SpritePreviewModel? sprite = GetSelectedSprite();
				if (sprite?.TileAddress >= 0) {
					MemoryToolsWindow.ShowInMemoryTools(CpuType.GetVramMemoryType(sprite.UseExtendedVram), sprite.TileAddress);
				}
			}
		};
	}

	private ContextMenuAction GetViewInTileViewerAction() {
		return new ContextMenuAction() {
			ActionType = ActionType.ViewInTileViewer,
			Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.SpriteViewer_ViewInTileViewer),
			OnClick = () => {
				SpritePreviewModel? sprite = GetSelectedSprite();
				if (sprite?.TileAddress >= 0 && _data.Palette != null) {
					DebugPaletteInfo pal = _data.Palette.Value;
					int paletteOffset = (int)(pal.SpritePaletteOffset / pal.ColorsPerPalette);
					TileViewerWindow.OpenAtTile(CpuType, CpuType.GetVramMemoryType(sprite.UseExtendedVram), sprite.TileAddress, sprite.Format, TileLayout.Normal, sprite.Palette + paletteOffset);
				}
			}
		};
	}

	private ContextMenuAction GetCopyHdPackFormatActionSeparator() {
		return new ContextMenuSeparator() { IsVisible = () => CpuType == CpuType.Nes };
	}

	private ContextMenuAction GetCopyHdPackFormatAction() {
		return new ContextMenuAction() {
			ActionType = ActionType.CopyToHdPackFormat,
			IsVisible = () => CpuType == CpuType.Nes,
			IsEnabled = () => {
				SpritePreviewModel? sprite = GetSelectedSprite();
				return sprite != null && HdPackCopyHelper.IsActionAllowed(CpuType.GetVramMemoryType(sprite.UseExtendedVram));
			},
			OnClick = () => {
				SpritePreviewModel? sprite = GetSelectedSprite();
				if (sprite != null && sprite.TileAddress >= 0 && _data.Palette != null) {
					HdPackCopyHelper.CopyToHdPackFormat(sprite.TileAddress, CpuType.GetVramMemoryType(sprite.UseExtendedVram), _data.Palette.Value.GetRawPalette(), sprite.Palette, true, sprite.Height > 8);
				}
			}
		};
	}

	private SpritePreviewModel? GetSelectedSprite() {
		return PreviewPanelSprite ?? SelectedSprite;
	}

	private void Config_PropertyChanged(object? sender, PropertyChangedEventArgs e) {
		RefreshTab();
	}

	public DynamicTooltip? GetPreviewPanel(PixelPoint p, DynamicTooltip? tooltipToUpdate) {
		SpritePreviewModel? sprite = GetMatchingSprite(p, out _);
		return sprite == null ? null : GetPreviewPanel(sprite, tooltipToUpdate);
	}

	public DynamicTooltip? GetPreviewPanel(SpritePreviewModel sprite, DynamicTooltip? existingTooltip) {
		if (_data.Palette == null) {
			return null;
		}

		TooltipEntries entries = existingTooltip?.Items ?? new();
		entries.StartUpdate();
		if (sprite.SpritePreview != null) {
			entries.AddPicture("Sprite", sprite.SpritePreview, 48.0 / sprite.Width);
		}

		DebugPaletteInfo palette = _data.Palette.Value;
		int paletteSize = (int)Math.Pow(2, sprite.Bpp);
		int paletteIndex = (sprite.Palette >= 0 ? sprite.Palette : 0) + (int)(palette.SpritePaletteOffset / paletteSize);
		if (sprite.Bpp is > 1 and < 8) {
			entries.AddEntry("Palette", new TooltipPaletteEntry(paletteIndex, paletteSize, palette.GetRgbPalette(), palette.GetRawPalette(), palette.RawFormat));
		}

		entries.AddEntry("Sprite index", sprite.SpriteIndex.ToString());
		entries.AddEntry("X, Y",
			"$" + sprite.RawX.ToString("X2") + ", $" + sprite.RawY.ToString("X2") + Environment.NewLine +
			sprite.X + ", " + sprite.Y
		);
		entries.AddEntry("Size", sprite.Width + "x" + sprite.Height);

		entries.AddSeparator("TileSeparator");

		entries.AddEntry("Tile index", "$" + sprite.TileIndex.ToString("X2"));

		MemoryType memType = CpuType.GetVramMemoryType(sprite.UseExtendedVram);
		if (memType.IsRelativeMemory()) {
			entries.AddEntry("Tile address (" + memType.GetShortName() + ")", FormatAddress(sprite.TileAddress, memType));

			AddressInfo absAddress = DebugApi.GetAbsoluteAddress(new AddressInfo() { Address = sprite.TileAddress, Type = memType });
			if (absAddress.Address >= 0) {
				entries.AddEntry("Tile address (" + absAddress.Type.GetShortName() + ")", FormatAddress(absAddress.Address, absAddress.Type));
			}
		} else {
			entries.AddEntry("Tile address", FormatAddress(sprite.TileAddress, memType));
		}

		entries.AddSeparator("PaletteSeparator");

		entries.AddEntry("Palette index", sprite.Palette.ToString());
		if (sprite.PaletteAddress >= 0) {
			entries.AddEntry("Palette address", "$" + sprite.PaletteAddress.ToString("X2"));
		}

		entries.AddSeparator("MiscSeparator");

		entries.AddEntry("Visibility", ResourceHelper.GetEnumText(sprite.Visibility));
		if (sprite.Mode != DebugSpriteMode.Undefined) {
			entries.AddEntry("Mode", sprite.Mode);
		}

		entries.AddEntry("Horizontal mirror", sprite.HorizontalMirror);
		entries.AddEntry("Vertical mirror", sprite.VerticalMirror);
		if (sprite.TransformEnabled != NullableBoolean.Undefined) {
			entries.AddEntry("Transform", sprite.TransformEnabled);
			if (sprite.TransformParamIndex >= 0) {
				entries.AddEntry("Transform Param Index", sprite.TransformParamIndex);
			}
		}

		entries.AddEntry("Mosaic", sprite.MosaicEnabled);
		entries.AddEntry("Second table", sprite.UseSecondTable);

		if (sprite.Priority != DebugSpritePriority.Undefined) {
			entries.AddEntry("Priority", ResourceHelper.GetEnumText(sprite.Priority));
		}

		entries.EndUpdate();

		if (existingTooltip != null) {
			return existingTooltip;
		} else {
			return new DynamicTooltip() { Items = entries };
		}
	}

	private string FormatAddress(int address, MemoryType memType) {
		if (memType.IsWordAddressing()) {
			return $"${address / 2:X4}.w";
		} else {
			return $"${address:X4}";
		}
	}

	[MemberNotNull(nameof(ViewerBitmap))]
	private void InitBitmap(int width, int height) {
		if (ViewerBitmap == null || ViewerBitmap.PixelSize.Width != width || ViewerBitmap.PixelSize.Height != height) {
			ViewerBitmap = new DynamicBitmap(new PixelSize(width, height), new Vector(96, 96), PixelFormat.Bgra8888, AlphaFormat.Premul);
		}
	}

	private void InitGrid(int spriteCount) {
		double dpiScale = LayoutHelper.GetLayoutScale(_spriteGrid);
		if (_spriteGrid.Children.Count != spriteCount) {
			_spriteGrid.Children.Clear();
			_spriteGrid.ColumnDefinitions.Clear();
			_spriteGrid.RowDefinitions.Clear();
			for (int i = 0; i < 8; i++) {
				_spriteGrid.ColumnDefinitions.Add(new() { Width = new GridLength(1, GridUnitType.Auto) });
			}

			for (int i = 0; i < spriteCount / 8; i++) {
				_spriteGrid.RowDefinitions.Add(new() { Height = new GridLength(1, GridUnitType.Auto) });
			}

			for (int i = 0; i < spriteCount; i++) {
				SpritePreviewPanel preview = new SpritePreviewPanel(SpritePreviews[i], Config);
				preview.InnerHeight = 32.0 / dpiScale;
				preview.InnerWidth = 32.0 / dpiScale;
				Grid.SetColumn(preview, i % 8);
				Grid.SetRow(preview, i / 8);

				preview.DataContext = SpritePreviews[i];
				preview.PointerPressed += SpritePreview_PointerPressed;
				preview.PointerMoved += SpritePreview_PointerMoved;
				_spriteGrid.Children.Add(preview);
			}
		}
	}

	private void SpriteGrid_PointerExited(object? sender, PointerEventArgs e) {
		TooltipHelper.HideTooltip(_spriteGrid);
		PreviewPanelTooltip = null;
		PreviewPanelSprite = null;
	}

	private void SpritePreview_PointerMoved(object? sender, PointerEventArgs e) {
		if (sender is Control ctrl && ctrl.DataContext is SpritePreviewModel sprite) {
			PreviewPanelTooltip = GetPreviewPanel(sprite, PreviewPanelTooltip);
			PreviewPanelSprite = sprite;

			TooltipHelper.ShowTooltip(_spriteGrid, PreviewPanelTooltip, 15);
		}
	}

	private void SpritePreview_PointerPressed(object? sender, PointerPressedEventArgs e) {
		if (sender is Control ctrl && ctrl.DataContext is SpritePreviewModel sprite) {
			SelectedSprite = sprite;
			UpdateSelection(sprite);
		}
	}

	public void SelectSprite(int spriteIndex) {
		SelectedSprite = SpritePreviews[spriteIndex];
		UpdateSelection(SelectedSprite);
	}

	private void InitPreviews(DebugSpriteInfo[] sprites, UInt32[] spritePreviews, DebugSpritePreviewInfo previewInfo) {
		if (SpritePreviews.Count != sprites.Length) {
			List<SpritePreviewModel> previews = new();
			for (int i = 0; i < sprites.Length; i++) {
				SpritePreviewModel model = new SpritePreviewModel();
				model.Init(ref sprites[i], spritePreviews, previewInfo);
				previews.Add(model);
			}

			SpritePreviews = previews;
		} else {
			for (int i = 0; i < sprites.Length; i++) {
				SpritePreviews[i].Init(ref sprites[i], spritePreviews, previewInfo);
			}
		}

		InitGrid(sprites.Length);
	}

	private void UpdateRamData(ref byte[] baseRam, ref byte[] extRam, ref byte[] combinedRam, MemoryType baseType, MemoryType extType) {
		if (baseType == MemoryType.None) {
			Array.Resize(ref baseRam, 0);
			Array.Resize(ref extRam, 0);
			Array.Resize(ref combinedRam, 0);
		} else if (extType != baseType) {
			DebugApi.GetMemoryState(extType, ref extRam);
			if (extRam.Length > 0) {
				DebugApi.GetMemoryState(baseType, ref baseRam);

				Array.Resize(ref combinedRam, extRam.Length + baseRam.Length);
				Array.Copy(baseRam, 0, combinedRam, 0, baseRam.Length);
				Array.Copy(extRam, 0, combinedRam, baseRam.Length, extRam.Length);
			} else {
				DebugApi.GetMemoryState(baseType, ref combinedRam);
			}
		} else {
			Array.Resize(ref extRam, 0);
			DebugApi.GetMemoryState(baseType, ref combinedRam);
		}
	}

	private void UpdateVramData() {
		UpdateRamData(ref _baseVram, ref _extVram, ref _coreData.Vram, CpuType.GetVramMemoryType(), CpuType.GetVramMemoryType(getExtendedRam: true));
	}

	private void UpdateSpriteRamData() {
		UpdateRamData(ref _baseSpriteRam, ref _extSpriteRam, ref _coreData.SpriteRam, CpuType.GetSpriteRamMemoryType(), CpuType.GetSpriteRamMemoryType(getExtendedRam: true));
	}

	public void RefreshData() {
		lock (_updateLock) {
			_coreData.PpuState = DebugApi.GetPpuState(CpuType);
			_coreData.PpuToolsState = DebugApi.GetPpuToolsState(CpuType);

			UpdateVramData();

			int spriteRamSize;
			MemoryType spriteRamType = CpuType.GetSpriteRamMemoryType();
			if (spriteRamType == MemoryType.None) {
				//SMS-specific
				spriteRamSize = 0x100;
			} else {
				spriteRamSize = DebugApi.GetMemorySize(spriteRamType);
				MemoryType spriteRamExtType = CpuType.GetSpriteRamMemoryType(getExtendedRam: true);
				if (spriteRamType != spriteRamExtType) {
					spriteRamSize += DebugApi.GetMemorySize(spriteRamExtType);
				}
			}

			if (Config.Source == SpriteViewerSource.SpriteRam) {
				UpdateSpriteRamData();
			} else {
				MemoryType cpuMemory = CpuType.ToMemoryType();
				DebugApi.GetMemoryValues(cpuMemory, (uint)Config.SourceOffset, (uint)(Config.SourceOffset + spriteRamSize - 1), ref _coreData.SpriteRam);

				Dispatcher.UIThread.Post(() => MaxSourceOffset = Math.Max(0, DebugApi.GetMemorySize(cpuMemory) - spriteRamSize));
			}

			DebugPaletteInfo palette = DebugApi.GetPaletteInfo(CpuType);
			_coreData.Palette = palette.ColorCount > 0 ? palette : null;
		}

		RefreshTab();
	}

	private void RefreshTab() {
		if (_refreshPending) {
			return;
		}

		_refreshPending = true;
		Dispatcher.UIThread.Post(() => {
			InternalRefreshTab();
			_refreshPending = false;
		});
	}

	private void InternalRefreshTab() {
		if (Disposed) {
			return;
		}

		lock (_updateLock) {
			_coreData.CopyTo(_data);
		}

		if (_data.PpuState == null || _data.Palette == null || _data.PpuToolsState == null) {
			return;
		}

		GetSpritePreviewOptions options = new GetSpritePreviewOptions() {
			Background = Config.Background
		};

		DebugSpritePreviewInfo previewInfo = DebugApi.GetSpritePreviewInfo(CpuType, options, _data.PpuState, _data.PpuToolsState);
		InitBitmap((int)previewInfo.Width, (int)previewInfo.Height);

		UInt32[] palette = _data.Palette.Value.GetRgbPalette();

		LeftClipSize = Config.ShowOffscreenRegions ? 0 : (int)previewInfo.VisibleX;
		RightClipSize = Config.ShowOffscreenRegions ? 0 : (int)(previewInfo.Width - (previewInfo.VisibleWidth + previewInfo.VisibleX));
		TopClipSize = Config.ShowOffscreenRegions ? 0 : (int)previewInfo.VisibleY;
		BottomClipSize = Config.ShowOffscreenRegions ? 0 : (int)(previewInfo.Height - (previewInfo.VisibleHeight + previewInfo.VisibleY));

		using (var framebuffer = ViewerBitmap.Lock(true)) {
			DebugApi.GetSpriteList(ref _spriteList, ref _spritePreviews, CpuType, options, _data.PpuState, _data.PpuToolsState, _data.Vram, _data.SpriteRam, palette, framebuffer.FrameBuffer.Address);
		}

		InitPreviews(_spriteList, _spritePreviews, previewInfo);

		if (Config.ShowOutline) {
			List<Rect> spriteRects = new List<Rect>();
			foreach (SpritePreviewModel sprite in SpritePreviews) {
				(Rect mainRect, Rect alt1, Rect alt2, Rect alt3) = sprite.GetPreviewRect();
				spriteRects.Add(mainRect);
				if (alt1 != default) {
					spriteRects.Add(alt1);
				}

				if (alt2 != default) {
					spriteRects.Add(alt2);
				}

				if (alt3 != default) {
					spriteRects.Add(alt3);
				}
			}

			ViewerBitmap.HighlightRects = spriteRects;
		} else {
			ViewerBitmap.HighlightRects = null;
		}

		ViewerBitmap.Invalidate();

		int selectedIndex = SelectedSprite?.SpriteIndex ?? -1;
		if (selectedIndex >= 0 && selectedIndex < SpritePreviews.Count) {
			SelectedSprite = SpritePreviews[selectedIndex];
			UpdateSelectionPreview();
		} else {
			SelectedSprite = null;
		}

		ListView.RefreshList();

		UpdateTooltips();
		UpdateSelection(SelectedSprite);

		UpdateMouseOverRect();
	}

	private void UpdateSelectionPreview() {
		SelectedPreviewPanel = SelectedSprite != null ? GetPreviewPanel(SelectedSprite, SelectedPreviewPanel) : null;
	}

	private void UpdateMouseOverRect() {
		MouseOverRect = PreviewPanelSprite != null
			? PreviewPanelSprite.GetPreviewRect().Item1
			: (Rect?)(ViewerMousePos != null && GetMatchingSprite(ViewerMousePos.Value, out Rect matchingRect) is SpritePreviewModel sprite
				? matchingRect
				: null);
	}

	private void UpdateTooltips() {
		if (PreviewPanelSprite != null && PreviewPanelTooltip != null) {
			GetPreviewPanel(PreviewPanelSprite, PreviewPanelTooltip);
		} else if (ViewerMousePos != null && ViewerTooltip != null) {
			SpritePreviewModel? sprite = GetMatchingSprite(ViewerMousePos.Value, out _);
			if (sprite != null) {
				GetPreviewPanel(sprite, ViewerTooltip);
			}
		}
	}

	public void UpdateSelection(SpritePreviewModel? sprite) {
		SelectionRect = sprite != null ? sprite.GetPreviewRect().Item1 : default;
	}

	public SpritePreviewModel? GetMatchingSprite(PixelPoint p, out Rect matchingRect) {
		Point point = p.ToPoint(1);
		for (int i = SpritePreviews.Count - 1; i >= 0; i--) {
			SpritePreviewModel sprite = SpritePreviews[i];
			(Rect mainRect, Rect alt1, Rect alt2, Rect alt3) = sprite.GetPreviewRect();
			if (mainRect.Contains(point)) {
				matchingRect = mainRect;
				return sprite;
			} else if (alt1.Contains(point)) {
				matchingRect = alt1;
				return sprite;
			} else if (alt2.Contains(point)) {
				matchingRect = alt2;
				return sprite;
			} else if (alt3.Contains(point)) {
				matchingRect = alt3;
				return sprite;
			}
		}

		matchingRect = default;
		return null;
	}

	public void OnGameLoaded() {
		RefreshData();
	}
}

public class SpriteViewerData {
	public BaseState? PpuState;
	public BaseState? PpuToolsState;
	public byte[] SpriteRam = Array.Empty<byte>();
	public byte[] Vram = Array.Empty<byte>();
	public DebugPaletteInfo? Palette = null;

	public void CopyTo(SpriteViewerData dst) {
		dst.PpuState = PpuState;
		dst.Palette = Palette;
		dst.PpuToolsState = PpuToolsState;
		CopyArray(SpriteRam, ref dst.SpriteRam);
		CopyArray(Vram, ref dst.Vram);
	}

	private void CopyArray<T>(T[] src, ref T[] dst) {
		if (src.Length != dst.Length) {
			Array.Resize(ref dst, src.Length);
		}

		Array.Copy(src, dst, src.Length);
	}
}
