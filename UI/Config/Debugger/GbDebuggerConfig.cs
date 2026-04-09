using System.Reactive;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Media;
using Nexen.Debugger;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class GbDebuggerConfig : ViewModelBase {
	[Reactive] public partial bool GbBreakOnInvalidOamAccess { get; set; } = false;
	[Reactive] public partial bool GbBreakOnInvalidVramAccess { get; set; } = false;
	[Reactive] public partial bool GbBreakOnDisableLcdOutsideVblank { get; set; } = false;
	[Reactive] public partial bool GbBreakOnInvalidOpCode { get; set; } = false;
	[Reactive] public partial bool GbBreakOnNopLoad { get; set; } = false;
	[Reactive] public partial bool GbBreakOnOamCorruption { get; set; } = false;
}
