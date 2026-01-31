#pragma once
#include "pch.h"
#include <memory>
#include <array>
#include "SMS/SmsTypes.h"
#include "Shared/SettingTypes.h"
#include "Shared/ColorUtilities.h"
#include "Utilities/ISerializable.h"

class Emulator;
class SmsConsole;
class SmsCpu;
class SmsControlManager;
class SmsMemoryManager;

/// <summary>VDP memory access types for debugging.</summary>
enum class SmsVdpMemAccess : uint8_t {
	None = 0,            ///< No access
	BgLoadTable = 1,     ///< Loading name table
	BgLoadTile = 2,      ///< Loading tile patterns
	SpriteEval = 3,      ///< Sprite evaluation
	SpriteLoadTable = 4, ///< Loading sprite table
	SpriteLoadTile = 5,  ///< Loading sprite tiles
	CpuSlot = 6          ///< CPU access slot
};

/// <summary>
/// Sega Master System Video Display Processor (VDP) - TMS9918A derivative.
/// Handles all video output for SMS, Game Gear, SG-1000, and ColecoVision.
/// </summary>
/// <remarks>
/// The SMS VDP is derived from the TMS9918A with enhancements:
/// - 256×192 resolution (Mode 4) or legacy TMS modes
/// - 64 sprites, 8 per scanline (or 4 in legacy modes)
/// - 32-color palette (64 in Game Gear mode)
/// - Hardware scrolling (per-line for columns 24-31)
/// - Sprite collision detection
///
/// **Display Modes:**
/// - Mode 4: SMS native mode (8×8 tiles, 16KB VRAM)
/// - Mode 0: Graphics I (TMS9918 compatible)
/// - Mode 1: Text (40 columns)
/// - Mode 2: Graphics II (pattern table mirroring)
/// - Mode 3: Multicolor
///
/// **Memory:**
/// - 16KB Video RAM
/// - 32 bytes CRAM (Color RAM) - SMS palette
/// - 64 bytes CRAM (Color RAM) - Game Gear palette
///
/// **Game Gear Differences:**
/// - 160×144 visible resolution (centered in 256×192)
/// - 12-bit color (4096 colors, vs SMS 6-bit)
/// - Higher resolution CRAM (32 words vs 32 bytes)
///
/// **Sprite Attributes:**
/// - Y position (offset by 1)
/// - X position (can be offset -8 via register bit)
/// - Tile index
/// - Attribute byte (palette, priority - Mode 4 only)
///
/// **Interrupts:**
/// - Vertical blank (line 192/193)
/// - Line counter (programmable scanline IRQ)
/// </remarks>
class SmsVdp final : public ISerializable {
public:
	/// <summary>Left border offset in pixels.</summary>
	static constexpr int SmsVdpLeftBorder = 8;

private:
	/// <summary>Emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Console instance.</summary>
	SmsConsole* _console = nullptr;

	/// <summary>CPU for interrupt signaling.</summary>
	SmsCpu* _cpu = nullptr;

	/// <summary>Controller manager for pause button.</summary>
	SmsControlManager* _controlManager = nullptr;

	/// <summary>Memory manager for timing.</summary>
	SmsMemoryManager* _memoryManager = nullptr;

	/// <summary>16KB Video RAM.</summary>
	std::unique_ptr<uint8_t[]> _videoRam;

	/// <summary>Internal palette RAM (expanded to RGB555).</summary>
	uint16_t _internalPaletteRam[0x20] = {};

	/// <summary>SMS/SG palette (fixed TMS9918-style colors).</summary>
	uint16_t _smsSgPalette[0x10] = {
	    ColorUtilities::Rgb222To555(0x00),
	    ColorUtilities::Rgb222To555(0x00),
	    ColorUtilities::Rgb222To555(0x08),
	    ColorUtilities::Rgb222To555(0x0C),
	    ColorUtilities::Rgb222To555(0x10),
	    ColorUtilities::Rgb222To555(0x30),
	    ColorUtilities::Rgb222To555(0x01),
	    ColorUtilities::Rgb222To555(0x3C),
	    ColorUtilities::Rgb222To555(0x02),
	    ColorUtilities::Rgb222To555(0x03),
	    ColorUtilities::Rgb222To555(0x05),
	    ColorUtilities::Rgb222To555(0x0F),
	    ColorUtilities::Rgb222To555(0x04),
	    ColorUtilities::Rgb222To555(0x33),
	    ColorUtilities::Rgb222To555(0x15),
	    ColorUtilities::Rgb222To555(0x3F),
	};

