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
/// UART / ComLynx serial port (§1–§11 in lynx-uart-comlynx-hardware-reference.md):
///   The Lynx UART is a simple 11-bit serial transceiver (1 start + 8 data +
///   1 parity/mark + 1 stop) clocked by Timer 4 underflows (§5, §8). Each Timer 4
///   underflow advances internal TX/RX countdown counters by one bit-time.
///   After 11 ticks, a complete serial frame has been transmitted or received.
///
///   ComLynx (§11) is a shared open-collector bus — all transmitted data is
///   received by the sender (mandatory self-loopback) and any connected remote
///   units. Without a physical cable, the self-loopback still occurs, so games
///   that poll the serial port will see their own transmitted data echoed back.
///
///   Key behaviors:
///   - SERCTL ($FD8C) has DIFFERENT bit meanings on read (§3) vs write (§2)
///   - TX idle check uses bit 31 sentinel (UartTxInactive = 0x80000000)
///   - RX uses a 32-entry circular queue with overrun detection (§7)
///   - IRQ is level-sensitive (HW Bug §9.1): re-asserts every TickUart()
///   - Timer 4 does NOT set TimerDone / fire normal timer IRQ (§5)
///   - Break signal auto-retransmits as long as TXBRK bit is set (§6.2)
///   - Inter-byte RX gap of 55 ticks (11+44) between queued bytes (§7.3)
///   - TX loopback front-inserts into RX queue for priority (§7.2)
///
/// Performance notes:
///   TickUart() is called on every Timer 4 underflow (hot path when Timer 4
///   is running). The method is kept branchless-friendly: the common case
///   (TX inactive, RX inactive) hits two fast bit-test early-exits.
///
/// References:
///   - ~docs/plans/lynx-uart-comlynx-hardware-reference.md
///   - Epyx hardware reference: monlynx.de/lynx/hardware.html
///   - Handy emulator: bspruck/handy-fork mikie.cpp
///   - Mednafen/Beetle Lynx: libretro/beetle-lynx-libretro mikie.cpp
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

	/// <summary>Sentinel value for TX countdown: bit 31 set = transmitter idle.
	/// Using bit 31 as a flag allows a single bit-test to distinguish active vs idle,
	/// which is faster than comparing against a magic value in the hot path.
	/// See §6.3 — UART_TX_INACTIVE.</summary>
	static constexpr uint32_t UartTxInactive = 0x80000000;

	/// <summary>Sentinel value for RX countdown: bit 31 set = receiver idle.
	/// See §7.4 — UART_RX_INACTIVE.</summary>
	static constexpr uint32_t UartRxInactive = 0x80000000;

	/// <summary>Break code: bit 15 set in RX data indicates a break was received
	/// (sustained low on the serial line for an entire frame).
	/// See §7.4 — UART_BREAK_CODE, §8 — break = ≥24 bit periods low.</summary>
	static constexpr uint16_t UartBreakCode = 0x8000;

	/// <summary>Maximum RX queue depth (§7.4). Sized to handle burst scenarios where
	/// multiple bytes arrive before the game processes them. 32 is generous —
	/// real ComLynx traffic rarely exceeds a few bytes/frame.
	/// Must be a power of 2 for bitwise-AND modulo optimization.</summary>
	static constexpr int UartMaxRxQueue = 32;

	/// <summary>Timer 4 ticks per TX serial frame (§6.3, §8):
	/// 1 start + 8 data + 1 parity + 1 stop = 11 bit-times.</summary>
	static constexpr uint32_t UartTxTimePeriod = 11;

	/// <summary>Timer 4 ticks per RX serial frame (§7.4, §8):
	/// 1 start + 8 data + 1 parity + 1 stop = 11 bit-times.</summary>
	static constexpr uint32_t UartRxTimePeriod = 11;

	/// <summary>Inter-byte delay for queued RX data (§7.3, §7.4).
	/// After one byte is delivered from the RX queue, the next byte waits
	/// UartRxTimePeriod + UartRxNextDelay = 11 + 44 = 55 ticks before delivery.
	/// This simulates the physical wire delay between serial frames.
	/// Without this, a game could read the entire queue in one burst,
	/// which wouldn't match real hardware timing.</summary>
	static constexpr uint32_t UartRxNextDelay = 44;

	/// <summary>UART receive circular queue (§7.1, §7.4).
	/// Holds incoming serial data until delivered to UartRxData via countdown.
	/// Both external ComLynxRxData (back-insert) and loopback ComLynxTxLoopback
	/// (front-insert per §7.2) share this queue.</summary>
	uint16_t _uartRxQueue[UartMaxRxQueue] = {};

	/// <summary>Write pointer — next position for ComLynxRxData back-insertion (§7.1).</summary>
	uint32_t _uartRxInputPtr = 0;

	/// <summary>Read pointer — next position for delivery and front-insertion (§7.2, §7.3).</summary>
	uint32_t _uartRxOutputPtr = 0;

	/// <summary>Number of bytes waiting in RX queue for delivery.</summary>
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
	/// Called from TickTimer/CascadeTimer when Timer 4 underflows (§5).
	/// Hot path: ~62,500 calls/sec at default 9600 baud.
	/// The idle check (bit 31 test) ensures minimal overhead when no serial
	/// activity is occurring.
	///
	/// TX flow (§6.1): countdown 11→0 then idle or retransmit break (§6.2).
	/// RX flow (§7.3): countdown→0 delivers byte, sets inter-byte delay (55 ticks)
	/// or goes inactive when queue empties.</summary>
	void TickUart();

	/// <summary>Update Timer 4 IRQ line based on UART status (§9.1, §10).
	/// Level-sensitive (HW Bug §9.1): re-asserts IRQ every tick while
	/// condition persists, even if software already cleared the pending bit.
	/// Serial interrupt uses bit 4 ($10) in INTSET/INTRST registers.</summary>
	void UpdateUartIrq();

	/// <summary>Self-loopback: routes TX output to this unit's own RX queue (§7.2, §11).
	/// On real hardware, ComLynx is open-collector so the transmitter always
	/// sees its own output. Data is FRONT-inserted (§7.2) so loopback bytes
	/// are received before any externally-queued data, enabling collision
	/// detection on the ComLynx bus.
	/// Separation from ComLynxRxData() allows future multi-unit networking
	/// to call ComLynxRxData() on remote instances (back-insertion).</summary>
	void ComLynxTxLoopback(uint16_t data);

