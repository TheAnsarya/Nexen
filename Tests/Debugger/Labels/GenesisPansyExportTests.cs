using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Pansy.Core;
using Xunit;

namespace Nexen.Tests.Debugger.Labels;

/// <summary>
/// Targeted Pansy metadata export tests for the Sega Genesis / Mega Drive platform.
/// Validates memory region mappings, platform ID, symbol roundtrip, and
/// PlatformDefaults alignment for downstream Peony/Poppy tooling.
/// </summary>
public class GenesisPansyExportTests {
	// Pansy format constants
	private const string MAGIC = "PANSY\0\0\0";
	private const ushort VERSION = 0x0100;

	// Pansy section types
	private const uint SECTION_CODE_DATA_MAP = 0x0001;
	private const uint SECTION_SYMBOLS = 0x0002;
	private const uint SECTION_MEMORY_REGIONS = 0x0004;

	// Platform ID for Genesis per Pansy spec
	private const byte PLATFORM_GENESIS = 0x05;

	// CDL flags (Pansy spec)
	private const byte CDL_CODE = 0x01;
	private const byte CDL_DATA = 0x02;

	// Genesis memory map constants (M68000, 24-bit address bus)
	private const uint ROM_START = 0x000000;
	private const uint ROM_END = 0x3fffff;
	private const uint Z80_START = 0xa00000;
	private const uint Z80_END = 0xa0ffff;
	private const uint IO_START = 0xa10000;
	private const uint IO_END = 0xa1001f;
	private const uint VDP_START = 0xc00000;
	private const uint VDP_END = 0xc0001f;
	private const uint WRAM_START = 0xff0000;
	private const uint WRAM_END = 0xffffff;

	#region Platform ID Tests

	[Fact]
	public void PlatformId_Genesis_IsHex05() {
		Assert.Equal(0x05, PLATFORM_GENESIS);
		Assert.Equal(PansyLoader.PLATFORM_GENESIS, PLATFORM_GENESIS);
	}

	[Fact]
	public void PlatformId_Genesis_MatchesPansyCoreConstant() {
		Assert.Equal(PansyLoader.PLATFORM_GENESIS, (byte)0x05);
	}

