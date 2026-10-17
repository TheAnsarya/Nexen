#pragma once
#include "pch.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryType.h"
#include "PCE/PceTypes.h"
#include "PCE/PceConstants.h"
#include "Utilities/ISerializable.h"

class PceConsole;
class PceVce;
class PceVpc;

/// <summary>VDC horizontal display state machine states.</summary>
enum class PceVdcModeH {
	Hds,  ///< Horizontal Display Start (left border/blanking)
	Hdw,  ///< Horizontal Display Width (active display)
	Hde,  ///< Horizontal Display End (right border)
	Hsw,  ///< Horizontal Sync Width (H-sync period)
};

/// <summary>VDC vertical display state machine states.</summary>
enum class PceVdcModeV {
	Vds,  ///< Vertical Display Start (top border)
	Vdw,  ///< Vertical Display Width (active display)
	Vde,  ///< Vertical Display End (bottom border)
	Vsw,  ///< Vertical Sync Width (V-sync period)
};

/// <summary>VDC internal event types for state machine.</summary>
enum class PceVdcEvent {
	None,           ///< No pending event
	LatchScrollY,   ///< Latch vertical scroll value
	LatchScrollX,   ///< Latch horizontal scroll value
	HdsIrqTrigger,  ///< Trigger H-blank interrupt
	IncRcrCounter,  ///< Increment raster counter
};

/// <summary>Background tile rendering data.</summary>
struct PceTileInfo {
	uint16_t TileData[2];  ///< Tile pattern data (2 planes)
	uint16_t TileAddr;     ///< VRAM address of tile
	uint8_t Palette;       ///< Palette index (0-15)
};

/// <summary>Sprite rendering data.</summary>
struct PceSpriteInfo {
	uint16_t TileData[4];     ///< Sprite pattern data (4 planes)
	int16_t X;                ///< Screen X position
	uint16_t TileAddress;     ///< VRAM address of sprite tile
	uint8_t Index;            ///< Sprite index (0-63)
	uint8_t Palette;          ///< Palette index (0-15)
	bool HorizontalMirroring; ///< Flip sprite horizontally
	bool ForegroundPriority;  ///< Draw in front of BG
	bool LoadSp23;            ///< Load sprite planes 2-3
};

/// <summary>
/// PC Engine Video Display Controller (HuC6270).
/// Handles all background and sprite rendering for the PC Engine/TurboGrafx-16.
/// </summary>
/// <remarks>
/// The VDC is responsible for:
/// - Background tilemap rendering with scrolling
/// - 64 sprites (16 per scanline limit)
/// - VRAM-to-VRAM DMA and SATB (Sprite Attribute Table Block) transfers
/// - Raster interrupts for H-blank effects
///
/// **Display Characteristics:**
/// - Base resolution: 256×240 (NTSC), variable width
/// - Configurable display timing (HSW, HDS, HDW, HDE)
/// - Configurable display height via VSW, VDS, VDW, VDE
///
/// **Background Layer:**
/// - Configurable BAT (Background Attribute Table) size: 32/64/128×32/64
/// - 8×8 pixel tiles
/// - 16 palettes × 16 colors
///
/// **Sprites:**
/// - 64 sprites total, 16 per scanline limit
/// - Sizes: 16×16, 16×32, 16×64, 32×16, 32×32, 32×64
/// - Collision detection (sprite 0)
/// - Priority control (front/back of BG)
///
/// **DMA Features:**
/// - VRAM-to-VRAM DMA for fast data copies
/// - Automatic SATB transfer from VRAM to sprite RAM
///
/// **Interrupts:**
/// - Vertical blank (after last visible line)
/// - Horizontal blank (RCR register match)
/// - Sprite overflow (more than 16 sprites on line)
/// - Sprite collision (sprite 0 overlap)
/// - VRAM-to-VRAM DMA complete
/// - SATB transfer complete
///
/// **SuperGrafx:**
/// - Two VDC chips for dual-layer display
/// - VPC (Video Priority Controller) combines output
/// </remarks>
class PceVdc final : public ISerializable {
private:
	/// <summary>VDC register and rendering state.</summary>
	PceVdcState _state = {};

	/// <summary>Emulator instance for frame output.</summary>
	Emulator* _emu = nullptr;
	PceConsole* _console = nullptr;
	PceVce* _vce = nullptr;
	PceVpc* _vpc = nullptr;
	uint16_t* _vram = nullptr;
	uint16_t* _spriteRam = nullptr;

	uint16_t _rowBuffer[PceConstants::MaxScreenWidth] = {};

	uint16_t _vramOpenBus = 0;

	uint16_t _lastDrawHClock = 0;
	uint16_t _xStart = 0;

	PceVdcModeH _hMode = PceVdcModeH::Hds;
	int16_t _hModeCounter = 0;

	PceVdcModeV _vMode = PceVdcModeV::Vds;
	int16_t _vModeCounter = 0;

	uint16_t _screenOffsetX = 0;
	bool _needRcrIncrement = false;
	bool _needVCounterClock = false;
	bool _needVertBlankIrq = false;
	bool _verticalBlankDone = false;

