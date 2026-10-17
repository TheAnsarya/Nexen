using System;
using System.Collections.Generic;
using System.Reactive;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Media;
using Nexen.Debugger;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;
public sealed partial class DebuggerConfig : BaseWindowConfig<DebuggerConfig> {
	public DockEntryDefinition? SavedDockLayout { get; set; } = null;

	[Reactive] public partial bool ShowSettingsPanel { get; set; } = true;

	[Reactive] public partial bool ShowByteCode { get; set; } = false;
	[Reactive] public partial bool ShowMemoryValues { get; set; } = true;
	[Reactive] public partial bool UseLowerCaseDisassembly { get; set; } = false;

	[Reactive] public partial bool ShowJumpLabels { get; set; } = false;
	[Reactive] public partial AddressDisplayType AddressDisplayType { get; set; } = AddressDisplayType.CpuAddress;

	[Reactive] public partial bool DrawPartialFrame { get; set; } = false;

	[Reactive] public partial SnesDebuggerConfig Snes { get; set; } = new();
	[Reactive] public partial NesDebuggerConfig Nes { get; set; } = new();
	[Reactive] public partial GbDebuggerConfig Gameboy { get; set; } = new();
	[Reactive] public partial GbaDebuggerConfig Gba { get; set; } = new();
	[Reactive] public partial PceDebuggerConfig Pce { get; set; } = new();
	[Reactive] public partial SmsDebuggerConfig Sms { get; set; } = new();
	[Reactive] public partial WsDebuggerConfig Ws { get; set; } = new();
	[Reactive] public partial LynxDebuggerConfig Lynx { get; set; } = new();
	[Reactive] public partial Atari2600DebuggerConfig Atari2600 { get; set; } = new();
	[Reactive] public partial ChannelFDebuggerConfig ChannelF { get; set; } = new();

	[Reactive] public partial bool BreakOnUninitRead { get; set; } = false;
	[Reactive] public partial bool BreakOnOpen { get; set; } = true;
	[Reactive] public partial bool BreakOnPowerCycleReset { get; set; } = true;

	[Reactive] public partial bool AutoResetCdl { get; set; } = true;
	[Reactive] public partial bool DisableDefaultLabels { get; set; } = false;

	[Reactive] public partial bool UsePredictiveBreakpoints { get; set; } = true;
	[Reactive] public partial bool SingleBreakpointPerInstruction { get; set; } = true;

	[Reactive] public partial bool CopyAddresses { get; set; } = true;
	[Reactive] public partial bool CopyByteCode { get; set; } = true;
	[Reactive] public partial bool CopyComments { get; set; } = true;
	[Reactive] public partial bool CopyBlockHeaders { get; set; } = true;

	[Reactive] public partial bool KeepActiveStatementInCenter { get; set; } = false;

	[Reactive] public partial bool ShowMemoryMappings { get; set; } = true;

	[Reactive] public partial bool RefreshWhileRunning { get; set; } = false;

	[Reactive] public partial bool BringToFrontOnBreak { get; set; } = true;
	[Reactive] public partial bool BringToFrontOnPause { get; set; } = false;
	[Reactive] public partial bool FocusGameOnResume { get; set; } = false;

	[Reactive] public partial CodeDisplayMode UnidentifiedBlockDisplay { get; set; } = CodeDisplayMode.Hide;
	[Reactive] public partial CodeDisplayMode VerifiedDataDisplay { get; set; } = CodeDisplayMode.Hide;

	[Reactive] public partial int BreakOnValue { get; set; } = 0;
	[Reactive] public partial int BreakInCount { get; set; } = 1;
	[Reactive] public partial BreakInMetric BreakInMetric { get; set; } = BreakInMetric.CpuInstructions;

	[Reactive] public partial bool ShowSelectionLength { get; set; } = false;
	[Reactive] public partial WatchFormatStyle WatchFormat { get; set; } = WatchFormatStyle.Hex;

	[Reactive] public partial UInt32 CodeOpcodeColor { get; set; } = Color.FromRgb(22, 37, 37).ToUInt32();
	[Reactive] public partial UInt32 CodeLabelDefinitionColor { get; set; } = Colors.Blue.ToUInt32();
	[Reactive] public partial UInt32 CodeImmediateColor { get; set; } = Colors.Chocolate.ToUInt32();
	[Reactive] public partial UInt32 CodeAddressColor { get; set; } = Colors.DarkRed.ToUInt32();
	[Reactive] public partial UInt32 CodeCommentColor { get; set; } = Colors.Green.ToUInt32();
	[Reactive] public partial UInt32 CodeEffectiveAddressColor { get; set; } = Colors.SteelBlue.ToUInt32();

	[Reactive] public partial UInt32 CodeVerifiedDataColor { get; set; } = Color.FromRgb(255, 252, 236).ToUInt32();
	[Reactive] public partial UInt32 CodeUnidentifiedDataColor { get; set; } = Color.FromRgb(255, 242, 242).ToUInt32();
	[Reactive] public partial UInt32 CodeUnexecutedCodeColor { get; set; } = Color.FromRgb(225, 244, 228).ToUInt32();

	[Reactive] public partial UInt32 CodeExecBreakpointColor { get; set; } = Color.FromRgb(140, 40, 40).ToUInt32();
	[Reactive] public partial UInt32 CodeWriteBreakpointColor { get; set; } = Color.FromRgb(40, 120, 80).ToUInt32();
	[Reactive] public partial UInt32 CodeReadBreakpointColor { get; set; } = Color.FromRgb(40, 40, 200).ToUInt32();
	[Reactive] public partial UInt32 ForbidBreakpointColor { get; set; } = Color.FromRgb(115, 115, 115).ToUInt32();

	[Reactive] public partial UInt32 CodeActiveStatementColor { get; set; } = Colors.Yellow.ToUInt32();
	[Reactive] public partial UInt32 CodeActiveMidInstructionColor { get; set; } = Color.FromRgb(255, 220, 40).ToUInt32();

	[Reactive] public partial List<int> LabelListColumnWidths { get; set; } = new();
	[Reactive] public partial List<int> FunctionListColumnWidths { get; set; } = new();
	[Reactive] public partial List<int> BreakpointListColumnWidths { get; set; } = new();
	[Reactive] public partial List<int> WatchListColumnWidths { get; set; } = new();
	[Reactive] public partial List<int> CallStackColumnWidths { get; set; } = new();
	[Reactive] public partial List<int> FindResultColumnWidths { get; set; } = new();

	public DebuggerConfig() {
	}
}

public sealed partial class CfgColor : ReactiveObject {
	[Reactive] public partial UInt32 ColorCode { get; set; }
}

public enum BreakInMetric {
	CpuInstructions,
	PpuCycles,
	Scanlines,
	Frames
}

public enum CodeDisplayMode {
	Hide,
	Show,
	Disassemble
}

public enum AddressDisplayType {
	CpuAddress,
	AbsAddress,
	Both,
	BothCompact
}
