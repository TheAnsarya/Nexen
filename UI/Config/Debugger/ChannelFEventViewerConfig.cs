using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;

public sealed partial class ChannelFEventViewerConfig : ViewModelBase {
	[Reactive] public partial EventViewerCategoryCfg MarkedBreakpoints { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[0]);

	[Reactive] public partial EventViewerCategoryCfg IoWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[1]);
	[Reactive] public partial EventViewerCategoryCfg IoRead { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[2]);

	[Reactive] public partial bool ShowPreviousFrameEvents { get; set; } = true;

	public InteropChannelFEventViewerConfig ToInterop() {
		return new InteropChannelFEventViewerConfig() {
			MarkedBreakpoints = this.MarkedBreakpoints,
			IoWrite = this.IoWrite,
			IoRead = this.IoRead,
			ShowPreviousFrameEvents = this.ShowPreviousFrameEvents
		};
	}
}
