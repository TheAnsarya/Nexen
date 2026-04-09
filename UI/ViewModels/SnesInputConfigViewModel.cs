using System;
using System.Linq;
using System.Reactive.Linq;
using Nexen.Config;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels; 
public sealed partial class SnesInputConfigViewModel : DisposableViewModel {
	[Reactive] public partial SnesConfig Config { get; set; }

	[ObservableAsProperty] private bool _hasMultitap1;
	[ObservableAsProperty] private bool _hasMultitap2;

	public Enum[] AvailableControllerTypesP1 => new Enum[] {
		ControllerType.None,
		ControllerType.SnesController,
		ControllerType.SnesMouse,
		ControllerType.SuperScope,
		ControllerType.Multitap,
		ControllerType.SnesRumbleController,
	};

	public Enum[] AvailableControllerTypesP2 => new Enum[] {
		ControllerType.None,
		ControllerType.SnesController,
		ControllerType.SnesMouse,
		ControllerType.SuperScope,
		ControllerType.Multitap
	};

	public Enum[] AvailableControllerTypesMultitap => new Enum[] {
		ControllerType.None,
		ControllerType.SnesController,
		ControllerType.SnesMouse,
		ControllerType.SuperScope,
	};

	[Obsolete("For designer only")]
	public SnesInputConfigViewModel() : this(new SnesConfig()) { }

	public SnesInputConfigViewModel(SnesConfig config) {
		Config = config;

		AddDisposable(_hasMultitap1Helper = this.WhenAnyValue(x => x.Config.Port1.Type)
			.Select(x => x == ControllerType.Multitap)
			.ToProperty(this, _ => _.HasMultitap1));

		AddDisposable(_hasMultitap2Helper = this.WhenAnyValue(x => x.Config.Port2.Type)
			.Select(x => x == ControllerType.Multitap)
			.ToProperty(this, _ => _.HasMultitap2));
	}
}
