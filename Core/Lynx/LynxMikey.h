#pragma once
#include "pch.h"
#include "Lynx/LynxTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class LynxConsole;
class LynxCpu;
class LynxMemoryManager;
class LynxCart;
class LynxApu;
class LynxEeprom;

/// <summary>
/// Mikey chip — timers, audio, display DMA, interrupts, and UART/ComLynx.
///
/// UART / ComLynx serial port:
///   The Lynx UART is a simple 11-bit serial transceiver (1 start + 8 data +
///   1 parity/mark + 1 stop) clocked by Timer 4 underflows. Each Timer 4
///   underflow advances internal TX/RX countdown counters by one bit-time.
///   After 11 ticks, a complete serial frame has been transmitted or received.
///
///   ComLynx is a shared open-collector bus — all transmitted data is received
///   by the sender (mandatory self-loopback) and any connected remote units.
///   Without a physical cable, the self-loopback still occurs, so games that
///   poll the serial port will see their own transmitted data echoed back.
///
///   Key behaviors:
///   - SERCTL ($FD8C) has DIFFERENT bit meanings on read vs write
///   - TX idle check uses bit 31 sentinel (UartTxInactive = 0x80000000)
///   - RX uses a 32-entry circular queue with overrun detection
///   - IRQ is level-sensitive (HW Bug 13.2): re-asserts every TickUart()
///   - Timer 4 does NOT set TimerDone / fire normal timer IRQ
///   - Break signal auto-retransmits as long as TXBRK bit is set
///   - Inter-byte RX gap of 44 ticks (4x frame period) between queued bytes
///
/// Performance notes:
///   TickUart() is called on every Timer 4 underflow (hot path when Timer 4
///   is running). The method is kept branchless-friendly: the common case
///   (TX inactive, RX inactive) hits two fast bit-test early-exits.
/// </summary>
class LynxMikey final : public ISerializable {
private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;
	LynxCpu* _cpu = nullptr;
	LynxMemoryManager* _memoryManager = nullptr;
	LynxCart* _cart = nullptr;
	LynxApu* _apu = nullptr;
	LynxEeprom* _eeprom = nullptr;

	LynxMikeyState _state = {};

	// I/O direction and data registers for EEPROM/misc I/O
	uint8_t _ioDir = 0;   // IODIR ($FD88) — direction (1=output, 0=input)
	uint8_t _ioData = 0;  // IODAT ($FD89) — data

	// Frame buffer output (160x102 pixels, 32-bit ARGB)
	uint32_t _frameBuffer[LynxConstants::ScreenWidth * LynxConstants::ScreenHeight] = {};

	// ======================================================================
	// UART / ComLynx constants
	// ======================================================================

	/// <summary>Sentinel value for TX/RX countdown: bit 31 set = transmitter/receiver idle.
	/// Using bit 31 as a flag allows a single bit-test to distinguish active vs idle,
	/// which is faster than comparing against a magic value in the hot path.</summary>
	static constexpr uint32_t UartTxInactive = 0x80000000;
	static constexpr uint32_t UartRxInactive = 0x80000000;

	/// <summary>Break code: bit 15 set in RX data indicates a break was received
	/// (sustained low on the serial line for an entire frame).</summary>
	static constexpr uint16_t UartBreakCode = 0x8000;

	/// <summary>Maximum RX queue depth. Sized to handle burst scenarios where
	/// multiple bytes arrive before the game processes them. 32 is generous —
	/// real ComLynx traffic rarely exceeds a few bytes/frame.</summary>
	static constexpr int UartMaxRxQueue = 32;

	/// <summary>Timer 4 ticks per serial frame: 1 start + 8 data + 1 parity + 1 stop = 11.</summary>
	static constexpr uint32_t UartTxTimePeriod = 11;
	static constexpr uint32_t UartRxTimePeriod = 11;

	/// <summary>Inter-byte delay for queued RX data (44 = 4× frame period).
	/// After one byte is delivered from the RX queue, this delay simulates
	/// the physical wire delay before the next byte becomes available.
	/// Without this, a game could read the entire queue in one burst,
	/// which wouldn't match real hardware timing.</summary>
	static constexpr uint32_t UartRxNextDelay = 44;

