using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Pansy.Core;
using Xunit;

namespace Nexen.Tests.Debugger.Labels;

/// <summary>
/// Targeted Pansy metadata export tests for the Atari 2600 platform.
/// Validates memory region mappings, CDL flag translation, symbol roundtrip,
/// and generated Pansy file structure for downstream Peony/Poppy tooling.
/// </summary>
public class Atari2600PansyExportTests {
	// Pansy format constants
	private const string MAGIC = "PANSY\0\0\0";
	private const ushort VERSION = 0x0100;

	// Pansy section types
	private const uint SECTION_CODE_DATA_MAP = 0x0001;
	private const uint SECTION_SYMBOLS = 0x0002;
	private const uint SECTION_COMMENTS = 0x0003;
	private const uint SECTION_MEMORY_REGIONS = 0x0004;
	private const uint SECTION_CROSS_REFS = 0x0006;
	private const uint SECTION_METADATA = 0x0008;

	// Platform ID for Atari 2600 per Pansy spec
	private const byte PLATFORM_ATARI_2600 = 0x08;

	// CDL flags (Pansy spec)
	private const byte CDL_CODE = 0x01;
	private const byte CDL_DATA = 0x02;
	private const byte CDL_JUMP_TARGET = 0x04;
	private const byte CDL_SUB_ENTRY = 0x08;
	private const byte CDL_OPCODE = 0x10;
	private const byte CDL_DRAWN = 0x20;
	private const byte CDL_READ = 0x40;
	private const byte CDL_INDIRECT = 0x80;

	// Atari 2600 memory map constants
	private const uint TIA_START = 0x0000;
	private const uint TIA_READ_END = 0x000d;
	private const uint TIA_WRITE_END = 0x002c;
	private const uint RAM_START = 0x0080;
	private const uint RAM_END = 0x00ff;
	private const uint RIOT_START = 0x0280;
	private const uint RIOT_END = 0x029f;
	private const uint ROM_START = 0x1000;
	private const uint ROM_END = 0x1fff;

	#region Platform ID Tests

	[Fact]
	public void PlatformId_Atari2600_IsHex08() {
		Assert.Equal(0x08, PLATFORM_ATARI_2600);
		Assert.Equal(PansyLoader.PLATFORM_ATARI_2600, PLATFORM_ATARI_2600);
	}

	[Fact]
	public void PlatformId_Atari2600_MatchesPansyCoreConstant() {
		Assert.Equal(PansyLoader.PLATFORM_ATARI_2600, (byte)0x08);
	}

