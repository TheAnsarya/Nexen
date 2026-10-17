using System;
using Nexen.Config;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
public sealed partial class GenesisInputConfigViewModel : DisposableViewModel {
	[Reactive] public partial GenesisConfig Config { get; set; }

	public Enum[] AvailableControllerTypesP12 => new Enum[] {
		ControllerType.None,
		ControllerType.GenesisController,
	};

	[Obsolete("For designer only")]
	public GenesisInputConfigViewModel() : this(new GenesisConfig()) { }

	public GenesisInputConfigViewModel(GenesisConfig config) {
		Config = config;
	}
}
