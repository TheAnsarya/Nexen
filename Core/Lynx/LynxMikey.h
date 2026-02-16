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

	void Serialize(Serializer& s) override;
};