	/// <summary>Original SG-1000 palette values.</summary>
	static constexpr uint16_t _originalSgPalette[0x10] = {
	    0x0000, 0x0000, 0x2324, 0x3f6b, 0x754a, 0x7dcf, 0x255a, 0x7ba8,
	    0x295f, 0x3dff, 0x2b1a, 0x433c, 0x1ec4, 0x5d79, 0x6739, 0x7fff};

	/// <summary>Active SG palette pointer.</summary>
	const uint16_t* _activeSgPalette = nullptr;

	/// <summary>Debug flag: disable background layer.</summary>
	bool _disableBackground = false;

	/// <summary>Debug flag: disable sprites.</summary>
	bool _disableSprites = false;

	/// <summary>Debug flag: remove 8-sprite-per-line limit.</summary>
	bool _removeSpriteLimit = false;

	/// <summary>Current console model.</summary>
	SmsModel _model = {};

	/// <summary>Console hardware revision.</summary>
	SmsRevision _revision = {};

	/// <summary>Double-buffered frame output.</summary>
	std::array<std::unique_ptr<uint16_t[]>, 2> _outputBuffers;

	/// <summary>Current output buffer pointer.</summary>
	uint16_t* _currentOutputBuffer = nullptr;

	/// <summary>VDP register state.</summary>
	SmsVdpState _state = {};

	/// <summary>Last master clock for timing.</summary>
	uint64_t _lastMasterClock = 0;

	// Background rendering state
	/// <summary>Background pixel shifters.</summary>
	uint32_t _bgShifters[4] = {};

	/// <summary>Background priority bits.</summary>
	uint32_t _bgPriority = 0;

	/// <summary>Background palette bits.</summary>
	uint32_t _bgPalette = 0;

	/// <summary>Current background tile address.</summary>
	uint16_t _bgTileAddr = 0;

	/// <summary>Background vertical offset.</summary>
	uint16_t _bgOffsetY = 0;

	/// <summary>Minimum cycle for drawing.</summary>
	uint16_t _minDrawCycle = 0;

	/// <summary>Available pixels to render.</summary>
	uint8_t _pixelsAvailable = 0;

	/// <summary>Background horizontal mirror flag.</summary>
	bool _bgHorizontalMirror = false;

	/// <summary>Sprite shift register data.</summary>
	struct SpriteShifter {
		uint8_t TileData[4] = {};  ///< Tile pattern data
		uint16_t TileAddr = 0;     ///< Tile address in VRAM
		int16_t SpriteX = 0;       ///< X position
		uint8_t SpriteRow = 0;     ///< Row within sprite
		bool HardwareSprite = false; ///< Valid sprite flag
	};

	// Sprite evaluation state
	/// <summary>Evaluation counter.</summary>
	uint8_t _evalCounter = 0;

	/// <summary>Number of sprites in range.</summary>
	uint8_t _inRangeSpriteCount = 0;

	/// <summary>Sprite overflow pending flag.</summary>
	bool _spriteOverflowPending = false;

	/// <summary>Current sprite index being evaluated.</summary>
	uint8_t _spriteIndex = 0;

	/// <summary>In-range sprite evaluation index.</summary>
	uint8_t _inRangeSpriteIndex = 0;

	/// <summary>Total sprite count.</summary>
	uint8_t _spriteCount = 0;

	/// <summary>Indices of sprites in Y range.</summary>
	uint8_t _inRangeSprites[64] = {};

	/// <summary>Sprite shifter data for rendering.</summary>
	SpriteShifter _spriteShifters[64];

	/// <summary>Color RAM (6-bit SMS or raw GG).</summary>
	uint8_t _paletteRam[0x40] = {};

