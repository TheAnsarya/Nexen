#pragma once
#include "pch.h"
#include "SNES/Coprocessors/SA1/Sa1Cpu.h"
#include "SNES/Coprocessors/SA1/Sa1Types.h"
#include "SNES/Coprocessors/SA1/Sa1.h"
#include "SNES/IMemoryHandler.h"

/// <summary>
/// SA-1 BW-RAM (Battery-backed Work RAM) memory handler for SA-1 CPU access.
/// Provides access to the BW-RAM (up to 256KB) with optional bitmap mode.
/// Used for game save data and as additional work memory.
/// </summary>
/// <remarks>
/// BW-RAM access regions:
/// - $00-$3F:$6000-$7FFF + $80-$BF:$6000-$7FFF: Optional bitmap mode + bank select
/// - $60-$6F:$0000-$FFFF: Always uses bitmap mode
/// 
/// Bitmap mode allows efficient pixel-level access for graphics operations,
/// supporting 2BPP (4 pixels per byte) or 4BPP (2 pixels per byte) modes.
/// </remarks>
class Sa1BwRamHandler : public IMemoryHandler {
private:
	/// <summary>Pointer to BW-RAM data buffer.</summary>
	uint8_t* _ram;

	/// <summary>Address mask based on BW-RAM size.</summary>
	uint32_t _mask;

	/// <summary>Pointer to SA-1 state for mode and bank settings.</summary>
	Sa1State* _state;

	/// <summary>
	/// Calculates the BW-RAM address with bank offset.
	/// </summary>
	/// <param name="addr">Input address.</param>
	/// <returns>BW-RAM address with bank applied.</returns>
	uint32_t GetBwRamAddress(uint32_t addr) {
		return ((_state->Sa1BwBank * 0x2000) | (addr & 0x1FFF));
	}

	/// <summary>
	/// Reads from BW-RAM with automatic mode selection.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte (or pixel value in bitmap mode).</returns>
	__forceinline uint8_t InternalRead(uint32_t addr) {
		// $60-$6F always uses bitmap mode
		if ((addr & 0x600000) == 0x600000) {
			return ReadBitmapMode(addr - 0x600000);
		} else {
			addr = GetBwRamAddress(addr);
			if (_state->Sa1BwMode) {
				// Bitmap mode is enabled for this region
				return ReadBitmapMode(addr);
			} else {
				// Return regular memory content
				return _ram[addr & _mask];
			}
		}
	}

	/// <summary>
	/// Writes value with write protection check.
	/// </summary>
	/// <param name="addr">BW-RAM address.</param>
	/// <param name="value">Data byte to write.</param>
	__forceinline void WriteValue(uint32_t addr, uint8_t value) {
		// Check if writes are enabled globally
		if (_state->CpuBwWriteEnabled || _state->Sa1BwWriteEnabled) {
			_ram[addr] = value;
		} else {
			// Check address against write-protected area size
			uint32_t size = 256 << (_state->BwWriteProtectedArea > 0x0A ? 0x0A : _state->BwWriteProtectedArea);
			if ((addr & 0x3FFFF) >= size) {
				_ram[addr] = value;
			}
		}
	}

public:
	/// <summary>
	/// Creates a new SA-1 BW-RAM handler.
	/// </summary>
	/// <param name="bwRam">Pointer to BW-RAM buffer.</param>
	/// <param name="bwRamSize">Size of BW-RAM in bytes.</param>
	/// <param name="state">Pointer to SA-1 state.</param>
	Sa1BwRamHandler(uint8_t* bwRam, uint32_t bwRamSize, Sa1State* state) : IMemoryHandler(MemoryType::SnesSaveRam) {
		_ram = bwRam;
		_mask = bwRamSize - 1;
		_state = state;
	}

	/// <summary>
	/// Reads from BW-RAM.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte.</returns>
	uint8_t Read(uint32_t addr) override {
		return InternalRead(addr);
	}

