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
public sealed partial class GameboyConfigViewModel : DisposableViewModel {
	[Reactive] public partial GameboyConfig Config { get; set; }
	[Reactive] public partial GameboyConfig OriginalConfig { get; set; }
	[Reactive] public partial GameboyConfigTab SelectedTab { get; set; } = 0;

	public ReactiveCommand<Button, Unit> SetupPlayer { get; }

	public GameboyConfigViewModel() {
		Config = ConfigManager.Config.Gameboy;
		OriginalConfig = Config.Clone();

		SetupPlayer = ReactiveCommand.Create<Button>(btn => this.OpenSetup(btn, 0));

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
	}

	private async void OpenSetup(Button btn, int port) {
		PixelPoint startPosition = btn.PointToScreen(new Point(-7, btn.Bounds.Height));
		ControllerConfigWindow wnd = new ControllerConfigWindow();
		ControllerConfig cfg = Config.Controller.Clone();
		cfg.Type = ControllerType.GameboyController;
		wnd.DataContext = new ControllerConfigViewModel(ControllerType.GameboyController, cfg, Config.Controller, 0);

		if (await wnd.ShowDialogAtPosition<bool>(TopLevel.GetTopLevel(btn) as Visual, startPosition)) {
			Config.Controller = cfg;
		}
	}
}

public enum GameboyConfigTab {
	General,
	Audio,
	Emulation,
	Input,
	Video
}
