using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Controls;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.Windows; 
public partial class NesHeaderEditWindow : NexenWindow {
	NesHeaderEditViewModel _model;

	public NesHeaderEditWindow() {
		_model = new NesHeaderEditViewModel();
		DataContext = _model;

		InitializeComponent();

#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private async void Ok_OnClick(object sender, RoutedEventArgs e) {
		if (await _model.Save(this)) {
			Close();
		}
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		Close();
	}
}
