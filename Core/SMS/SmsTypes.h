#pragma once
#include "pch.h"
#include "Shared/BaseState.h"
#include "Shared/MemoryType.h"

/// <summary>
/// Supported SMS-family console models.
/// </summary>
enum class SmsModel {
	Sms,          ///< Sega Master System
	GameGear,     ///< Sega Game Gear (portable, different palette)
	Sg,           ///< SG-1000 (earlier Sega console)
	ColecoVision  ///< ColecoVision (compatible hardware)
};

/// <summary>
/// Zilog Z80 CPU state for SMS/Game Gear emulation.
/// Includes main registers, alternate set, and interrupt state.
/// </summary>
struct SmsCpuState : public BaseState {
	uint64_t CycleCount;   ///< Total CPU cycles executed
	uint16_t PC;           ///< Program Counter
	uint16_t SP;           ///< Stack Pointer

	// Main register set
	uint8_t A;             ///< Accumulator
	uint8_t Flags;         ///< Status flags (S,Z,H,P/V,N,C)

	uint8_t B;             ///< B register
	uint8_t C;             ///< C register
	uint8_t D;             ///< D register
	uint8_t E;             ///< E register

	uint8_t H;             ///< H register (high byte of HL)
	uint8_t L;             ///< L register (low byte of HL)

	uint8_t IXL;           ///< IX low byte
	uint8_t IXH;           ///< IX high byte

	uint8_t IYL;           ///< IY low byte
	uint8_t IYH;           ///< IY high byte

	uint8_t I;             ///< Interrupt Vector register
	uint8_t R;             ///< Memory Refresh register

	// Alternate (shadow) register set
	uint8_t AltA;          ///< Alternate accumulator
	uint8_t AltFlags;      ///< Alternate flags
	uint8_t AltB;          ///< Alternate B
	uint8_t AltC;          ///< Alternate C
	uint8_t AltD;          ///< Alternate D
	uint8_t AltE;          ///< Alternate E
	uint8_t AltH;          ///< Alternate H
	uint8_t AltL;          ///< Alternate L

	// Interrupt state
	uint8_t ActiveIrqs;    ///< Active IRQ sources
	bool NmiLevel;         ///< NMI line level
	bool NmiPending;       ///< NMI request pending
	bool Halted;           ///< CPU in HALT state

	bool IFF1;             ///< Interrupt Flip-Flop 1 (current enable)
	bool IFF2;             ///< Interrupt Flip-Flop 2 (saved during NMI)

	uint8_t IM;            ///< Interrupt Mode (0, 1, or 2)

	// Internal flags needed to properly emulate the behavior of the undocumented F3/F5 flags
	uint8_t FlagsChanged;  ///< Tracks when flags modified (for undocumented behavior)
	uint16_t WZ;           ///< Internal temporary register (affects undocumented flags)
};

/// <summary>
/// Z80 CPU status flag bits.
/// </summary>
namespace SmsCpuFlags {
enum SmsCpuFlags {
	Carry = 0x01,      ///< Carry/borrow flag (bit 0)
	AddSub = 0x02,     ///< Add/Subtract flag for BCD (bit 1)
	Parity = 0x04,     ///< Parity/Overflow flag (bit 2)
	F3 = 0x08,         ///< Undocumented flag 3 (bit 3)
	HalfCarry = 0x10,  ///< Half-carry for BCD (bit 4)
	F5 = 0x20,         ///< Undocumented flag 5 (bit 5)
	Zero = 0x40,       ///< Zero flag (bit 6)
	Sign = 0x80,       ///< Sign flag (bit 7)
};
} // namespace SmsCpuFlags

/// <summary>
/// SMS Video Display Processor (VDP) state.
/// TMS9918A-derived chip with SMS-specific enhancements.
/// </summary>
struct SmsVdpState : public BaseState {
	uint32_t FrameCount;         ///< Total frames rendered
	uint16_t Cycle;              ///< Current cycle within scanline
	uint16_t Scanline;           ///< Current scanline
	uint16_t VCounter;           ///< Vertical counter (can wrap differently)

