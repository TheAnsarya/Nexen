using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class TileEditorConfig : BaseWindowConfig<TileEditorConfig> {
	[Reactive] public partial double ImageScale { get; set; } = 8;
	[Reactive] public partial bool ShowGrid { get; set; } = false;
	[Reactive] public partial TileBackground Background { get; set; } = TileBackground.Transparent;
}
