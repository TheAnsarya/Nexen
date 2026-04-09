using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Nexen.Config;
using Nexen.Utilities;
using Nexen.ViewModels;
using Nexen.Windows;

namespace Nexen.Debugger.Views; 
public partial class ControllerListView : UserControl {
	public ControllerListView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
