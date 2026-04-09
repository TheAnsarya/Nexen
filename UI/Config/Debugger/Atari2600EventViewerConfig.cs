using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;

public sealed partial class Atari2600EventViewerConfig : ViewModelBase {
	[Reactive] public partial EventViewerCategoryCfg Irq { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[0]);
	[Reactive] public partial EventViewerCategoryCfg MarkedBreakpoints { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[1]);

	[Reactive] public partial EventViewerCategoryCfg TiaWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[2]);
	[Reactive] public partial EventViewerCategoryCfg TiaRead { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[3]);
	[Reactive] public partial EventViewerCategoryCfg RiotWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[4]);
	[Reactive] public partial EventViewerCategoryCfg RiotRead { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[5]);

	[Reactive] public partial bool ShowPreviousFrameEvents { get; set; } = true;

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
