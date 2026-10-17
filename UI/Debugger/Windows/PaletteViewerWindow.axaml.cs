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
using Nexen.Utilities;

namespace Nexen.Debugger.Windows;
public partial class PaletteViewerWindow : NexenWindow, INotificationHandler {
	private PaletteViewerViewModel _model;
	private PaletteSelector _palSelector;

	[Obsolete("For designer only")]
	public PaletteViewerWindow() : this(CpuType.Snes) { }

	public PaletteViewerWindow(CpuType cpuType) {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif

		PaletteSelector palSelector = this.GetControl<PaletteSelector>("palSelector");
		Border border = this.GetControl<Border>("selectorBorder");
		_palSelector = palSelector;
		_model = new PaletteViewerViewModel(cpuType);
		_model.InitActions(this, palSelector, border);
		DataContext = _model;

		_model.Config.LoadWindowSettings(this);

		if (Design.IsDesignMode) {
			return;
		}

		palSelector.PointerMoved += PalSelector_PointerMoved;
		palSelector.PointerExited += PalSelector_PointerExited;
		PointerWheelChanged += Window_PointerWheelChanged;
	}

	private void Window_PointerWheelChanged(object? sender, PointerWheelEventArgs e) {
		if (e.KeyModifiers == KeyModifiers.Control) {
			double delta = e.GetDeltaY();
			if (delta > 0) {
				_model.ZoomIn();
			} else if (delta < 0) {
				_model.ZoomOut();
			}

			e.Handled = true;
		}
	}

	private void PalSelector_PointerExited(object? sender, Avalonia.Input.PointerEventArgs e) {
		if (sender is PaletteSelector viewer) {
			TooltipHelper.HideTooltip(viewer);
		}

		_model.ViewerTooltip = null;
		_model.ViewerMouseOverPalette = -1;
	}

	private void PalSelector_PointerMoved(object? sender, Avalonia.Input.PointerEventArgs e) {
		if (sender is PaletteSelector viewer) {
			int index = viewer.GetPaletteIndexFromPoint(e.GetCurrentPoint(viewer).Position);
			if (_model.ViewerMouseOverPalette == index) {
				return;
			}

			_model.ViewerTooltip = _model.GetPreviewPanel(index, _model.ViewerTooltip);
			_model.ViewerMouseOverPalette = index;

			if (_model.ViewerTooltip != null) {
				TooltipHelper.ShowTooltip(viewer, _model.ViewerTooltip, 15);
			} else {
				TooltipHelper.HideTooltip(viewer);
			}
		}
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
		ConfigManager.Config.Debug.PaletteViewer = _model.Config;
		_palSelector.PointerMoved -= PalSelector_PointerMoved;
		_palSelector.PointerExited -= PalSelector_PointerExited;
		PointerWheelChanged -= Window_PointerWheelChanged;
	}

	private void OnSettingsClick(object sender, RoutedEventArgs e) {
		_model.Config.ShowSettingsPanel = !_model.Config.ShowSettingsPanel;
	}

	public void ProcessNotification(NotificationEventArgs e) {
		ToolRefreshHelper.ProcessNotification(this, e, _model.RefreshTiming, _model, _model.RefreshData);
	}

	protected override void OnKeyDown(KeyEventArgs e) {
		base.OnKeyDown(e);
		if (FocusManager?.GetFocusedElement() is Border) {
			//Only process keys if the border around the scrollviewer is focused
			_palSelector.ProcessKeyDown(e);
		}
	}
}
