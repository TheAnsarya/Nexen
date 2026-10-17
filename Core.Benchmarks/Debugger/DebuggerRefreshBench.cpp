#include "pch.h"
#include <array>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstring>
#include "Debugger/DebugTypes.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/StringUtilities.h"

// =============================================================================
// Debugger Disassembly View Refresh Benchmarks
// =============================================================================
// When the debugger window is open, every emulated frame triggers a full
// refresh of the disassembly view, register panel, and memory viewer.
//
// Per-refresh the following happens for each of ~64 visible rows:
//   1. Disassembly cache lookup (array index on DisassemblyInfo vector)
//   2. GetNthSegment for comment line extraction
//   3. HexUtilities::ToHex for address/byte-code formatting
//   4. CodeLineData struct fill + memcpy of text/comment strings
//   5. Label/comment hash map lookup (unordered_map per address)
//
// This file benchmarks each step in isolation so we can identify which is
// the bottleneck without requiring a full Debugger/Console instance.
//
// Issue #749 tracking: debugger refresh-path performance audit.
// =============================================================================

// ---------------------------------------------------------------------------
// 1. Disassembly Cache Lookup Pattern
// ---------------------------------------------------------------------------
// Disassembler::GetDisassemblyInfo() does: src.Cache[info.Address]
// This is an array access into a vector<DisassemblyInfo>.
// With 32KB NES ROM = 32,768 entries; 512KB SNES ROM = 524,288 entries.
// For a 64-row window, 64 random accesses into this array per refresh.

namespace {
	// Minimal placeholder for DisassemblyInfo to measure lookup cost
	// without depending on the full Disassembler machinery
	struct BenchDisassemblyInfo {
		uint8_t  OpCode = 0;
		uint8_t  CpuFlags = 0;
		uint8_t  MemType = 0;
		uint8_t  Initialized = 0;
		uint8_t  OpSize = 1;

		bool IsInitialized() const { return Initialized != 0; }
	};

	constexpr uint32_t kNesRomSize  = 32 * 1024;       // 32KB NES PRG ROM
	constexpr uint32_t kSnesRomSize = 512 * 1024;      // 512KB SNES ROM
	constexpr uint32_t kWindowRows  = 64;               // Typical disassembly window height

	// Simulate realistic disassembly results for a window of instructions
	// Each result has an address, flags, and comment line
	struct BenchDisasmRow {
		int32_t  CpuAddress;
		uint16_t Flags;
		int16_t  CommentLine;
		int32_t  AbsAddress;
	};

	// Build a fake 64-row window starting at the given address
	std::vector<BenchDisasmRow> BuildWindowRows(int32_t startAddr, uint32_t count) {
		std::vector<BenchDisasmRow> rows;
		rows.reserve(count);
		int32_t addr = startAddr;
		for (uint32_t i = 0; i < count; i++) {
			BenchDisasmRow row = {};
			row.CpuAddress  = addr;
			row.AbsAddress  = addr % kNesRomSize;
			row.Flags       = (i % 5 == 0) ? LineFlags::Comment : LineFlags::VerifiedCode;
			row.CommentLine = (row.Flags & LineFlags::Comment) ? 0 : -1;
			rows.push_back(row);
			addr += (i % 3 == 0) ? 3 : (i % 2 == 0) ? 2 : 1; // Variable instruction sizes
		}
		return rows;
	}
} // anonymous namespace

