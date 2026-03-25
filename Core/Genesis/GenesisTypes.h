#pragma once
#include "pch.h"

// ===== M68000 CPU State =====

struct GenesisM68kState {
	uint32_t D[8] = {};       // Data registers D0-D7
	uint32_t A[8] = {};       // Address registers A0-A7 (A7 is active stack pointer)
	uint32_t PC = 0;          // Program counter
	uint32_t USP = 0;         // User stack pointer
	uint32_t SSP = 0;         // Supervisor stack pointer
	uint16_t SR = 0x2700;     // Status register (supervisor mode, interrupts masked)
	uint64_t CycleCount = 0;  // Total cycles executed
	bool Stopped = false;     // STOP instruction halted CPU
};

// M68000 status register bits
namespace M68kFlags {
	constexpr uint16_t Carry    = 0x0001;  // C
	constexpr uint16_t Overflow = 0x0002;  // V
	constexpr uint16_t Zero     = 0x0004;  // Z
	constexpr uint16_t Negative = 0x0008;  // N
	constexpr uint16_t Extend   = 0x0010;  // X
	constexpr uint16_t CcrMask  = 0x001F;  // CCR (lower byte)
	constexpr uint16_t IntMask  = 0x0700;  // Interrupt priority mask (bits 8-10)
	constexpr uint16_t Supervisor = 0x2000; // S bit
	constexpr uint16_t Trace    = 0x8000;  // T bit
}

// ===== VDP State =====

struct GenesisVdpState {
	// Registers (24 VDP registers, R0-R23)
	uint8_t Registers[24] = {};

	// Memory
	// VRAM: 64KB accessed via _vram array
	// CRAM: 64 entries x 16-bit (9-bit color: 0BBB0GGG0RRR)
	uint16_t Cram[64] = {};
	// VSRAM: 40 entries x 16-bit (10-bit vertical scroll values)
	uint16_t Vsram[40] = {};

	// Internal state
	uint16_t StatusRegister = 0x3400; // Initial: FIFO empty + VBlank
	uint32_t AddressRegister = 0;     // VRAM/CRAM/VSRAM address pointer
	uint8_t CodeRegister = 0;         // Access mode (VRAM read/write, CRAM, VSRAM)
	bool WritePending = false;        // Second control word expected
	uint16_t DataPortBuffer = 0;      // Read buffer

	// Counters
	uint16_t HCounter = 0;
	uint16_t VCounter = 0;
	uint32_t FrameCount = 0;
	uint8_t HIntCounter = 0;         // H-interrupt line counter

	// DMA state
	bool DmaActive = false;
	uint8_t DmaMode = 0;             // 0=68k copy, 1=fill, 2=VRAM copy
};

// VDP register helpers
namespace VdpReg {
	constexpr uint8_t ModeSet1      = 0;   // R0: Mode set register 1
	constexpr uint8_t ModeSet2      = 1;   // R1: Mode set register 2
	constexpr uint8_t PlaneABase    = 2;   // R2: Plane A nametable base address
	constexpr uint8_t WindowBase    = 3;   // R3: Window nametable base address
	constexpr uint8_t PlaneBBase    = 4;   // R4: Plane B nametable base address
	constexpr uint8_t SpriteBase    = 5;   // R5: Sprite attribute table base address
	constexpr uint8_t BgColor       = 7;   // R7: Background color (palette + entry)
	constexpr uint8_t HIntCounter   = 10;  // R10: H-interrupt counter
	constexpr uint8_t ModeSet3      = 11;  // R11: Mode set register 3
	constexpr uint8_t ModeSet4      = 12;  // R12: Mode set register 4
	constexpr uint8_t HScrollBase   = 13;  // R13: H-scroll data table base address
	constexpr uint8_t AutoIncrement = 15;  // R15: Auto-increment value
	constexpr uint8_t PlaneSize     = 16;  // R16: Plane size (width × height)
	constexpr uint8_t WindowHPos    = 17;  // R17: Window H position
	constexpr uint8_t WindowVPos    = 18;  // R18: Window V position
	constexpr uint8_t DmaLenLow    = 19;  // R19: DMA length low
	constexpr uint8_t DmaLenHigh   = 20;  // R20: DMA length high
	constexpr uint8_t DmaSrcLow    = 21;  // R21: DMA source low
	constexpr uint8_t DmaSrcMid    = 22;  // R22: DMA source mid
	constexpr uint8_t DmaSrcHigh   = 23;  // R23: DMA source high / DMA mode
}

// VDP status register bits
namespace VdpStatus {
	constexpr uint16_t FifoEmpty    = 0x0200;
	constexpr uint16_t FifoFull     = 0x0100;
	constexpr uint16_t VInterrupt   = 0x0080;
	constexpr uint16_t SprOverflow  = 0x0040;
	constexpr uint16_t SprCollision = 0x0020;
	constexpr uint16_t OddFrame     = 0x0010;
	constexpr uint16_t VBlanking    = 0x0008;
	constexpr uint16_t HBlanking    = 0x0004;
	constexpr uint16_t DmaBusy      = 0x0002;
	constexpr uint16_t PalMode      = 0x0001;

	// Aliases used in VDP implementation
	constexpr uint16_t VBlankFlag   = VBlanking;
	constexpr uint16_t VIntPending  = VInterrupt;
	constexpr uint16_t HIntPending  = HBlanking;
}

// ===== Controller State =====

struct GenesisControllerState {
	bool Up = false;
	bool Down = false;
	bool Left = false;
	bool Right = false;
	bool A = false;
	bool B = false;
	bool C = false;
	bool Start = false;
	// 6-button extras
	bool X = false;
	bool Y = false;
	bool Z = false;
	bool Mode = false;
};

// ===== I/O Port State =====

struct GenesisIoState {
	uint8_t DataPort[3] = {};   // Data ports (A, B, C)
	uint8_t CtrlPort[3] = {};   // Control ports (direction)
	uint8_t TxData[3] = {};
	uint8_t RxData[3] = {};
	uint8_t SCtrl[3] = {};
	uint8_t ThCount[2] = {};    // TH transition count for 6-button
	uint8_t ThState[2] = {};    // Current TH state
};

// ===== PSG (SN76489) State =====

struct GenesisPsgChannelState {
	uint16_t ToneCounter = 0;    // 10-bit tone period register
	uint8_t Volume = 0x0F;       // 4-bit attenuation (0x0F = silent)
};

struct GenesisPsgState {
	GenesisPsgChannelState Channels[4] = {};  // 3 tone + 1 noise
	uint8_t LatchedRegister = 0;              // Currently latched register (0-7)
	uint8_t NoiseMode = 0;                    // Noise mode (bit 2) + shift rate (bits 0-1)
	uint16_t NoiseShiftRegister = 0x8000;     // 16-bit LFSR
	uint32_t WriteCount = 0;                  // Total writes for diagnostics
};

// ===== Combined State =====

struct GenesisState {
	GenesisM68kState Cpu;
	GenesisVdpState Vdp;
	GenesisIoState Io;
	GenesisPsgState Psg;
};
