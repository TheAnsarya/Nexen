#pragma once
#include "pch.h"
#include "GBA/GbaTypes.h"
#include "Utilities/ISerializable.h"

class GbaMemoryManager;
class GbaCpu;
class GbaRomPrefetch;

/// <summary>
/// GBA DMA controller implementation.
/// Provides 4 independent DMA channels with different priorities and triggers.
/// Channel 3 supports video capture DMA, channels 1-2 support audio DMA.
/// </summary>
class GbaDmaController final : public ISerializable {
private:
	/// <summary>CPU reference - halted during DMA.</summary>
	GbaCpu* _cpu = nullptr;

	/// <summary>Memory manager for bus access and IRQ signaling.</summary>
	GbaMemoryManager* _memoryManager = nullptr;

	/// <summary>ROM prefetch buffer - affected by DMA transfers.</summary>
	GbaRomPrefetch* _prefetcher = nullptr;

	/// <summary>State for all 4 DMA channels.</summary>
	GbaDmaControllerState _state = {};

	/// <summary>Currently active DMA channel (-1 if none).</summary>
	int8_t _dmaActiveChannel = -1;

	/// <summary>Flag indicating one or more DMAs are pending.</summary>
	bool _dmaPending = false;

	/// <summary>Flag indicating DMA is currently executing.</summary>
	bool _dmaRunning = false;

	/// <summary>Flag indicating DMA needs to start.</summary>
	bool _needStart = false;

	/// <summary>Counter for idle cycles after DMA completion.</summary>
	uint32_t _idleCycleCounter = 0;

	/// <summary>Gets the index of the highest-priority pending DMA.</summary>
	/// <returns>Channel index (0-3) or -1 if none ready.</returns>
	int GetPendingDmaIndex();

	/// <summary>
	/// Executes a DMA transfer for the specified channel.
	/// </summary>
	/// <param name="ch">DMA channel state.</param>
	/// <param name="chIndex">Channel index (0-3).</param>
	void RunDma(GbaDmaChannel& ch, uint8_t chIndex);

public:
	/// <summary>
	/// Initializes DMA controller with hardware references.
	/// </summary>
	/// <param name="cpu">CPU reference.</param>
	/// <param name="memoryManager">Memory manager.</param>
	/// <param name="prefetcher">ROM prefetch buffer.</param>
	void Init(GbaCpu* cpu, GbaMemoryManager* memoryManager, GbaRomPrefetch* prefetcher);

	/// <summary>Gets reference to DMA controller state.</summary>
	/// <returns>Reference to state.</returns>
	GbaDmaControllerState& GetState();

	/// <summary>Checks if video capture DMA (channel 3, special trigger) is enabled.</summary>
	/// <returns>True if video capture DMA is active.</returns>
	bool IsVideoCaptureDmaEnabled();

	/// <summary>Checks if any DMA is currently running.</summary>
	/// <returns>True if DMA is executing.</returns>
	[[nodiscard]] bool IsRunning() { return _dmaRunning; }

	/// <summary>Gets the currently active DMA channel for debugging.</summary>
	/// <returns>Channel index or -1.</returns>
	int8_t DebugGetActiveChannel();

	/// <summary>
	/// Triggers a specific DMA channel if it matches the trigger condition.
	/// </summary>
	/// <param name="trigger">Trigger type (Immediate, VBlank, HBlank, Special).</param>
	/// <param name="channel">Channel index (0-3).</param>
	/// <param name="forceStop">If true, disables repeat mode.</param>
	void TriggerDmaChannel(GbaDmaTrigger trigger, uint8_t channel, bool forceStop = false);

	/// <summary>
	/// Triggers all DMA channels matching the trigger condition.
	/// </summary>
	/// <param name="trigger">Trigger type.</param>
	void TriggerDma(GbaDmaTrigger trigger);

	/// <summary>Checks if any DMA is pending and ready to start.</summary>
	/// <returns>True if DMA needs to start.</returns>
	__forceinline bool HasPendingDma() { return _needStart; }

	/// <summary>
	/// Runs pending DMA transfers if conditions allow.
	/// </summary>
	/// <param name="allowStartDma">True if DMA can start this cycle.</param>
	__noinline void RunPendingDma(bool allowStartDma);

	/// <summary>
	/// Resets idle cycle counter and marks access as non-sequential.
	/// Called after DMA completion.
	/// </summary>
	/// <param name="mode">Access mode to modify.</param>
	__forceinline void ResetIdleCounter(GbaAccessModeVal& mode) {
		if (_idleCycleCounter) {
			// Next access immediately after DMA should always be non-sequential
			mode &= ~GbaAccessMode::Sequential;
		}
		_idleCycleCounter = 0;
	}

	/// <summary>Resets idle cycle counter.</summary>
	__forceinline void ResetIdleCounter() {
		_idleCycleCounter = 0;
	}

	/// <summary>Checks if parallel execution is possible during DMA.</summary>
	/// <returns>True if parallel access is allowed.</returns>
	bool CanRunInParallelWithDma();

	/// <summary>Reads from a DMA control register.</summary>
	/// <param name="addr">Register address.</param>
	/// <returns>Register value.</returns>
	uint8_t ReadRegister(uint32_t addr);

	/// <summary>Writes to a DMA control register.</summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRegister(uint32_t addr, uint8_t value);

	/// <summary>Serializes DMA state for save states.</summary>
	void Serialize(Serializer& s) override;
};
