using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;

public sealed partial class GbaDebuggerConfig : ViewModelBase {
	[Reactive] public partial bool BreakOnInvalidOpCode { get; set; } = false;
	[Reactive] public partial bool BreakOnNopLoad { get; set; } = false;
	[Reactive] public partial bool BreakOnUnalignedMemAccess { get; set; } = false;

	[Reactive] public partial GbaDisassemblyMode DisassemblyMode { get; set; } = GbaDisassemblyMode.Default;
}

public enum GbaDisassemblyMode : byte {
	Default,
	Arm,
	Thumb
}
