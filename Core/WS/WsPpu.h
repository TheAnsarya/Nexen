#pragma once
#include "pch.h"
#include <memory>
#include <array>
#include "WS/WsTypes.h"
#include "Shared/Emulator.h"
#include "Shared/SettingTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class WsTimer;
class WsConsole;
class WsMemoryManager;

/// <summary>
/// WonderSwan / WonderSwan Color PPU emulator.
/// Renders 224x144 display with hardware sprite and tile support.
/// </summary>
/// <remarks>
/// The WonderSwan PPU provides:
/// - 2 background layers (scrollable tilemaps)
/// - 128 sprites (up to 32 per scanline)
/// - Multiple video modes for different color depths
///
/// **Video Modes:**
/// - Monochrome: 8 shades of gray (4bpp palette → shade LUT)
/// - Color 2bpp: 4 colors per tile, 16 palettes
/// - Color 4bpp: 16 colors per tile, 16 palettes
/// - Color 4bpp Packed: Alternative 4bpp format
///
/// **Memory:**
/// - 16KB VRAM for tiles, tilemaps, and sprites
/// - Palette RAM at $FE00-$FFFF in VRAM
/// - Sprite table copied from VRAM each frame
///
/// **Display:**
/// - 224×144 visible area
/// - Original WS: Vertical orientation (rotated 90°)
/// - WS Color: Horizontal orientation
/// - 75.47 Hz refresh rate
///
/// **Features:**
/// - Hardware windows for BG/sprite masking
/// - Per-layer enable/disable
/// - LCD icons (battery, headphones, etc.)
/// </remarks>
class WsPpu final : public ISerializable {
private:
	/// <summary>PPU state (registers, mode, scanline).</summary>
	WsPpuState _state = {};

	/// <summary>Emulator instance for frame output.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Console instance for model detection.</summary>
	WsConsole* _console = nullptr;

	/// <summary>Memory manager for bus access.</summary>
	WsMemoryManager* _memoryManager = nullptr;

	/// <summary>Timer for HBlank/VBlank sync.</summary>
	WsTimer* _timer = nullptr;

	/// <summary>Double-buffered output.</summary>
	std::array<std::unique_ptr<uint16_t[]>, 2> _outputBuffers;

	/// <summary>Current frame buffer.</summary>
	uint16_t* _currentBuffer = nullptr;

	/// <summary>VRAM pointer (16KB).</summary>
	uint8_t* _vram = nullptr;

	/// <summary>Sprite attribute cache (copied from VRAM).</summary>
	uint8_t _spriteRam[512] = {};

	/// <summary>Per-pixel rendering data.</summary>
	struct PixelData {
		uint8_t Palette;   ///< Palette index
		uint8_t Color;     ///< Color within palette
		uint8_t Priority;  ///< Layer priority
	};

	/// <summary>Scanline pixel data for 2 layers.</summary>
	PixelData _rowData[2][224];

	/// <summary>Rendered screen height (varies by model).</summary>
	uint16_t _screenHeight = 0;

	/// <summary>Rendered screen width (varies by model).</summary>
	uint16_t _screenWidth = 0;

	/// <summary>Show LCD status icons.</summary>
	bool _showIcons = false;

	/// <summary>Processes end of scanline - advances to next line.</summary>
	void ProcessEndOfScanline();

	/// <summary>Copies sprite data from VRAM to sprite cache.</summary>
	void ProcessSpriteCopy();

	/// <summary>Draws LCD status icons (battery, volume, etc.).</summary>
	void DrawIcons();

	/// <summary>Draws a single LCD icon.</summary>
	void DrawIcon(bool visible, const uint16_t icon[11], uint8_t position);

	/// <summary>Gets LCD status for icon display.</summary>
	uint8_t GetLcdStatus();

	/// <summary>Sends completed frame to video output.</summary>
	void SendFrame();

	/// <summary>Draws one scanline in the specified video mode.</summary>
	template <WsVideoMode mode>
	void DrawScanline();

	/// <summary>Draws sprites for the current scanline.</summary>
	template <WsVideoMode mode>
	void DrawSprites();

	/// <summary>Draws a background layer for the current scanline.</summary>
	template <WsVideoMode mode, int layerIndex>
	void DrawBackground();

	/// <summary>Gets pixel color from tile data.</summary>
	template <WsVideoMode mode>
	__forceinline uint16_t GetPixelColor(uint16_t tileAddr, uint8_t column);

