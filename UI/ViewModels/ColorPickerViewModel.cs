using Avalonia.Media;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the color picker dialog.
/// </summary>
public sealed partial class ColorPickerViewModel : ViewModelBase {
	/// <summary>Gets or sets the selected color.</summary>
	[Reactive] public partial Color Color { get; set; }
}
