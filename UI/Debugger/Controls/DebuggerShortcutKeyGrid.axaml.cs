using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Utilities;

namespace Nexen.Debugger.Controls; 
public partial class DebuggerShortcutKeyGrid : UserControl {
	public static readonly StyledProperty<List<DebuggerShortcutInfo>> ShortcutsProperty = AvaloniaProperty.Register<DebuggerShortcutKeyGrid, List<DebuggerShortcutInfo>>(nameof(Shortcuts));

	public List<DebuggerShortcutInfo> Shortcuts {
		get { return GetValue(ShortcutsProperty); }
		set { SetValue(ShortcutsProperty, value); }
	}

	public DebuggerShortcutKeyGrid() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
