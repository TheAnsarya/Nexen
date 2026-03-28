using Nexen.Config;
using Nexen.Interop;
using Nexen.ViewModels;

namespace Nexen.Debugger.ViewModels;
public sealed class DebuggerOptionsViewModel : ViewModelBase {
	public DebuggerConfig Config { get; }

	public bool IsSnes { get; }
	public bool IsSpc { get; }
	public bool IsNes { get; }
	public bool IsGameboy { get; }
	public bool IsPce { get; }
	public bool IsSms { get; }
	public bool IsGba { get; }
	public bool IsWs { get; }
	public bool IsLynx { get; }
	public bool IsChannelF { get; }

	public bool HasSpecificBreakOptions { get; }

	public DebuggerOptionsViewModel() : this(new DebuggerConfig(), CpuType.Snes) { }

	public DebuggerOptionsViewModel(DebuggerConfig config, CpuType cpuType) {
		Config = config;
		IsSnes = cpuType == CpuType.Snes;
		IsSpc = cpuType == CpuType.Spc;
		IsNes = cpuType == CpuType.Nes;
		IsGameboy = cpuType == CpuType.Gameboy;
		IsPce = cpuType == CpuType.Pce;
		IsSms = cpuType == CpuType.Sms;
		IsGba = cpuType == CpuType.Gba;
		IsWs = cpuType == CpuType.Ws;
		IsLynx = cpuType == CpuType.Lynx;
		IsChannelF = cpuType == CpuType.ChannelF;

		HasSpecificBreakOptions = IsSnes || IsSpc || IsNes || IsGameboy || IsPce || IsSms || IsGba || IsWs || IsLynx || IsChannelF;
	}
}
