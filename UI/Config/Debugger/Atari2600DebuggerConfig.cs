using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;
public sealed partial class Atari2600DebuggerConfig : ViewModelBase {
	[Reactive] public partial bool BreakOnBrk { get; set; } = false;
}