// Benchmark: Sequential disassembly cache lookup (NES 32KB ROM size)
static void BM_DebugRefresh_CacheLookup_Sequential_NES(benchmark::State& state) {
	std::vector<BenchDisassemblyInfo> cache(kNesRomSize);
	for (uint32_t i = 0; i < kNesRomSize; i++) {
		cache[i].OpCode      = static_cast<uint8_t>(i & 0xFF);
		cache[i].Initialized = 1;
		cache[i].OpSize      = (uint8_t)(1 + (i % 3)); // 1-3 bytes
	}

	auto rows = BuildWindowRows(0x8000, kWindowRows);

	for (auto _ : state) {
		uint32_t sum = 0;
		for (const auto& row : rows) {
			const auto& info = cache[row.AbsAddress];
			sum += info.IsInitialized() ? info.OpSize : 1;
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * kWindowRows);
}
BENCHMARK(BM_DebugRefresh_CacheLookup_Sequential_NES);

// Benchmark: Sequential disassembly cache lookup (SNES 512KB ROM — larger cache)
static void BM_DebugRefresh_CacheLookup_Sequential_SNES(benchmark::State& state) {
	std::vector<BenchDisassemblyInfo> cache(kSnesRomSize);
	for (uint32_t i = 0; i < kSnesRomSize; i++) {
		cache[i].OpCode      = static_cast<uint8_t>(i & 0xFF);
		cache[i].Initialized = 1;
		cache[i].OpSize      = (uint8_t)(1 + (i % 3));
	}

	auto rows = BuildWindowRows(0x808000, kWindowRows);
	// Remap to SNES ROM range
	for (auto& row : rows) {
		row.AbsAddress = row.CpuAddress % kSnesRomSize;
	}

	for (auto _ : state) {
		uint32_t sum = 0;
		for (const auto& row : rows) {
			const auto& info = cache[row.AbsAddress];
			sum += info.IsInitialized() ? info.OpSize : 1;
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * kWindowRows);
}
BENCHMARK(BM_DebugRefresh_CacheLookup_Sequential_SNES);

// Benchmark: Random-access disassembly cache lookup (worst case: scattered jump targets)
static void BM_DebugRefresh_CacheLookup_Random_NES(benchmark::State& state) {
	std::vector<BenchDisassemblyInfo> cache(kNesRomSize);
	for (uint32_t i = 0; i < kNesRomSize; i++) {
		cache[i].Initialized = 1;
		cache[i].OpSize = 1 + (i % 3);
	}

	// Random-ish addresses (simulates window containing mixed code/jump targets)
	std::array<uint32_t, kWindowRows> addrs{};
	for (uint32_t i = 0; i < kWindowRows; i++) {
		addrs[i] = (i * 397 + 0x1337) % kNesRomSize;
	}

	for (auto _ : state) {
		uint32_t sum = 0;
		for (uint32_t addr : addrs) {
			sum += cache[addr].OpSize;
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * kWindowRows);
}
BENCHMARK(BM_DebugRefresh_CacheLookup_Random_NES);

// ---------------------------------------------------------------------------
// 2. Comment Line Extraction via GetNthSegment
// ---------------------------------------------------------------------------
// Disassembler::GetLineData() calls:
//   StringUtilities::GetNthSegment(labelManager->GetComment(addr), '\n', line)
// This is called for every comment row in the window. Each GetNthSegment
// scans the string character by character to find the Nth '\n'-delimited segment.

// Benchmark: GetNthSegment with single-line comments (most common)
static void BM_DebugRefresh_GetNthSegment_SingleLine(benchmark::State& state) {
	std::string comment = "This is a typical single-line comment for an instruction.";
	int line = 0;
	for (auto _ : state) {
		std::string seg = StringUtilities::GetNthSegment(comment, '\n', line);
		benchmark::DoNotOptimize(seg.data());
		line = (line + 1) % 3;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DebugRefresh_GetNthSegment_SingleLine);

// Benchmark: GetNthSegment with multi-line comments (2nd and 3rd line)
static void BM_DebugRefresh_GetNthSegment_MultiLine_Line2(benchmark::State& state) {
	std::string comment = "Line one: initialize stack\nLine two: load accumulator\nLine three: store to PPUCTRL";
	for (auto _ : state) {
		std::string seg = StringUtilities::GetNthSegment(comment, '\n', 1); // Second line
		benchmark::DoNotOptimize(seg.data());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DebugRefresh_GetNthSegment_MultiLine_Line2);

// Benchmark: GetNthSegment simulating a full 64-row window
// (8 rows have comments, each comment needs GetNthSegment)
static void BM_DebugRefresh_GetNthSegment_WindowPass(benchmark::State& state) {
	// 8 typical comments in a 64-row window
	std::array<std::string, 8> comments = {
		"Set carry flag before ADC",
		"Load X with loop counter\nUsed by inner loop at $8100",
		"; Main render loop",
		"JSR to OAM DMA transfer",
		"Clear zero page variables\nInitialized on startup",
		"Check NMI flag",
		"Increment frame counter",
		"Store frame count to RAM"
	};

	for (auto _ : state) {
		uint32_t totalLen = 0;
		for (int i = 0; i < 8; i++) {
			std::string seg = StringUtilities::GetNthSegment(comments[i], '\n', 0);
			totalLen += static_cast<uint32_t>(seg.size());
		}
		benchmark::DoNotOptimize(totalLen);
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_DebugRefresh_GetNthSegment_WindowPass);

// ---------------------------------------------------------------------------
// 3. HexUtilities Address Formatting
// ---------------------------------------------------------------------------
// Every disassembly line formats the address (2-3 hex chars) and opcode
// bytes (2 hex chars each, up to 3 bytes). HexUtilities::ToHex() is called
// for each. This is the dominant string-building cost per row.

// Benchmark: Format a 16-bit address as hex (ToHex<uint16_t>)
static void BM_DebugRefresh_AddressFormat_16Bit(benchmark::State& state) {
	uint16_t addr = 0x8000;
	for (auto _ : state) {
		std::string hex = HexUtilities::ToHex(addr);
		benchmark::DoNotOptimize(hex.data());
		addr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DebugRefresh_AddressFormat_16Bit);

// Benchmark: Format a 24-bit (SNES) address
static void BM_DebugRefresh_AddressFormat_24Bit(benchmark::State& state) {
	uint32_t addr = 0x808000;
	for (auto _ : state) {
		// SNES addresses are 24-bit — format as 6 hex chars
		std::string hex = HexUtilities::ToHex(static_cast<uint32_t>(addr & 0xFFFFFF));
		benchmark::DoNotOptimize(hex.data());
		addr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DebugRefresh_AddressFormat_24Bit);

// Benchmark: Format 3 opcode bytes as hex (LDA abs,Y = $B9 $00 $80)
static void BM_DebugRefresh_OpcodeBytes_3Bytes(benchmark::State& state) {
	uint8_t b0 = 0xB9, b1 = 0x00, b2 = 0x80;
	for (auto _ : state) {
		std::string s;
		s = HexUtilities::ToHex(b0);
		s += " ";
		s += HexUtilities::ToHex(b1);
		s += " ";
		s += HexUtilities::ToHex(b2);
		benchmark::DoNotOptimize(s.data());
		b0++;
	}
	state.SetItemsProcessed(state.iterations() * 3);
}
BENCHMARK(BM_DebugRefresh_OpcodeBytes_3Bytes);

// Benchmark: Full address + bytes format for 64-row window
static void BM_DebugRefresh_FormatFullWindow(benchmark::State& state) {
	auto rows = BuildWindowRows(0x8000, kWindowRows);
	// Simulate 3-byte instructions with known opcode data
	uint8_t opcodes[kWindowRows][3];
	for (uint32_t i = 0; i < kWindowRows; i++) {
		opcodes[i][0] = static_cast<uint8_t>(i * 37 + 0xBD);
		opcodes[i][1] = static_cast<uint8_t>(i * 13);
		opcodes[i][2] = static_cast<uint8_t>(i * 7 + 0x80);
	}

	for (auto _ : state) {
		uint32_t totalLen = 0;
		for (uint32_t i = 0; i < kWindowRows; i++) {
			std::string line;
			line.reserve(32);
			line += HexUtilities::ToHex(static_cast<uint16_t>(rows[i].CpuAddress));
			line += "  ";
			line += HexUtilities::ToHex(opcodes[i][0]);
			line += " ";
			line += HexUtilities::ToHex(opcodes[i][1]);
			line += " ";
			line += HexUtilities::ToHex(opcodes[i][2]);
			totalLen += static_cast<uint32_t>(line.size());
		}
		benchmark::DoNotOptimize(totalLen);
	}
	state.SetItemsProcessed(state.iterations() * kWindowRows);
}
BENCHMARK(BM_DebugRefresh_FormatFullWindow);

// ---------------------------------------------------------------------------
// 4. Label/Comment HashMap Lookup
// ---------------------------------------------------------------------------
// LabelManager stores labels/comments in unordered_map<AddressInfo, string>.
// For every row in the window, the debugger does a map lookup to check for
// labels and comments. This is one of the dominant costs at scale.

// Benchmark: unordered_map lookup with address key (hit rate = 100%)
static void BM_DebugRefresh_LabelMapLookup_Hit(benchmark::State& state) {
	std::unordered_map<int32_t, std::string> labelMap;
	// Pre-populate with 1000 labels (typical labeled ROM)
	for (int32_t i = 0; i < 1000; i++) {
		labelMap[0x8000 + i * 4] = "label_" + std::to_string(i);
	}

	// Simulate 64-row window where all rows have labels
	auto rows = BuildWindowRows(0x8000, kWindowRows);
	for (auto& row : rows) {
		row.Flags |= LineFlags::Label;
	}

	for (auto _ : state) {
		uint32_t found = 0;
		for (const auto& row : rows) {
			auto it = labelMap.find(row.CpuAddress);
			if (it != labelMap.end()) {
				found++;
				benchmark::DoNotOptimize(it->second.data());
			}
		}
		benchmark::DoNotOptimize(found);
	}
	state.SetItemsProcessed(state.iterations() * kWindowRows);
}
BENCHMARK(BM_DebugRefresh_LabelMapLookup_Hit);

// Benchmark: unordered_map lookup with address key (hit rate = 10% — more realistic)
static void BM_DebugRefresh_LabelMapLookup_SparseMiss(benchmark::State& state) {
	std::unordered_map<int32_t, std::string> labelMap;
	// Only 100 labels: most lookups will be misses
	for (int32_t i = 0; i < 100; i++) {
		labelMap[0x8000 + i * 50] = "fn_" + std::to_string(i);
	}

	auto rows = BuildWindowRows(0x8000, kWindowRows);

	for (auto _ : state) {
		uint32_t found = 0;
		for (const auto& row : rows) {
			auto it = labelMap.find(row.CpuAddress);
			if (it != labelMap.end()) {
				found++;
				benchmark::DoNotOptimize(it->second.data());
			}
		}
		benchmark::DoNotOptimize(found);
	}
	state.SetItemsProcessed(state.iterations() * kWindowRows);
}
BENCHMARK(BM_DebugRefresh_LabelMapLookup_SparseMiss);

// Benchmark: Sorted vector lookup (alternative to unordered_map for label storage)
// This measures whether a sorted vector + binary search would be faster
// for the sparse hit case common in most games.
static void BM_DebugRefresh_LabelSortedVectorLookup(benchmark::State& state) {
	using LabelEntry = std::pair<int32_t, std::string>;
	std::vector<LabelEntry> sortedLabels;
	for (int32_t i = 0; i < 100; i++) {
		sortedLabels.emplace_back(0x8000 + i * 50, "fn_" + std::to_string(i));
	}
	std::sort(sortedLabels.begin(), sortedLabels.end(),
		[](const LabelEntry& a, const LabelEntry& b) { return a.first < b.first; });

	auto rows = BuildWindowRows(0x8000, kWindowRows);

	for (auto _ : state) {
		uint32_t found = 0;
		for (const auto& row : rows) {
			// Binary search
			auto it = std::lower_bound(sortedLabels.begin(), sortedLabels.end(),
				LabelEntry{row.CpuAddress, {}},
				[](const LabelEntry& a, const LabelEntry& b) { return a.first < b.first; });
			if (it != sortedLabels.end() && it->first == row.CpuAddress) {
				found++;
				benchmark::DoNotOptimize(it->second.data());
			}
		}
		benchmark::DoNotOptimize(found);
	}
	state.SetItemsProcessed(state.iterations() * kWindowRows);
}
BENCHMARK(BM_DebugRefresh_LabelSortedVectorLookup);

// ---------------------------------------------------------------------------
// 5. CodeLineData Struct Fill + memcpy
// ---------------------------------------------------------------------------
// GetLineData() fills a CodeLineData struct (2,016 bytes total: Text[1000] +
// Comment[1000] + other fields). The memcpy of text strings dominates.

// Benchmark: Fill one CodeLineData struct (simulates GetLineData per row)
static void BM_DebugRefresh_CodeLineDataFill_SingleRow(benchmark::State& state) {
	CodeLineData data = {};
	// Simulate a typical disassembly line: "LDA $8000,Y"
	constexpr char kLine[] = "LDA $8000,Y";
	constexpr char kComment[] = "; Load A from table";

	for (auto _ : state) {
		data = {};
		data.Address = 0x8130;
		data.OpSize = 3;
		data.Flags = LineFlags::VerifiedCode;
		data.ByteCode[0] = 0xB9;
		data.ByteCode[1] = 0x00;
		data.ByteCode[2] = 0x80;
		memcpy(data.Text, kLine, sizeof(kLine));
		memcpy(data.Comment, kComment, sizeof(kComment));
		benchmark::DoNotOptimize(data.Text[0]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DebugRefresh_CodeLineDataFill_SingleRow);

// Benchmark: Fill 64 CodeLineData structs (full window refresh)
static void BM_DebugRefresh_CodeLineDataFill_FullWindow(benchmark::State& state) {
	alignas(64) CodeLineData dataArray[kWindowRows] = {};

	constexpr char kLines[][24] = {
		"LDA $8000,Y",  "STA $0200,X",  "JSR $C100",    "BEQ $8050",
		"ROL",          "CLC",          "ADC #$10",     "TAX",
		"DEX",          "BNE $8020",    "PHA",          "PLA",
		"INY",          "CPY #$20",     "RTS",          "NOP"
	};
	constexpr uint32_t kLineCount = 16;

	for (auto _ : state) {
		for (uint32_t i = 0; i < kWindowRows; i++) {
			dataArray[i] = {};
			dataArray[i].Address = static_cast<int32_t>(0x8000 + i * 2);
			dataArray[i].OpSize  = static_cast<uint8_t>(1 + (i % 3));
			dataArray[i].Flags   = LineFlags::VerifiedCode;
			const char* line = kLines[i % kLineCount];
			memcpy(dataArray[i].Text, line, strlen(line) + 1);
		}
		benchmark::DoNotOptimize(dataArray[0].Text[0]);
	}
	state.SetItemsProcessed(state.iterations() * kWindowRows);
}
BENCHMARK(BM_DebugRefresh_CodeLineDataFill_FullWindow);

// ---------------------------------------------------------------------------
// 6. Memory Access Counter Read (MemoryAccessCounter per row)
// ---------------------------------------------------------------------------
// The memory viewer shows read/write/execute counts per address.
// For a 64-row disassembly window, 64 lookups into an access-count array.

// Benchmark: Access counter array lookup for 64 rows
static void BM_DebugRefresh_AccessCounterLookup_64Rows(benchmark::State& state) {
	// Access counter: one uint64_t per address in ROM
	std::vector<uint64_t> exeCounts(kNesRomSize, 0);
	// Simulate varying counts (some hot, most cold)
	for (uint32_t i = 0; i < kNesRomSize; i += 4) {
		exeCounts[i] = static_cast<uint64_t>(i * 7 + 1);
	}

	auto rows = BuildWindowRows(0x8000, kWindowRows);

	for (auto _ : state) {
		uint64_t total = 0;
		for (const auto& row : rows) {
			total += exeCounts[row.AbsAddress];
		}
		benchmark::DoNotOptimize(total);
	}
	state.SetItemsProcessed(state.iterations() * kWindowRows);
}
BENCHMARK(BM_DebugRefresh_AccessCounterLookup_64Rows);

// ---------------------------------------------------------------------------
// 7. Full Window Refresh Simulation
// ---------------------------------------------------------------------------
// Combines cache lookup + GetNthSegment + address format + label map lookup
// for all 64 rows. This is the closest to the real per-frame refresh cost.

static void BM_DebugRefresh_FullWindowRefresh_Simulation(benchmark::State& state) {
	// Setup
	std::vector<BenchDisassemblyInfo> cache(kNesRomSize);
	for (uint32_t i = 0; i < kNesRomSize; i++) {
		cache[i].Initialized = 1;
		cache[i].OpSize = 1 + (i % 3);
	}

	std::unordered_map<int32_t, std::string> labelMap;
	for (int32_t i = 0; i < 50; i++) {
		labelMap[0x8000 + i * 30] = "sub_" + std::to_string(i);
	}

	std::array<std::string, 5> comments = {
		"Load from OAM table",
		"Increment loop counter\nLoop exits when X=0",
		"Store sprite Y coordinate",
		"JSR to PPU wait",
		"Store result"
	};

	auto rows = BuildWindowRows(0x8000, kWindowRows);
	alignas(64) CodeLineData dataArray[kWindowRows] = {};

	for (auto _ : state) {
		for (uint32_t i = 0; i < kWindowRows; i++) {
			const auto& row = rows[i];
			auto& data = dataArray[i];
			data = {};
			data.Address = row.CpuAddress;

			// 1. Cache lookup
			const auto& info = cache[row.AbsAddress];
			data.OpSize = info.OpSize;
			data.Flags  = LineFlags::VerifiedCode;

			// 2. Address format
			std::string addrStr = HexUtilities::ToHex(static_cast<uint16_t>(row.CpuAddress));
			memcpy(data.Text, addrStr.data(), std::min<size_t>(addrStr.size() + 1, sizeof(data.Text)));

			// 3. Label map lookup
			auto labelIt = labelMap.find(row.CpuAddress);
			if (labelIt != labelMap.end()) {
				data.Flags |= LineFlags::Label;
			}

			// 4. Comment extraction (1 in 8 rows)
			if (i % 8 == 0) {
				const auto& cmtSrc = comments[i % comments.size()];
				std::string seg = StringUtilities::GetNthSegment(cmtSrc, '\n', 0);
				memcpy(data.Comment, seg.data(), std::min<size_t>(seg.size() + 1, sizeof(data.Comment)));
			}
		}
		benchmark::DoNotOptimize(dataArray[0].Text[0]);
	}
	state.SetItemsProcessed(state.iterations() * kWindowRows);
}
BENCHMARK(BM_DebugRefresh_FullWindowRefresh_Simulation);
