#include "pch.h"
#include <benchmark/benchmark.h>
#include <array>
#include <cstring>
#include "WS/WsTypes.h"

// =============================================================================
// WonderSwan Flash Cart & RTC Benchmarks
// =============================================================================
// Benchmarks for Flash command protocol throughput and RTC serial protocol
// performance. Covers the hot paths in WsCart::FlashWrite/FlashRead and
// WsRtc::Write/Read for #1093 and #1092.

// =============================================================================
// Flash Command Sequence Benchmarks
// =============================================================================

namespace {
	// Lightweight Flash state machine simulation (mirrors WsCart::FlashWrite)
	struct FlashState {
		WsFlashMode mode = WsFlashMode::Idle;
		uint8_t cycle = 0;
		bool softwareId = false;
	};

	void FlashWrite(FlashState& state, uint32_t addr, uint8_t value) {
		if (state.mode == WsFlashMode::WriteByte) {
			state.mode = WsFlashMode::Idle;
			state.cycle = 0;
			return;
		}

		if (state.mode == WsFlashMode::Erase) {
			if (state.cycle == 3) {
				if ((addr & 0xffff) == 0x5555 && value == 0xaa) {
					state.cycle++;
				} else {
					state.mode = WsFlashMode::Idle;
					state.cycle = 0;
				}
			} else if (state.cycle == 4) {
				if ((addr & 0xffff) == 0x2aaa && value == 0x55) {
					state.cycle++;
				} else {
					state.mode = WsFlashMode::Idle;
					state.cycle = 0;
				}
			} else if (state.cycle == 5) {
				state.mode = WsFlashMode::Idle;
				state.cycle = 0;
			}
			return;
		}

		// Idle: process command sequence
		if (state.cycle == 0) {
			if ((addr & 0xffff) == 0x5555 && value == 0xaa) {
				state.cycle++;
			} else if (value == 0xf0) {
				state.softwareId = false;
				state.cycle = 0;
			}
		} else if (state.cycle == 1) {
			if ((addr & 0xffff) == 0x2aaa && value == 0x55) {
				state.cycle++;
			} else {
				state.cycle = 0;
			}
		} else if (state.cycle == 2) {
			if ((addr & 0xffff) == 0x5555) {
				switch (value) {
					case 0x80:
						state.mode = WsFlashMode::Erase;
						state.cycle = 3;
						break;
					case 0x90:
						state.softwareId = true;
						state.cycle = 0;
						break;
					case 0xa0:
						state.mode = WsFlashMode::WriteByte;
						state.cycle = 0;
						break;
					case 0xf0:
						state.softwareId = false;
						state.cycle = 0;
						break;
					default:
						state.cycle = 0;
						break;
				}
			} else {
				state.cycle = 0;
			}
		}
	}

	uint8_t FlashRead(const uint8_t* rom, uint32_t romSize, bool softwareId, uint32_t addr) {
		if (softwareId) {
			return ((addr & 0x01) == 0) ? 0xbf : 0xb5;
		}
		return (addr < romSize) ? rom[addr] : 0xff;
	}
}

// Benchmark: Full 3-cycle Software ID enter + exit
static void BM_WsFlash_SoftwareIdEnterExit(benchmark::State& state) {
	for (auto _ : state) {
		FlashState fs;
		// Enter Software ID
		FlashWrite(fs, 0x5555, 0xaa);
		FlashWrite(fs, 0x2aaa, 0x55);
		FlashWrite(fs, 0x5555, 0x90);
		benchmark::DoNotOptimize(fs.softwareId);

		// Exit Software ID
		FlashWrite(fs, 0x5555, 0xaa);
		FlashWrite(fs, 0x2aaa, 0x55);
		FlashWrite(fs, 0x5555, 0xf0);
		benchmark::DoNotOptimize(fs.softwareId);
	}
}
BENCHMARK(BM_WsFlash_SoftwareIdEnterExit);

