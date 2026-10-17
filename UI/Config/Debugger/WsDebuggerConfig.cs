using System.Reactive;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Media;
using Nexen.Debugger;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class WsDebuggerConfig : ViewModelBase {
	[Reactive] public partial bool BreakOnUndefinedOpCode { get; set; } = false;
}
