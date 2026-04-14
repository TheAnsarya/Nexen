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
public sealed partial class WsConfigViewModel : DisposableViewModel {
	[Reactive] public partial WsConfig Config { get; set; }
	[Reactive] public partial WsConfig OriginalConfig { get; set; }
	[Reactive] public partial WsConfigTab SelectedTab { get; set; } = 0;

	public ReactiveCommand<Button, Unit> SetupPlayerHorizontal { get; }
	public ReactiveCommand<Button, Unit> SetupPlayerVertical { get; }

	public WsConfigViewModel() {
		Config = ConfigManager.Config.Ws;
		OriginalConfig = Config.Clone();

		SetupPlayerHorizontal = ReactiveCommand.Create<Button>(btn => this.OpenSetup(btn, 0));
		SetupPlayerVertical = ReactiveCommand.Create<Button>(btn => this.OpenSetup(btn, 1));

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
	}

	private async void OpenSetup(Button btn, int port) {
		PixelPoint startPosition = btn.PointToScreen(new Point(-7, btn.Bounds.Height));
		ControllerConfigWindow wnd = new ControllerConfigWindow();
		ControllerConfig orgCfg = port == 0 ? Config.ControllerHorizontal : Config.ControllerVertical;
		ControllerConfig cfg = port == 0 ? Config.ControllerHorizontal.Clone() : Config.ControllerVertical.Clone();
		cfg.Type = port == 0 ? ControllerType.WsController : ControllerType.WsControllerVertical;
		wnd.DataContext = new ControllerConfigViewModel(port == 0 ? ControllerType.WsController : ControllerType.WsControllerVertical, cfg, orgCfg, port);

		if (await wnd.ShowDialogAtPosition<bool>(TopLevel.GetTopLevel(btn) as Visual, startPosition)) {
			if (port == 0) {
				Config.ControllerHorizontal = cfg;
			} else {
				Config.ControllerVertical = cfg;
			}
		}
	}
}

public enum WsConfigTab {
	General,
	Audio,
	Emulation,
	Input,
	Video
}