// Benchmark: WriteByte command sequence (3 cycles + 1 data write)
static void BM_WsFlash_WriteByteSequence(benchmark::State& state) {
	for (auto _ : state) {
		FlashState fs;
		FlashWrite(fs, 0x5555, 0xaa);
		FlashWrite(fs, 0x2aaa, 0x55);
		FlashWrite(fs, 0x5555, 0xa0);
		FlashWrite(fs, 0x1000, 0x42); // data write
		benchmark::DoNotOptimize(fs.mode);
	}
}
BENCHMARK(BM_WsFlash_WriteByteSequence);

// Benchmark: Full 6-cycle sector erase sequence
static void BM_WsFlash_SectorEraseSequence(benchmark::State& state) {
	for (auto _ : state) {
		FlashState fs;
		// Setup (3 cycles)
		FlashWrite(fs, 0x5555, 0xaa);
		FlashWrite(fs, 0x2aaa, 0x55);
		FlashWrite(fs, 0x5555, 0x80);
		// Confirm (3 cycles)
		FlashWrite(fs, 0x5555, 0xaa);
		FlashWrite(fs, 0x2aaa, 0x55);
		FlashWrite(fs, 0x1000, 0x30); // sector erase
		benchmark::DoNotOptimize(fs.mode);
	}
}
BENCHMARK(BM_WsFlash_SectorEraseSequence);

// Benchmark: Flash read normal mode
static void BM_WsFlash_ReadNormal(benchmark::State& state) {
	std::array<uint8_t, 0x80000> rom;
	rom.fill(0x42);
	for (auto _ : state) {
		uint8_t val = FlashRead(rom.data(), static_cast<uint32_t>(rom.size()), false, 0x1234);
		benchmark::DoNotOptimize(val);
	}
}
BENCHMARK(BM_WsFlash_ReadNormal);

// Benchmark: Flash read Software ID mode
static void BM_WsFlash_ReadSoftwareId(benchmark::State& state) {
	for (auto _ : state) {
		uint8_t mfr = FlashRead(nullptr, 0, true, 0x00);
		uint8_t dev = FlashRead(nullptr, 0, true, 0x01);
		benchmark::DoNotOptimize(mfr);
		benchmark::DoNotOptimize(dev);
	}
}
BENCHMARK(BM_WsFlash_ReadSoftwareId);

// Benchmark: Command sequence with invalid addresses (abort path)
static void BM_WsFlash_AbortedSequence(benchmark::State& state) {
	for (auto _ : state) {
		FlashState fs;
		FlashWrite(fs, 0x5555, 0xaa); // cycle 0 OK
		FlashWrite(fs, 0x1234, 0x55); // cycle 1 WRONG addr → abort
		benchmark::DoNotOptimize(fs.cycle);
	}
}
BENCHMARK(BM_WsFlash_AbortedSequence);

// =============================================================================
// RTC Serial Protocol Benchmarks
// =============================================================================

namespace {
	// Port 0xCA bit helpers
	uint8_t RtcBuildPort(uint8_t data, uint8_t clock, bool cs) {
		return (data & 0x01) | ((clock & 0x01) << 1) | (cs ? 0x10 : 0x00);
	}

	// Lightweight RTC protocol state (mirrors WsRtc serial protocol)
	struct RtcState {
		uint8_t command = 0;
		uint8_t bitCounter = 0;
		uint8_t clk = 0;
		bool chipSelect = false;
		uint8_t dataBuffer[7] = {};
		uint8_t dataSize = 0;
		uint8_t dataIndex = 0;
		uint8_t dataBitCounter = 0;
		bool reading = false;
		uint8_t bitOut = 0;
	};

