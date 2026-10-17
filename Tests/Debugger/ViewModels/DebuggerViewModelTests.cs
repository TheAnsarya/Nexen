using Nexen.Config;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using System.Reflection;
using Xunit;

namespace Nexen.Tests.Debugger.ViewModels;

/// <summary>
/// Comprehensive tests for debugger ViewModel logic that can be tested
/// without native emulation core dependencies.
/// </summary>
public class DebuggerViewModelTests {
	private static void SetPrivateField<T>(object instance, string fieldName, T value) {
		FieldInfo? field = instance.GetType().GetField(fieldName, BindingFlags.Instance | BindingFlags.NonPublic);
		Assert.NotNull(field);
		field!.SetValue(instance, value);
	}

	#region TraceLogger GetAutoFormat — All CPU Types

	/// <summary>
	/// Helper to create a fully-enabled trace logger config.
	/// </summary>
	private static TraceLoggerCpuConfig AllEnabledConfig(StatusFlagFormat statusFormat = StatusFlagFormat.Hexadecimal) => new() {
		ShowRegisters = true,
		ShowStatusFlags = true,
		StatusFormat = statusFormat,
		ShowEffectiveAddresses = true,
		ShowMemoryValues = true,
		ShowFramePosition = true,
		ShowFrameCounter = true,
		ShowClockCounter = true,
		ShowByteCode = true
	};

	/// <summary>
	/// Helper to create a config with everything disabled.
	/// </summary>
	private static TraceLoggerCpuConfig AllDisabledConfig() => new() {
		ShowRegisters = false,
		ShowStatusFlags = false,
		StatusFormat = StatusFlagFormat.Hexadecimal,
		ShowEffectiveAddresses = false,
		ShowMemoryValues = false,
		ShowFramePosition = false,
		ShowFrameCounter = false,
		ShowClockCounter = false,
		ShowByteCode = false
	};

	[Theory]
	[InlineData(CpuType.Snes)]
	[InlineData(CpuType.Sa1)]
	public void GetAutoFormat_SnesFamilyCpu_Uses16BitRegisters(CpuType cpuType) {
		var cfg = AllEnabledConfig();
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, cpuType);