public:
	/// <summary>Initialize Mikey chip with emulator references.
	/// Resets all timers, palette, UART, and display state to power-on defaults (§13).</summary>
	void Init(Emulator* emu, LynxConsole* console, LynxCpu* cpu, LynxMemoryManager* memoryManager);

	/// <summary>Set APU reference (deferred initialization due to construction order).</summary>
	void SetApu(LynxApu* apu) { _apu = apu; }

	/// <summary>Set EEPROM reference (deferred initialization).</summary>
	void SetEeprom(LynxEeprom* eeprom) { _eeprom = eeprom; }

	/// <summary>Read a Mikey register.
	/// Handles timers ($FD00–$FD1F), audio ($FD20–$FD4F), interrupts ($FD80–$FD81),
	/// display ($FD92–$FD95), palette ($FDA0–$FDBF), serial ($FD8C–$FD8D),
	/// I/O ($FD88–$FD89), and hardware revision ($FD84).</summary>
	uint8_t ReadRegister(uint8_t addr);

	/// <summary>Write a Mikey register. Same address ranges as ReadRegister.
	/// Note: SERCTL ($FD8C) has different bit meanings on write (§2) vs read (§3).</summary>
	void WriteRegister(uint8_t addr, uint8_t value);

	/// <summary>Tick all 8 timers for the current cycle.
	/// Each timer runs at its configured prescaler rate. Timer 4 underflows
	/// drive the UART via TickUart() (§5).</summary>
	void Tick(uint64_t currentCycle);

	/// <summary>Set an IRQ source bit in the pending register and update the CPU IRQ line.</summary>
	void SetIrqSource(LynxIrqSource::LynxIrqSource source);

	/// <summary>Clear an IRQ source bit from the pending register and update the CPU IRQ line.</summary>
	void ClearIrqSource(LynxIrqSource::LynxIrqSource source);

	/// <summary>Check if any IRQ sources are pending.</summary>
	bool HasPendingIrq() const;

	/// <summary>Get the 160×102 ARGB frame buffer for display output.</summary>
	uint32_t* GetFrameBuffer() { return _frameBuffer; }

	/// <summary>Get mutable reference to Mikey state (for debugger/serialization).</summary>
	LynxMikeyState& GetState() { return _state; }

	/// <summary>Get current frame count from the console.</summary>
	uint32_t GetFrameCount() const;

	/// <summary>Inject received data into the UART RX queue (§7.1) for ComLynx networking.
	/// Back-inserts at the tail of the circular queue. Used by external/remote
	/// Lynx units to deliver data over the ComLynx bus.</summary>
	void ComLynxRxData(uint16_t data);

	/// <summary>Serialize/deserialize all Mikey state for save states.
	/// Includes timers, IRQ, display, palette, UART, RX queue, and I/O.</summary>
	void Serialize(Serializer& s) override;
};