	[Fact]
	public void PlatformId_Genesis_ResolvesToCorrectName() {
		var pansyFile = BuildMinimalPansyFile([], SECTION_CODE_DATA_MAP, PLATFORM_GENESIS);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal("Sega Genesis", PansyLoader.GetPlatformName(loader.Platform));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	#endregion

	#region Memory Region Tests

	[Fact]
	public void MemoryRegions_Genesis_HasFiveRegions() {
		var regionData = BuildGenesisMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_GENESIS);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(5, loader.MemoryRegions.Count);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_ROM_CorrectRange() {
		var regionData = BuildGenesisMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_GENESIS);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var rom = loader.MemoryRegions.First(r => r.Name == "ROM");
			Assert.Equal(ROM_START, rom.Start);
			Assert.Equal(ROM_END, rom.End);
			Assert.Equal((byte)MemoryRegionType.ROM, rom.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_Z80_CorrectRange() {
		var regionData = BuildGenesisMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_GENESIS);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var z80 = loader.MemoryRegions.First(r => r.Name == "Z80_Address_Space");
			Assert.Equal(Z80_START, z80.Start);
			Assert.Equal(Z80_END, z80.End);
			Assert.Equal((byte)MemoryRegionType.RAM, z80.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_IO_CorrectRange() {
		var regionData = BuildGenesisMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_GENESIS);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var io = loader.MemoryRegions.First(r => r.Name == "IO_Registers");
			Assert.Equal(IO_START, io.Start);
			Assert.Equal(IO_END, io.End);
			Assert.Equal((byte)MemoryRegionType.IO, io.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_VDP_CorrectRange() {
		var regionData = BuildGenesisMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_GENESIS);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var vdp = loader.MemoryRegions.First(r => r.Name == "VDP_Registers");
			Assert.Equal(VDP_START, vdp.Start);
			Assert.Equal(VDP_END, vdp.End);
			Assert.Equal((byte)MemoryRegionType.IO, vdp.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_WorkRAM_CorrectRange() {
		var regionData = BuildGenesisMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_GENESIS);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var wram = loader.MemoryRegions.First(r => r.Name == "Work_RAM");
			Assert.Equal(WRAM_START, wram.Start);
			Assert.Equal(WRAM_END, wram.End);
			Assert.Equal((byte)MemoryRegionType.WRAM, wram.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_AllBanksAreZero() {
		var regionData = BuildGenesisMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_GENESIS);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			foreach (var region in loader.MemoryRegions) {
				Assert.Equal(0, region.Bank);
			}
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_SortedByStartAddress() {
		var regionData = BuildGenesisMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_GENESIS);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var starts = loader.MemoryRegions.Select(r => r.Start).ToList();
			for (int i = 1; i < starts.Count; i++) {
				Assert.True(starts[i] >= starts[i - 1],
					$"Region {i} start (0x{starts[i]:x6}) should be >= region {i - 1} start (0x{starts[i - 1]:x6})");
			}
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_NoOverlap() {
		var regionData = BuildGenesisMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_GENESIS);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var sorted = loader.MemoryRegions.OrderBy(r => r.Start).ToList();
			for (int i = 1; i < sorted.Count; i++) {
				Assert.True(sorted[i].Start > sorted[i - 1].End,
					$"Region '{sorted[i].Name}' overlaps with '{sorted[i - 1].Name}'");
			}
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_PlatformDefaults_MatchExporter() {
		var defaults = PlatformDefaults.GetDefaultRegions(PLATFORM_GENESIS);
		Assert.NotNull(defaults);
		Assert.True(defaults.Length >= 4, "PlatformDefaults should have at least 4 regions for Genesis");

		Assert.Contains(defaults, r => r.Name.Contains("ROM", StringComparison.OrdinalIgnoreCase));
		Assert.Contains(defaults, r => r.Name.Contains("Z80", StringComparison.OrdinalIgnoreCase));
		Assert.Contains(defaults, r => r.Name.Contains("VDP", StringComparison.OrdinalIgnoreCase));
		Assert.Contains(defaults, r => r.Name.Contains("RAM", StringComparison.OrdinalIgnoreCase));
	}

	#endregion

	#region Symbol Tests

	[Fact]
	public void Symbols_VdpRegister_Roundtrip() {
		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0xc00000, "VDP_DATA", SymbolType.Constant),
			(0xc00004, "VDP_CTRL", SymbolType.Constant),
		};
		var symbolData = BuildSymbolSection(symbols);
		var pansyFile = BuildMinimalPansyFile(symbolData, SECTION_SYMBOLS, PLATFORM_GENESIS, romSize: 0x400000);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(2, loader.Symbols.Count);
			Assert.Contains(loader.Symbols, s => s.Value == "VDP_DATA" && s.Key == 0xc00000);
			Assert.Contains(loader.Symbols, s => s.Value == "VDP_CTRL" && s.Key == 0xc00004);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Symbols_IoRegisters_Roundtrip() {
		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0xa10001, "VERSION", SymbolType.Constant),
			(0xa10003, "DATA1", SymbolType.Constant),
			(0xa10005, "DATA2", SymbolType.Constant),
		};
		var symbolData = BuildSymbolSection(symbols);
		var pansyFile = BuildMinimalPansyFile(symbolData, SECTION_SYMBOLS, PLATFORM_GENESIS, romSize: 0x400000);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(3, loader.Symbols.Count);
			Assert.Contains(loader.Symbols, s => s.Value == "VERSION");
			Assert.Contains(loader.Symbols, s => s.Value == "DATA1");
			Assert.Contains(loader.Symbols, s => s.Value == "DATA2");
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Symbols_InterruptVectors_Roundtrip() {
		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0x000000, "SSP", SymbolType.InterruptVector),
			(0x000004, "RESET_VECTOR", SymbolType.InterruptVector),
			(0x000070, "LEVEL6_VECTOR", SymbolType.InterruptVector),
		};
		var symbolData = BuildSymbolSection(symbols);
		var pansyFile = BuildMinimalPansyFile(symbolData, SECTION_SYMBOLS, PLATFORM_GENESIS, romSize: 0x400000);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(3, loader.Symbols.Count);
			Assert.Equal("SSP", loader.Symbols[0x000000]);
			var sspEntry = loader.SymbolEntries[0x000000];
			Assert.Equal("SSP", sspEntry.Name);
			Assert.Equal(SymbolType.InterruptVector, sspEntry.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Symbols_PlatformDefaults_HasVdpAndIo() {
		var defaults = PlatformDefaults.GetDefaultSymbolEntries(PLATFORM_GENESIS);
		Assert.NotNull(defaults);
		Assert.True(defaults.Count >= 10, "Genesis should have at least 10 default symbols");

		Assert.Contains(defaults, s => s.Value.Name == "VDP_DATA");
		Assert.Contains(defaults, s => s.Value.Name == "VDP_CTRL");
		Assert.Contains(defaults, s => s.Value.Name == "Z80_BUSREQ");
		Assert.Contains(defaults, s => s.Value.Name == "Z80_RESET");
		Assert.Contains(defaults, s => s.Value.Name == "PSG");
	}

	#endregion

	#region CDL Flag Tests

	[Fact]
	public void CdlFlags_CodeByte_MapsToCode() {
		byte[] cdl = new byte[0x100000]; // 1MB ROM
		cdl[0] = CDL_CODE;
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_GENESIS, romSize: 0x100000);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.True(loader.CodeOffsets.Contains(0));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CdlFlags_DataByte_MapsToData() {
		byte[] cdl = new byte[0x100000];
		cdl[0x100] = CDL_DATA;
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_GENESIS, romSize: 0x100000);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.True(loader.DataOffsets.Contains(0x100));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	#endregion

	#region Helper Methods

	private static byte[] BuildGenesisMemoryRegions() {
		var regions = new List<(uint Start, uint End, string Name, byte Type, byte Bank)> {
			(ROM_START, ROM_END, "ROM", (byte)MemoryRegionType.ROM, 0),
			(Z80_START, Z80_END, "Z80_Address_Space", (byte)MemoryRegionType.RAM, 0),
			(IO_START, IO_END, "IO_Registers", (byte)MemoryRegionType.IO, 0),
			(VDP_START, VDP_END, "VDP_Registers", (byte)MemoryRegionType.IO, 0),
			(WRAM_START, WRAM_END, "Work_RAM", (byte)MemoryRegionType.WRAM, 0),
		};

		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);
		foreach (var (start, end, name, type, bank) in regions) {
			writer.Write(start);
			writer.Write(end);
			writer.Write(type);
			writer.Write(bank);
			writer.Write((ushort)0); // Flags
			byte[] nameBytes = Encoding.UTF8.GetBytes(name);
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
		}
		return ms.ToArray();
	}

	private static byte[] BuildSymbolSection(List<(uint Address, string Name, SymbolType Type)> symbols) {
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);
		foreach (var (address, name, type) in symbols) {
			writer.Write(address);
			writer.Write((byte)type);
			writer.Write((byte)0); // Flags
			byte[] nameBytes = Encoding.UTF8.GetBytes(name);
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
			writer.Write((ushort)0); // Value length
		}
		return ms.ToArray();
	}

	private static byte[] BuildMinimalPansyFile(byte[] sectionData, uint sectionType, byte platform, uint romSize = 0x400000, uint romCrc = 0xdeadbeef) {
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		int sectionCount = sectionData.Length > 0 ? 1 : 0;

		// Header (32 bytes)
		writer.Write(Encoding.ASCII.GetBytes(MAGIC));
		writer.Write(VERSION);
		writer.Write((ushort)0); // Flags
		writer.Write(platform);
		writer.Write((byte)0);   // Reserved
		writer.Write((ushort)0); // Reserved
		writer.Write(romSize);
		writer.Write(romCrc);
		writer.Write((uint)sectionCount);
		writer.Write((uint)0);   // Reserved

		if (sectionCount > 0) {
			uint dataOffset = 32 + 16;

			writer.Write(sectionType);
			writer.Write(dataOffset);
			writer.Write((uint)sectionData.Length);
			writer.Write((uint)sectionData.Length);

			writer.Write(sectionData);
		}

		return ms.ToArray();
	}

	private static string WriteTempPansy(byte[] data) {
		var path = Path.Combine(Path.GetTempPath(), $"genesis_test_{Guid.NewGuid():N}.pansy");
		File.WriteAllBytes(path, data);
		return path;
	}

	private static void CleanupTemp(string path) {
		if (File.Exists(path)) File.Delete(path);
	}

	#endregion
}
