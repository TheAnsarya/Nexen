using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Config;

public sealed class Atari2600EventViewerConfig : ViewModelBase {
	[Reactive] public EventViewerCategoryCfg Irq { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[0]);
	[Reactive] public EventViewerCategoryCfg MarkedBreakpoints { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[1]);

	[Reactive] public EventViewerCategoryCfg TiaWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[2]);
	[Reactive] public EventViewerCategoryCfg TiaRead { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[3]);
	[Reactive] public EventViewerCategoryCfg RiotWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[4]);
	[Reactive] public EventViewerCategoryCfg RiotRead { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[5]);

	[Reactive] public bool ShowPreviousFrameEvents { get; set; } = true;

	public InteropAtari2600EventViewerConfig ToInterop() {
		return new InteropAtari2600EventViewerConfig() {
			Irq = this.Irq,
			MarkedBreakpoints = this.MarkedBreakpoints,
			TiaWrite = this.TiaWrite,
			TiaRead = this.TiaRead,
			RiotWrite = this.RiotWrite,
			RiotRead = this.RiotRead,
			ShowPreviousFrameEvents = this.ShowPreviousFrameEvents
		};
	}
}
