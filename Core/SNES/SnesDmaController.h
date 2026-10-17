#pragma once
#include "pch.h"
#include "SNES/SnesCpuTypes.h"
#include "SNES/DmaControllerTypes.h"
#include "Utilities/ISerializable.h"

class SnesMemoryManager;

/// <summary>
/// SNES DMA/HDMA Controller - Handles all 8 DMA channels.
/// Provides both general-purpose DMA and horizontal-blank DMA (HDMA) for effects.
/// </summary>
/// <remarks>
/// DMA Features:
/// - 8 independent channels (0-7)
/// - Bus A (CPU address space) ↔ Bus B (PPU/APU registers) transfers
/// - Fixed or incrementing/decrementing addressing
/// - Multiple transfer patterns (1, 2, or 4 bytes per unit)
/// 
/// HDMA Features:
/// - Automatic transfers during each H-blank
/// - Table-driven with line counts and repeat modes
/// - Used for gradient effects, window changes, parallax scrolling
/// - Channels 0-7 can operate as DMA or HDMA independently
/// </remarks>
class SnesDmaController final : public ISerializable {
private:
	/// <summary>Flag bit indicating HDMA channel mode in channel enable register.</summary>
	static constexpr uint8_t HdmaChannelFlag = 0x40;

	/// <summary>DMA controller register state.</summary>
	SnesDmaControllerState _state = {};

	/// <summary>Flag indicating pending DMA/HDMA work to process.</summary>
	bool _needToProcess = false;

	/// <summary>Flag indicating HDMA transfer pending this scanline.</summary>
	bool _hdmaPending = false;

	/// <summary>Flag indicating HDMA table reload needed at frame start.</summary>
	bool _hdmaInitPending = false;

	/// <summary>Flag for DMA startup delay timing.</summary>
	bool _dmaStartDelay = false;

	/// <summary>Flag indicating CPU-initiated DMA is pending.</summary>
	bool _dmaPending = false;

	/// <summary>Clock cycle counter for DMA timing.</summary>
	uint32_t _dmaClockCounter = 0;

	/// <summary>Currently active DMA channel (for debugger event viewer).</summary>
	uint8_t _activeChannel = 0;

	/// <summary>Memory manager for bus A/B access.</summary>
	SnesMemoryManager* _memoryManager;

	/// <summary>Copies a single byte during DMA transfer.</summary>
	/// <param name="addressBusA">Address on Bus A (CPU side).</param>
	/// <param name="addressBusB">Address on Bus B (PPU/APU side, $2100-$21FF).</param>
	/// <param name="fromBtoA">True for B→A transfer, false for A→B.</param>
	void CopyDmaByte(uint32_t addressBusA, uint16_t addressBusB, bool fromBtoA);

	/// <summary>Executes a general-purpose DMA transfer for one channel.</summary>
	/// <param name="channel">Channel configuration to process.</param>
	void RunDma(DmaChannelConfig& channel);

	/// <summary>Executes HDMA transfer for one channel during H-blank.</summary>
	/// <param name="channel">Channel configuration to process.</param>
	void RunHdmaTransfer(DmaChannelConfig& channel);

	/// <summary>Processes all active HDMA channels for current scanline.</summary>
	/// <returns>True if any transfers occurred.</returns>
	bool ProcessHdmaChannels();

	/// <summary>Checks if this is the last active HDMA channel in priority order.</summary>
	/// <param name="channel">Channel number to check.</param>
	/// <returns>True if this is the last active HDMA channel.</returns>
	[[nodiscard]] bool IsLastActiveHdmaChannel(uint8_t channel);

	/// <summary>Initializes HDMA channels at start of frame.</summary>
	/// <returns>True if any HDMA channels were initialized.</returns>
	bool InitHdmaChannels();

	/// <summary>Synchronizes timing at DMA transfer start.</summary>
	void SyncStartDma();

	/// <summary>Synchronizes timing at DMA transfer end.</summary>
	void SyncEndDma();

	/// <summary>Updates the need-to-process flag based on pending work.</summary>
	void UpdateNeedToProcessFlag();

	/// <summary>Checks if any DMA channel has pending transfer.</summary>
	/// <returns>True if any channel is actively transferring.</returns>
	[[nodiscard]] bool HasActiveDmaChannel();

public:
	/// <summary>Constructs the DMA controller.</summary>
	/// <param name="memoryManager">Memory manager for bus access.</param>
	SnesDmaController(SnesMemoryManager* memoryManager);

	/// <summary>Gets mutable reference to DMA controller state.</summary>
	/// <returns>Reference to controller register state.</returns>
	SnesDmaControllerState& GetState();

	/// <summary>Resets the DMA controller to initial state.</summary>
	void Reset();

	/// <summary>Initiates HDMA transfers for current H-blank.</summary>
	void BeginHdmaTransfer();

	/// <summary>Initiates HDMA table loading at frame start.</summary>
	void BeginHdmaInit();

	/// <summary>Checks if there are pending DMA/HDMA transfers.</summary>
	/// <returns>True if transfers are pending.</returns>
	[[nodiscard]] __forceinline bool HasPendingTransfer() { return _needToProcess; }

	/// <summary>Processes all pending DMA and HDMA transfers.</summary>
	/// <returns>True if any transfers were processed.</returns>
	bool ProcessPendingTransfers();

	/// <summary>Writes to DMA controller register.</summary>
	/// <param name="addr">Register address ($4300-$437F).</param>
	/// <param name="value">Value to write.</param>
	void Write(uint16_t addr, uint8_t value);

	/// <summary>Reads DMA controller register.</summary>
	/// <param name="addr">Register address ($4300-$437F).</param>
	/// <returns>Register value.</returns>
	uint8_t Read(uint16_t addr);

	/// <summary>Gets the currently active DMA channel number.</summary>
	/// <returns>Active channel (0-7) or 0 if none.</returns>
	uint8_t GetActiveChannel();

	/// <summary>Gets configuration for a specific DMA channel.</summary>
	/// <param name="channel">Channel number (0-7).</param>
	/// <returns>Copy of channel configuration.</returns>
	DmaChannelConfig GetChannelConfig(uint8_t channel);

	/// <summary>Serializes DMA controller state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;
};