	/// <summary>Gets the current background color.</summary>
	__forceinline uint16_t GetBgColor() {
		if (_state.Mode == WsVideoMode::Monochrome) {
			// Monochrome: Use shade LUT
			uint8_t bgBrightness = _state.BwShades[_state.BgColor & 0x07] ^ 0x0F;
			return bgBrightness | (bgBrightness << 4) | (bgBrightness << 8);
		} else {
			// Color: Read from palette RAM
			return _vram[0xFE00 | (_state.BgColor << 1)] | ((_vram[0xFE00 | (_state.BgColor << 1) | 1] & 0x0F) << 8);
		}
	}

	/// <summary>Converts palette/color to RGB value.</summary>
	__forceinline uint16_t GetPixelRgbColor(WsVideoMode mode, uint8_t color, uint8_t palette) {
		switch (mode) {
			case WsVideoMode::Monochrome: {
				// Monochrome: Palette → Shade LUT → RGB
				uint8_t shadeValue = _state.BwPalettes[(palette << 2) | color];
				uint8_t brightness = _state.BwShades[shadeValue] ^ 0x0F;
				return brightness | (brightness << 4) | (brightness << 8);
			}

			case WsVideoMode::Color2bpp:
			case WsVideoMode::Color4bpp:
			case WsVideoMode::Color4bppPacked: {
				// Color: Direct palette RAM lookup
				uint16_t paletteAddr = 0xFE00 | (palette << 5) | (color << 1);
				return _vram[paletteAddr] | ((_vram[paletteAddr + 1] & 0x0F) << 8);
			}
		}

		return 0;
	}

	void ProcessHblank();

public:
	WsPpu(Emulator* emu, WsConsole* console, WsMemoryManager* memoryManager, WsTimer* timer, uint8_t* vram);
	~WsPpu();

	__forceinline void Exec() {
		if (_state.Scanline == WsConstants::ScreenHeight) {
			ProcessSpriteCopy();
		}

		if (_state.Cycle < 224) {
			if (_state.Scanline < WsConstants::ScreenHeight + 1 && _state.Scanline > 0) {
				// Palette lookup + output pixel on the first 224 cycles
				uint8_t rowIndex = (_state.Scanline & 0x01) ^ 1;
				PixelData& data = _rowData[rowIndex][_state.Cycle];
				uint16_t screenWidth = _showIcons ? _screenWidth : WsConstants::ScreenWidth;
				uint16_t offset = (_state.Scanline - 1) * screenWidth + _state.Cycle;
				_currentBuffer[offset] = data.Priority == 0 ? GetBgColor() : GetPixelRgbColor(_state.Mode, data.Color, data.Palette);
			}
		} else {
			if (_state.Cycle == 224) {
				ProcessHblank();
			} else if (_state.Cycle == 255) {
				ProcessEndOfScanline();
				_state.Cycle = -1;
			}
		}

		_state.Cycle++;
		_emu->ProcessPpuCycle<CpuType::Ws>();
	}

	void SetVideoMode(WsVideoMode mode);

	uint8_t ReadPort(uint16_t port);
	void WritePort(uint16_t port, uint8_t value);

	uint8_t ReadLcdConfigPort(uint16_t port);
	void WriteLcdConfigPort(uint16_t port, uint8_t value);

	[[nodiscard]] uint16_t GetCycle() { return _state.Cycle; }
	[[nodiscard]] uint16_t GetScanline() { return _state.Scanline; }
	[[nodiscard]] uint16_t GetScanlineCount() { return _state.LastScanline + 1; }
	[[nodiscard]] uint32_t GetFrameCount() { return _state.FrameCount; }
	[[nodiscard]] uint16_t GetScreenWidth() { return _showIcons ? _screenWidth : WsConstants::ScreenWidth; }
	[[nodiscard]] uint16_t GetScreenHeight() { return _showIcons ? _screenHeight : WsConstants::ScreenHeight; }
	WsPpuState& GetState() { return _state; }

	[[nodiscard]] uint16_t GetVisibleScanlineCount();
	[[nodiscard]] uint16_t* GetScreenBuffer(bool prevFrame);

	void DebugSendFrame();
	void SetOutputToBgColor();

	void ShowVolumeIcon();

	void Serialize(Serializer& s) override;
};
