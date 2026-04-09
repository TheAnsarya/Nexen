using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config.Shortcuts; 
public sealed partial class ShortcutKeyInfo : ReactiveObject {
	[Reactive] public partial EmulatorShortcut Shortcut { get; set; }
	[Reactive] public partial KeyCombination KeyCombination { get; set; } = new KeyCombination();
	[Reactive] public partial KeyCombination KeyCombination2 { get; set; } = new KeyCombination();
}
