using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class PaletteViewerConfig : BaseWindowConfig<PaletteViewerConfig> {
	[Reactive] public partial bool ShowSettingsPanel { get; set; } = true;
	[Reactive] public partial bool ShowPaletteIndexes { get; set; } = false;
	[Reactive] public partial int Zoom { get; set; } = 3;

	[Reactive] public partial RefreshTimingConfig RefreshTiming { get; set; } = new();

	public PaletteViewerConfig() {
	}
}
