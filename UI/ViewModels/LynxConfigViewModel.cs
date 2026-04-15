using System;
using System.Reactive;
using Avalonia;
using Avalonia.Controls;
using Avalonia.VisualTree;
using Nexen.Config;
using Nexen.Utilities;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;

/// <summary>
/// ViewModel for the Lynx configuration dialog.
/// </summary>
/// <remarks>
/// Manages the Lynx settings UI with tabs for:
/// - General (boot ROM, rotation, frame blending)
/// - Audio (per-channel volume)
/// - Emulation (sprite/background toggles)
/// - Input (controller mapping)
/// - Video (filter settings)
/// </remarks>
public sealed partial class LynxConfigViewModel : DisposableViewModel {
	/// <summary>Current Lynx configuration being edited.</summary>
	[Reactive] public partial LynxConfig Config { get; set; }
	/// <summary>Original configuration for cancel/revert.</summary>
	[Reactive] public partial LynxConfig OriginalConfig { get; set; }
	/// <summary>Currently selected configuration tab.</summary>
	[Reactive] public partial LynxConfigTab SelectedTab { get; set; } = 0;

	/// <summary>Command to open controller setup dialog.</summary>
	public ReactiveCommand<Button, Unit> SetupPlayer { get; }

	/// <summary>Create a new Lynx configuration view model.</summary>
	public LynxConfigViewModel() {
		Config = ConfigManager.Config.Lynx;
		OriginalConfig = Config.Clone();

		SetupPlayer = ReactiveCommand.Create<Button>(btn => this.OpenSetup(btn, 0));

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
	}

	/// <summary>Open controller configuration popup window.</summary>
	private async void OpenSetup(Button btn, int port) {
		PixelPoint startPosition = btn.PointToScreen(new Point(-7, btn.Bounds.Height));
		ControllerConfigWindow wnd = new ControllerConfigWindow();
		ControllerConfig orgCfg = Config.Controller;
		ControllerConfig cfg = Config.Controller.Clone();
		cfg.Type = ControllerType.LynxController;
		wnd.DataContext = new ControllerConfigViewModel(ControllerType.LynxController, cfg, orgCfg, port);

		if (await wnd.ShowDialogAtPosition<bool>(TopLevel.GetTopLevel(btn) as Visual, startPosition)) {
			Config.Controller = cfg;
		}
	}
}

/// <summary>Configuration tab selection for the Lynx settings dialog.</summary>
public enum LynxConfigTab {
	/// <summary>General settings (boot ROM, rotation).</summary>
	General,
	/// <summary>Audio settings (per-channel volume).</summary>
	Audio,
	/// <summary>Emulation settings (layer toggles).</summary>
	Emulation,
	/// <summary>Input settings (controller mapping).</summary>
	Input,
	/// <summary>Video settings (filters).</summary>
	Video
}
