using Avalonia.Media;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;

public sealed partial class GenesisEventViewerConfig : ViewModelBase {
	[Reactive] public partial EventViewerCategoryCfg VdpControlWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x00, 0x75, 0x97));
	[Reactive] public partial EventViewerCategoryCfg VdpControlRead { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xD1, 0xDD, 0x42));
	[Reactive] public partial EventViewerCategoryCfg VdpDataWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xB4, 0x7A, 0xDA));
	[Reactive] public partial EventViewerCategoryCfg VdpDataRead { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xE2, 0x51, 0xF7));
	[Reactive] public partial EventViewerCategoryCfg VdpHvcounterRead { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x4A, 0x7C, 0xD9));

	[Reactive] public partial EventViewerCategoryCfg PsgWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xFF, 0x5E, 0x5E));
	[Reactive] public partial EventViewerCategoryCfg IoWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x18, 0x98, 0xE4));
	[Reactive] public partial EventViewerCategoryCfg IoRead { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x9F, 0x93, 0xC6));

	[Reactive] public partial EventViewerCategoryCfg Irq { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xC4, 0xF4, 0x7A));
	[Reactive] public partial EventViewerCategoryCfg Nmi { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xF4, 0xF4, 0x7A));
	[Reactive] public partial EventViewerCategoryCfg MarkedBreakpoints { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x18, 0x98, 0xE4));

	[Reactive] public partial bool ShowPreviousFrameEvents { get; set; } = true;

	public InteropGenesisEventViewerConfig ToInterop() {
		return new InteropGenesisEventViewerConfig() {
			Irq = Irq,
			Nmi = Nmi,
			MarkedBreakpoints = MarkedBreakpoints,

			VdpControlWrite = VdpControlWrite,
			VdpControlRead = VdpControlRead,
			VdpDataWrite = VdpDataWrite,
			VdpDataRead = VdpDataRead,
			VdpHvcounterRead = VdpHvcounterRead,

			PsgWrite = PsgWrite,
			IoWrite = IoWrite,
			IoRead = IoRead,

			ShowPreviousFrameEvents = ShowPreviousFrameEvents
		};
	}
}
