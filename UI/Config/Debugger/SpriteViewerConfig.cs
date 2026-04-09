using System.Collections.Generic;
using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class SpriteViewerConfig : BaseWindowConfig<SpriteViewerConfig> {
	[Reactive] public partial bool ShowSettingsPanel { get; set; } = true;

	[Reactive] public partial bool ShowOutline { get; set; } = false;
	[Reactive] public partial bool ShowOffscreenRegions { get; set; } = false;
	[Reactive] public partial SpriteBackground Background { get; set; } = SpriteBackground.Gray;

	[Reactive] public partial SpriteViewerSource Source { get; set; } = SpriteViewerSource.SpriteRam;
	[Reactive] public partial int SourceOffset { get; set; } = 0;

	[Reactive] public partial bool DimOffscreenSprites { get; set; } = true;
	[Reactive] public partial bool ShowListView { get; set; } = false;
	[Reactive] public partial double ListViewHeight { get; set; } = 100;
	[Reactive] public partial List<int> ColumnWidths { get; set; } = new();

	[Reactive] public partial double ImageScale { get; set; } = 2;
	[Reactive] public partial RefreshTimingConfig RefreshTiming { get; set; } = new();

	public SpriteViewerConfig() {
	}
}

public enum SpriteViewerSource {
	SpriteRam,
	CpuMemory
}
