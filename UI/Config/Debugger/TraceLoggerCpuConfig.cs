using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class TraceLoggerCpuConfig : BaseConfig<TraceLoggerCpuConfig> {
	[Reactive] public partial bool Enabled { get; set; } = true;

	[Reactive] public partial bool ShowRegisters { get; set; } = true;
	[Reactive] public partial bool ShowStatusFlags { get; set; } = true;
	[Reactive] public partial StatusFlagFormat StatusFormat { get; set; } = StatusFlagFormat.Text;

	[Reactive] public partial bool ShowEffectiveAddresses { get; set; } = true;
	[Reactive] public partial bool ShowMemoryValues { get; set; } = true;
	[Reactive] public partial bool ShowByteCode { get; set; } = false;

	[Reactive] public partial bool ShowClockCounter { get; set; } = false;
	[Reactive] public partial bool ShowFrameCounter { get; set; } = false;
	[Reactive] public partial bool ShowFramePosition { get; set; } = true;

	[Reactive] public partial bool UseLabels { get; set; } = true;
	[Reactive] public partial bool IndentCode { get; set; } = false;

	[Reactive] public partial bool UseCustomFormat { get; set; } = false;
	[Reactive] public partial string Format { get; set; } = "";
	[Reactive] public partial string Condition { get; set; } = "";
}

public enum StatusFlagFormat {
	Hexadecimal = 0,
	Text = 1,
	CompactText = 2
}
