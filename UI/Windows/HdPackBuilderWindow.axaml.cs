using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Nexen.ViewModels;

namespace Nexen.Windows; 
public partial class HdPackBuilderWindow : NexenWindow {
	private HdPackBuilderViewModel _model;

	public HdPackBuilderWindow() {
		_model = new HdPackBuilderViewModel();
		DataContext = _model;

		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	protected override void OnClosing(WindowClosingEventArgs e) {
		base.OnClosing(e);
		_model.StopRecording();
		_model.Dispose();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
