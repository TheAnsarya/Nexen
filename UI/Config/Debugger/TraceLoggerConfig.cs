using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Serialization;
using Avalonia;
using Avalonia.Media;
using Nexen.Debugger;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;
public sealed partial class TraceLoggerConfig : BaseWindowConfig<TraceLoggerConfig> {
	[Reactive] public partial bool AutoRefresh { get; set; } = true;
	[Reactive] public partial bool RefreshOnBreakPause { get; set; } = true;
	[Reactive] public partial bool ShowToolbar { get; set; } = true;

	[Reactive] public partial TraceLoggerCpuConfig SnesConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig SpcConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig NecDspConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig Sa1Config { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig GsuConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig Cx4Config { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig St018Config { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig GbConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig NesConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig PceConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig SmsConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig GbaConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig WsConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig LynxConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig GenesisConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig ChannelFConfig { get; set; } = new();
	[Reactive] public partial TraceLoggerCpuConfig Atari2600Config { get; set; } = new();

	public TraceLoggerConfig() {
	}

	public TraceLoggerCpuConfig GetCpuConfig(CpuType type) {
		return type switch {
			CpuType.Snes => SnesConfig,
			CpuType.Spc => SpcConfig,
			CpuType.NecDsp => NecDspConfig,
			CpuType.Sa1 => Sa1Config,
			CpuType.Gsu => GsuConfig,
			CpuType.Cx4 => Cx4Config,
			CpuType.St018 => St018Config,
			CpuType.Gameboy => GbConfig,
			CpuType.Nes => NesConfig,
			CpuType.Pce => PceConfig,
			CpuType.Sms => SmsConfig,
			CpuType.Gba => GbaConfig,
			CpuType.Ws => WsConfig,
			CpuType.Lynx => LynxConfig,
			CpuType.Genesis => GenesisConfig,
			CpuType.ChannelF => ChannelFConfig,
			CpuType.Atari2600 => Atari2600Config,
			_ => throw new NotImplementedException("Unsupport cpu type")
		};
	}
}
