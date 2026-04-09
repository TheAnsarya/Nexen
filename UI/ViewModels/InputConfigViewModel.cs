using Avalonia.Controls;
using Nexen.Config;
using Nexen.Utilities;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the input configuration tab.
/// </summary>
public sealed partial class InputConfigViewModel : DisposableViewModel {
	/// <summary>Gets or sets the current input configuration.</summary>
	[Reactive] public partial InputConfig Config { get; set; }

	/// <summary>Gets or sets the original input configuration for revert.</summary>
	[Reactive] public partial InputConfig OriginalConfig { get; set; }

	/// <summary>
	/// Initializes a new instance of the <see cref="InputConfigViewModel"/> class.
	/// </summary>
	public InputConfigViewModel() {
		Config = ConfigManager.Config.Input;
		OriginalConfig = Config.Clone();

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
	}
}
