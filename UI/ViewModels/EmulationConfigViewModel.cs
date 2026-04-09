using Avalonia.Controls;
using Nexen.Config;
using Nexen.Utilities;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the emulation configuration tab.
/// </summary>
public sealed partial class EmulationConfigViewModel : DisposableViewModel {
	/// <summary>Gets or sets the current emulation configuration.</summary>
	[Reactive] public partial EmulationConfig Config { get; set; }

	/// <summary>Gets or sets the original emulation configuration for revert.</summary>
	[Reactive] public partial EmulationConfig OriginalConfig { get; set; }

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
