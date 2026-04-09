using Avalonia.Media;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class SmsEventViewerConfig : ViewModelBase {
	[Reactive] public partial EventViewerCategoryCfg VdpPaletteWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xC9, 0x29, 0x29));
	[Reactive] public partial EventViewerCategoryCfg VdpVramWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xB4, 0x7A, 0xDA));
	[Reactive] public partial EventViewerCategoryCfg VdpVCounterRead { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x53, 0xD7, 0x44));
	[Reactive] public partial EventViewerCategoryCfg VdpHCounterRead { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x4A, 0x7C, 0xD9));
	[Reactive] public partial EventViewerCategoryCfg VdpVramRead { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xE2, 0x51, 0xF7));
	[Reactive] public partial EventViewerCategoryCfg VdpControlPortRead { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xD1, 0xDD, 0x42));
	[Reactive] public partial EventViewerCategoryCfg VdpControlPortWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x00, 0x75, 0x97));

	[Reactive] public partial EventViewerCategoryCfg PsgWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xFF, 0x5E, 0x5E));
	[Reactive] public partial EventViewerCategoryCfg IoWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x18, 0x98, 0xE4));
	[Reactive] public partial EventViewerCategoryCfg IoRead { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x9F, 0x93, 0xC6));

	[Reactive] public partial EventViewerCategoryCfg MemoryControlWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x4A, 0xFE, 0xAC));

	[Reactive] public partial EventViewerCategoryCfg GameGearPortWrite { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xF9, 0xFE, 0xAC));
	[Reactive] public partial EventViewerCategoryCfg GameGearPortRead { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xAC, 0xFE, 0xAC));

	[Reactive] public partial EventViewerCategoryCfg Irq { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xC4, 0xF4, 0x7A));
	[Reactive] public partial EventViewerCategoryCfg Nmi { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0xF4, 0xF4, 0x7A));

	[Reactive] public partial EventViewerCategoryCfg MarkedBreakpoints { get; set; } = new EventViewerCategoryCfg(Color.FromRgb(0x18, 0x98, 0xE4));

	[Reactive] public partial bool ShowPreviousFrameEvents { get; set; } = true;

	public InteropSmsEventViewerConfig ToInterop() {
		return new InteropSmsEventViewerConfig() {
			Irq = this.Irq,
			Nmi = this.Nmi,
			MarkedBreakpoints = this.MarkedBreakpoints,

			VdpPaletteWrite = this.VdpPaletteWrite,
			VdpVramWrite = this.VdpVramWrite,

			VdpVCounterRead = this.VdpVCounterRead,
			VdpHCounterRead = this.VdpHCounterRead,
			VdpVramRead = this.VdpVramRead,
			VdpControlPortRead = this.VdpControlPortRead,
			VdpControlPortWrite = this.VdpControlPortWrite,

			PsgWrite = this.PsgWrite,
			IoWrite = this.IoWrite,
			IoRead = this.IoRead,

			MemoryControlWrite = this.MemoryControlWrite,
			GameGearPortWrite = this.GameGearPortWrite,
			GameGearPortRead = this.GameGearPortRead,

			ShowPreviousFrameEvents = this.ShowPreviousFrameEvents
		};
	}
}
