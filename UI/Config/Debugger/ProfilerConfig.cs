using System.Collections.Generic;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class ProfilerConfig : BaseWindowConfig<ProfilerConfig> {
	[Reactive] public partial List<int> ColumnWidths { get; set; } = new();
	[Reactive] public partial bool AutoRefresh { get; set; } = true;
	[Reactive] public partial bool RefreshOnBreakPause { get; set; } = true;
}
