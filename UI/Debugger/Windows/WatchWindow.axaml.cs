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
public class WatchWindow : NexenWindow, INotificationHandler {
	private WatchWindowViewModel _model;

	[Obsolete("For designer only")]
	public WatchWindow() : this(new WatchWindowViewModel()) { }

	public WatchWindow(WatchWindowViewModel model) {
		InitializeComponent();
#if DEBUG
		this.AttachDevTools();
#endif

		_model = model;
		DataContext = model;

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
				Dispatcher.UIThread.Post(() => _model.UpdateAvailableTabs());
				break;

			case ConsoleNotificationType.PpuFrameDone:
				if (TopLevel.GetTopLevel(this)?.FocusManager?.GetFocusedElement() is TextBox) {
					return;
				}

				if (!ToolRefreshHelper.LimitFps(this, 20)) {
					_model.RefreshData();
				}

				break;
		}
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
