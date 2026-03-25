using System;
using Nexen.Config;

namespace Nexen.ViewModels;

public sealed class Atari2600InputConfigViewModel : DisposableViewModel {
	[ReactiveUI.Fody.Helpers.Reactive] public Atari2600Config Config { get; set; }

	public Enum[] AvailableControllerTypesP12 => new Enum[] {
		ControllerType.None,
		ControllerType.Atari2600Joystick,
		ControllerType.Atari2600Paddle,
		ControllerType.Atari2600Keypad,
		ControllerType.Atari2600DrivingController,
		ControllerType.Atari2600BoosterGrip,
	};

	public Atari2600InputConfigViewModel() {
		Config = ConfigManager.Config.Atari2600;
	}

	public Atari2600InputConfigViewModel(Atari2600Config config) {
		Config = config;
	}
}
