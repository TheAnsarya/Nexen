using System.Collections.Generic;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class RegisterViewerConfig : BaseWindowConfig<RegisterViewerConfig> {
	[Reactive] public partial RefreshTimingConfig RefreshTiming { get; set; } = new();
	[Reactive] public partial List<int> ColumnWidths { get; set; } = new();
}
