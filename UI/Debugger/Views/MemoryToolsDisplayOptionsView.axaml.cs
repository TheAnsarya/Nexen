using System.Collections.Generic;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Nexen.Config;

namespace Nexen.Debugger.Views; 
public partial class MemoryToolsDisplayOptionsView : UserControl {
	public MemoryToolsDisplayOptionsView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