	/// <summary>Scanlines per frame (262 NTSC, 313 PAL).</summary>
	uint16_t _scanlineCount = 262;

	/// <summary>Current region.</summary>
	ConsoleRegion _region = ConsoleRegion::Ntsc;

	/// <summary>Pending write type.</summary>
	SmsVdpWriteType _writePending = SmsVdpWriteType::None;

	/// <summary>Pending VRAM read flag.</summary>
	bool _readPending = false;

	/// <summary>H-counter latch request pending.</summary>
	bool _latchRequest = false;

	/// <summary>H-counter latch position.</summary>
	uint8_t _latchPos = 0;

	/// <summary>CRAM dot artifact pending.</summary>
	bool _needCramDot = false;

	/// <summary>CRAM dot artifact color.</summary>
	uint16_t _cramDotColor = 0;

	// SG-1000 mode state
	/// <summary>Background tile index (legacy modes).</summary>
	uint16_t _bgTileIndex = 0;

	/// <summary>Background pattern data (legacy modes).</summary>
	uint8_t _bgPatternData = 0;

	/// <summary>Text mode rendering step.</summary>
	uint8_t _textModeStep = 0;

	/// <summary>Memory access schedule for debugging.</summary>
	SmsVdpMemAccess _memAccess[342] = {};

	/// <summary>Updates IRQ line state.</summary>
	void UpdateIrqState();

	/// <summary>Updates display mode from registers.</summary>
	void UpdateDisplayMode();

	/// <summary>Reads V-counter value.</summary>
	uint8_t ReadVerticalCounter();

	/// <summary>Reads VRAM at address.</summary>
	__forceinline uint8_t ReadVram(uint16_t addr, SmsVdpMemAccess type);

	/// <summary>Writes VRAM at address.</summary>
	__forceinline void WriteVram(uint16_t addr, uint8_t value, SmsVdpMemAccess type);

	/// <summary>Debug: processes memory access view.</summary>
	void DebugProcessMemoryAccessView();

	/// <summary>Processes VRAM access slot.</summary>
	__forceinline void ProcessVramAccess();

	/// <summary>Processes pending VRAM write.</summary>
	void ProcessVramWrite();

	/// <summary>Reverses bit order of byte.</summary>
	uint8_t ReverseBitOrder(uint8_t val);

	/// <summary>Main execution step.</summary>
	__forceinline void Exec();

	/// <summary>Execution during forced blank.</summary>
	__forceinline void ExecForcedBlank();

	/// <summary>Processes VBlank during forced blank.</summary>
	__forceinline void ProcessForcedBlankVblank();

	/// <summary>Gets visible pixel index.</summary>
	int GetVisiblePixelIndex();

	/// <summary>Loads background tiles (Mode 4).</summary>
	__forceinline void LoadBgTilesSms();

	/// <summary>Loads background tiles (legacy modes).</summary>
	void LoadBgTilesSg();

	/// <summary>Loads tiles in text mode.</summary>
	void LoadBgTilesSgTextMode();

	/// <summary>Outputs background pixel.</summary>
	void PushBgPixel(uint8_t color, int index);

	/// <summary>Draws current pixel to output.</summary>
	__forceinline void DrawPixel();

	/// <summary>Processes scanline timing events.</summary>
	void ProcessScanlineEvents();

	/// <summary>Processes end of scanline.</summary>
	void ProcessEndOfScanline();

	/// <summary>Evaluates sprites for current scanline.</summary>
	__forceinline void ProcessSpriteEvaluation();

	/// <summary>Gets sprite tile address (Mode 4).</summary>
	uint16_t GetSmsSpriteTileAddr(uint8_t sprTileIndex, uint8_t spriteRow, uint8_t i);

	/// <summary>Loads sprite tiles (Mode 4).</summary>
	void LoadSpriteTilesSms();

	/// <summary>Loads additional sprites beyond 8 limit.</summary>
	void LoadExtraSpritesSms();

	/// <summary>Gets final pixel color.</summary>
	__forceinline uint16_t GetPixelColor();

	/// <summary>Loads sprite tiles (legacy modes).</summary>
	void LoadSpriteTilesSg();

