using System;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class HistoryViewerConfig : BaseWindowConfig<HistoryViewerConfig> {
	[Reactive] public partial int Volume { get; set; } = 100;
}
