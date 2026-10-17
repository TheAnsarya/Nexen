#pragma once
#include "pch.h"
#include "NES/INesMemoryHandler.h"
#include "Utilities/Serializer.h"

/// <summary>
/// Handles open bus behavior for unmapped NES addresses.
/// </summary>
/// <remarks>
/// **Open Bus Behavior:**
/// When the NES CPU reads from an address with no hardware responding,
/// it reads the last value that was on the data bus. This is called
/// "open bus" or "floating bus" behavior.
///
/// **Two Bus Values:**
/// - External bus: Value from external memory/device reads
/// - Internal bus: Value from internal CPU operations
///
/// **Special Case - $4015:**
/// Reading the APU status register ($4015) updates only the internal
/// bus, not the external bus. This is important for accurate emulation
/// of some games that rely on open bus behavior.
///
/// **Games Using Open Bus:**
/// Some games intentionally read unmapped addresses expecting the
/// last bus value, often for copy protection or strange effects.
/// Accurate open bus emulation is required for these games.
///
/// **Debugger Behavior:**
/// PeekRam() returns addr>>8 as a fake value to help identify
/// open bus reads in the debugger without affecting emulation.
/// </remarks>
class OpenBusHandler : public INesMemoryHandler, public ISerializable {
private:
	/// <summary>Last value on external data bus.</summary>
	uint8_t _externalOpenBus = 0;

	/// <summary>Last value on CPU internal data bus.</summary>
	uint8_t _internalOpenBus = 0;

public:
	OpenBusHandler() {
	}

	/// <summary>
	/// Reads open bus value (returns last external bus value).
	/// </summary>
	/// <param name="addr">Address being read (ignored).</param>
	/// <returns>Last external bus value.</returns>
	uint8_t ReadRam(uint16_t addr) override {
		return _externalOpenBus;
	}

	/// <summary>
	/// Peeks open bus (returns addr>>8 for debugger identification).
	/// </summary>
	/// <param name="addr">Address being peeked.</param>
	/// <returns>High byte of address (fake value for debugging).</returns>
	uint8_t PeekRam(uint16_t addr) override {
		return addr >> 8; // Fake open bus for debugger
	}

	/// <summary>Gets the external open bus value.</summary>
	/// <returns>Last external bus value.</returns>
	__forceinline uint8_t GetOpenBus() {
		return _externalOpenBus;
	}

	/// <summary>Gets the internal open bus value.</summary>
	/// <returns>Last internal bus value.</returns>
	__forceinline uint8_t GetInternalOpenBus() {
		return _internalOpenBus;
	}

	/// <summary>
	/// Sets the open bus value.
	/// </summary>
	/// <param name="value">New bus value.</param>
	/// <param name="setInternalOnly">If true, only update internal bus ($4015 special case).</param>
	__forceinline void SetOpenBus(uint8_t value, bool setInternalOnly) {
		// Reads to $4015 don't update the value on the external bus
		// Only the CPU's internal bus is updated
		if (!setInternalOnly) {
			_externalOpenBus = value;
		}
		_internalOpenBus = value;
	}

	/// <summary>
	/// Gets memory ranges (none - open bus handles unmapped addresses).
	/// </summary>
	/// <param name="ranges">Range collection (not modified).</param>
	void GetMemoryRanges(MemoryRanges& ranges) override {
	}

	/// <summary>
	/// Writes to open bus (no-op).
	/// </summary>
	/// <param name="addr">Address being written.</param>
	/// <param name="value">Value being written.</param>
	void WriteRam(uint16_t addr, uint8_t value) override {
	}

	/// <summary>Serializes open bus state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_internalOpenBus);
		SV(_externalOpenBus);
	}
};
