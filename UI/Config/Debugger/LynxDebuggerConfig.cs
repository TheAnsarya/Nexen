using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;
public sealed partial class LynxDebuggerConfig : ViewModelBase {
	[Reactive] public partial bool BreakOnBrk { get; set; } = false;
}
