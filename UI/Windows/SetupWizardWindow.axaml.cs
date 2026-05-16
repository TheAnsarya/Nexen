using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Platform;
using Nexen.Config;
using Nexen.ViewModels;

namespace Nexen.Windows;
public partial class SetupWizardWindow : NexenWindow {
	private SetupWizardViewModel _model;
	private bool _setupCompleted;

	public SetupWizardWindow() {
		_model = new SetupWizardViewModel();
		DataContext = _model;
		InitializeComponent();

#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);
		SetupWizardMetricsStore.RecordLaunch(_model.HasResumedDraft);

		//Manually center the window (Avalonia on Linux doesn't seem to
		//work with "CenterScreen" as startup location
		Screen? screen = Screens.ScreenFromVisual(this);
		if (screen != null) {
			PixelPoint pos = new PixelPoint(
				(int)((screen.Bounds.Width - Bounds.Width) / 2),
				(int)((screen.Bounds.Height - Bounds.Height) / 2)
			);
			Position = pos;
		}
	}

	private void btnOk_OnClick(object? sender, RoutedEventArgs e) {
		if (_model.Confirm(this)) {
			SetupWizardMetricsStore.RecordCompletion(_model.GetBacktrackCount());
			_setupCompleted = true;
			Close();
		}
	}

	private void Cancel_OnClick(object? sender, RoutedEventArgs e) {
		Close();
	}

	protected override void OnClosing(WindowClosingEventArgs e) {
		if (!_setupCompleted) {
			SetupWizardMetricsStore.RecordCancel(_model.GetBacktrackCount());
			_model.SaveResumeState();
		}

		base.OnClosing(e);
	}

	private void ResetStorageToRecommended_OnClick(object? sender, RoutedEventArgs e) {
		_model.ResetStorageToRecommended();
	}

	private void StartFresh_OnClick(object? sender, RoutedEventArgs e) {
		SetupWizardMetricsStore.RecordStartFresh();
		_model.DiscardResumeStateAndReset();
	}
}
