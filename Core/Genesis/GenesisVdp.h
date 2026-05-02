#pragma once
#include "pch.h"
#include "Genesis/GenesisTypes.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "Utilities/ISerializable.h"

class Emulator;
class GenesisConsole;
class GenesisM68k;
class GenesisMemoryManager;

class GenesisVdp final : public ISerializable {
private:
	static constexpr uint32_t VramSize = 0x10000;   // 64KB VRAM
	static constexpr uint32_t CramSize = 128;       // 64 words = 128 bytes
	static constexpr uint32_t VsramSize = 80;       // 40 words = 80 bytes
	static constexpr uint32_t MaxScreenWidth = 320;
	static constexpr uint32_t MaxScreenHeight = 240; // PAL can be 240

	Emulator* _emu = nullptr;
	GenesisConsole* _console = nullptr;
	GenesisM68k* _cpu = nullptr;
	GenesisMemoryManager* _memoryManager = nullptr;

	GenesisVdpState _state = {};

	uint8_t _vram[VramSize] = {};
	uint16_t _cram[64] = {};     // Color RAM (64 entries, 9-bit BGR)
	uint16_t _vsram[40] = {};   // Vertical scroll RAM (40 entries)

	// Output frame buffers (double-buffered, RGB555)
	uint16_t _outputBuffers[2][MaxScreenWidth * MaxScreenHeight] = {};
	uint8_t _currentBuffer = 0;

	// Rendering state
	uint16_t _scanline = 0;
	uint16_t _hCounter = 0;
	uint16_t _currentLineCycleTarget = 488;
	uint8_t _lineCycleRemainder = 0;
	uint64_t _lastRunCycle = 0;

	// Screen dimensions based on mode
	uint16_t _screenWidth = 320;
	uint16_t _screenHeight = 224;
	uint16_t _totalLines = 262; // NTSC

	// FIFO emulation (simplified)
	bool _pendingControlWrite = false;
	uint16_t _firstControlWord = 0;
	uint8_t _accessMode = 0; // 0=VRAM read, 1=VRAM write, 3=CRAM write, 5=VSRAM write
	uint16_t _addressReg = 0;
	uint8_t _autoIncrement = 0;
	bool _displayEnabledLast = false;
	bool _displayDisabledLogged = false;
	uint32_t _lastFrameLog = 0;

	// DMA pacing state (latched when a DMA transfer starts)
	bool _dmaInitialized = false;
	uint8_t _dmaLatchedMode = 0;
	uint32_t _dmaRemainingWords = 0;
	uint32_t _dmaSourceAddress = 0;
	uint16_t _dmaCopySourceAddress = 0;
	uint8_t _dmaFillByte = 0;
	bool _dmaFillDataPending = false;
	uint8_t _dmaStartupDelayCyclesRemaining = 0;
	uint8_t _dmaBusCycleRemainder = 0;

	// Internal methods
	void ProcessScanline();
	void RenderScanline();
	void RenderBackground(uint16_t* lineBuffer, uint8_t planeIndex);
	void RenderSprites(uint16_t* lineBuffer);
	void OutputPixel(uint16_t x, uint16_t y, uint16_t color);
	uint16_t CramToRgb555(uint16_t cramColor);
	void ProcessDma();
	uint8_t GetDmaWordPeriodCycles() const;
	bool IsActiveDisplayExternalDmaSlot() const;

	// Register helpers
	bool IsDisplayEnabled() const { return (_state.Registers[1] & 0x40) != 0; }
	bool IsH40Mode() const { return (_state.Registers[12] & 0x01) != 0; } // H40 = 320px, H32 = 256px
	bool IsPalMode() const { return _totalLines == 313; }
	bool IsInterlaceMode() const { return (_state.Registers[12] & 0x02) != 0; }
	bool IsVBlankInterruptEnabled() const { return (_state.Registers[1] & 0x20) != 0; }
	bool IsHBlankInterruptEnabled() const { return (_state.Registers[0] & 0x10) != 0; }
	uint16_t GetPlaneWidth() const;
	uint16_t GetPlaneHeight() const;
	uint16_t GetPlaneABase() const { return (uint16_t)((_state.Registers[2] & 0x38) << 10); }
	uint16_t GetPlaneBBase() const { return (uint16_t)((_state.Registers[4] & 0x07) << 13); }
	uint16_t GetWindowBase() const { return (uint16_t)((_state.Registers[3] & 0x3E) << 10); }
	uint16_t GetSpriteTableBase() const { return (uint16_t)((_state.Registers[5] & 0x7F) << 9); }
	uint16_t GetHScrollBase() const { return (uint16_t)((_state.Registers[13] & 0x3F) << 10); }
	uint8_t GetBackgroundPaletteIndex() const { return (_state.Registers[7] >> 4) & 3; }
	uint8_t GetBackgroundColorIndex() const { return _state.Registers[7] & 0x0F; }

public:
	GenesisVdp();
	~GenesisVdp();

	void Init(Emulator* emu, GenesisConsole* console, GenesisM68k* cpu, GenesisMemoryManager* memoryManager);
	void Reset(bool hardReset);

	// Run VDP up to target master clock
	void Run(uint64_t targetCycle);

	// Port access
	uint16_t ReadDataPort();
	uint16_t ReadControlPort();
	uint16_t ReadHVCounter();
	void WriteDataPort(uint16_t value);
	void WriteControlPort(uint16_t value);
	void AcknowledgeInterrupt(uint8_t level);

	// Frame output
	uint16_t* GetScreenBuffer(bool previousBuffer) {
		return _outputBuffers[previousBuffer ? (_currentBuffer ^ 1) : _currentBuffer];
	}

	uint16_t GetScreenWidth() const { return _screenWidth; }
	uint16_t GetScreenHeight() const { return _screenHeight; }
	uint32_t GetFrameCount() const { return _state.FrameCount; }
	uint8_t* GetVramPointer() { return _vram; }
	uint16_t* GetCramPointer() { return _cram; }

	void SetRegion(bool pal);

	bool IsInVBlank() const { return _scanline >= _screenHeight; }

	GenesisVdpState GetState() const;

	void Serialize(Serializer& s) override;
};
