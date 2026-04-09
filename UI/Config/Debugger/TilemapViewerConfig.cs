using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class TilemapViewerConfig : BaseWindowConfig<TilemapViewerConfig> {
	[Reactive] public partial bool ShowSettingsPanel { get; set; } = true;

	[Reactive] public partial double ImageScale { get; set; } = 1;

	[Reactive] public partial bool ShowGrid { get; set; }
	[Reactive] public partial bool ShowScrollOverlay { get; set; }

	[Reactive] public partial bool NesShowAttributeGrid { get; set; }
	[Reactive] public partial bool NesShowAttributeByteGrid { get; set; }
	[Reactive] public partial bool NesShowTilemapGrid { get; set; }

	[Reactive] public partial TilemapHighlightMode TileHighlightMode { get; set; } = TilemapHighlightMode.None;
	[Reactive] public partial TilemapHighlightMode AttributeHighlightMode { get; set; } = TilemapHighlightMode.None;
	[Reactive] public partial TilemapDisplayMode DisplayMode { get; set; } = TilemapDisplayMode.Default;

	[Reactive] public partial RefreshTimingConfig RefreshTiming { get; set; } = new();

	public TilemapViewerConfig() {
	}
}
