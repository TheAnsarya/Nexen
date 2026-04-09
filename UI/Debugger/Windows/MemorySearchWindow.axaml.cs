using System;
using System.ComponentModel;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Nexen.Debugger.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;

namespace Nexen.Debugger.Windows;
public partial class MemorySearchWindow : NexenWindow, INotificationHandler {
	private MemorySearchViewModel _model;

	public MemorySearchWindow() {
		_model = new MemorySearchViewModel();
		DataContext = _model;

		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif

		if (Design.IsDesignMode) {
			return;
		}

		_model.Config.LoadWindowSettings(this);
	}

	protected override void OnClosing(WindowClosingEventArgs e) {
		base.OnClosing(e);
		_model.Config.SaveWindowSettings(this);
	}

	public void ProcessNotification(NotificationEventArgs e) {
		switch (e.NotificationType) {
			case ConsoleNotificationType.GameLoaded:
				Dispatcher.UIThread.Post(() => _model.OnGameLoaded());
				break;

			case ConsoleNotificationType.PpuFrameDone:
				if (!ToolRefreshHelper.LimitFps(this, 30)) {
					_model.RefreshData(false);
				}

				break;

			case ConsoleNotificationType.CodeBreak:
				_model.RefreshData(true);
				break;
		}
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
