using System;
using Avalonia.Media;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class FontConfig : BaseConfig<FontConfig> {
	[Reactive] public partial string FontFamily { get; set; } = "";
	[Reactive] public partial double FontSize { get; set; } = 12;
}
