using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Serialization;
using Avalonia;
using Avalonia.Media;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class HexEditorConfig : BaseWindowConfig<HexEditorConfig> {
	[Reactive] public partial bool ShowOptionPanel { get; set; } = true;
	[Reactive] public partial bool AutoRefresh { get; set; } = true;
	[Reactive] public partial bool IgnoreRedundantWrites { get; set; } = false;

	[Reactive] public partial int BytesPerRow { get; set; } = 16;

	[Reactive] public partial bool HighDensityTextMode { get; set; } = false;

	[Reactive] public partial bool ShowCharacters { get; set; } = true;
	[Reactive] public partial bool ShowTooltips { get; set; } = true;

	[Reactive] public partial bool HideUnusedBytes { get; set; } = false;
	[Reactive] public partial bool HideReadBytes { get; set; } = false;
	[Reactive] public partial bool HideWrittenBytes { get; set; } = false;
	[Reactive] public partial bool HideExecutedBytes { get; set; } = false;

	[Reactive] public partial HighlightFadeSpeed FadeSpeed { get; set; } = HighlightFadeSpeed.Normal;
	[Reactive] public partial HighlightConfig ReadHighlight { get; set; } = new() { Highlight = true, ColorCode = Colors.Blue.ToUInt32() };
	[Reactive] public partial HighlightConfig WriteHighlight { get; set; } = new() { Highlight = true, ColorCode = Colors.Red.ToUInt32() };
	[Reactive] public partial HighlightConfig ExecHighlight { get; set; } = new() { Highlight = true, ColorCode = Colors.Green.ToUInt32() };

	[Reactive] public partial HighlightConfig LabelHighlight { get; set; } = new() { Highlight = false, ColorCode = Colors.LightPink.ToUInt32() };
	[Reactive] public partial HighlightConfig CodeHighlight { get; set; } = new() { Highlight = false, ColorCode = Colors.DarkSeaGreen.ToUInt32() };
	[Reactive] public partial HighlightConfig DataHighlight { get; set; } = new() { Highlight = false, ColorCode = Colors.LightSteelBlue.ToUInt32() };

	[Reactive] public partial HighlightConfig FrozenHighlight { get; set; } = new() { Highlight = true, ColorCode = Colors.Magenta.ToUInt32() };

	[Reactive] public partial HighlightConfig NesPcmDataHighlight { get; set; } = new() { Highlight = false, ColorCode = Colors.Khaki.ToUInt32() };
	[Reactive] public partial HighlightConfig NesDrawnChrRomHighlight { get; set; } = new() { Highlight = false, ColorCode = Colors.Thistle.ToUInt32() };

	[Reactive] public partial bool HighlightBreakpoints { get; set; } = false;

	[Reactive] public partial MemoryType MemoryType { get; set; } = MemoryType.SnesMemory;

	public HexEditorConfig() {
	}
}

public enum HighlightFadeSpeed {
	NoFade,
	Slow,
	Normal,
	Fast
}

public static partial class HighlightFadeSpeedExtensions {
	public static int ToFrameCount(this HighlightFadeSpeed speed) {
		return speed switch {
			HighlightFadeSpeed.NoFade => 0,
			HighlightFadeSpeed.Slow => 600,
			HighlightFadeSpeed.Normal => 300,
			HighlightFadeSpeed.Fast => 120,
			_ => 0
		};
	}
}

public sealed partial class HighlightConfig : ReactiveObject {
	[Reactive] public partial bool Highlight { get; set; }
	[Reactive] public partial UInt32 ColorCode { get; set; }

	[JsonIgnore]
	public Color Color {
		get { return Color.FromUInt32(ColorCode); }
		set { ColorCode = value.ToUInt32(); }
	}
}
