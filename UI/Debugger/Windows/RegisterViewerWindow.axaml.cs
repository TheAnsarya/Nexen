using System;
using System.ComponentModel;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;

namespace Nexen.Debugger.Windows; 
public partial class RegisterViewerWindow : NexenWindow, INotificationHandler {
	private RegisterViewerWindowViewModel _model;

	[Obsolete("For designer only")]
	public RegisterViewerWindow() : this(new()) { }

	public RegisterViewerWindow(RegisterViewerWindowViewModel model) {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif

		_model = model;
		DataContext = model;

		if (Design.IsDesignMode) {
			return;
		}

		_model.InitMenu(this);
		_model.Config.LoadWindowSettings(this);
	}

	protected override void OnClosing(WindowClosingEventArgs e) {
		base.OnClosing(e);
		_model.Config.SaveWindowSettings(this);
		ConfigManager.Config.Debug.RegisterViewer = _model.Config;
	}

	public void ProcessNotification(NotificationEventArgs e) {
		ToolRefreshHelper.ProcessNotification(this, e, _model.RefreshTiming, _model, _model.RefreshData);
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
