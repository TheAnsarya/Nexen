using Avalonia.Controls;
using Nexen.Config;
using Nexen.Utilities;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the emulation configuration tab.
/// </summary>
public class EmulationConfigViewModel : DisposableViewModel {
	/// <summary>Gets or sets the current emulation configuration.</summary>
	[Reactive] public EmulationConfig Config { get; set; }

	/// <summary>Gets or sets the original emulation configuration for revert.</summary>
	[Reactive] public EmulationConfig OriginalConfig { get; set; }

	/// <summary>
	/// Initializes a new instance of the <see cref="EmulationConfigViewModel"/> class.
	/// </summary>
	public EmulationConfigViewModel() {
		Config = ConfigManager.Config.Emulation;
		OriginalConfig = Config.Clone();

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
	}
}