		Assert.Contains("A:[A,4h]", format);
		Assert.Contains("X:[X,4h]", format);
		Assert.Contains("Y:[Y,4h]", format);
		Assert.Contains("S:[SP,4h]", format);
		Assert.Contains("D:[D,4h]", format);
		Assert.Contains("DB:[DB,2h]", format);
		Assert.Contains("P:[P,h]", format);
	}

	[Theory]
	[InlineData(CpuType.Nes)]
	[InlineData(CpuType.Spc)]
	[InlineData(CpuType.Pce)]
	public void GetAutoFormat_8BitCpu_Uses8BitRegisters(CpuType cpuType) {
		var cfg = AllEnabledConfig();
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, cpuType);

		Assert.Contains("A:[A,2h]", format);
		Assert.Contains("X:[X,2h]", format);
		Assert.Contains("Y:[Y,2h]", format);
		Assert.Contains("S:[SP,2h]", format);
		Assert.Contains("P:[P,h]", format);
	}

	[Theory]
	[InlineData(CpuType.Gba)]
	[InlineData(CpuType.St018)]
	public void GetAutoFormat_ArmCpu_Uses32BitRegistersAndCpsr(CpuType cpuType) {
		var cfg = AllEnabledConfig();
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, cpuType);

		// ARM has R0-R15 32-bit registers
		Assert.Contains("R0:[R0,8h]", format);
		Assert.Contains("R15:[R15,8h]", format);
		Assert.Contains("CPSR:[CPSR,8h]", format);
		Assert.Contains("Mode:", format);
		Assert.DoesNotContain("A:[A", format);
	}

	[Fact]
	public void GetAutoFormat_Gameboy_UsesCorrectRegisterSet() {
		var cfg = AllEnabledConfig();
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, CpuType.Gameboy);

		Assert.Contains("A:[A,2h]", format);
		Assert.Contains("B:[B,2h]", format);
		Assert.Contains("C:[C,2h]", format);
		Assert.Contains("D:[D,2h]", format);
		Assert.Contains("E:[E,2h]", format);
		Assert.Contains("HL:[H,2h][L,2h]", format);
		Assert.Contains("S:[SP,4h]", format);
		Assert.Contains("F:[PS,h]", format);
	}

	[Fact]
	public void GetAutoFormat_Sms_UsesZ80RegisterSet() {
		var cfg = AllEnabledConfig();
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, CpuType.Sms);

		Assert.Contains("A:[A,2h]", format);
		Assert.Contains("IX:[IX,4h]", format);
		Assert.Contains("IY:[IY,4h]", format);
		Assert.Contains("HL:[H,2h][L,2h]", format);
		Assert.Contains("F:[PS,h]", format);
	}

	[Fact]
	public void GetAutoFormat_Ws_UsesX86RegisterSet() {
		var cfg = AllEnabledConfig();
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, CpuType.Ws);

		Assert.Contains("AX:[AX,4h]", format);
		Assert.Contains("BX:[BX,4h]", format);
		Assert.Contains("CX:[CX,4h]", format);
		Assert.Contains("DX:[DX,4h]", format);
		Assert.Contains("DS:[DS,4h]", format);
		Assert.Contains("BP:[BP,4h]", format);
		Assert.Contains("F:[F,4h]", format);
	}

	[Fact]
	public void GetAutoFormat_Gsu_Uses16BitRegistersAndSfr() {
		var cfg = AllEnabledConfig();
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, CpuType.Gsu);

		Assert.Contains("SRC:[SRC,2h]", format);
		Assert.Contains("DST:[DST,2h]", format);
		Assert.Contains("R0:[R0,4h]", format);
		Assert.Contains("R15:[R15,4h]", format);
		Assert.Contains("SFR:[SFR,h]", format);
		Assert.DoesNotContain("CPSR", format);
	}

	[Fact]
	public void GetAutoFormat_Lynx_Uses8BitRegisters() {
		var cfg = AllEnabledConfig();
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, CpuType.Lynx);

		Assert.Contains("A:[A,2h]", format);
		Assert.Contains("X:[X,2h]", format);
		Assert.Contains("Y:[Y,2h]", format);
		Assert.Contains("S:[SP,2h]", format);
		Assert.Contains("P:[P,h]", format);
	}

	[Theory]
	[InlineData(CpuType.Nes)]
	[InlineData(CpuType.Snes)]
	[InlineData(CpuType.Gameboy)]
	[InlineData(CpuType.Gba)]
	[InlineData(CpuType.Sms)]
	[InlineData(CpuType.Ws)]
	[InlineData(CpuType.Lynx)]
	[InlineData(CpuType.Atari2600)]
	public void GetAutoFormat_AllCpuTypes_AlwaysContainsDisassembly(CpuType cpuType) {
		var cfg = AllEnabledConfig();
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, cpuType);

		Assert.Contains("[Disassembly]", format);
	}

	[Theory]
	[InlineData(CpuType.Nes)]
	[InlineData(CpuType.Snes)]
	[InlineData(CpuType.Gba)]
	public void GetAutoFormat_FramePositionEnabled_ContainsScanlineAndCycle(CpuType cpuType) {
		var cfg = AllDisabledConfig();
		cfg.ShowFramePosition = true;
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, cpuType);

		Assert.Contains("V:[Scanline,3]", format);
		Assert.Contains("H:[Cycle,3]", format);
	}

	[Theory]
	[InlineData(CpuType.Nes)]
	[InlineData(CpuType.Snes)]
	[InlineData(CpuType.Gba)]
	public void GetAutoFormat_AllDisabled_OnlyContainsDisassemblyAndAlign(CpuType cpuType) {
		var cfg = AllDisabledConfig();
		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, cpuType);

		Assert.StartsWith("[Disassembly]", format);
		Assert.DoesNotContain("A:[A", format);
		Assert.DoesNotContain("P:[P", format);
		Assert.DoesNotContain("[Scanline", format);
		Assert.DoesNotContain("[FrameCount]", format);
		Assert.DoesNotContain("[CycleCount]", format);
		Assert.DoesNotContain("[ByteCode]", format);
	}

	[Theory]
	[InlineData(StatusFlagFormat.Hexadecimal)]
	[InlineData(StatusFlagFormat.CompactText)]
	[InlineData(StatusFlagFormat.Text)]
	public void GetAutoFormat_AllStatusFormats_ProduceValidOutput(StatusFlagFormat format) {
		var cfg = AllEnabledConfig(format);
		string result = TraceLoggerOptionTab.GetAutoFormat(cfg, CpuType.Nes);

		Assert.NotEmpty(result);
		Assert.Contains("[Disassembly]", result);
		// Status flag presence depends on format, but output should never be empty
		Assert.True(result.Length > 20);
	}

	#endregion

	#region ProfiledFunctionViewModel Edge Cases

	[Fact]
	public void ProfiledFunction_ZeroCallCount_AvgCyclesIsZero() {
		var vm = new ProfiledFunctionViewModel();
		var func = new ProfiledFunction {
			ExclusiveCycles = 100,
			InclusiveCycles = 200,
			CallCount = 0,
			MinCycles = UInt64.MaxValue,
			MaxCycles = 0,
			Address = new AddressInfo { Address = -1 }
		};

		vm.Update(func, CpuType.Nes, 1000);
		_ = vm.FunctionName;

		Assert.Equal("0", vm.AvgCycles);
	}

	[Fact]
	public void ProfiledFunction_MinCyclesMaxValue_DisplaysNA() {
		var vm = new ProfiledFunctionViewModel();
		var func = new ProfiledFunction {
			ExclusiveCycles = 50,
			InclusiveCycles = 50,
			CallCount = 1,
			MinCycles = UInt64.MaxValue,
			MaxCycles = 0,
			Address = new AddressInfo { Address = -1 }
		};

		vm.Update(func, CpuType.Nes, 1000);
		_ = vm.FunctionName;

		Assert.Equal("n/a", vm.MinCycles);
	}

	[Fact]
	public void ProfiledFunction_MaxCyclesZero_DisplaysNA() {
		var vm = new ProfiledFunctionViewModel();
		var func = new ProfiledFunction {
			ExclusiveCycles = 50,
			InclusiveCycles = 50,
			CallCount = 1,
			MinCycles = 50,
			MaxCycles = 0,
			Address = new AddressInfo { Address = -1 }
		};

		vm.Update(func, CpuType.Nes, 1000);
		_ = vm.FunctionName;

		Assert.Equal("n/a", vm.MaxCycles);
	}

	[Fact]
	public void ProfiledFunction_NormalData_CalculatesPercentagesCorrectly() {
		var vm = new ProfiledFunctionViewModel();
		var func = new ProfiledFunction {
			ExclusiveCycles = 250,
			InclusiveCycles = 500,
			CallCount = 5,
			MinCycles = 80,
			MaxCycles = 120,
			Address = new AddressInfo { Address = 0x8000 }
		};

		vm.Update(func, CpuType.Nes, 1000);
		_ = vm.FunctionName;

		Assert.Equal("25.00", vm.ExclusivePercent);
		Assert.Equal("50.00", vm.InclusivePercent);
		Assert.Equal("100", vm.AvgCycles); // 500 / 5
		Assert.Equal("80", vm.MinCycles);
		Assert.Equal("120", vm.MaxCycles);
		Assert.Equal("250", vm.ExclusiveCycles);
		Assert.Equal("500", vm.InclusiveCycles);
		Assert.Equal("5", vm.CallCount);
	}

	[Fact]
	public void ProfiledFunction_LargeCycleCounts_NoOverflow() {
		var vm = new ProfiledFunctionViewModel();
		var func = new ProfiledFunction {
			ExclusiveCycles = 5_000_000_000,
			InclusiveCycles = 10_000_000_000,
			CallCount = 1_000_000,
			MinCycles = 1000,
			MaxCycles = 50000,
			Address = new AddressInfo { Address = 0x1000 }
		};

		vm.Update(func, CpuType.Snes, 20_000_000_000);
		_ = vm.FunctionName;

		Assert.Equal("25.00", vm.ExclusivePercent);
		Assert.Equal("50.00", vm.InclusivePercent);
		Assert.Equal("10000", vm.AvgCycles); // 10B / 1M
	}

	#endregion

	#region ProfilerTab Grid Logic

	[Fact]
	public void ProfilerTab_EmptyData_ShowsNoRows() {
		var tab = new ProfilerTab {
			CpuType = CpuType.Nes
		};

		var rows = Array.Empty<ProfiledFunction>();

		SetPrivateField(tab, "_coreProfilerData", rows);
		SetPrivateField(tab, "_dataSize", 0);

		tab.RefreshGrid();

		Assert.Empty(tab.GridData);
	}

	[Fact]
	public void ProfilerTab_MultipleRealFunctions_ExcludesResetRow() {
		var tab = new ProfilerTab {
			CpuType = CpuType.Snes
		};

		var rows = new[] {
			new ProfiledFunction {
				Address = new AddressInfo { Address = -1 },
				ExclusiveCycles = 100,
				InclusiveCycles = 100
			},
			new ProfiledFunction {
				Address = new AddressInfo { Address = 0x8000 },
				ExclusiveCycles = 200,
				InclusiveCycles = 300
			},
			new ProfiledFunction {
				Address = new AddressInfo { Address = 0x9000 },
				ExclusiveCycles = 150,
				InclusiveCycles = 250
			}
		};

		SetPrivateField(tab, "_coreProfilerData", rows);
		SetPrivateField(tab, "_dataSize", rows.Length);

		tab.RefreshGrid();

		Assert.Equal(2, tab.GridData.Count);
		Assert.All(tab.GridData, g => Assert.DoesNotContain("[Reset]", g.FunctionName));
	}

	[Fact]
	public void ProfilerTab_OnlyResetFunction_ShowsResetRow() {
		var tab = new ProfilerTab {
			CpuType = CpuType.Gameboy
		};

		var rows = new[] {
			new ProfiledFunction {
				Address = new AddressInfo { Address = -1 },
				ExclusiveCycles = 100,
				InclusiveCycles = 100
			}
		};

		SetPrivateField(tab, "_coreProfilerData", rows);
		SetPrivateField(tab, "_dataSize", rows.Length);

		tab.RefreshGrid();

		Assert.Single(tab.GridData);
		Assert.Equal("[Reset]", tab.GridData[0].FunctionName);
	}

	#endregion
}