	/// <summary>
	/// Peeks BW-RAM without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Data byte.</returns>
	uint8_t Peek(uint32_t addr) override {
		return InternalRead(addr);
	}

	/// <summary>
	/// Peeks a block of BW-RAM data.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer for data (4KB).</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override {
		for (int i = 0; i < 0x1000; i++) {
			output[i] = InternalRead(addr + i);
		}
	}

	/// <summary>
	/// Writes to BW-RAM with automatic mode selection.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data byte (or pixel value in bitmap mode).</param>
	void Write(uint32_t addr, uint8_t value) override {
		// $60-$6F always uses bitmap mode
		if ((addr & 0x600000) == 0x600000) {
			WriteBitmapMode(addr - 0x600000, value);
		} else {
			addr = GetBwRamAddress(addr);
			if (_state->Sa1BwMode) {
				WriteBitmapMode(addr, value);
			} else {
				WriteValue(addr & _mask, value);
			}
		}
	}

	/// <summary>
	/// Reads a pixel value in bitmap mode.
	/// </summary>
	/// <param name="addr">Virtual pixel address.</param>
	/// <returns>Pixel value (0-3 in 2BPP, 0-15 in 4BPP).</returns>
	/// <remarks>
	/// In 2BPP mode: 4 pixels per byte, each pixel is 2 bits.
	/// In 4BPP mode: 2 pixels per byte, each pixel is 4 bits.
	/// This allows efficient graphics operations using linear addressing.
	/// </remarks>
	uint8_t ReadBitmapMode(uint32_t addr) {
		if (_state->BwRam2BppMode) {
			// 2BPP: 4 pixels per byte, extract 2-bit pixel
			return (_ram[(addr >> 2) & _mask] >> ((addr & 0x03) * 2)) & 0x03;
		} else {
			// 4BPP: 2 pixels per byte, extract 4-bit pixel
			return (_ram[(addr >> 1) & _mask] >> ((addr & 0x01) * 4)) & 0x0F;
		}
	}

	/// <summary>
	/// Writes a pixel value in bitmap mode.
	/// </summary>
	/// <param name="addr">Virtual pixel address.</param>
	/// <param name="value">Pixel value to write.</param>
	/// <remarks>
	/// Performs read-modify-write to update only the target pixel bits
	/// within the byte, preserving adjacent pixels.
	/// </remarks>
	void WriteBitmapMode(uint32_t addr, uint8_t value) {
		if (_state->BwRam2BppMode) {
			// 2BPP: Calculate bit shift for this pixel position
			uint8_t shift = (addr & 0x03) * 2;
			addr = (addr >> 2) & _mask;
			// Mask out old pixel, OR in new value
			WriteValue(addr, (_ram[addr] & ~(0x03 << shift)) | ((value & 0x03) << shift));
		} else {
			// 4BPP: Calculate bit shift for this pixel position
			uint8_t shift = (addr & 0x01) * 4;
			addr = (addr >> 1) & _mask;
			// Mask out old pixel, OR in new value
			WriteValue(addr, (_ram[addr] & ~(0x0F << shift)) | ((value & 0x0F) << shift));
		}
	}

	/// <summary>
	/// Gets absolute address for debugging.
	/// </summary>
	/// <param name="addr">Relative address.</param>
	/// <returns>Absolute address info pointing to underlying BW-RAM byte.</returns>
	/// <remarks>
	/// In bitmap mode, the returned address is the underlying byte address,
	/// not the virtual pixel address.
	/// </remarks>
	AddressInfo GetAbsoluteAddress(uint32_t addr) override {
		AddressInfo info;
		if ((addr & 0x600000) == 0x600000) {
			// Bitmap region: convert pixel address to byte address
			info.Address = ((addr - 0x600000) >> (_state->BwRam2BppMode ? 2 : 1)) & _mask;
		} else {
			info.Address = GetBwRamAddress(addr) & _mask;
		}
		info.Type = MemoryType::SnesSaveRam;
		return info;
	}
};