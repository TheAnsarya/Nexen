using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class TileViewerConfig : BaseWindowConfig<TileViewerConfig> {
	[Reactive] public partial bool ShowSettingsPanel { get; set; } = true;

	[Reactive] public partial double ImageScale { get; set; } = 3;
	[Reactive] public partial bool ShowTileGrid { get; set; } = false;

	[Reactive] public partial string SelectedPreset { get; set; } = "PPU";

	[Reactive] public partial MemoryType Source { get; set; }
	[Reactive] public partial TileFormat Format { get; set; } = TileFormat.Bpp4;
	[Reactive] public partial TileLayout Layout { get; set; } = TileLayout.Normal;
	[Reactive] public partial TileFilter Filter { get; set; } = TileFilter.None;
	[Reactive] public partial TileBackground Background { get; set; } = TileBackground.Default;
	[Reactive] public partial int RowCount { get; set; } = 64;
	[Reactive] public partial int ColumnCount { get; set; } = 32;
	[Reactive] public partial int StartAddress { get; set; } = 0;
	[Reactive] public partial bool UseGrayscalePalette { get; set; } = false;

	[Reactive] public partial RefreshTimingConfig RefreshTiming { get; set; } = new();

	public TileViewerConfig() {
	}
}