	/// <summary>Loads extra sprites (legacy modes).</summary>
	void LoadExtraSpritesSg();

	/// <summary>Shifts sprite data for rendering.</summary>
	void ShiftSprite(uint8_t sprIndex);

	/// <summary>Shifts sprite data (legacy modes).</summary>
	void ShiftSpriteSg(uint8_t sprIndex);

	/// <summary>Checks if zoomed sprite rendering is allowed.</summary>
	__forceinline bool IsZoomedSpriteAllowed(int spriteIndex);

	/// <summary>Writes to VDP register.</summary>
	void WriteRegister(uint8_t reg, uint8_t value);

	/// <summary>Writes to SMS palette RAM.</summary>
	void WriteSmsPalette(uint8_t addr, uint8_t value);

	/// <summary>Writes to Game Gear palette RAM.</summary>
	void WriteGameGearPalette(uint8_t addr, uint16_t value);

	/// <summary>Initializes SMS post-BIOS state.</summary>
	void InitSmsPostBiosState();

	/// <summary>Initializes Game Gear power-on state.</summary>
	void InitGgPowerOnState();

	/// <summary>Updates VDP configuration.</summary>
	void UpdateConfig();

public:
	/// <summary>Initializes VDP with all dependencies.</summary>
	void Init(Emulator* emu, SmsConsole* console, SmsCpu* cpu, SmsControlManager* controlManager, SmsMemoryManager* memoryManager);
	~SmsVdp();

	/// <summary>Runs VDP until specified master clock.</summary>
	void Run(uint64_t runTo);

	/// <summary>Writes to VDP I/O port.</summary>
	void WritePort(uint8_t port, uint8_t value);

	/// <summary>Reads from VDP I/O port.</summary>
	uint8_t ReadPort(uint8_t port);

	/// <summary>Peeks VDP port (no side effects).</summary>
	uint8_t PeekPort(uint8_t port);

	/// <summary>Requests H-counter latch at specified X.</summary>
	void SetLocationLatchRequest(uint8_t x);

	/// <summary>Internal H-counter latch at cycle.</summary>
	void InternalLatchHorizontalCounter(uint16_t cycle);

	/// <summary>Latches H-counter now.</summary>
	void LatchHorizontalCounter();

	/// <summary>Sets console region (NTSC/PAL).</summary>
	void SetRegion(ConsoleRegion region);

	/// <summary>Debug: forces frame output.</summary>
	void DebugSendFrame();

	/// <summary>Gets current scanline.</summary>
	[[nodiscard]] uint16_t GetScanline() { return _state.Scanline; }

	/// <summary>Gets total scanline count.</summary>
	[[nodiscard]] uint16_t GetScanlineCount() { return _scanlineCount; }

	/// <summary>Gets current cycle within scanline.</summary>
	[[nodiscard]] uint16_t GetCycle() { return _state.Cycle; }

	/// <summary>Gets current frame count.</summary>
	[[nodiscard]] uint16_t GetFrameCount() { return _state.FrameCount; }

	/// <summary>Gets pixel brightness at coordinates.</summary>
	uint32_t GetPixelBrightness(uint8_t x, uint8_t y);

	/// <summary>Gets viewport Y offset for GG.</summary>
	int GetViewportYOffset();

	/// <summary>Gets active SG-1000 palette.</summary>
	const uint16_t* GetSmsSgPalette() { return _activeSgPalette; }

	/// <summary>Gets VDP state reference.</summary>
	SmsVdpState& GetState() { return _state; }

	/// <summary>Debug: writes palette value.</summary>
	void DebugWritePalette(uint8_t addr, uint8_t value);

	/// <summary>Gets screen buffer (current or previous).</summary>
	uint16_t* GetScreenBuffer(bool previousBuffer) {
		return previousBuffer ? ((_currentOutputBuffer == _outputBuffers[0].get()) ? _outputBuffers[1].get() : _outputBuffers[0].get()) : _currentOutputBuffer;
	}

	/// <summary>Serializes VDP state for save states.</summary>
	void Serialize(Serializer& s) override;
};
