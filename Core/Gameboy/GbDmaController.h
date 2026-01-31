#pragma once
#include "pch.h"
#include "Gameboy/GbTypes.h"
#include "Utilities/ISerializable.h"

class GbMemoryManager;
class GbPpu;
class GbCpu;
class Gameboy;

/// <summary>
/// Game Boy OAM DMA controller implementation.
/// Handles OAM DMA ($FF46) for DMG and HDMA/GDMA for CGB.
/// OAM DMA copies 160 bytes from ROM/RAM to OAM ($FE00-$FE9F).
/// </summary>
class GbDmaController final : public ISerializable {
private:
	/// <summary>DMA controller state including source, counter, and CGB HDMA state.</summary>
	GbDmaControllerState _state = {};

	/// <summary>Memory manager for bus access during DMA.</summary>
	GbMemoryManager* _memoryManager = nullptr;

	/// <summary>PPU reference for OAM writes.</summary>
	GbPpu* _ppu = nullptr;

	/// <summary>CPU reference for halt state checking (DMA pauses during halt).</summary>
	GbCpu* _cpu = nullptr;

	/// <summary>Gameboy instance for CGB mode detection.</summary>
	Gameboy* _gameboy = nullptr;

	/// <summary>Processes a block of HDMA data (CGB only).</summary>
	void ProcessDmaBlock();

	/// <summary>Calculates the current OAM source read address.</summary>
	/// <returns>16-bit address to read from.</returns>
	uint16_t GetOamReadAddress();

public:
	/// <summary>
	/// Initializes the DMA controller with hardware references.
	/// </summary>
	/// <param name="gameboy">Gameboy instance for CGB detection.</param>
	/// <param name="memoryManager">Memory manager for DMA transfers.</param>
	/// <param name="ppu">PPU for OAM access.</param>
	/// <param name="cpu">CPU for halt state checking.</param>
	void Init(Gameboy* gameboy, GbMemoryManager* memoryManager, GbPpu* ppu, GbCpu* cpu);

	/// <summary>Gets the current DMA controller state.</summary>
	/// <returns>Copy of DMA state.</returns>
	GbDmaControllerState GetState();

	/// <summary>
	/// Executes one cycle of OAM DMA.
	/// Transfers one byte per cycle when active.
	/// </summary>
	void Exec();

	/// <summary>Gets the last OAM address written during DMA.</summary>
	/// <returns>Offset into OAM (0-159).</returns>
	uint8_t GetLastWriteAddress();

	/// <summary>
	/// Checks if the given address conflicts with OAM DMA source.
	/// </summary>
	/// <param name="addr">Address to check.</param>
	/// <returns>True if address conflicts with DMA.</returns>
	bool IsOamDmaConflict(uint16_t addr);

	/// <summary>
	/// Processes read conflicts during OAM DMA.
	/// Returns DMA source address if conflict exists.
	/// </summary>
	/// <param name="addr">Address being read.</param>
	/// <returns>Address to actually read from.</returns>
	uint16_t ProcessOamDmaReadConflict(uint16_t addr);

	/// <summary>Checks if OAM DMA is currently running.</summary>
	/// <returns>True if DMA is active.</returns>
	bool IsOamDmaRunning();

	/// <summary>Reads from DMA register ($FF46).</summary>
	/// <returns>Last written DMA source high byte.</returns>
	uint8_t Read();

	/// <summary>
	/// Writes to DMA register ($FF46) to start OAM DMA.
	/// </summary>
	/// <param name="value">High byte of source address.</param>
	void Write(uint8_t value);

	/// <summary>Reads from CGB HDMA registers ($FF51-$FF55).</summary>
	/// <param name="addr">Register address.</param>
	/// <returns>Register value.</returns>
	uint8_t ReadCgb(uint16_t addr);

	/// <summary>Writes to CGB HDMA registers ($FF51-$FF55).</summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteCgb(uint16_t addr, uint8_t value);

	/// <summary>Processes H-Blank DMA for CGB (called during HBlank).</summary>
	void ProcessHdma();

	/// <summary>Serializes DMA state for save states.</summary>
	void Serialize(Serializer& s) override;
};