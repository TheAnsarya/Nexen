using System;
using System.Reactive.Linq;
using Avalonia.Threading;
using Nexen.Config;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels; 
public sealed partial class PceInputConfigViewModel : DisposableViewModel {
	[Reactive] public partial PcEngineConfig Config { get; set; }
	[Reactive] public partial bool HasTurboTap { get; private set; }

	public Enum[] AvailableControllerTypesP1 => new Enum[] {
		ControllerType.None,
		ControllerType.PceController,
		ControllerType.PceAvenuePad6,
		ControllerType.PceTurboTap,
	};

	public Enum[] AvailableControllerTypesTurboTap => new Enum[] {
		ControllerType.None,
		ControllerType.PceController,
		ControllerType.PceAvenuePad6,
	};

	[Obsolete("For designer only")]
	public PceInputConfigViewModel() : this(new PcEngineConfig()) { }

	public PceInputConfigViewModel(PcEngineConfig config) {
		Config = config;

		AddDisposable(this.WhenAnyValue(x => x.Config.Port1.Type).Subscribe(t => Dispatcher.UIThread.Post(() => HasTurboTap = Config.Port1.Type == ControllerType.PceTurboTap)));
	}
}
