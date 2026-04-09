using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class SnesDebuggerConfig : ViewModelBase {
	[Reactive] public partial bool BreakOnBrk { get; set; } = false;
	[Reactive] public partial bool BreakOnCop { get; set; } = false;
	[Reactive] public partial bool BreakOnWdm { get; set; } = false;
	[Reactive] public partial bool BreakOnStp { get; set; } = false;
	[Reactive] public partial bool BreakOnInvalidPpuAccess { get; set; } = false;
	[Reactive] public partial bool BreakOnReadDuringAutoJoy { get; set; } = false;

	[Reactive] public partial bool SpcBreakOnBrk { get; set; } = false;
	[Reactive] public partial bool SpcBreakOnStpSleep { get; set; } = false;

	[Reactive] public partial bool UseAltSpcOpNames { get; set; } = false;
	[Reactive] public partial bool IgnoreDspReadWrites { get; set; } = true;
}