	void RtcWrite(RtcState& s, uint8_t value) {
		uint8_t data = value & 0x01;
		uint8_t clk = (value >> 1) & 0x01;
		bool cs = (value >> 4) & 0x01;

		if (!cs) {
			s.chipSelect = false;
			s.command = 0;
			s.bitCounter = 0;
			s.dataSize = 0;
			s.dataIndex = 0;
			s.dataBitCounter = 0;
			s.reading = false;
			s.bitOut = 0;
			s.clk = 0;
			return;
		}

		if (!s.chipSelect && cs) {
			s.chipSelect = true;
			s.command = 0;
			s.bitCounter = 0;
			s.dataSize = 0;
			s.dataIndex = 0;
			s.dataBitCounter = 0;
			s.reading = false;
			s.bitOut = 0;
			s.clk = clk;
			return;
		}

		// Rising edge
		if (clk && !s.clk) {
			if (s.bitCounter < 4) {
				s.command = (s.command << 1) | data;
				s.bitCounter++;
				if (s.bitCounter == 4) {
					s.reading = s.command & 0x01;
					uint8_t reg = (s.command >> 1) & 0x07;
					switch (reg) {
						case 0: s.dataSize = 1; break;
						case 1: s.dataSize = 7; break;
						case 2: s.dataSize = 2; break;
						default: s.dataSize = 0; break;
					}
					s.dataIndex = 0;
					s.dataBitCounter = 0;
				}
			} else if (s.dataIndex < s.dataSize) {
				if (s.reading) {
					s.bitOut = (s.dataBuffer[s.dataIndex] >> (7 - s.dataBitCounter)) & 0x01;
				} else {
					s.dataBuffer[s.dataIndex] = (s.dataBuffer[s.dataIndex] << 1) | data;
				}
				s.dataBitCounter++;
				if (s.dataBitCounter >= 8) {
					s.dataBitCounter = 0;
					s.dataIndex++;
				}
			}
		}

		s.clk = clk;
	}

	// Send 4-bit command MSB-first via clock edges
	void RtcSendCommand(RtcState& s, uint8_t cmd) {
		for (int i = 3; i >= 0; i--) {
			uint8_t bit = (cmd >> i) & 0x01;
			// Clock low with data
			RtcWrite(s, RtcBuildPort(bit, 0, true));
			// Clock high (rising edge samples)
			RtcWrite(s, RtcBuildPort(bit, 1, true));
		}
	}

	// Read 8 data bits MSB-first
	uint8_t RtcReadByte(RtcState& s) {
		uint8_t result = 0;
		for (int i = 0; i < 8; i++) {
			RtcWrite(s, RtcBuildPort(0, 0, true));
			RtcWrite(s, RtcBuildPort(0, 1, true));
			result = (result << 1) | s.bitOut;
		}
		return result;
	}

	// Write 8 data bits MSB-first
	void RtcWriteByte(RtcState& s, uint8_t value) {
		for (int i = 7; i >= 0; i--) {
			uint8_t bit = (value >> i) & 0x01;
			RtcWrite(s, RtcBuildPort(bit, 0, true));
			RtcWrite(s, RtcBuildPort(bit, 1, true));
		}
	}
}

