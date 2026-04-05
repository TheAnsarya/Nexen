using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Config;
public sealed class Atari2600DebuggerConfig : ViewModelBase {
	[Reactive] public bool BreakOnBrk { get; set; } = false;
}
