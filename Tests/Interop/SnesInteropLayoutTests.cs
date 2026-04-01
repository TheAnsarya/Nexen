using Nexen.Interop;
using System.Runtime.InteropServices;
using Xunit;

namespace Nexen.Tests.Interop;

public class SnesInteropLayoutTests {
	[Fact]
	public void SnesPpuState_BgMode_IsFirstField() {
		int bgModeOffset = (int)Marshal.OffsetOf<SnesPpuState>(nameof(SnesPpuState.BgMode));
		Assert.Equal(0, bgModeOffset);
	}

	[Fact]
	public void SnesPpuState_Cycle_IsAfterRegisterBlock() {
		int cgramLatchOffset = (int)Marshal.OffsetOf<SnesPpuState>(nameof(SnesPpuState.CgramAddressLatch));
		int cycleOffset = (int)Marshal.OffsetOf<SnesPpuState>(nameof(SnesPpuState.Cycle));

		Assert.True(cycleOffset > cgramLatchOffset);
	}

	[Fact]
	public void SnesPpuState_Mode7_IsAfterLayers() {
		int layersOffset = (int)Marshal.OffsetOf<SnesPpuState>(nameof(SnesPpuState.Layers));
		int mode7Offset = (int)Marshal.OffsetOf<SnesPpuState>(nameof(SnesPpuState.Mode7));

		Assert.True(mode7Offset > layersOffset);
	}
}
