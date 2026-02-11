using System;
using System.ComponentModel;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Debugger.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;

namespace Nexen.Debugger.Windows;
public class SpriteViewerWindow : NexenWindow, INotificationHandler {
	private SpriteViewerViewModel _model;

	[Obsolete("For designer only")]
	public SpriteViewerWindow() : this(CpuType.Snes) { }

	public SpriteViewerWindow(CpuType cpuType) {
		InitializeComponent();
#if DEBUG
		this.AttachDevTools();
#endif

		ScrollPictureViewer scrollViewer = this.GetControl<ScrollPictureViewer>("picViewer");
		PictureViewer picViewer = scrollViewer.InnerViewer;
		Grid spriteGrid = this.GetControl<Grid>("spriteGrid");
		var listView = this.GetControl<DataGrid>("ListView");
		_model = new SpriteViewerViewModel(cpuType, picViewer, scrollViewer, spriteGrid, listView, this);
		DataContext = _model;

		_model.Config.LoadWindowSettings(this);

		if (Design.IsDesignMode) {
			return;
		}

		MouseViewerModelEvents.InitEvents(_model, this, picViewer);
		picViewer.PositionClicked += PicViewer_PositionClicked;
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);
		if (Design.IsDesignMode) {
			return;
		}

		_model.RefreshData();
	}

	protected override void OnClosing(WindowClosingEventArgs e) {
		base.OnClosing(e);
		_model.Config.SaveWindowSettings(this);
		ConfigManager.Config.Debug.SpriteViewer = _model.Config;
	}

	private void PicViewer_PositionClicked(object? sender, PositionClickedEventArgs e) {
		PixelPoint p = e.Position;
		SpritePreviewModel? sprite = _model.GetMatchingSprite(p, out _);
		_model.SelectedSprite = sprite;
		_model.UpdateSelection(sprite);
		e.Handled = true;
	}

	private void OnSettingsClick(object sender, RoutedEventArgs e) {
		_model.Config.ShowSettingsPanel = !_model.Config.ShowSettingsPanel;
	}

	public void ProcessNotification(NotificationEventArgs e) {
		if (e.NotificationType is ConsoleNotificationType.CodeBreak or ConsoleNotificationType.StateLoaded) {
			_model.ListView.ForceRefresh();
		}

		ToolRefreshHelper.ProcessNotification(this, e, _model.RefreshTiming, _model, _model.RefreshData);
	}
}
