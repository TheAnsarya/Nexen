using System;
using Avalonia.Media;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class EventViewerCategoryCfg : ViewModelBase {
	[Reactive] public partial bool Visible { get; set; } = true;
	[Reactive] public partial UInt32 Color { get; set; }

	public EventViewerCategoryCfg() { }

	public EventViewerCategoryCfg(Color color) {
		Color = color.ToUInt32();
	}

	public static implicit operator InteropEventViewerCategoryCfg(EventViewerCategoryCfg cfg) {
		return new InteropEventViewerCategoryCfg() {
			Visible = cfg.Visible,
			Color = cfg.Color
		};
	}
}
