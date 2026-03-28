using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Config;

public sealed class ChannelFEventViewerConfig : ViewModelBase {
	[Reactive] public EventViewerCategoryCfg MarkedBreakpoints { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[0]);

	[Reactive] public EventViewerCategoryCfg IoWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[1]);
	[Reactive] public EventViewerCategoryCfg IoRead { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[2]);

	[Reactive] public bool ShowPreviousFrameEvents { get; set; } = true;
}
