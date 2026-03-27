#pragma once
#include "pch.h"
#include "ChannelF/ChannelFTypes.h"

/// <summary>
/// Memory bus for the Fairchild Channel F system.
///
/// Program memory map (16-bit, read-only except VRAM):
///   $0000-$03FF  BIOS ROM 1 (1KB)
///   $0400-$07FF  BIOS ROM 2 (1KB)
///   $0800+       Cartridge ROM (2K-32K)
///
/// I/O port space (separate 8-bit space):
///   Port 0: Console switches / video color+sound control
///   Port 1: Video row/column latch, controller input
///   Port 4: Right controller
///   Port 5: Left controller, sound output
///
/// Video RAM is 128x64 pixels at 2bpp (4 colors), accessed
/// indirectly via I/O ports and DC0-based addressing.
/// </summary>
class ChannelFMemoryManager {
public:
	static constexpr uint32_t BiosSize = ChannelFConstants::BiosSize;
	static constexpr uint32_t MaxCartSize = ChannelFConstants::MaxCartSize;
	static constexpr uint32_t VramSize = ChannelFConstants::VramSize;
	static constexpr uint32_t ScreenWidth = ChannelFConstants::ScreenWidth;
	static constexpr uint32_t ScreenHeight = ChannelFConstants::ScreenHeight;
	static constexpr uint8_t NumColors = ChannelFConstants::NumColors;

private:
	// ROM storage
	vector<uint8_t> _biosRom;
	vector<uint8_t> _cartRom;
	uint32_t _cartSize = 0;

	// Video RAM (one byte per pixel: 2-bit color index)
	uint8_t _vram[ChannelFConstants::VramSize] = {};

	// I/O port latches
	uint8_t _portLatch[256] = {};

	// Video state
	uint8_t _videoColor = 0;
	uint8_t _videoX = 0;
	uint8_t _videoY = 0;

	// Audio state
	uint8_t _soundTone = 0;
	uint8_t _soundFreq = 0;

	// Controller state
	uint8_t _controller1 = 0xff;
	uint8_t _controller2 = 0xff;
	uint8_t _consoleButtons = 0xff;

public:
	ChannelFMemoryManager();

	void LoadBios(const uint8_t* data, uint32_t size);
	void LoadCart(const uint8_t* data, uint32_t size);
	void Reset();

	// CPU memory bus
	[[nodiscard]] uint8_t ReadMemory(uint16_t addr) const;
	void WriteMemory(uint16_t addr, uint8_t value);

	// CPU I/O port bus
	[[nodiscard]] uint8_t ReadPort(uint8_t port) const;
	void WritePort(uint8_t port, uint8_t value);

	// Controller input (set by control manager each frame)
	void SetControllerState(uint8_t ctrl1, uint8_t ctrl2, uint8_t console);

	// State accessors for GetConsoleState and serialization
	[[nodiscard]] ChannelFVideoState GetVideoState() const;
	[[nodiscard]] ChannelFAudioState GetAudioState() const;
	[[nodiscard]] ChannelFPortState GetPortState() const;
	void SetVideoState(const ChannelFVideoState& state);
	void SetAudioState(const ChannelFAudioState& state);
	void SetPortState(const ChannelFPortState& state);

	// Accessors for debugger and rendering
	[[nodiscard]] const uint8_t* GetVram() const { return _vram; }
	[[nodiscard]] uint8_t* GetVramData() { return _vram; }
	[[nodiscard]] const uint8_t* GetBiosData() const { return _biosRom.data(); }
	[[nodiscard]] uint32_t GetBiosSize() const { return (uint32_t)_biosRom.size(); }
	[[nodiscard]] const uint8_t* GetCartData() const { return _cartRom.data(); }
	[[nodiscard]] uint32_t GetCartSize() const { return _cartSize; }
};
