#pragma once
#include "pch.h"
#include "Shared/BaseState.h"

namespace ChannelFConstants {
	static constexpr uint32_t ScreenWidth = 128;
	static constexpr uint32_t ScreenHeight = 64;
	static constexpr uint32_t PixelCount = ScreenWidth * ScreenHeight;
	static constexpr uint32_t CpuClockHz = 1789773;
	static constexpr uint32_t CyclesPerFrame = 29829;
	static constexpr double Fps = 60.0;
	static constexpr uint32_t BiosSize = 0x0800;
	static constexpr uint32_t MaxCartSize = 0x10000;
	static constexpr uint32_t VramSize = ScreenWidth * ScreenHeight;
	static constexpr uint8_t NumColors = 8;
	static constexpr uint32_t ScratchpadSize = 64;
}

struct ChannelFCpuState : BaseState {
	uint64_t CycleCount = 0;
	uint16_t PC0 = 0;        // Active program counter
	uint16_t PC1 = 0;        // Backup/stack program counter
	uint16_t DC0 = 0;        // Active data counter
	uint16_t DC1 = 0;        // Backup data counter
	uint16_t InterruptVector = 0; // 3853 SMI interrupt vector
	uint8_t A = 0;            // Accumulator
	uint8_t W = 0;            // Status register (flags + ICB)
	uint8_t ISAR = 0;         // Indirect Scratchpad Address Register
	bool InterruptsEnabled = false;
	bool IrqLine = false;     // External interrupt request
	uint8_t Scratchpad[64] = {};  // Internal scratchpad RAM
};

struct ChannelFVideoState : BaseState {
	uint8_t Color = 0;        // Current drawing color (2 bits)
	uint8_t X = 0;            // Current X position
	uint8_t Y = 0;            // Current Y position
};

struct ChannelFAudioState : BaseState {
	uint8_t ToneSelect = 0;   // 2-bit tone select (port 0 bits 5-6): 0=silence, 1=1kHz, 2=500Hz, 3=120Hz
	bool SoundEnabled = false;
};

struct ChannelFPortState : BaseState {
	uint8_t Port0 = 0;
	uint8_t Port1 = 0;
	uint8_t Port4 = 0xff;    // Right controller (default: no buttons)
	uint8_t Port5 = 0xff;    // Left controller (default: no buttons)
};

struct ChannelFState : BaseState {
	ChannelFCpuState Cpu = {};
	ChannelFVideoState Video = {};
	ChannelFAudioState Audio = {};
	ChannelFPortState Ports = {};
};
