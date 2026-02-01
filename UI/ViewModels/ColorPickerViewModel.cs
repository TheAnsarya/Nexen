using Avalonia.Media;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels {
	/// <summary>
	/// ViewModel for the color picker dialog.
	/// </summary>
	public class ColorPickerViewModel : ViewModelBase {
		/// <summary>Gets or sets the selected color.</summary>
		[Reactive] public Color Color { get; set; }
	}
}
