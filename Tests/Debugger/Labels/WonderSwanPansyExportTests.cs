using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Pansy.Core;
using Xunit;

namespace Nexen.Tests.Debugger.Labels;

/// <summary>
/// Targeted Pansy metadata export tests for the WonderSwan platform.
/// Validates memory region mappings, platform ID, symbol roundtrip, and
/// PlatformDefaults alignment for downstream Peony/Poppy tooling.
/// </summary>
public class WonderSwanPansyExportTests {
	// Pansy format constants
	private const string MAGIC = "PANSY\0\0\0";
	private const ushort VERSION = 0x0100;

	// Pansy section types
	private const uint SECTION_CODE_DATA_MAP = 0x0001;
	private const uint SECTION_SYMBOLS = 0x0002;
	private const uint SECTION_MEMORY_REGIONS = 0x0004;

	// Platform ID for WonderSwan per Pansy spec
	private const byte PLATFORM_WONDERSWAN = 0x0a;

	// CDL flags (Pansy spec)
	private const byte CDL_CODE = 0x01;
	private const byte CDL_DATA = 0x02;

	// WonderSwan memory map constants (V30MZ, 20-bit segmented addressing)
	private const uint RAM_START = 0x00000;
	private const uint RAM_END = 0x03fff;
	private const uint IO_START = 0x00000;
	private const uint IO_END = 0x000ff;
	private const uint ROM_START = 0x20000;
	private const uint ROM_END = 0xfffff;

	#region Platform ID Tests

	[Fact]
	public void PlatformId_WonderSwan_IsHex0A() {
		Assert.Equal(0x0a, PLATFORM_WONDERSWAN);
		Assert.Equal(PansyLoader.PLATFORM_WONDERSWAN, PLATFORM_WONDERSWAN);
	}

	[Fact]
	public void PlatformId_WonderSwan_MatchesPansyCoreConstant() {
		Assert.Equal(PansyLoader.PLATFORM_WONDERSWAN, (byte)0x0a);
	}

	[Fact]
	public void PlatformId_WonderSwan_ResolvesToCorrectName() {
		var pansyFile = BuildMinimalPansyFile([], SECTION_CODE_DATA_MAP, PLATFORM_WONDERSWAN);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal("WonderSwan", PansyLoader.GetPlatformName(loader.Platform));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	#endregion

	#region Memory Region Tests

	[Fact]
	public void MemoryRegions_WonderSwan_HasThreeRegions() {
		var regionData = BuildWonderSwanMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_WONDERSWAN);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(3, loader.MemoryRegions.Count);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_RAM_CorrectRange() {
		var regionData = BuildWonderSwanMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_WONDERSWAN);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var ram = loader.MemoryRegions.First(r => r.Name == "RAM");
			Assert.Equal(RAM_START, ram.Start);
			Assert.Equal(RAM_END, ram.End);
			Assert.Equal((byte)MemoryRegionType.RAM, ram.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_IO_CorrectRange() {
		var regionData = BuildWonderSwanMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_WONDERSWAN);
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
	public void MemoryRegions_ROM_CorrectRange() {
		var regionData = BuildWonderSwanMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_WONDERSWAN);
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
	public void MemoryRegions_AllBanksAreZero() {
		var regionData = BuildWonderSwanMemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_WONDERSWAN);
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
	public void MemoryRegions_PlatformDefaults_MatchExporter() {
		var defaults = PlatformDefaults.GetDefaultRegions(PLATFORM_WONDERSWAN);
		Assert.NotNull(defaults);
		Assert.True(defaults.Length >= 2, "PlatformDefaults should have at least 2 regions for WonderSwan");

		Assert.Contains(defaults, r => r.Name.Contains("RAM", StringComparison.OrdinalIgnoreCase));
		Assert.Contains(defaults, r => r.Name.Contains("ROM", StringComparison.OrdinalIgnoreCase));
	}

	#endregion

	#region Symbol Tests

	[Fact]
	public void Symbols_DisplayControl_Roundtrip() {
		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0x00, "DISPLAY_CTRL", SymbolType.Constant),
			(0x02, "LCD_LINE", SymbolType.Constant),
		};
		var symbolData = BuildSymbolSection(symbols);
		var pansyFile = BuildMinimalPansyFile(symbolData, SECTION_SYMBOLS, PLATFORM_WONDERSWAN, romSize: 0x100000);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(2, loader.Symbols.Count);
			Assert.Contains(loader.Symbols, s => s.Value == "DISPLAY_CTRL");
			Assert.Contains(loader.Symbols, s => s.Value == "LCD_LINE");
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Symbols_SoundRegisters_Roundtrip() {
		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0x80, "SND_CH1_PITCH_LO", SymbolType.Constant),
			(0x81, "SND_CH1_PITCH_HI", SymbolType.Constant),
			(0x88, "SND_CH2_PITCH_LO", SymbolType.Constant),
		};
		var symbolData = BuildSymbolSection(symbols);
		var pansyFile = BuildMinimalPansyFile(symbolData, SECTION_SYMBOLS, PLATFORM_WONDERSWAN, romSize: 0x100000);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(3, loader.Symbols.Count);
			Assert.Contains(loader.Symbols, s => s.Value == "SND_CH1_PITCH_LO");
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Symbols_PlatformDefaults_HasDisplayAndSound() {
		var defaults = PlatformDefaults.GetDefaultSymbolEntries(PLATFORM_WONDERSWAN);
		Assert.NotNull(defaults);
		Assert.True(defaults.Count >= 10, "WonderSwan should have at least 10 default symbols");

		Assert.Contains(defaults, s => s.Value.Name == "DISPLAY_CTRL");
		Assert.Contains(defaults, s => s.Value.Name.Contains("SND", StringComparison.OrdinalIgnoreCase));
	}

	#endregion

	#region CDL Flag Tests

	[Fact]
	public void CdlFlags_CodeByte_MapsToCode() {
		byte[] cdl = new byte[0x10000]; // 64KB ROM
		cdl[0] = CDL_CODE;
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_WONDERSWAN, romSize: 0x10000);
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
		byte[] cdl = new byte[0x10000];
		cdl[0x100] = CDL_DATA;
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_WONDERSWAN, romSize: 0x10000);
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

	private static byte[] BuildWonderSwanMemoryRegions() {
		var regions = new List<(uint Start, uint End, string Name, byte Type, byte Bank)> {
			(RAM_START, RAM_END, "RAM", (byte)MemoryRegionType.RAM, 0),
			(IO_START, IO_END, "IO_Registers", (byte)MemoryRegionType.IO, 0),
			(ROM_START, ROM_END, "ROM", (byte)MemoryRegionType.ROM, 0),
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

	private static byte[] BuildMinimalPansyFile(byte[] sectionData, uint sectionType, byte platform, uint romSize = 0x100000, uint romCrc = 0xdeadbeef) {
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
		var path = Path.Combine(Path.GetTempPath(), $"ws_test_{Guid.NewGuid():N}.pansy");
		File.WriteAllBytes(path, data);
		return path;
	}

	private static void CleanupTemp(string path) {
		if (File.Exists(path)) File.Delete(path);
	}

	#endregion
}
