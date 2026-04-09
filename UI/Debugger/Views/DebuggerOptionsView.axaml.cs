using System.Collections.Generic;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Data.Converters;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Nexen.Config;
using Nexen.Interop;

namespace Nexen.Debugger.Views; 
public partial class DebuggerOptionsView : UserControl {
	public DebuggerOptionsView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