	// UART receive queue
	uint16_t _uartRxQueue[UartMaxRxQueue] = {};
	uint32_t _uartRxInputPtr = 0;
	uint32_t _uartRxOutputPtr = 0;
	uint32_t _uartRxWaiting = 0;

	// Timer linking chains:
	// Chain 1: Timer 0 (H) → Timer 2 (V) → Timer 4
	// Chain 2: Timer 1 → Timer 3 → Timer 5 → Timer 7
	// Timer 6: standalone (audio sample rate)
	static constexpr int _timerLinkTarget[8] = { 2, 3, 4, 5, -1, 7, -1, -1 };

	// Clock source prescaler periods (in master clock cycles)
	// Lynx master clock = 16 MHz. Timer clock sources:
	//   0 = 1 MHz (÷16), 1 = 500 kHz (÷32), 2 = 250 kHz (÷64),
	//   3 = 125 kHz (÷128), 4 = 62.5 kHz (÷256), 5 = 31.25 kHz (÷512),
	//   6 = 15.625 kHz (÷1024), 7 = linked (cascade from source timer)
	// Prescaler periods in CPU cycles (master clock / 4).
	// Master clock = 16 MHz, CPU clock = 4 MHz (1 CPU cycle = 4 master clocks).
	// Source 0 = 1 MHz (÷4 CPU), Source 1 = 500 kHz (÷8), etc.
	static constexpr uint32_t _prescalerPeriods[8] = { 4, 8, 16, 32, 64, 128, 256, 0 };

	// Timer register layout: 4 registers per timer
	// Timer 0-3 at $FD00-$FD0F, Timer 4-7 at $FD10-$FD1F
	// Offset 0: BACKUP, 1: CTLA, 2: COUNT, 3: CTLB
	__forceinline int GetTimerIndex(uint8_t addr) const;
	__forceinline int GetTimerRegOffset(uint8_t addr) const;

	void TickTimer(int index, uint64_t currentCycle);
	void CascadeTimer(int sourceIndex);
	void UpdatePalette(int index);
	void UpdateIrqLine();
	void RenderScanline();
	/// <summary>Advance UART TX/RX state by one Timer 4 tick (one bit-time).
	/// Called from TickTimer/CascadeTimer when Timer 4 underflows.
	/// Hot path: ~62,500 calls/sec at default 9600 baud.
	/// The idle check (bit 31 test) ensures minimal overhead when no serial
	/// activity is occurring.</summary>
	void TickUart();

	/// <summary>Update Timer 4 IRQ line based on UART status.
	/// Level-sensitive (HW Bug 13.2): re-asserts IRQ every tick while
	/// condition persists, even if software already cleared the pending bit.</summary>
	void UpdateUartIrq();

	/// <summary>Self-loopback: routes TX output to this unit's own RX queue.
	/// On real hardware, ComLynx is open-collector so the transmitter always
	/// sees its own output. Separation from ComLynxRxData() allows future
	/// multi-unit networking to call ComLynxRxData() on remote instances.</summary>
	void ComLynxTxLoopback(uint16_t data);

public:
	void Init(Emulator* emu, LynxConsole* console, LynxCpu* cpu, LynxMemoryManager* memoryManager);

	void SetApu(LynxApu* apu) { _apu = apu; }
	void SetEeprom(LynxEeprom* eeprom) { _eeprom = eeprom; }

	uint8_t ReadRegister(uint8_t addr);
	void WriteRegister(uint8_t addr, uint8_t value);

	void Tick(uint64_t currentCycle);

	void SetIrqSource(LynxIrqSource::LynxIrqSource source);
	void ClearIrqSource(LynxIrqSource::LynxIrqSource source);
	bool HasPendingIrq() const;

	uint32_t* GetFrameBuffer() { return _frameBuffer; }
	LynxMikeyState& GetState() { return _state; }
	uint32_t GetFrameCount() const;

	/// <summary>Inject received data into the UART RX queue (for ComLynx networking)</summary>
	void ComLynxRxData(uint16_t data);

	void Serialize(Serializer& s) override;
};