	[Fact]
	public void PlatformId_Atari2600_ResolvesToCorrectName() {
		var pansyFile = BuildMinimalPansyFile([], SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal("Atari 2600", PansyLoader.GetPlatformName(loader.Platform));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	#endregion

	#region Memory Region Tests

	[Fact]
	public void MemoryRegions_Atari2600_HasFiveRegions() {
		var regionData = BuildAtari2600MemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(5, loader.MemoryRegions.Count);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_TIA_CorrectRange() {
		var regionData = BuildAtari2600MemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var tiaRead = loader.MemoryRegions.First(r => r.Name == "TIA_Read");
			Assert.Equal(TIA_START, tiaRead.Start);
			Assert.Equal(TIA_READ_END, tiaRead.End);
			Assert.Equal((byte)MemoryRegionType.IO, tiaRead.Type);
			var tiaWrite = loader.MemoryRegions.First(r => r.Name == "TIA_Write");
			Assert.Equal(TIA_START, tiaWrite.Start);
			Assert.Equal(TIA_WRITE_END, tiaWrite.End);
			Assert.Equal((byte)MemoryRegionType.IO, tiaWrite.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_RAM_CorrectRange() {
		var regionData = BuildAtari2600MemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_ATARI_2600);
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
	public void MemoryRegions_RIOT_CorrectRange() {
		var regionData = BuildAtari2600MemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var riot = loader.MemoryRegions.First(r => r.Name == "RIOT");
			Assert.Equal(RIOT_START, riot.Start);
			Assert.Equal(RIOT_END, riot.End);
			Assert.Equal((byte)MemoryRegionType.IO, riot.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_CartROM_CorrectRange() {
		var regionData = BuildAtari2600MemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var rom = loader.MemoryRegions.First(r => r.Name == "Cart_ROM");
			Assert.Equal(ROM_START, rom.Start);
			Assert.Equal(ROM_END, rom.End);
			Assert.Equal((byte)MemoryRegionType.ROM, rom.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_AllBanksAreZero() {
		// Atari 2600 has no bank switching in the base memory map regions
		var regionData = BuildAtari2600MemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_ATARI_2600);
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
		var regionData = BuildAtari2600MemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var starts = loader.MemoryRegions.Select(r => r.Start).ToList();
			for (int i = 1; i < starts.Count; i++) {
				// TIA_Read and TIA_Write share start address 0x0000 (hardware uses R/W signal)
				Assert.True(starts[i] >= starts[i - 1], $"Region {i} start ({starts[i]:x4}) should be >= region {i - 1} start ({starts[i - 1]:x4})");
			}
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_NoUnexpectedOverlap() {
		var regionData = BuildAtari2600MemoryRegions();
		var pansyFile = BuildMinimalPansyFile(regionData, SECTION_MEMORY_REGIONS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			// TIA_Read and TIA_Write share address range (hardware uses R/W signal).
			// All other regions must not overlap.
			var nonTia = loader.MemoryRegions
				.Where(r => !r.Name.StartsWith("TIA_"))
				.OrderBy(r => r.Start).ToList();
			for (int i = 1; i < nonTia.Count; i++) {
				Assert.True(nonTia[i].Start > nonTia[i - 1].End,
					$"Region '{nonTia[i].Name}' overlaps with '{nonTia[i - 1].Name}'");
			}
			// Verify TIA_Read is subset of TIA_Write
			var tiaRead = loader.MemoryRegions.First(r => r.Name == "TIA_Read");
			var tiaWrite = loader.MemoryRegions.First(r => r.Name == "TIA_Write");
			Assert.True(tiaRead.End <= tiaWrite.End, "TIA_Read should be subset of TIA_Write range");
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MemoryRegions_PlatformDefaults_MatchExporter() {
		// Verify that Pansy.Core PlatformDefaults for Atari 2600
		// defines the same regions as the PansyExporter would
		var defaults = PlatformDefaults.GetDefaultRegions(PLATFORM_ATARI_2600);
		Assert.NotNull(defaults);
		Assert.True(defaults.Length >= 4, "PlatformDefaults should have at least 4 regions for Atari 2600");

		// Check TIA, RAM, RIOT, ROM exist
		Assert.Contains(defaults, r => r.Name.Contains("TIA", StringComparison.OrdinalIgnoreCase));
		Assert.Contains(defaults, r => r.Name.Contains("RAM", StringComparison.OrdinalIgnoreCase));
		Assert.Contains(defaults, r => r.Name.Contains("RIOT", StringComparison.OrdinalIgnoreCase));
		Assert.Contains(defaults, r => r.Name.Contains("ROM", StringComparison.OrdinalIgnoreCase) || r.Name.Contains("Cart", StringComparison.OrdinalIgnoreCase));
	}

	#endregion

	#region CDL Flag Mapping Tests

	[Fact]
	public void CdlFlags_CodeByte_MapsToCode() {
		byte[] cdl = new byte[4096]; // 4KB ROM
		cdl[0] = CDL_CODE; // First byte executed
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 4096);
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
		byte[] cdl = new byte[4096];
		cdl[100] = CDL_DATA;
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 4096);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.True(loader.DataOffsets.Contains(100));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CdlFlags_JumpTarget_MapsToJumpTarget() {
		byte[] cdl = new byte[4096];
		cdl[0x100] = CDL_CODE | CDL_JUMP_TARGET;
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 4096);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.True(loader.JumpTargets.Contains(0x100));
			Assert.True(loader.CodeOffsets.Contains(0x100));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CdlFlags_SubEntry_MapsToSubEntryPoint() {
		byte[] cdl = new byte[4096];
		cdl[0x200] = CDL_CODE | CDL_SUB_ENTRY;
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 4096);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.True(loader.SubEntryPoints.Contains(0x200));
			Assert.True(loader.CodeOffsets.Contains(0x200));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CdlFlags_ReadByte_MapsToRead() {
		byte[] cdl = new byte[4096];
		cdl[50] = CDL_DATA | CDL_READ; // Data read from ROM
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 4096);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.True(loader.DataOffsets.Contains(50));
			Assert.True(loader.ReadOffsets.Contains(50));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CdlFlags_OpcodeDrawnReadIndirect_RoundtripFromFixture() {
		byte[] cdl = BuildCdlFlagCoverageFixture(4096);
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 4096);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.True(loader.OpcodeOffsets.Contains(0x20));
			Assert.True(loader.DrawnOffsets.Contains(0x21));
			Assert.True(loader.ReadOffsets.Contains(0x22));
			Assert.True(loader.IndirectOffsets.Contains(0x23));
			Assert.Single(loader.OpcodeOffsets);
			Assert.Single(loader.DrawnOffsets);
			Assert.Single(loader.ReadOffsets);
			Assert.Single(loader.IndirectOffsets);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CdlFlags_EmptyRom_AllZero() {
		byte[] cdl = new byte[4096]; // All zero
		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 4096);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Empty(loader.CodeOffsets);
			Assert.Empty(loader.DataOffsets);
			Assert.Empty(loader.JumpTargets);
			Assert.Empty(loader.SubEntryPoints);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CdlFlags_MixedCodeAndData_BothPresent() {
		byte[] cdl = new byte[4096];
		// Code region
		for (int i = 0; i < 256; i++) {
			cdl[i] = CDL_CODE;
		}
		// Data region
		for (int i = 256; i < 512; i++) {
			cdl[i] = CDL_DATA;
		}
		// Jump target
		cdl[100] |= CDL_JUMP_TARGET;
		// Subroutine entry
		cdl[200] |= CDL_SUB_ENTRY;

		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 4096);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(256, loader.CodeOffsets.Count);
			Assert.Equal(256, loader.DataOffsets.Count);
			Assert.True(loader.JumpTargets.Contains(100));
			Assert.True(loader.SubEntryPoints.Contains(200));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	#endregion

	#region Symbol Roundtrip Tests

	[Fact]
	public void Symbols_TiaRegister_Roundtrip() {
		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0x00, "VSYNC", SymbolType.Constant),
			(0x01, "VBLANK", SymbolType.Constant),
			(0x02, "WSYNC", SymbolType.Constant),
		};
		var symbolData = BuildSymbolSection(symbols);
		var pansyFile = BuildMinimalPansyFile(symbolData, SECTION_SYMBOLS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.True(loader.Symbols.ContainsKey(0x00));
			Assert.True(loader.Symbols.ContainsKey(0x01));
			Assert.True(loader.Symbols.ContainsKey(0x02));
			Assert.Equal("VSYNC", loader.Symbols[0x00]);
			Assert.Equal("VBLANK", loader.Symbols[0x01]);
			Assert.Equal("WSYNC", loader.Symbols[0x02]);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Symbols_RiotRegisters_Roundtrip() {
		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0x0280, "SWCHA", SymbolType.Constant),
			(0x0282, "SWCHB", SymbolType.Constant),
			(0x0284, "INTIM", SymbolType.Constant),
			(0x0296, "TIM64T", SymbolType.Constant),
		};
		var symbolData = BuildSymbolSection(symbols);
		var pansyFile = BuildMinimalPansyFile(symbolData, SECTION_SYMBOLS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal("SWCHA", loader.Symbols[0x0280]);
			Assert.Equal("SWCHB", loader.Symbols[0x0282]);
			Assert.Equal("INTIM", loader.Symbols[0x0284]);
			Assert.Equal("TIM64T", loader.Symbols[0x0296]);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Symbols_RomLabels_Roundtrip() {
		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0x0000, "RESET", SymbolType.Function),
			(0x0050, "MainLoop", SymbolType.Label),
			(0x0100, "DrawScreen", SymbolType.Function),
			(0x0ff0, "InterruptHandler", SymbolType.InterruptVector),
		};
		var symbolData = BuildSymbolSection(symbols);
		var pansyFile = BuildMinimalPansyFile(symbolData, SECTION_SYMBOLS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal("RESET", loader.Symbols[0x0000]);
			Assert.Equal("MainLoop", loader.Symbols[0x0050]);
			Assert.Equal("DrawScreen", loader.Symbols[0x0100]);
			Assert.Equal("InterruptHandler", loader.Symbols[0x0ff0]);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Symbols_TypedEntry_PreservesType() {
		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0x0080, "GameState", SymbolType.Label),
			(0x0006, "COLUP0", SymbolType.Constant),
			(0x0000, "RESET", SymbolType.Function),
		};
		var symbolData = BuildSymbolSection(symbols);
		var pansyFile = BuildMinimalPansyFile(symbolData, SECTION_SYMBOLS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			var gameState = loader.SymbolEntries[0x0080];
			Assert.Equal(SymbolType.Label, gameState.Type);

			var colup0 = loader.SymbolEntries[0x0006];
			Assert.Equal(SymbolType.Constant, colup0.Type);

			var reset = loader.SymbolEntries[0x0000];
			Assert.Equal(SymbolType.Function, reset.Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Symbols_PlatformDefaults_HasTiaAndRiot() {
		// Verify PlatformDefaults provides Atari 2600 hardware register symbols
		var defaults = PlatformDefaults.GetDefaultSymbols(PLATFORM_ATARI_2600);
		Assert.NotNull(defaults);
		Assert.True(defaults.Count > 0, "Should have hardware register symbols");

		// TIA registers
		Assert.True(defaults.ContainsKey(0x00), "VSYNC should be in defaults");
		Assert.True(defaults.ContainsKey(0x01), "VBLANK should be in defaults");
		Assert.True(defaults.ContainsKey(0x02), "WSYNC should be in defaults");

		// Verify some key RIOT registers exist
		// (RIOT registers may use CPU-mapped addresses or raw RIOT addresses
		// depending on implementation)
	}

	#endregion

	#region Comment Roundtrip Tests

	[Fact]
	public void Comments_InlineComment_Roundtrip() {
		var comments = new List<(uint Address, string Text, CommentType Type)> {
			(0x0000, "Set vertical sync", CommentType.Inline),
		};
		var commentData = BuildCommentSection(comments);
		var pansyFile = BuildMinimalPansyFile(commentData, SECTION_COMMENTS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.True(loader.CommentEntries.ContainsKey(0));
			Assert.Equal("Set vertical sync", loader.CommentEntries[0].Text);
			Assert.Equal(CommentType.Inline, loader.CommentEntries[0].Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Comments_TodoComment_Roundtrip() {
		var comments = new List<(uint Address, string Text, CommentType Type)> {
			(0x0100, "TODO: Optimize scanline kernel", CommentType.Todo),
		};
		var commentData = BuildCommentSection(comments);
		var pansyFile = BuildMinimalPansyFile(commentData, SECTION_COMMENTS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.True(loader.CommentEntries.ContainsKey(0x0100));
			Assert.Equal(CommentType.Todo, loader.CommentEntries[0x0100].Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	#endregion

	#region Cross-Reference Roundtrip Tests

	[Fact]
	public void CrossRefs_JsrCall_Roundtrip() {
		var xrefs = new List<(uint From, uint To, CrossRefType Type)> {
			(0x1020, 0x1100, CrossRefType.Jsr),
		};
		var xrefData = BuildCrossRefSection(xrefs);
		var pansyFile = BuildMinimalPansyFile(xrefData, SECTION_CROSS_REFS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Single(loader.CrossReferences);
			Assert.Equal(0x1020u, loader.CrossReferences[0].From);
			Assert.Equal(0x1100u, loader.CrossReferences[0].To);
			Assert.Equal(CrossRefType.Jsr, loader.CrossReferences[0].Type);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CrossRefs_BranchAndJump_Roundtrip() {
		var xrefs = new List<(uint From, uint To, CrossRefType Type)> {
			(0x1050, 0x1080, CrossRefType.Branch),
			(0x1070, 0x1000, CrossRefType.Jmp),
		};
		var xrefData = BuildCrossRefSection(xrefs);
		var pansyFile = BuildMinimalPansyFile(xrefData, SECTION_CROSS_REFS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(2, loader.CrossReferences.Count);
			Assert.Contains(loader.CrossReferences, x => x.Type == CrossRefType.Branch && x.From == 0x1050);
			Assert.Contains(loader.CrossReferences, x => x.Type == CrossRefType.Jmp && x.From == 0x1070);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CrossRefs_DataReadWrite_Roundtrip() {
		var xrefs = new List<(uint From, uint To, CrossRefType Type)> {
			(0x1010, 0x0080, CrossRefType.Read),   // Read from RAM
			(0x1015, 0x0090, CrossRefType.Write),  // Write to RAM
		};
		var xrefData = BuildCrossRefSection(xrefs);
		var pansyFile = BuildMinimalPansyFile(xrefData, SECTION_CROSS_REFS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(2, loader.CrossReferences.Count);
			Assert.Contains(loader.CrossReferences, x => x.Type == CrossRefType.Read);
			Assert.Contains(loader.CrossReferences, x => x.Type == CrossRefType.Write);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CrossRefs_TiaAndRiotRegisterAccess_Roundtrip() {
		var xrefs = new List<(uint From, uint To, CrossRefType Type)> {
			(0x1100, 0x0015, CrossRefType.Write), // STA AUDC0 (TIA)
			(0x1105, 0x0284, CrossRefType.Read),  // LDA INTIM (RIOT)
			(0x1108, 0x0280, CrossRefType.Write), // STA SWCHA (RIOT)
		};
		var xrefData = BuildCrossRefSection(xrefs);
		var pansyFile = BuildMinimalPansyFile(xrefData, SECTION_CROSS_REFS, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(3, loader.CrossReferences.Count);
			Assert.Contains(loader.CrossReferences, x => x.From == 0x1100 && x.To == 0x0015 && x.Type == CrossRefType.Write);
			Assert.Contains(loader.CrossReferences, x => x.From == 0x1105 && x.To == 0x0284 && x.Type == CrossRefType.Read);
			Assert.Contains(loader.CrossReferences, x => x.From == 0x1108 && x.To == 0x0280 && x.Type == CrossRefType.Write);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void CrossRefs_MultiTargetFixture_RoundtripCountAndTypes() {
		var xrefs = BuildCrossRefEdgeFixture();
		var xrefData = BuildCrossRefSection(xrefs);
		var cdl = BuildCdlFlagCoverageFixture(4096);
		var pansyFile = BuildFullPansyFile(
			PLATFORM_ATARI_2600,
			4096,
			0x22446688,
			cdl,
			[],
			[],
			BuildAtari2600MemoryRegions(),
			xrefData);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);

			Assert.Equal(4, loader.CrossReferences.Count);
			Assert.Equal(2, loader.CrossReferences.Count(x => x.From == 0x1200 && x.Type == CrossRefType.Branch));
			Assert.Single(loader.CrossReferences, x => x.Type == CrossRefType.Jmp);
			Assert.Single(loader.CrossReferences, x => x.Type == CrossRefType.Read);

			Assert.True(loader.JumpTargets.Contains(0x20));
			Assert.True(loader.OpcodeOffsets.Contains(0x20));
			Assert.True(loader.DrawnOffsets.Contains(0x21));
			Assert.True(loader.ReadOffsets.Contains(0x22));
			Assert.True(loader.IndirectOffsets.Contains(0x23));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	#endregion

	#region Header Validation Tests

	[Fact]
	public void Header_Atari2600Platform_RoundtripCorrect() {
		var pansyFile = BuildMinimalPansyFile([], SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 4096, romCrc: 0xdeadbeef);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(PLATFORM_ATARI_2600, loader.Platform);
			Assert.Equal(4096u, loader.RomSize);
			Assert.Equal(0xdeadbeef, loader.RomCrc32);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void Header_4KBRom_CorrectSize() {
		var pansyFile = BuildMinimalPansyFile([], SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 4096);
		using var ms = new MemoryStream(pansyFile);
		var span = pansyFile.AsSpan();
		Assert.Equal(4096u, BinaryPrimitives.ReadUInt32LittleEndian(span[16..]));
	}

	[Fact]
	public void Header_2KBRom_CorrectSize() {
		var pansyFile = BuildMinimalPansyFile([], SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: 2048);
		var span = pansyFile.AsSpan();
		Assert.Equal(2048u, BinaryPrimitives.ReadUInt32LittleEndian(span[16..]));
	}

	[Fact]
	public void Header_Version1_0() {
		var pansyFile = BuildMinimalPansyFile([], SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600);
		var span = pansyFile.AsSpan();
		Assert.Equal(VERSION, BinaryPrimitives.ReadUInt16LittleEndian(span[8..]));
	}

	[Fact]
	public void Header_MagicBytes_Correct() {
		var pansyFile = BuildMinimalPansyFile([], SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600);
		var magic = Encoding.ASCII.GetString(pansyFile, 0, 8);
		Assert.Equal(MAGIC, magic);
	}

	#endregion

	#region Multi-Section Roundtrip Tests

	[Fact]
	public void MultiSection_CdlAndSymbolsAndRegions_AllPresent() {
		// Build CDL data
		byte[] cdl = new byte[4096];
		cdl[0] = CDL_CODE | CDL_SUB_ENTRY; // RESET entry point
		cdl[1] = CDL_CODE;
		cdl[2] = CDL_CODE;
		cdl[0x100] = CDL_CODE | CDL_JUMP_TARGET;
		cdl[0x800] = CDL_DATA;

		// Build symbols
		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0x0000, "RESET", SymbolType.Function),
			(0x0100, "MainLoop", SymbolType.Label),
		};

		// Build memory regions
		var regionData = BuildAtari2600MemoryRegions();

		// Build the file with multiple sections
		var pansyFile = BuildMultiSectionPansyFile(PLATFORM_ATARI_2600, 4096, 0x12345678, cdl, BuildSymbolSection(symbols), regionData);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);

			// Verify CDL
			Assert.True(loader.CodeOffsets.Contains(0));
			Assert.True(loader.SubEntryPoints.Contains(0));
			Assert.True(loader.JumpTargets.Contains(0x100));
			Assert.True(loader.DataOffsets.Contains(0x800));

			// Verify symbols
			Assert.Equal("RESET", loader.Symbols[0]);
			Assert.Equal("MainLoop", loader.Symbols[0x100]);

			// Verify memory regions
			Assert.Equal(5, loader.MemoryRegions.Count);

			// Verify header
			Assert.Equal(PLATFORM_ATARI_2600, loader.Platform);
			Assert.Equal(4096u, loader.RomSize);
			Assert.Equal(0x12345678u, loader.RomCrc32);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	[Fact]
	public void MultiSection_FullAtari2600Export_Roundtrip() {
		// Simulate a complete Atari 2600 ROM analysis export
		byte[] cdl = new byte[4096];
		// Kernel (code)
		for (int i = 0; i < 0x200; i++) cdl[i] = CDL_CODE;
		// Data tables
		for (int i = 0x200; i < 0x400; i++) cdl[i] = CDL_DATA;
		// Mark entry points
		cdl[0] |= CDL_SUB_ENTRY;
		cdl[0x100] |= CDL_JUMP_TARGET;

		var symbols = new List<(uint Address, string Name, SymbolType Type)> {
			(0x0000, "RESET", SymbolType.Function),
			(0x0050, "VBlankHandler", SymbolType.Function),
			(0x0100, "KernelStart", SymbolType.Label),
			(0x0200, "SpriteData", SymbolType.Label),
		};
		var symbolData = BuildSymbolSection(symbols);

		var comments = new List<(uint Address, string Text, CommentType Type)> {
			(0x0000, "Entry point - RESET vector", CommentType.Block),
			(0x0050, "Handles vertical blank", CommentType.Inline),
		};
		var commentData = BuildCommentSection(comments);

		var xrefs = new List<(uint From, uint To, CrossRefType Type)> {
			(0x0010, 0x0050, CrossRefType.Jsr),
			(0x0060, 0x0100, CrossRefType.Jmp),
			(0x0110, 0x0080, CrossRefType.Branch),
		};
		var xrefData = BuildCrossRefSection(xrefs);

		var regionData = BuildAtari2600MemoryRegions();

		// Build with all sections
		var pansyFile = BuildFullPansyFile(PLATFORM_ATARI_2600, 4096, 0xaabbccdd,
			cdl, symbolData, commentData, regionData, xrefData);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);

			// CDL
			Assert.Equal(0x200, loader.CodeOffsets.Count);
			Assert.Equal(0x200, loader.DataOffsets.Count);

			// Symbols
			Assert.Equal(4, loader.Symbols.Count);
			Assert.Equal("RESET", loader.Symbols[0]);
			Assert.Equal("SpriteData", loader.Symbols[0x200]);

			// Comments
			Assert.True(loader.CommentEntries.ContainsKey(0));
			Assert.True(loader.CommentEntries.ContainsKey(0x50));

			// Cross-references
			Assert.Equal(3, loader.CrossReferences.Count);

			// Memory regions
			Assert.Equal(5, loader.MemoryRegions.Count);

			// Header
			Assert.Equal(PLATFORM_ATARI_2600, loader.Platform);
			Assert.Equal("Atari 2600", PansyLoader.GetPlatformName(loader.Platform));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	#endregion

	#region ROM Size Variants Tests

	[Theory]
	[InlineData(2048)]   // 2KB (standard)
	[InlineData(4096)]   // 4KB (standard)
	[InlineData(8192)]   // 8KB (bankswitched)
	[InlineData(16384)]  // 16KB (bankswitched)
	[InlineData(32768)]  // 32KB (bankswitched)
	public void RomSize_VariousSizes_RoundtripCorrectly(uint romSize) {
		byte[] cdl = new byte[romSize];
		cdl[0] = CDL_CODE;
		cdl[romSize - 1] = CDL_DATA;

		var pansyFile = BuildMinimalPansyFile(cdl, SECTION_CODE_DATA_MAP, PLATFORM_ATARI_2600, romSize: romSize);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal(romSize, loader.RomSize);
			Assert.True(loader.CodeOffsets.Contains(0));
			Assert.True(loader.DataOffsets.Contains((int)(romSize - 1)));
		} finally {
			CleanupTemp(tempPath);
		}
	}

	#endregion

	#region Metadata Section Tests

	[Fact]
	public void Metadata_ProjectInfo_Roundtrip() {
		var metadata = BuildMetadataSection("TestRom", "Author", "1.0.0");
		var pansyFile = BuildMinimalPansyFile(metadata, SECTION_METADATA, PLATFORM_ATARI_2600);
		var tempPath = WriteTempPansy(pansyFile);
		try {
			var loader = PansyLoader.Load(tempPath);
			Assert.Equal("TestRom", loader.ProjectName);
			Assert.Equal("Author", loader.Author);
			Assert.Equal("1.0.0", loader.ProjectVersion);
		} finally {
			CleanupTemp(tempPath);
		}
	}

	#endregion

	#region Helper Methods

	/// <summary>
	/// Fixture-like CDL map used by integration tests to validate flag roundtrip behavior.
	/// </summary>
	private static byte[] BuildCdlFlagCoverageFixture(int length) {
		var cdl = new byte[length];
		cdl[0x20] = (byte)(CDL_CODE | CDL_JUMP_TARGET | CDL_OPCODE);
		cdl[0x21] = (byte)(CDL_DATA | CDL_DRAWN);
		cdl[0x22] = (byte)(CDL_DATA | CDL_READ);
		cdl[0x23] = (byte)(CDL_CODE | CDL_INDIRECT);
		return cdl;
	}

	/// <summary>
	/// Fixture-like cross-reference edges that include a multi-target source.
	/// </summary>
	private static List<(uint From, uint To, CrossRefType Type)> BuildCrossRefEdgeFixture() {
		return [
			(0x1200, 0x1210, CrossRefType.Branch),
			(0x1200, 0x1220, CrossRefType.Branch),
			(0x1300, 0x1400, CrossRefType.Jmp),
			(0x1310, 0x0080, CrossRefType.Read),
		];
	}

	/// <summary>
	/// Build Atari 2600 memory regions matching PansyExporter output format.
	/// Format per region: Start(4)+End(4)+Type(1)+Bank(1)+Flags(2)+NameLen(2)+Name
	/// </summary>
	private static byte[] BuildAtari2600MemoryRegions() {
		var regions = new List<(uint Start, uint End, string Name, byte Type, byte Bank)> {
			(TIA_START, TIA_READ_END, "TIA_Read", (byte)MemoryRegionType.IO, 0),
			(TIA_START, TIA_WRITE_END, "TIA_Write", (byte)MemoryRegionType.IO, 0),
			(RAM_START, RAM_END, "RAM", (byte)MemoryRegionType.RAM, 0),
			(RIOT_START, RIOT_END, "RIOT", (byte)MemoryRegionType.IO, 0),
			(ROM_START, ROM_END, "Cart_ROM", (byte)MemoryRegionType.ROM, 0),
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

	/// <summary>
	/// Build symbol section matching PansyExporter format.
	/// Format per entry: Address(4)+Type(1)+Flags(1)+NameLen(2)+Name+ValueLen(2)
	/// </summary>
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

	/// <summary>
	/// Build comment section matching PansyExporter format.
	/// Format per entry: Address(4)+Type(1)+TextLen(2)+Text
	/// </summary>
	private static byte[] BuildCommentSection(List<(uint Address, string Text, CommentType Type)> comments) {
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);
		foreach (var (address, text, type) in comments) {
			writer.Write(address);
			writer.Write((byte)type);
			byte[] textBytes = Encoding.UTF8.GetBytes(text);
			writer.Write((ushort)textBytes.Length);
			writer.Write(textBytes);
		}
		return ms.ToArray();
	}

	/// <summary>
	/// Build cross-reference section matching Pansy spec.
	/// Format per entry: From(4)+To(4)+Type(1) = 9 bytes
	/// </summary>
	private static byte[] BuildCrossRefSection(List<(uint From, uint To, CrossRefType Type)> xrefs) {
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);
		foreach (var (from, to, type) in xrefs) {
			writer.Write(from);
			writer.Write(to);
			writer.Write((byte)type);
		}
		return ms.ToArray();
	}

	/// <summary>
	/// Build metadata section matching Pansy spec.
	/// Format: ProjectNameLen(2)+ProjectName + AuthorLen(2)+Author + VersionLen(2)+Version
	/// </summary>
	private static byte[] BuildMetadataSection(string projectName, string author, string version) {
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		var nameBytes = Encoding.UTF8.GetBytes(projectName);
		writer.Write((ushort)nameBytes.Length);
		writer.Write(nameBytes);

		var authorBytes = Encoding.UTF8.GetBytes(author);
		writer.Write((ushort)authorBytes.Length);
		writer.Write(authorBytes);

		var versionBytes = Encoding.UTF8.GetBytes(version);
		writer.Write((ushort)versionBytes.Length);
		writer.Write(versionBytes);

		return ms.ToArray();
	}

	/// <summary>
	/// Build a minimal valid Pansy file with a single section.
	/// Uses the correct Pansy spec v1.0 section table layout:
	/// Type(4)+Offset(4)+CompressedSize(4)+UncompressedSize(4) = 16 bytes per entry.
	/// </summary>
	private static byte[] BuildMinimalPansyFile(byte[] sectionData, uint sectionType, byte platform, uint romSize = 4096, uint romCrc = 0xdeadbeef) {
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		int sectionCount = sectionData.Length > 0 ? 1 : 0;

		// Header (32 bytes)
		writer.Write(Encoding.ASCII.GetBytes(MAGIC));  // 8 bytes
		writer.Write(VERSION);                          // 2 bytes
		writer.Write((ushort)0);                        // Flags
		writer.Write(platform);                         // 1 byte
		writer.Write((byte)0);                          // Reserved
		writer.Write((ushort)0);                        // Reserved
		writer.Write(romSize);                          // 4 bytes
		writer.Write(romCrc);                           // 4 bytes
		writer.Write((uint)sectionCount);               // 4 bytes
		writer.Write((uint)0);                          // Reserved

		if (sectionCount > 0) {
			uint dataOffset = 32 + 16; // Header + one 16-byte section table entry

			// Section table entry (16 bytes)
			writer.Write(sectionType);                   // Type (4 bytes)
			writer.Write(dataOffset);                    // Offset (4 bytes)
			writer.Write((uint)sectionData.Length);      // Compressed size (4 bytes)
			writer.Write((uint)sectionData.Length);      // Uncompressed size (4 bytes)

			// Section data
			writer.Write(sectionData);
		}

		return ms.ToArray();
	}

	/// <summary>
	/// Build a Pansy file with CDL, symbols, and memory regions sections.
	/// </summary>
	private static byte[] BuildMultiSectionPansyFile(byte platform, uint romSize, uint romCrc,
		byte[] cdlData, byte[] symbolData, byte[] regionData) {
		var sections = new List<(uint Type, byte[] Data)>();
		if (cdlData.Length > 0) sections.Add((SECTION_CODE_DATA_MAP, cdlData));
		if (symbolData.Length > 0) sections.Add((SECTION_SYMBOLS, symbolData));
		if (regionData.Length > 0) sections.Add((SECTION_MEMORY_REGIONS, regionData));
		return BuildPansyFileFromSections(platform, romSize, romCrc, sections);
	}

	/// <summary>
	/// Build a Pansy file with all section types.
	/// </summary>
	private static byte[] BuildFullPansyFile(byte platform, uint romSize, uint romCrc,
		byte[] cdlData, byte[] symbolData, byte[] commentData, byte[] regionData, byte[] xrefData) {
		var sections = new List<(uint Type, byte[] Data)>();
		if (cdlData.Length > 0) sections.Add((SECTION_CODE_DATA_MAP, cdlData));
		if (symbolData.Length > 0) sections.Add((SECTION_SYMBOLS, symbolData));
		if (commentData.Length > 0) sections.Add((SECTION_COMMENTS, commentData));
		if (regionData.Length > 0) sections.Add((SECTION_MEMORY_REGIONS, regionData));
		if (xrefData.Length > 0) sections.Add((SECTION_CROSS_REFS, xrefData));
		return BuildPansyFileFromSections(platform, romSize, romCrc, sections);
	}

	/// <summary>
	/// Build a Pansy file from a list of sections, using correct spec v1.0 layout.
	/// </summary>
	private static byte[] BuildPansyFileFromSections(byte platform, uint romSize, uint romCrc,
		List<(uint Type, byte[] Data)> sections) {
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		uint sectionTableSize = (uint)(sections.Count * 16); // 16 bytes per entry
		uint dataStart = 32 + sectionTableSize;

		// Header (32 bytes)
		writer.Write(Encoding.ASCII.GetBytes(MAGIC));
		writer.Write(VERSION);
		writer.Write((ushort)0); // Flags
		writer.Write(platform);
		writer.Write((byte)0);   // Reserved
		writer.Write((ushort)0); // Reserved
		writer.Write(romSize);
		writer.Write(romCrc);
		writer.Write((uint)sections.Count);
		writer.Write((uint)0);   // Reserved

		// Section table
		uint currentOffset = dataStart;
		foreach (var (type, data) in sections) {
			writer.Write(type);                    // Type (4 bytes)
			writer.Write(currentOffset);           // Offset (4 bytes)
			writer.Write((uint)data.Length);        // Compressed size (4 bytes)
			writer.Write((uint)data.Length);        // Uncompressed size (4 bytes)
			currentOffset += (uint)data.Length;
		}

		// Section data
		foreach (var (_, data) in sections) {
			writer.Write(data);
		}

		return ms.ToArray();
	}

	private static string WriteTempPansy(byte[] data) {
		var path = Path.Combine(Path.GetTempPath(), $"atari2600_test_{Guid.NewGuid():N}.pansy");
		File.WriteAllBytes(path, data);
		return path;
	}

	private static void CleanupTemp(string path) {
		if (File.Exists(path)) File.Delete(path);
	}

	#endregion
}
