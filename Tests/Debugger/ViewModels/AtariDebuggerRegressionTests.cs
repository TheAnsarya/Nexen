using Nexen.Config;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using System.Reflection;
using Xunit;

namespace Nexen.Tests.Debugger.ViewModels;

/// <summary>
/// Regression tests for Atari 2600 debugger formatting behavior.
/// </summary>
public class AtariDebuggerRegressionTests {
	private static void SetPrivateField<T>(object instance, string fieldName, T value) {
		FieldInfo? field = instance.GetType().GetField(fieldName, BindingFlags.Instance | BindingFlags.NonPublic);
		Assert.NotNull(field);
		field!.SetValue(instance, value);
	}

	[Theory]
	[InlineData(StatusFlagFormat.Hexadecimal, "P:[P,h]")]
	[InlineData(StatusFlagFormat.CompactText, "P:[P]")]
	[InlineData(StatusFlagFormat.Text, "P:[P,8]")]
	public void GetAutoFormat_Atari2600_Uses6502RegisterAndStatusTokens(StatusFlagFormat statusFormat, string expectedStatusToken) {
		var cfg = new TraceLoggerCpuConfig {
			ShowRegisters = true,
			ShowStatusFlags = true,
			StatusFormat = statusFormat,
			ShowEffectiveAddresses = false,
			ShowMemoryValues = false,
			ShowFramePosition = false,
			ShowFrameCounter = false,
			ShowClockCounter = false,
			ShowByteCode = false
		};

		string format = TraceLoggerOptionTab.GetAutoFormat(cfg, CpuType.Atari2600);

		Assert.Contains("A:[A,2h]", format);
		Assert.Contains("X:[X,2h]", format);
		Assert.Contains("Y:[Y,2h]", format);
		Assert.Contains("S:[SP,2h]", format);
		Assert.Contains(expectedStatusToken, format);
		Assert.DoesNotContain("CPSR", format);
	}

	[Fact]
	public void ProfiledFunctionViewModel_Update_ZeroTotalCycles_UsesZeroPercents() {
		var vm = new ProfiledFunctionViewModel();
		var profiledFunc = new ProfiledFunction {
			ExclusiveCycles = 90,
			InclusiveCycles = 180,
			CallCount = 2,
			MinCycles = 40,
			MaxCycles = 120,
			Address = new AddressInfo { Address = -1 }
		};

		vm.Update(profiledFunc, CpuType.Atari2600, 0);
		_ = vm.FunctionName;

		Assert.Equal("0.00", vm.ExclusivePercent);
		Assert.Equal("0.00", vm.InclusivePercent);
		Assert.Equal("90", vm.ExclusiveCycles);
		Assert.Equal("180", vm.InclusiveCycles);
		Assert.Equal("90", vm.AvgCycles);
	}

	[Fact]
	public void ProfilerTab_RefreshGrid_HidesResetRow_WhenRealRowsExist() {
		var tab = new ProfilerTab {
			CpuType = CpuType.Atari2600
		};

		var rows = new[] {
			new ProfiledFunction {
				Address = new AddressInfo { Address = -1 },
				ExclusiveCycles = 100,
				InclusiveCycles = 100
			},
			new ProfiledFunction {
				Address = new AddressInfo { Address = 0x1234 },
				ExclusiveCycles = 50,
				InclusiveCycles = 50
			}
		};

		SetPrivateField(tab, "_coreProfilerData", rows);
		SetPrivateField(tab, "_dataSize", rows.Length);

		tab.RefreshGrid();

		Assert.Single(tab.GridData);
		Assert.DoesNotContain("[Reset]", tab.GridData[0].FunctionName);
	}

	[Fact]
	public void ProfilerTab_RefreshGrid_ShowsResetRow_WhenNoRealRowsExist() {
		var tab = new ProfilerTab {
			CpuType = CpuType.Atari2600
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
}
