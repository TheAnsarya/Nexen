using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Config.Shortcuts;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.Views;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels; 
public sealed partial class NesInputConfigViewModel : DisposableViewModel {
	[Reactive] public partial NesConfig Config { get; set; }

	public List<ShortcutKeyInfo> ShortcutKeys { get; set; }

	private MainWindowViewModel MainWindow { get; }

	[Reactive] public partial bool ShowMapperInput { get; private set; }
	[Reactive] public partial bool HasFourScore { get; private set; }
	[ObservableAsProperty] private bool _hasFourPlayerAdapter;
	[ObservableAsProperty] private bool _hasExpansionHub;
	[ObservableAsProperty] private string _expConfigLabel = "";
	[ObservableAsProperty] private Enum[] _availableControllerTypesExpansionHub = [];

	public Enum[] AvailableControllerTypesP1 => new Enum[] {
		ControllerType.None,
		ControllerType.NesController,
		ControllerType.FamicomController,
		ControllerType.NesZapper,
		ControllerType.FourScore,
		ControllerType.NesArkanoidController,
		ControllerType.PowerPadSideA,
		ControllerType.PowerPadSideB,
		ControllerType.SnesController,
		ControllerType.SnesMouse,
		ControllerType.SuborMouse,
		ControllerType.VbController
	};

	public Enum[] AvailableControllerTypesP2 => new Enum[] {
		ControllerType.None,
		ControllerType.NesController,
		ControllerType.FamicomControllerP2,
		ControllerType.NesZapper,
		ControllerType.FourScore,
		ControllerType.NesArkanoidController,
		ControllerType.PowerPadSideA,
		ControllerType.PowerPadSideB,
		ControllerType.SnesController,
		ControllerType.SnesMouse,
		ControllerType.SuborMouse,
		ControllerType.VbController
	};

	public Enum[] AvailableControllerTypesFourPlayer => new Enum[] {
		ControllerType.None,
		ControllerType.NesController,
	};

	public Enum[] AvailableControllerTypesTwoPlayer => new Enum[] {
		ControllerType.None,
		ControllerType.NesController,
		ControllerType.Pachinko,
		ControllerType.SnesController,
		ControllerType.SnesMouse,
		ControllerType.SuborMouse,
		ControllerType.VbController,
	};

	public Enum[] AvailableExpansionTypes => new Enum[] {
		ControllerType.None,
		ControllerType.FamicomZapper,
		ControllerType.TwoPlayerAdapter,
		ControllerType.FourPlayerAdapter,
		ControllerType.FamicomArkanoidController,
		ControllerType.OekaKidsTablet,
		ControllerType.FamilyTrainerMatSideA,
		ControllerType.FamilyTrainerMatSideB,
		ControllerType.KonamiHyperShot,
		ControllerType.FamilyBasicKeyboard,
		ControllerType.PartyTap,
		ControllerType.Pachinko,
		ControllerType.ExcitingBoxing,
		ControllerType.JissenMahjong,
		ControllerType.SuborKeyboard,
		ControllerType.BarcodeBattler,
		ControllerType.HoriTrack,
		ControllerType.BandaiHyperShot,
		ControllerType.AsciiTurboFile,
		ControllerType.BattleBox
	};

	public Enum[] AvailableControllerTypesMapperInput => new Enum[] {
		ControllerType.BandaiMicrophone,
	};

	[Obsolete("For designer only")]
	public NesInputConfigViewModel() : this(new NesConfig(), new PreferencesConfig()) { }

	public NesInputConfigViewModel(NesConfig config, PreferencesConfig preferences) {
		Config = config;
		MainWindow = MainWindowViewModel.Instance;

		AddDisposable(this.WhenAnyValue(x => x.Config.Port1.Type, x => x.Config.Port2.Type).Subscribe(t => Dispatcher.UIThread.Post(() => {
			HasFourScore = Config.Port1.Type == ControllerType.FourScore || Config.Port2.Type == ControllerType.FourScore;
			if (HasFourScore) {
				Config.Port1.Type = ControllerType.FourScore;
				Config.Port2.Type = ControllerType.None;
			}
		})));

		AddDisposable(_hasFourPlayerAdapterHelper = this.WhenAnyValue(x => x.Config.ExpPort.Type).Select(t => t == ControllerType.FourPlayerAdapter).ToProperty(this, _ => _.HasFourPlayerAdapter));

		AddDisposable(_hasExpansionHubHelper =
			this.WhenAnyValue(x => x.Config.ExpPort.Type)
				.Select(t => t is ControllerType.TwoPlayerAdapter or ControllerType.FourPlayerAdapter)
				.ToProperty(this, _ => _.HasExpansionHub)
		);

		AddDisposable(_expConfigLabelHelper =
			this.WhenAnyValue(x => x.Config.ExpPort.Type)
				.Select(t => ResourceHelper.GetViewLabel(nameof(NesInputConfigView), t == ControllerType.TwoPlayerAdapter ? "lblTwoPlayerAdapterConfig" : "lblFourPlayerAdapterConfig"))
				.ToProperty(this, _ => _.ExpConfigLabel)
		);

		AddDisposable(_availableControllerTypesExpansionHubHelper =
			this.WhenAnyValue(x => x.Config.ExpPort.Type)
				.Select(t => t == ControllerType.TwoPlayerAdapter ? AvailableControllerTypesTwoPlayer : AvailableControllerTypesFourPlayer)
				.ToProperty(this, _ => _.AvailableControllerTypesExpansionHub)
		);

		AddDisposable(this.WhenAnyValue(x => x.MainWindow.RomInfo).Subscribe(x => ShowMapperInput = InputApi.HasControlDevice(ControllerType.BandaiMicrophone)));

		EmulatorShortcut[] displayOrder = new EmulatorShortcut[] {
			EmulatorShortcut.FdsSwitchDiskSide,
			EmulatorShortcut.FdsEjectDisk,
			EmulatorShortcut.FdsInsertNextDisk,
			EmulatorShortcut.VsInsertCoin1,
			EmulatorShortcut.VsInsertCoin2,
			EmulatorShortcut.VsInsertCoin3,
			EmulatorShortcut.VsInsertCoin4,
			EmulatorShortcut.VsServiceButton,
			EmulatorShortcut.VsServiceButton2
		};

		Dictionary<EmulatorShortcut, ShortcutKeyInfo> shortcuts = new Dictionary<EmulatorShortcut, ShortcutKeyInfo>();
		foreach (ShortcutKeyInfo shortcut in preferences.ShortcutKeys) {
			shortcuts[shortcut.Shortcut] = shortcut;
		}

		ShortcutKeys = [];

		if (Design.IsDesignMode) {
			return;
		}

		for (int i = 0; i < displayOrder.Length; i++) {
			ShortcutKeys.Add(shortcuts[displayOrder[i]]);
		}
	}
}