	uint16_t _latchClockY = UINT16_MAX;
	uint16_t _latchClockX = UINT16_MAX;

	uint8_t _spriteCount = 0;
	uint16_t _spriteRow = 0;
	uint16_t _evalStartCycle = 0;
	uint16_t _evalEndCycle = 0;
	int16_t _evalLastCycle = 0;
	bool _hasSpriteOverflow = false;

	uint16_t _loadBgStart = 0;
	uint16_t _loadBgEnd = 0;
	int16_t _loadBgLastCycle = 0;
	uint8_t _tileCount = 0;
	bool _allowVramAccess = false;

	bool _pendingMemoryRead = false;
	bool _pendingMemoryWrite = false;
	int8_t _transferDelay = 0;

	bool _vramDmaRunning = false;
	bool _vramDmaReadCycle = false;
	uint16_t _vramDmaBuffer = 0;
	uint16_t _vramDmaPendingCycles = 0;

	PceVdcEvent _nextEvent = PceVdcEvent::None;
	uint16_t _nextEventCounter = 0;
	uint64_t _hSyncStartClock = 0;
	bool _allowDma = true;

	bool _isVdc2 = false;
	MemoryType _vramType = MemoryType::PceVideoRam;
	MemoryType _spriteRamType = MemoryType::PceSpriteRam;

	bool _xPosHasSprite[1024 + 32] = {};
	uint8_t _drawSpriteCount = 0;
	uint8_t _totalSpriteCount = 0;
	bool _rowHasSprite0 = false;
	uint16_t _loadSpriteStart = 0;
	PceSpriteInfo _sprites[64] = {};
	PceSpriteInfo _drawSprites[64] = {};
	PceTileInfo _tiles[100] = {};

	template <uint16_t bitMask = 0xFFFF>
	void UpdateReg(uint16_t& reg, uint8_t value, bool msb) {
		if (msb) {
			reg = ((reg & 0xFF) | (value << 8)) & bitMask;
		} else {
			reg = ((reg & 0xFF00) | value) & bitMask;
		}
	}

	void ProcessVramRead();
	void ProcessVramWrite();
	__noinline void ProcessVramAccesses();

	void QueueMemoryRead();
	void QueueMemoryWrite();
	void WaitForVramAccess();

	uint8_t GetClockDivider();
	uint16_t GetScanlineCount();
	uint16_t DotsToClocks(int dots);
	void TriggerHdsIrqs();

	__noinline void IncrementRcrCounter();
	__noinline void IncScrollY();
	__noinline void ProcessEndOfScanline();

	void TriggerDmaStart();
	__noinline void TriggerVerticalBlank();
	__noinline void ProcessSatbTransfer();
	__noinline void ProcessVramDmaTransfer();
	__noinline void SetVertMode(PceVdcModeV vMode);
	__noinline void SetHorizontalMode(PceVdcModeH hMode);
	void ClockVCounter();

	__noinline void ProcessVdcEvents();
	__noinline void ProcessEvent();

	__noinline void ProcessHorizontalSyncStart();
	__noinline void ProcessVerticalSyncStart();

	__forceinline uint8_t GetTilePixelColor(const uint16_t chr0, const uint16_t chr1, const uint8_t shift);
	__forceinline uint8_t GetSpritePixelColor(const uint16_t chrData[4], const uint8_t shift);

	__forceinline void ProcessSpriteEvaluation();
	__noinline void LoadSpriteTiles();

	bool IsDmaAllowed();

	template <bool skipRender>
	__forceinline void LoadBackgroundTiles();

	__noinline void LoadBackgroundTilesWidth2(uint16_t end, uint16_t scrollOffset, uint16_t columnMask, uint16_t row);
	__noinline void LoadBackgroundTilesWidth4(uint16_t end, uint16_t scrollOffset, uint16_t columnMask, uint16_t row);

	__forceinline void LoadBatEntry(uint16_t scrollOffset, uint16_t columnMask, uint16_t row);
	__forceinline void LoadTileDataCg0(uint16_t row);
	__forceinline void LoadTileDataCg1(uint16_t row);

	__forceinline uint16_t ReadVram(uint16_t addr);

	template <bool hasSprites, bool hasSprite0, bool skipRender>
	__forceinline void InternalDrawScanline();

	bool CheckUpdateLatchTiming(uint16_t clock);

public:
	PceVdc(Emulator* emu, PceConsole* console, PceVpc* vpc, PceVce* vce, bool isVdc2);
	~PceVdc();

	PceVdcState& GetState();

	[[nodiscard]] uint16_t GetHClock() { return _state.HClock; }
	[[nodiscard]] uint16_t GetScanline() { return _state.Scanline; }
	uint16_t* GetRowBuffer() { return _rowBuffer; }
	[[nodiscard]] uint16_t GetFrameCount() { return _state.FrameCount; }

	void Exec();
	void DrawScanline();

	uint8_t ReadRegister(uint16_t addr);
	void WriteRegister(uint16_t addr, uint8_t value);

	void Serialize(Serializer& s) override;
};
