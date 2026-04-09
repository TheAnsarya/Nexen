using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Platform;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Debugger.ViewModels;

/// <summary>
/// ViewModel for the tile editor window.
/// Provides pixel-level editing of individual tiles with palette selection and undo/redo support.
/// </summary>
public sealed partial class TileEditorViewModel : DisposableViewModel {
	/// <summary>
	/// Gets or sets the bitmap displaying the tile being edited.
	/// </summary>
	[Reactive] public partial DynamicBitmap ViewerBitmap { get; private set; }

	/// <summary>
	/// Gets or sets the array of RGB palette colors for display.
	/// </summary>
	[Reactive] public partial UInt32[] PaletteColors { get; set; } = [];

	/// <summary>
	/// Gets or sets the raw palette values for indexed formats.
	/// </summary>
	[Reactive] public partial UInt32[] RawPalette { get; set; } = [];

	/// <summary>
	/// Gets or sets the raw palette format.
	/// </summary>
	[Reactive] public partial RawPaletteFormat RawFormat { get; set; }

	/// <summary>
	/// Gets the number of columns in the palette grid.
	/// </summary>
	[Reactive] public partial int PaletteColumnCount { get; private set; } = 16;

	/// <summary>
	/// Gets or sets the currently selected color index in the palette.
	/// </summary>
	[Reactive] public partial int SelectedColor { get; set; } = 0;

	/// <summary>
	/// Gets or sets custom grid overlay definitions for the tile view.
	/// </summary>
	[Reactive] public partial List<GridDefinition>? CustomGrids { get; set; } = null;

	/// <summary>
	/// Gets the tile editor configuration settings.
	/// </summary>
	public TileEditorConfig Config { get; }

	/// <summary>
	/// Gets the menu actions for the File menu.
	/// </summary>
	public List<ContextMenuAction> FileMenuActions { get; private set; } = new();

	/// <summary>
	/// Gets the menu actions for the View menu.
	/// </summary>
	public List<ContextMenuAction> ViewMenuActions { get; private set; } = new();

	/// <summary>
	/// Gets the menu actions for the Tools menu.
	/// </summary>
	public List<ContextMenuAction> ToolsMenuActions { get; private set; } = new();

	/// <summary>
	/// Gets the toolbar button actions.
	/// </summary>
	public List<ContextMenuAction> ToolbarActions { get; private set; } = new();

	/// <summary>CPU type for the tile data.</summary>
	private CpuType _cpuType;

	/// <summary>List of memory addresses containing the tile data.</summary>
	private List<AddressInfo> _tileAddresses;

	/// <summary>Number of tile columns in the editor.</summary>
	private int _columnCount = 1;

	/// <summary>Number of tile rows in the editor.</summary>
	private int _rowCount = 1;

	/// <summary>Tile format (bit depth).</summary>
	private TileFormat _tileFormat;

	/// <summary>Pixel buffer for the tile being edited.</summary>
	private UInt32[] _tileBuffer = [];

	/// <summary>
	/// Designer-only constructor. Do not use in production code.
	/// </summary>
	[Obsolete("For designer only")]
	public TileEditorViewModel() : this(new() { new() }, 1, TileFormat.Bpp4, 0) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="TileEditorViewModel"/> class.
	/// </summary>
	/// <param name="tileAddresses">List of memory addresses for the tiles to edit.</param>
	/// <param name="columnCount">Number of tile columns.</param>
	/// <param name="format">The tile format (bit depth).</param>
	/// <param name="initialPalette">The initial palette index to select.</param>
	public TileEditorViewModel(List<AddressInfo> tileAddresses, int columnCount, TileFormat format, int initialPalette) {
		Config = ConfigManager.Config.Debug.TileEditor;
		_tileAddresses = tileAddresses;
		_columnCount = columnCount;
		_rowCount = tileAddresses.Count / columnCount;
		_cpuType = tileAddresses[0].Type.ToCpuType();
		_tileFormat = format;
		SelectedColor = initialPalette * GetColorsPerPalette(_tileFormat);

		PixelSize size = format.GetTileSize();
		_tileBuffer = new UInt32[size.Width * size.Height];
		ViewerBitmap = new DynamicBitmap(new PixelSize(size.Width * _columnCount, size.Height * _rowCount), new Vector(96, 96), PixelFormat.Bgra8888, AlphaFormat.Premul);

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(this.WhenAnyValue(x => x.Config.Background).Subscribe(x => RefreshViewer()));
		AddDisposable(this.WhenAnyValue(x => x.SelectedColor).Subscribe(x => RefreshViewer()));
		AddDisposable(this.WhenAnyValue(x => x.Config.ShowGrid).Subscribe(x => {
			if (Config.ShowGrid) {
				PixelSize tileSize = _tileFormat.GetTileSize();
				CustomGrids = new List<GridDefinition>() { new GridDefinition() {
					SizeX = tileSize.Width,
					SizeY = tileSize.Height,
					Color = Color.FromArgb(192, Colors.LightBlue.R, Colors.LightBlue.G, Colors.LightBlue.B)
				} };
			} else {
				CustomGrids = null;
			}
		}));
		AddDisposable(this.WhenAnyValue(x => x.Config.ImageScale).Subscribe(x => {
			if (Config.ImageScale < 4) {
				Config.ImageScale = 4;
			}
		}));

		RefreshViewer();
	}

