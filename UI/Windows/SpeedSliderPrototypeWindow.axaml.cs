using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Windows;

/// <summary>
/// Prototype dialog for slider-based speed interaction requested in Epic 21 (#1410).
/// This intentionally uses explicit Apply actions to minimize accidental runtime speed changes.
/// </summary>
public partial class SpeedSliderPrototypeWindow : NexenWindow {
	private SpeedSliderSelection _selection;

	public SpeedSliderPrototypeWindow() {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif

		_selection = SpeedSliderPrototypeMapper.FromPosition(SpeedSliderPrototypeMapper.ToPosition(ConfigManager.Config.Emulation.EmulationSpeed));
		SpeedSlider.Value = _selection.Position;
		CurrentSelectionLabel.Text = _selection.Label;
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void OnSpeedSliderChanged(object? sender, RangeBaseValueChangedEventArgs e) {
		_selection = SpeedSliderPrototypeMapper.FromSliderValue(e.NewValue);
		CurrentSelectionLabel.Text = _selection.Label;
	}

	private void OnApplyClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ApplySelection();
	}

	private void OnApplyAndCloseClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ApplySelection();
		Close();
	}

	private void OnCloseClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		Close();
	}

	private void ApplySelection() {
		if (_selection.Mode == SpeedSliderMode.Pause) {
			if (!EmuApi.IsPaused()) {
				EmuApi.Pause();
			}
			DisplayMessageHelper.DisplayMessage("EmulationSpeed", "EmulationPaused");
			return;
		}

		ConfigManager.Config.Emulation.EmulationSpeed = _selection.EmulationSpeed;
		ConfigManager.Config.Emulation.ApplyConfig();

		if (_selection.Mode == SpeedSliderMode.MaximumSpeed || _selection.EmulationSpeed == 0) {
			DisplayMessageHelper.DisplayMessage("EmulationSpeed", "EmulationMaximumSpeed");
		} else {
			DisplayMessageHelper.DisplayMessage("EmulationSpeed", "EmulationSpeedPercent", _selection.EmulationSpeed.ToString());
		}
	}
}
