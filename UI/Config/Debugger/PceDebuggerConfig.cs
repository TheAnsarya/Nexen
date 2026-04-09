using System.Reactive;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Media;
using Nexen.Debugger;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class PceDebuggerConfig : ViewModelBase {
	[Reactive] public partial bool BreakOnBrk { get; set; } = false;
	[Reactive] public partial bool BreakOnUnofficialOpCode { get; set; } = false;
	[Reactive] public partial bool BreakOnInvalidVramAddress { get; set; } = false;
}