	uint16_t AddressReg;         ///< VRAM address register
	uint8_t CodeReg;             ///< Command/code register
	bool ControlPortMsbToggle;   ///< Second byte of control port write pending

	uint8_t VramBuffer;          ///< Read-ahead buffer for VRAM reads
	uint8_t PaletteLatch;        ///< Latched palette write data
	uint8_t HCounterLatch;       ///< Latched horizontal counter

	// Status flags
	bool VerticalBlankIrqPending; ///< VBlank interrupt pending
	bool ScanlineIrqPending;      ///< Line interrupt pending
	bool SpriteOverflow;          ///< More than 8 sprites on scanline
	bool SpriteCollision;         ///< Two sprites overlapped
	uint8_t SpriteOverflowIndex;  ///< First overflow sprite index

	// Address computation
	uint16_t ColorTableAddress;      ///< Color table base (TMS modes)
	uint16_t BgPatternTableAddress;  ///< Background tile patterns

	uint16_t SpriteTableAddress;     ///< Sprite attribute table base
	uint16_t SpritePatternSelector;  ///< Sprite pattern table base

	uint16_t NametableHeight;        ///< Nametable height in tiles
	uint8_t VisibleScanlineCount;    ///< Active display lines

	// Color and scroll
	uint8_t TextColorIndex;          ///< Text mode foreground color
	uint8_t BackgroundColorIndex;    ///< Background/border color
	uint8_t HorizontalScroll;        ///< Horizontal scroll position
	uint8_t HorizontalScrollLatch;   ///< Latched H-scroll at line start
	uint8_t VerticalScroll;          ///< Vertical scroll position
	uint8_t VerticalScrollLatch;     ///< Latched V-scroll at frame start
	uint8_t ScanlineCounter;         ///< Line IRQ counter
	uint8_t ScanlineCounterLatch;    ///< Line IRQ reload value

	// Control register 0 bits
	bool SyncDisabled;               ///< Sync disabled (blank display)
	bool M2_AllowHeightChange;       ///< Mode 2: enable 224/240 line modes
	bool UseMode4;                   ///< SMS Mode 4 (vs TMS9918 modes)
	bool ShiftSpritesLeft;           ///< Shift sprites left 8 pixels
	bool EnableScanlineIrq;          ///< Enable line interrupt
	bool MaskFirstColumn;            ///< Hide leftmost 8 pixels
	bool HorizontalScrollLock;       ///< Lock top 2 rows H-scroll
	bool VerticalScrollLock;         ///< Lock right 8 columns V-scroll

	// Control register 1 bits
	bool Sg16KVramMode;              ///< SG-1000 16KB VRAM mode
	bool RenderingEnabled;           ///< Display enabled
	bool EnableVerticalBlankIrq;     ///< Enable VBlank interrupt
	bool M1_Use224LineMode;          ///< 224 line display mode
	bool M3_Use240LineMode;          ///< 240 line display mode
	bool UseLargeSprites;            ///< 8x16 sprites (vs 8x8)
	bool EnableDoubleSpriteSize;     ///< Double sprite pixel size

	// Nametable addressing
	uint16_t NametableAddress;          ///< Configured nametable address
	uint16_t EffectiveNametableAddress; ///< Actual address after masking
	uint16_t NametableAddressMask;      ///< Address mask for nametable
};

/// <summary>
/// PSG tone channel state for square wave generation.
/// SMS uses a SN76489-compatible PSG with 3 tone + 1 noise channels.
/// </summary>
struct SmsToneChannelState {
	uint16_t ReloadValue;   ///< Frequency divider reload (10-bit)
	uint16_t Timer;         ///< Current countdown timer
	uint8_t Output;         ///< Current output level (0 or 1)
	uint8_t Volume;         ///< Attenuation (0=max, 15=silent)
};

/// <summary>
/// PSG noise channel state using LFSR.
/// </summary>
struct SmsNoiseChannelState {
	uint16_t Timer;         ///< Current countdown timer
	uint16_t Lfsr;          ///< Linear Feedback Shift Register state
	uint8_t LfsrInputBit;   ///< Calculated feedback bit
	uint8_t Control;        ///< Noise mode and rate control
	uint8_t Output;         ///< Current output level (0 or 1)
	uint8_t Volume;         ///< Attenuation (0=max, 15=silent)
};