	/// <summary>
	/// Initializes the menu and toolbar actions for the tile editor.
	/// </summary>
	/// <param name="picViewer">The picture viewer control.</param>
	/// <param name="wnd">The parent window.</param>
	public void InitActions(PictureViewer picViewer, Window wnd) {
		FileMenuActions = AddDisposables(new List<ContextMenuAction>() {
			new ContextMenuAction() {
				ActionType = ActionType.ExportToPng,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.SaveAsPng),
				OnClick = () => picViewer.ExportToPng()
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Exit,
				OnClick = () => wnd.Close()
			}
		});

		ViewMenuActions = AddDisposables(new List<ContextMenuAction>() {
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

		ToolsMenuActions = AddDisposables(GetTools());
		ToolbarActions = AddDisposables(GetTools());

		DebugShortcutManager.RegisterActions(wnd, FileMenuActions);
		DebugShortcutManager.RegisterActions(wnd, ViewMenuActions);
		DebugShortcutManager.RegisterActions(wnd, ToolsMenuActions);
	}

	/// <summary>
	/// Gets the list of tile transformation tool actions.
	/// </summary>
	/// <returns>List of context menu actions for tile transformations.</returns>
	private List<ContextMenuAction> GetTools() {
		return new List<ContextMenuAction>() {
			new ContextMenuAction() {
				ActionType = ActionType.FlipHorizontal,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.TileEditor_FlipHorizontal),
				OnClick = () => Transform(TransformType.FlipHorizontal)
			},
			new ContextMenuAction() {
				ActionType = ActionType.FlipVertical,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.TileEditor_FlipVertical),
				OnClick = () => Transform(TransformType.FlipVertical)
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.RotateLeft,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.TileEditor_RotateLeft),
				IsEnabled = () => _columnCount == _rowCount,
				OnClick = () => Transform(TransformType.RotateLeft)
			},
			new ContextMenuAction() {
				ActionType = ActionType.RotateRight,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.TileEditor_RotateRight),
				IsEnabled = () => _columnCount == _rowCount,
				OnClick = () => Transform(TransformType.RotateRight)
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.TranslateLeft,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.TileEditor_TranslateLeft),
				OnClick = () => Transform(TransformType.TranslateLeft)
			},
			new ContextMenuAction() {
				ActionType = ActionType.TranslateRight,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.TileEditor_TranslateRight),
				OnClick = () => Transform(TransformType.TranslateRight)
			},
			new ContextMenuAction() {
				ActionType = ActionType.TranslateUp,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.TileEditor_TranslateUp),
				OnClick = () => Transform(TransformType.TranslateUp)
			},
			new ContextMenuAction() {
				ActionType = ActionType.TranslateDown,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.TileEditor_TranslateDown),
				OnClick = () => Transform(TransformType.TranslateDown)
			},
		};
	}

	/// <summary>
	/// Gets all pixel data from the tiles as color indices.
	/// </summary>
	/// <returns>List of color indices for each pixel.</returns>
	private List<int> GetTileData() {
		PixelSize tileSize = _tileFormat.GetTileSize();
		int width = tileSize.Width * _columnCount;
		int height = tileSize.Height * _rowCount;

		List<int> data = new List<int>(width * height);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				data.Add(GetColorAtPosition(new PixelPoint(x, y)));
			}
		}

		return data;
	}

	/// <summary>
	/// Applies a transformation to all tiles (flip, rotate, or translate).
	/// </summary>
	/// <param name="type">The type of transformation to apply.</param>
	private void Transform(TransformType type) {
		List<int> data = GetTileData();

		PixelSize tileSize = _tileFormat.GetTileSize();
		int width = tileSize.Width * _columnCount;
		int height = tileSize.Height * _rowCount;
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int newY = type switch {
					TransformType.FlipVertical => height - y - 1,
					TransformType.FlipHorizontal => y,
					TransformType.RotateLeft => x,
					TransformType.RotateRight => height - x - 1,
					TransformType.TranslateLeft => y,
					TransformType.TranslateRight => y,
					TransformType.TranslateUp => y < height - 1 ? (y + 1) : 0,
					TransformType.TranslateDown => y > 0 ? (y - 1) : height - 1,
					_ => y
				};

				int newX = type switch {
					TransformType.FlipVertical => x,
					TransformType.FlipHorizontal => width - x - 1,
					TransformType.RotateLeft => width - y - 1,
					TransformType.RotateRight => y,
					TransformType.TranslateLeft => x < width - 1 ? (x + 1) : 0,
					TransformType.TranslateRight => x > 0 ? (x - 1) : width - 1,
					TransformType.TranslateUp => x,
					TransformType.TranslateDown => x,
					_ => x
				};

				SetPixelColor(new PixelPoint(x, y), data[(newY * width) + newX]);
			}
		}

		RefreshViewer();
	}

	/// <summary>Contains tile and pixel coordinates within a tile.</summary>
	/// <param name="Column">Tile column index.</param>
	/// <param name="Row">Tile row index.</param>
	/// <param name="TileX">X coordinate within the tile.</param>
	/// <param name="TileY">Y coordinate within the tile.</param>
	private record TilePixelPositionInfo(int Column, int Row, int TileX, int TileY);

	/// <summary>
	/// Converts a pixel position to tile and sub-tile coordinates.
	/// </summary>
	/// <param name="position">The pixel position.</param>
	/// <returns>Position info with tile and sub-tile coordinates.</returns>
	private TilePixelPositionInfo GetPositionInfo(PixelPoint position) {
		PixelSize tileSize = _tileFormat.GetTileSize();

		int column = position.X / tileSize.Width;
		int row = position.Y / tileSize.Height;
		int tileX = position.X % tileSize.Width;
		int tileY = position.Y % tileSize.Height;
		return new(column, row, tileX, tileY);
	}

	/// <summary>
	/// Gets the palette color index at the specified pixel position.
	/// </summary>
	/// <param name="position">The pixel position to query.</param>
	/// <returns>The color index including palette offset.</returns>
	public int GetColorAtPosition(PixelPoint position) {
		TilePixelPositionInfo pos = GetPositionInfo(position);
		int paletteColorIndex = DebugApi.GetTilePixel(_tileAddresses[pos.Column + (pos.Row * _columnCount)], _tileFormat, pos.TileX, pos.TileY);
		return SelectedColor - (SelectedColor % GetColorsPerPalette(_tileFormat)) + paletteColorIndex;
	}

	/// <summary>
	/// Sets the selected color to the color at the specified position.
	/// </summary>
	/// <param name="position">The pixel position to sample.</param>
	public void SelectColor(PixelPoint position) {
		SelectedColor = GetColorAtPosition(position);
	}

	/// <summary>
	/// Sets the color of a pixel in the tile data.
	/// </summary>
	/// <param name="position">The pixel position to modify.</param>
	/// <param name="color">The color index to set.</param>
	private void SetPixelColor(PixelPoint position, int color) {
		TilePixelPositionInfo pos = GetPositionInfo(position);
		DebugApi.SetTilePixel(_tileAddresses[pos.Column + (pos.Row * _columnCount)], _tileFormat, pos.TileX, pos.TileY, color);
	}

	/// <summary>
	/// Updates a pixel at the specified position with the selected color or clears it.
	/// </summary>
	/// <param name="position">The pixel position to update.</param>
	/// <param name="clearPixel">If true, sets the pixel to color 0; otherwise uses the selected color.</param>
	public void UpdatePixel(PixelPoint position, bool clearPixel) {
		int pixelColor = clearPixel ? 0 : SelectedColor % GetColorsPerPalette(_tileFormat);
		SetPixelColor(position, pixelColor);
		RefreshViewer();
	}

	/// <summary>
	/// Refreshes the tile viewer display from current tile data and palette.
	/// </summary>
	private unsafe void RefreshViewer() {
		Dispatcher.UIThread.Post((Action)(() => {
			DebugPaletteInfo palette = DebugApi.GetPaletteInfo(_cpuType, new GetPaletteInfoOptions() { Format = _tileFormat });
			PaletteColors = palette.GetRgbPalette();
			RawPalette = palette.GetRawPalette();
			RawFormat = palette.RawFormat;
			PaletteColumnCount = PaletteColors.Length > 16 ? 16 : 4;

			PixelSize tileSize = _tileFormat.GetTileSize();
			int bytesPerTile = _tileFormat.GetBytesPerTile();

			using (var framebuffer = ViewerBitmap.Lock()) {
				for (int y = 0; y < _rowCount; y++) {
					for (int x = 0; x < _columnCount; x++) {
						fixed (UInt32* ptr = _tileBuffer) {
							AddressInfo addr = _tileAddresses[(y * _columnCount) + x];
							byte[] sourceData = DebugApi.GetMemoryValues(addr.Type, (uint)addr.Address, (uint)(addr.Address + bytesPerTile - 1));
							DebugApi.GetTileView(_cpuType, GetOptions(x, y), sourceData, sourceData.Length, PaletteColors, (IntPtr)ptr);
							UInt32* viewer = (UInt32*)framebuffer.FrameBuffer.Address;
							int rowPitch = ViewerBitmap.PixelSize.Width;
							int baseOffset = (x * tileSize.Width) + (y * tileSize.Height * rowPitch);
							for (int j = 0; j < tileSize.Height; j++) {
								for (int i = 0; i < tileSize.Width; i++) {
									viewer[baseOffset + (j * rowPitch) + i] = ptr[(j * tileSize.Width) + i];
								}
							}
						}
					}
				}
			}
		}));
	}

	/// <summary>
	/// Gets the number of colors per palette for the current tile format.
	/// </summary>
	/// <returns>The number of colors in each palette (2, 4, 16, or 256).</returns>
	public int GetColorsPerPalette() {
		return GetColorsPerPalette(_tileFormat);
	}

	/// <summary>
	/// Gets the number of colors per palette for a given tile format.
	/// </summary>
	/// <param name="format">The tile format to query.</param>
	/// <returns>The number of colors in each palette (2, 4, 16, or 256).</returns>
	private int GetColorsPerPalette(TileFormat format) {
		return format.GetBitsPerPixel() switch {
			1 => 2, //2-color palettes
			2 => 4, //4-color palettes
			4 => 16, //16-color palettes
			_ => 256
		};
	}

	/// <summary>
	/// Creates options for rendering a tile view.
	/// </summary>
	/// <param name="column">The tile column index.</param>
	/// <param name="row">The tile row index.</param>
	/// <returns>Options configured for the specified tile.</returns>
	private GetTileViewOptions GetOptions(int column, int row) {
		int tileIndex = (row * _columnCount) + column;

		return new GetTileViewOptions() {
			MemType = _tileAddresses[tileIndex].Type,
			Format = _tileFormat,
			Width = _tileFormat.GetTileSize().Width / 8,
			Height = _tileFormat.GetTileSize().Height / 8,
			Palette = SelectedColor / GetColorsPerPalette(_tileFormat),
			Layout = TileLayout.Normal,
			Filter = TileFilter.None,
			StartAddress = _tileAddresses[tileIndex].Address,
			Background = Config.Background,
			UseGrayscalePalette = false
		};
	}

	/// <summary>
	/// Specifies tile transformation operations available in the tile editor.
	/// </summary>
	public enum TransformType {
		/// <summary>Mirrors the tile horizontally.</summary>
		FlipHorizontal,
		/// <summary>Mirrors the tile vertically.</summary>
		FlipVertical,
		/// <summary>Rotates the tile 90 degrees counter-clockwise.</summary>
		RotateLeft,
		/// <summary>Rotates the tile 90 degrees clockwise.</summary>
		RotateRight,
		/// <summary>Shifts all pixels one position to the left with wraparound.</summary>
		TranslateLeft,
		/// <summary>Shifts all pixels one position to the right with wraparound.</summary>
		TranslateRight,
		/// <summary>Shifts all pixels one position up with wraparound.</summary>
		TranslateUp,
		/// <summary>Shifts all pixels one position down with wraparound.</summary>
		TranslateDown,
	}
}
