using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;

/// <summary>
/// Event viewer configuration for Lynx debugging.
/// </summary>
/// <remarks>
/// Configures color-coded visualization of hardware events:
/// - IRQ/breakpoints (CPU-level events)
/// - Mikey register access (timers, display, audio, UART)
/// - Suzy register access (sprites, math, collision)
/// - Audio/palette/timer-specific events
/// </remarks>
public sealed partial class LynxEventViewerConfig : ViewModelBase {
	/// <summary>IRQ assertion events.</summary>
	[Reactive] public partial EventViewerCategoryCfg Irq { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[0]);
	/// <summary>User-marked breakpoint hits.</summary>
	[Reactive] public partial EventViewerCategoryCfg MarkedBreakpoints { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[1]);

	/// <summary>Mikey register write events ($FD00-$FDFF).</summary>
	[Reactive] public partial EventViewerCategoryCfg MikeyRegisterWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[2]);
	/// <summary>Mikey register read events ($FD00-$FDFF).</summary>
	[Reactive] public partial EventViewerCategoryCfg MikeyRegisterRead { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[3]);
	/// <summary>Suzy register write events ($FC00-$FCFF).</summary>
	[Reactive] public partial EventViewerCategoryCfg SuzyRegisterWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[4]);
	/// <summary>Suzy register read events ($FC00-$FCFF).</summary>
	[Reactive] public partial EventViewerCategoryCfg SuzyRegisterRead { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[5]);
	/// <summary>Audio register write events ($FD20-$FD3F, $FD50).</summary>
	[Reactive] public partial EventViewerCategoryCfg AudioRegisterWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[6]);
	/// <summary>Audio register read events.</summary>
	[Reactive] public partial EventViewerCategoryCfg AudioRegisterRead { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[7]);
	/// <summary>Palette write events ($FDA0-$FDBF).</summary>
	[Reactive] public partial EventViewerCategoryCfg PaletteWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[8]);
	/// <summary>Timer register write events ($FD00-$FD1F).</summary>
	[Reactive] public partial EventViewerCategoryCfg TimerWrite { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[9]);
	/// <summary>Timer register read events ($FD00-$FD1F).</summary>
	[Reactive] public partial EventViewerCategoryCfg TimerRead { get; set; } = new EventViewerCategoryCfg(EventViewerColors.Colors[10]);

	/// <summary>Show events from the previous frame (wraparound display).</summary>
	[Reactive] public partial bool ShowPreviousFrameEvents { get; set; } = true;

	/// <summary>Convert to interop structure for C++ core.</summary>
	public InteropLynxEventViewerConfig ToInterop() {
		return new InteropLynxEventViewerConfig() {
			Irq = this.Irq,
			MarkedBreakpoints = this.MarkedBreakpoints,
			MikeyRegisterWrite = this.MikeyRegisterWrite,
			MikeyRegisterRead = this.MikeyRegisterRead,
			SuzyRegisterWrite = this.SuzyRegisterWrite,
			SuzyRegisterRead = this.SuzyRegisterRead,
			AudioRegisterWrite = this.AudioRegisterWrite,
			AudioRegisterRead = this.AudioRegisterRead,
			PaletteWrite = this.PaletteWrite,
			TimerWrite = this.TimerWrite,
			TimerRead = this.TimerRead,
			ShowPreviousFrameEvents = this.ShowPreviousFrameEvents
		};
	}
}