/// <summary>
/// Complete PSG (Programmable Sound Generator) state.
/// SN76489-compatible with Game Gear stereo extension.
/// </summary>
struct SmsPsgState {
	SmsToneChannelState Tone[3] = {}; ///< Three square wave channels
	SmsNoiseChannelState Noise = {};   ///< Noise channel
	uint8_t SelectedReg = 0;           ///< Currently selected register
	uint8_t GameGearPanningReg = 0xFF; ///< GG stereo panning (bit per channel per side)
};

/// <summary>
/// Memory-mapped register access type.
/// </summary>
enum class SmsRegisterAccess {
	None = 0,       ///< No access
	Read = 1,       ///< Read-only
	Write = 2,      ///< Write-only
	ReadWrite = 3   ///< Read and write
};

/// <summary>
/// Memory manager state including I/O control.
/// </summary>
struct SmsMemoryManagerState {
	bool IsReadRegister[0x100];    ///< Port is readable
	bool IsWriteRegister[0x100];   ///< Port is writable

	uint8_t OpenBus;               ///< Open bus value for unmapped reads

	// Game Gear specific registers
	uint8_t GgExtData;             ///< Game Gear EXT port data
	uint8_t GgExtConfig;           ///< Game Gear EXT port config
	uint8_t GgSendData;            ///< Game Gear serial send data
	uint8_t GgSerialConfig;        ///< Game Gear serial config

	// Memory enable flags
	bool ExpEnabled;               ///< Expansion slot enabled
	bool CartridgeEnabled;         ///< Cartridge slot enabled
	bool CardEnabled;              ///< Card slot enabled
	bool WorkRamEnabled;           ///< Work RAM enabled
	bool BiosEnabled;              ///< BIOS ROM enabled
	bool IoEnabled;                ///< I/O chip enabled
};

/// <summary>
/// Controller/input port state.
/// </summary>
struct SmsControlManagerState {
	uint8_t ControlPort;           ///< I/O control port value
};

/// <summary>
/// Complete SMS/Game Gear emulation state for save states.
/// </summary>
struct SmsState {
	SmsCpuState Cpu;                   ///< Z80 CPU state
	SmsVdpState Vdp;                   ///< VDP (video) state
	SmsPsgState Psg;                   ///< PSG (audio) state
	SmsMemoryManagerState MemoryManager; ///< Memory mapping state
	SmsControlManagerState ControlManager; ///< Input state
};

/// <summary>
/// SMS interrupt sources.
/// </summary>
enum class SmsIrqSource {
	None = 0,   ///< No interrupt
	Vdp = 1,    ///< VDP interrupt (VBlank or line)
};

/// <summary>
/// VDP write destination type.
/// </summary>
enum class SmsVdpWriteType : uint8_t {
	None = 0,    ///< No write pending
	Vram = 1,    ///< Writing to VRAM
	Palette = 2  ///< Writing to CRAM (palette)
};

/// <summary>
/// Lookup table for Z80 parity flag calculation.
/// Parity flag is set when byte has even number of set bits.
/// </summary>
class SmsCpuParityTable {
private:
	uint8_t _parityTable[0x100];   ///< Pre-computed parity for all byte values

public:
	/// <summary>
	/// Constructs parity table with pre-computed values.
	/// </summary>
	SmsCpuParityTable() {
		for (int i = 0; i < 256; i++) {
			int count = 0;
			for (int j = 0; j < 8; j++) {
				count += (i & (1 << j)) ? 1 : 0;
			}
			_parityTable[i] = count & 0x01 ? 0 : 1;
		}
	}

	/// <summary>
	/// Checks if byte has even parity.
	/// </summary>
	/// <param name="val">Byte value to check</param>
	/// <returns>True if even number of bits set</returns>
	__forceinline bool CheckParity(uint8_t val) {
		return _parityTable[val];
	}
};
