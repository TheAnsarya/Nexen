using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class RefreshTimingConfig : BaseConfig<RefreshTimingConfig> {
	[Reactive] public partial bool RefreshOnBreakPause { get; set; } = true;
	[Reactive] public partial bool AutoRefresh { get; set; } = true;

	[Reactive] public partial int RefreshScanline { get; set; } = 240;
	[Reactive] public partial int RefreshCycle { get; set; } = 0;

	public RefreshTimingConfig() {
	}
}