// Benchmark: Full read-datetime transaction (CS up, 4-bit cmd, 7 bytes out, CS down)
static void BM_WsRtc_ReadDateTimeTransaction(benchmark::State& state) {
	for (auto _ : state) {
		RtcState rtc;
		// Fill data buffer with sample BCD datetime
		rtc.dataBuffer[0] = 0x24; // Year
		rtc.dataBuffer[1] = 0x06; // Month
		rtc.dataBuffer[2] = 0x15; // Day
		rtc.dataBuffer[3] = 0x06; // DoW
		rtc.dataBuffer[4] = 0x14; // Hour
		rtc.dataBuffer[5] = 0x30; // Minute
		rtc.dataBuffer[6] = 0x00; // Second

		// CS low→high
		RtcWrite(rtc, RtcBuildPort(0, 0, false));
		RtcWrite(rtc, RtcBuildPort(0, 0, true));

		// Send command 0x03 (read datetime)
		RtcSendCommand(rtc, 0x03);

		// Manually fill buffer after command decode (simulating PrepareReadData)
		rtc.dataBuffer[0] = 0x24;
		rtc.dataBuffer[1] = 0x06;
		rtc.dataBuffer[2] = 0x15;
		rtc.dataBuffer[3] = 0x06;
		rtc.dataBuffer[4] = 0x14;
		rtc.dataBuffer[5] = 0x30;
		rtc.dataBuffer[6] = 0x00;

		// Read 7 bytes
		for (int i = 0; i < 7; i++) {
			uint8_t b = RtcReadByte(rtc);
			benchmark::DoNotOptimize(b);
		}

		// CS low
		RtcWrite(rtc, RtcBuildPort(0, 0, false));
	}
}
BENCHMARK(BM_WsRtc_ReadDateTimeTransaction);

// Benchmark: Write-datetime transaction
static void BM_WsRtc_WriteDateTimeTransaction(benchmark::State& state) {
	for (auto _ : state) {
		RtcState rtc;

		// CS low→high
		RtcWrite(rtc, RtcBuildPort(0, 0, false));
		RtcWrite(rtc, RtcBuildPort(0, 0, true));

		// Send command 0x02 (write datetime)
		RtcSendCommand(rtc, 0x02);

		// Write 7 BCD bytes
		RtcWriteByte(rtc, 0x24); // Year
		RtcWriteByte(rtc, 0x06); // Month
		RtcWriteByte(rtc, 0x15); // Day
		RtcWriteByte(rtc, 0x06); // DoW
		RtcWriteByte(rtc, 0x14); // Hour
		RtcWriteByte(rtc, 0x30); // Minute
		RtcWriteByte(rtc, 0x00); // Second

		// CS low (commits write)
		RtcWrite(rtc, RtcBuildPort(0, 0, false));
		benchmark::DoNotOptimize(rtc.dataBuffer);
	}
}
BENCHMARK(BM_WsRtc_WriteDateTimeTransaction);

// Benchmark: Read status (1 byte, shortest transaction)
static void BM_WsRtc_ReadStatusTransaction(benchmark::State& state) {
	for (auto _ : state) {
		RtcState rtc;
		rtc.dataBuffer[0] = 0x00;

		RtcWrite(rtc, RtcBuildPort(0, 0, false));
		RtcWrite(rtc, RtcBuildPort(0, 0, true));
		RtcSendCommand(rtc, 0x01);

		uint8_t b = RtcReadByte(rtc);
		benchmark::DoNotOptimize(b);

		RtcWrite(rtc, RtcBuildPort(0, 0, false));
	}
}
BENCHMARK(BM_WsRtc_ReadStatusTransaction);

// Benchmark: Port decode throughput
static void BM_WsRtc_PortDecode(benchmark::State& state) {
	for (auto _ : state) {
		for (uint16_t v = 0; v < 256; v++) {
			uint8_t data = v & 0x01;
			uint8_t clk = (v >> 1) & 0x01;
			bool cs = (v >> 4) & 0x01;
			benchmark::DoNotOptimize(data);
			benchmark::DoNotOptimize(clk);
			benchmark::DoNotOptimize(cs);
		}
	}
}
BENCHMARK(BM_WsRtc_PortDecode);

// Benchmark: BCD encode/decode throughput
static void BM_WsRtc_BcdRoundtrip(benchmark::State& state) {
	for (auto _ : state) {
		for (int i = 0; i < 100; i++) {
			uint8_t bcd = static_cast<uint8_t>((i / 10) << 4 | (i % 10));
			int decoded = (bcd >> 4) * 10 + (bcd & 0x0f);
			benchmark::DoNotOptimize(decoded);
		}
	}
}
BENCHMARK(BM_WsRtc_BcdRoundtrip);
