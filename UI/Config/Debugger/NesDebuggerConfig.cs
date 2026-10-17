using System.Reactive;
using System.Reactive.Linq;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class NesDebuggerConfig : ViewModelBase {
	[Reactive] public partial bool BreakOnBrk { get; set; } = false;
	[Reactive] public partial bool BreakOnUnofficialOpCode { get; set; } = false;
	[Reactive] public partial bool BreakOnUnstableOpCode { get; set; } = true;
	[Reactive] public partial bool BreakOnCpuCrash { get; set; } = true;

	[Reactive] public partial bool BreakOnBusConflict { get; set; } = false;
	[Reactive] public partial bool BreakOnDecayedOamRead { get; set; } = false;
	[Reactive] public partial bool BreakOnPpuScrollGlitch { get; set; } = false;
	[Reactive] public partial bool BreakOnExtOutputMode { get; set; } = true;
	[Reactive] public partial bool BreakOnInvalidVramAccess { get; set; } = false;
	[Reactive] public partial bool BreakOnInvalidOamWrite { get; set; } = false;
	[Reactive] public partial bool BreakOnDmaInputRead { get; set; } = false;
}
