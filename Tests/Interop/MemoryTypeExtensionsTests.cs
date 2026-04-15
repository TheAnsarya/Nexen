using System;
using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Interop;

public sealed class MemoryTypeExtensionsTests {
	[Fact]
	public void ToCpuType_MapsGenesisMemoryTypes() {
		Assert.Equal(CpuType.Genesis, MemoryType.GenesisMemory.ToCpuType());
		Assert.Equal(CpuType.Genesis, MemoryType.GenesisPrgRom.ToCpuType());
		Assert.Equal(CpuType.Genesis, MemoryType.GenesisWorkRam.ToCpuType());
		Assert.Equal(CpuType.Genesis, MemoryType.GenesisVideoRam.ToCpuType());
		Assert.Equal(CpuType.Genesis, MemoryType.GenesisPaletteRam.ToCpuType());
	}

	[Fact]
	public void ToCpuType_DoesNotThrowForConcreteMemoryTypes() {
		foreach (MemoryType memType in Enum.GetValues<MemoryType>()) {
			if (memType == MemoryType.None) {
				continue;
			}

			Exception? ex = Record.Exception(() => _ = memType.ToCpuType());
			Assert.Null(ex);
		}
	}

	[Fact]
	public void CanAccessMemoryType_DoesNotThrowWhenEnumeratingMemoryTypesForNes() {
		foreach (MemoryType memType in Enum.GetValues<MemoryType>()) {
			Exception? ex = Record.Exception(() => _ = CpuType.Nes.CanAccessMemoryType(memType));
			Assert.Null(ex);
		}
	}
}
