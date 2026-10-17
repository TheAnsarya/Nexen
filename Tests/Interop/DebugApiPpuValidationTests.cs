using System;
using System.Reflection;
using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Interop;

public class DebugApiPpuValidationTests {
	[Fact]
	public void IsValidPpuState_AcceptsGenesisVdpStateForGenesis() {
		GenesisVdpState state = new GenesisVdpState {
			Registers = new byte[24],
			Cram = new ushort[64],
			Vsram = new ushort[40]
		};

		bool result = InvokeIsValidPpuState(state, CpuType.Genesis);
		Assert.True(result);
	}

	[Fact]
	public void IsValidPpuState_AcceptsChannelFVideoStateForChannelF() {
		ChannelFVideoState state = new ChannelFVideoState();

		bool result = InvokeIsValidPpuState(state, CpuType.ChannelF);
		Assert.True(result);
	}

	[Fact]
	public void IsValidPpuState_RejectsMismatchedStateType() {
		NesPpuState state = new NesPpuState();

		bool result = InvokeIsValidPpuState(state, CpuType.Genesis);
		Assert.False(result);
	}

	[Fact]
	public void CpuTypeExtensions_MapGenesisToExpectedTypes() {
		Assert.Equal(ConsoleType.Genesis, CpuType.Genesis.GetConsoleType());
		Assert.Equal(MemoryType.GenesisMemory, CpuType.Genesis.ToMemoryType());
		Assert.Equal(MemoryType.GenesisPrgRom, CpuType.Genesis.GetPrgRomMemoryType());
		Assert.Equal(MemoryType.GenesisWorkRam, CpuType.Genesis.GetSystemRamType());
		Assert.Equal(MemoryType.GenesisVideoRam, CpuType.Genesis.GetVramMemoryType());
	}

	private static bool InvokeIsValidPpuState<T>(T state, CpuType cpuType) where T : struct, BaseState {
		MethodInfo? method = typeof(DebugApi).GetMethod("IsValidPpuState", BindingFlags.NonPublic | BindingFlags.Static);
		Assert.NotNull(method);

		MethodInfo genericMethod = method!.MakeGenericMethod(typeof(T));
		object[] args = [state, cpuType];
		object? result = genericMethod.Invoke(null, args);
		Assert.NotNull(result);
		return (bool)result;
	}
}
