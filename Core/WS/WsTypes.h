#pragma once
#include "pch.h"
#include "Shared/BaseState.h"
#include "Shared/SettingTypes.h"

/// <summary>
/// WonderSwan display constants.
/// </summary>
class WsConstants {
public:
	static constexpr uint32_t ScreenWidth = 224;   ///< Horizontal resolution
	static constexpr uint32_t ScreenHeight = 144;  ///< Vertical resolution
	static constexpr uint32_t ClocksPerScanline = 256;  ///< CPU clocks per scanline
	static constexpr uint32_t ScanlineCount = 159;      ///< Total scanlines per frame
	static constexpr uint32_t PixelCount = WsConstants::ScreenWidth * WsConstants::ScreenHeight;
	static constexpr uint32_t MaxPixelCount = WsConstants::ScreenWidth * (WsConstants::ScreenHeight + 13);
};


/// <summary>
/// WonderSwan CPU flags (V30MZ, x86-like). Each field represents a bit in the FLAGS register.
/// </summary>
struct WsCpuFlags {
	bool Carry;     ///< Carry flag (0x01)
	bool Parity;    ///< Parity flag (0x04)
	bool AuxCarry;  ///< Auxiliary carry (0x10)
	bool Zero;      ///< Zero flag (0x40)
	bool Sign;      ///< Sign flag (0x80)
	bool Trap;      ///< Trap flag (0x100)
	bool Irq;       ///< Interrupt enable (0x200)
	bool Direction; ///< Direction flag (0x400)
	bool Overflow;  ///< Overflow flag (0x800)
	bool Mode;      ///< Mode flag (0x8000)

	/// <summary>Pack all flags into a 16-bit value.</summary>
	uint16_t Get() {
		return (
			(uint8_t)Carry |
			((uint8_t)Parity << 2) |
			((uint8_t)AuxCarry << 4) |
			((uint8_t)Zero << 6) |
			((uint8_t)Sign << 7) |
			((uint8_t)Trap << 8) |
			((uint8_t)Irq << 9) |
			((uint8_t)Direction << 10) |
			((uint8_t)Overflow << 11) |
			((uint8_t)Mode << 15) |
			0x7002);
	}

	/// <summary>Unpack a 16-bit value into all flags.</summary>
	void Set(uint16_t f) {
		Carry = f & 0x01;
		Parity = f & 0x04;
		AuxCarry = f & 0x10;
		Zero = f & 0x40;
		Sign = f & 0x80;
		Trap = f & 0x100;
		Irq = f & 0x200;
		Direction = f & 0x400;
		Overflow = f & 0x800;
		Mode = f & 0x8000;
	}
};


/// <summary>
/// Complete WonderSwan CPU state (NEC V30MZ, 16-bit x86-like).
/// </summary>
struct WsCpuState : BaseState {
	uint64_t CycleCount;   ///< Total CPU cycles executed

	uint16_t CS;           ///< Code segment
	uint16_t IP;           ///< Instruction pointer

	uint16_t SS;           ///< Stack segment
	uint16_t SP;           ///< Stack pointer
	uint16_t BP;           ///< Base pointer

	uint16_t DS;           ///< Data segment
	uint16_t ES;           ///< Extra segment

	uint16_t SI;           ///< Source index
	uint16_t DI;           ///< Destination index

	uint16_t AX;           ///< Accumulator
	uint16_t BX;           ///< Base register
	uint16_t CX;           ///< Count register
	uint16_t DX;           ///< Data register

	WsCpuFlags Flags;      ///< CPU flags
	bool Halted;           ///< CPU halted
	bool PowerOff;         ///< Power off state
};

/// <summary>
/// WonderSwan background layer state.
/// Two background layers with independent scroll and tilemap addressing.
/// </summary>
struct WsBgLayer {
	uint16_t MapAddress;          ///< Tilemap base address in VRAM
	uint16_t MapAddressLatch;     ///< Latched tilemap address

	uint8_t ScrollX;              ///< Horizontal scroll position
	uint8_t ScrollXLatch;         ///< Latched H-scroll

	uint8_t ScrollY;              ///< Vertical scroll position
	uint8_t ScrollYLatch;         ///< Latched V-scroll

	bool Enabled;                 ///< Layer enabled
	bool EnabledLatch;            ///< Latched enable state

	/// <summary>Latch all values at start of frame/line.</summary>
	void Latch() {
		EnabledLatch = Enabled;
		ScrollXLatch = ScrollX;
		ScrollYLatch = ScrollY;
		MapAddressLatch = MapAddress;
	}
};

/// <summary>
/// WonderSwan rectangular window region.
/// Used for background clipping and sprite windowing.
/// </summary>
struct WsWindow {
	bool Enabled;                 ///< Window active
	bool EnabledLatch;            ///< Latched enable state

	uint8_t Left;                 ///< Left edge X coordinate
	uint8_t LeftLatch;            ///< Latched left edge
	uint8_t Right;                ///< Right edge X coordinate
	uint8_t RightLatch;           ///< Latched right edge
	uint8_t Top;                  ///< Top edge Y coordinate
	uint8_t TopLatch;             ///< Latched top edge
	uint8_t Bottom;               ///< Bottom edge Y coordinate
	uint8_t BottomLatch;          ///< Latched bottom edge

	/// <summary>Check if pixel is inside latched window bounds.</summary>
	/// <param name="x">Pixel X coordinate</param>
	/// <param name="y">Pixel Y coordinate</param>
	/// <returns>True if inside window</returns>
	bool IsInsideWindow(uint8_t x, uint8_t y) { return x >= LeftLatch && x <= RightLatch && y >= TopLatch && y <= BottomLatch; }

	/// <summary>Latch all values at start of frame/line.</summary>
	void Latch() {
		EnabledLatch = Enabled;
		LeftLatch = Left;
		RightLatch = Right;
		TopLatch = Top;
		BottomLatch = Bottom;
	}
};

/// <summary>
/// WonderSwan LCD segment icons state.
/// Hardware LCD segments for status display.
/// </summary>
struct WsLcdIcons {
	bool Sleep;        ///< Sleep mode icon
	bool Vertical;     ///< Vertical orientation icon
	bool Horizontal;   ///< Horizontal orientation icon
	bool Aux1;         ///< Auxiliary icon 1
	bool Aux2;         ///< Auxiliary icon 2
	bool Aux3;         ///< Auxiliary icon 3

	uint8_t Value;     ///< Raw icon register value
};


/// <summary>
/// WonderSwan video mode (color/bit depth).
/// </summary>
enum class WsVideoMode : uint8_t {
	Monochrome,        ///< 4 shades, mono
	Color2bpp,         ///< 2bpp color
	Color4bpp,         ///< 4bpp color
	Color4bppPacked    ///< 4bpp packed color
};

/// <summary>
/// WonderSwan PPU (Picture Processing Unit) state.
/// Handles background layers, sprites, windowing, and LCD control.
/// </summary>
struct WsPpuState : BaseState {
	uint32_t FrameCount;          ///< Total frames rendered
	uint16_t Cycle;               ///< Current cycle within scanline
	uint16_t Scanline;            ///< Current scanline

	WsBgLayer BgLayers[2];        ///< Two background layers
	WsWindow BgWindow;            ///< Background clipping window
	WsWindow SpriteWindow;        ///< Sprite clipping window
	bool DrawOutsideBgWindow;     ///< Draw BG outside window (vs inside)
	bool DrawOutsideBgWindowLatch; ///< Latched outside-window mode

	uint8_t BwPalettes[0x20 * 2]; ///< Monochrome palette data
	uint8_t BwShades[8];          ///< Grayscale shade values

	uint16_t SpriteTableAddress;  ///< Sprite attribute table base
	uint8_t FirstSpriteIndex;     ///< First sprite to process
	uint8_t SpriteCount;          ///< Number of sprites to render
	uint8_t SpriteCountLatch;     ///< Latched sprite count
	bool SpritesEnabled;          ///< Sprite layer enabled
	bool SpritesEnabledLatch;     ///< Latched sprite enable

	WsVideoMode Mode;             ///< Current video mode
	WsVideoMode NextMode;         ///< Video mode for next frame

	uint8_t BgColor;              ///< Background/border color index
	uint8_t IrqScanline;          ///< Scanline for line IRQ

	bool LcdEnabled;              ///< LCD panel enabled
	bool HighContrast;            ///< High contrast mode
	bool SleepEnabled;            ///< Sleep mode enabled

	uint8_t LcdControl;           ///< LCD control register

	WsLcdIcons Icons;             ///< LCD segment icons

	uint8_t LastScanline;         ///< Last rendered scanline
	uint8_t BackPorchScanline;    ///< Back porch scanline number

	uint32_t ShowVolumeIconFrame; ///< Frame to show volume icon
	uint8_t LcdTftConfig[8];      ///< SwanCrystal TFT configuration

	uint8_t Control;              ///< Control register ($00)
	uint8_t ScreenAddress;        ///< Screen address register ($07)
};

/// <summary>
/// Segment override prefix for memory access.
/// </summary>
enum class WsSegment : uint8_t {
	Default,  ///< Use default segment
	ES,       ///< Extra segment override
	SS,       ///< Stack segment override
	CS,       ///< Code segment override
	DS        ///< Data segment override
};


/// <summary>
/// WonderSwan interrupt sources (IF/IE bits).
/// </summary>
enum class WsIrqSource : uint8_t {
	UartSendReady = 0x01,        ///< UART send ready
	KeyPressed = 0x02,           ///< Key pressed
	Cart = 0x04,                 ///< Cartridge IRQ
	UartRecvReady = 0x08,        ///< UART receive ready
	Scanline = 0x10,             ///< Scanline IRQ
	VerticalBlankTimer = 0x20,   ///< VBlank timer
	VerticalBlank = 0x40,        ///< VBlank IRQ
	HorizontalBlankTimer = 0x80  ///< HBlank timer
};

/// <summary>
/// WonderSwan memory manager state.
/// Handles IRQ, system control, and memory bus configuration.
/// </summary>
struct WsMemoryManagerState {
	uint8_t ActiveIrqs;           ///< Currently pending IRQ flags
	uint8_t EnabledIrqs;          ///< IRQ enable mask
	uint8_t IrqVectorOffset;      ///< IRQ vector table offset

	uint8_t SystemControl2;       ///< System control register 2
	uint8_t SystemTest;           ///< System test register

	bool ColorEnabled;            ///< Color mode enabled (WonderSwan Color)
	bool Enable4bpp;              ///< 4bpp tile mode enabled
	bool Enable4bppPacked;        ///< 4bpp packed tile mode

	bool BootRomDisabled;         ///< Boot ROM disabled (mapped out)
	bool CartWordBus;             ///< 16-bit cartridge bus mode
	bool SlowRom;                 ///< Slow ROM access mode

	bool SlowSram;                ///< Slow SRAM access mode
	bool SlowPort;                ///< Slow I/O port access

	bool EnableLowBatteryNmi;     ///< Low battery triggers NMI
	bool PowerOffRequested;       ///< Power off requested
};

/// <summary>
/// WonderSwan controller/input state.
/// </summary>
struct WsControlManagerState {
	uint8_t InputSelect;          ///< Input multiplexer select
};

/// <summary>
/// WonderSwan DMA controller state.
/// General DMA and Sound DMA (streaming audio).
/// </summary>
struct WsDmaControllerState {
	// General DMA (GDMA)
	uint32_t GdmaSrc;             ///< GDMA source address (20-bit)
	uint32_t SdmaSrc;             ///< SDMA source address (20-bit)
	uint32_t SdmaLength;          ///< SDMA transfer length
	uint32_t SdmaSrcReloadValue;  ///< SDMA source reload value
	uint32_t SdmaLengthReloadValue; ///< SDMA length reload value

	uint16_t GdmaDest;            ///< GDMA destination (VRAM address)
	uint16_t GdmaLength;          ///< GDMA transfer length
	uint8_t GdmaControl;          ///< GDMA control register
	uint8_t SdmaControl;          ///< SDMA control register

	// Sound DMA configuration
	bool SdmaEnabled;             ///< SDMA enabled
	bool SdmaDecrement;           ///< Decrement source address
	bool SdmaHyperVoice;          ///< SDMA to HyperVoice channel
	bool SdmaRepeat;              ///< Auto-repeat at end
	bool SdmaHold;                ///< Hold last sample
	uint8_t SdmaFrequency;        ///< SDMA sample rate divider
	uint8_t SdmaTimer;            ///< SDMA timing counter
};

/// <summary>
/// WonderSwan timer state.
/// Horizontal and vertical blank timers.
/// </summary>
struct WsTimerState {
	uint16_t HTimer;              ///< H-blank timer counter
	uint16_t VTimer;              ///< V-blank timer counter

	uint16_t HReloadValue;        ///< H-blank timer reload
	uint16_t VReloadValue;        ///< V-blank timer reload

	uint8_t Control;              ///< Timer control register
	bool HBlankEnabled;           ///< H-blank timer enabled
	bool HBlankAutoReload;        ///< Auto-reload H-blank timer
	bool VBlankEnabled;           ///< V-blank timer enabled
	bool VBlankAutoReload;        ///< Auto-reload V-blank timer
};

/// <summary>
/// Base APU channel state shared by all channels.
/// </summary>
struct BaseWsApuState {
	uint16_t Frequency;           ///< Frequency register (11-bit)
	uint16_t Timer;               ///< Period counter

	bool Enabled;                 ///< Channel enabled
	uint8_t LeftVolume;           ///< Left output volume (0-15)
	uint8_t RightVolume;          ///< Right output volume (0-15)

	uint8_t SamplePosition;       ///< Current waveform position
	uint8_t LeftOutput;           ///< Current left output sample
	uint8_t RightOutput;          ///< Current right output sample

	/// <summary>Set both volume values from combined register.</summary>
	void SetVolume(uint8_t value) {
		RightVolume = value & 0x0F;
		LeftVolume = (value >> 4);
	}

	/// <summary>Get combined volume register value.</summary>
	uint8_t GetVolume() {
		return RightVolume | (LeftVolume << 4);
	}
};

/// <summary>
/// APU channel 1 state (basic waveform).
/// </summary>
struct WsApuCh1State : public BaseWsApuState {};

/// <summary>
/// APU channel 2 state (waveform + PCM voice).
/// </summary>
struct WsApuCh2State : public BaseWsApuState {
	bool PcmEnabled;              ///< PCM voice mode enabled
	bool MaxPcmVolumeRight;       ///< Right PCM at max volume
	bool HalfPcmVolumeRight;      ///< Right PCM at half volume
	bool MaxPcmVolumeLeft;        ///< Left PCM at max volume
	bool HalfPcmVolumeLeft;       ///< Left PCM at half volume
};

/// <summary>
/// APU channel 3 state (waveform + sweep).
/// </summary>
struct WsApuCh3State : public BaseWsApuState {
	uint16_t SweepScaler;         ///< Sweep frequency scaler
	bool SweepEnabled;            ///< Sweep enabled
	int8_t SweepValue;            ///< Sweep amount (signed)
	uint8_t SweepPeriod;          ///< Sweep period
	uint8_t SweepTimer;           ///< Sweep timer counter
	bool UseSweepCpuClock;        ///< Use CPU clock for sweep
};

/// <summary>
/// APU channel 4 state (waveform + noise).
/// </summary>
struct WsApuCh4State : public BaseWsApuState {
	bool NoiseEnabled;            ///< Noise mode enabled
	bool LfsrEnabled;             ///< LFSR clocking enabled
	uint8_t TapMode;              ///< LFSR tap configuration
	uint8_t TapShift;             ///< LFSR tap shift amount
	uint16_t Lfsr;                ///< 15-bit LFSR state
	uint8_t HoldLfsr;             ///< Hold LFSR value
};

/// <summary>
/// HyperVoice sample scaling mode.
/// </summary>
enum class WsHyperVoiceScalingMode : uint8_t {
	Unsigned,          ///< Unsigned input
	UnsignedNegated,   ///< Unsigned negated
	Signed,            ///< Signed input
	None               ///< No scaling
};

/// <summary>
/// HyperVoice output channel mode.
/// </summary>
enum class WsHyperVoiceChannelMode : uint8_t {
	Stereo,     ///< Stereo output
	MonoLeft,   ///< Mono to left only
	MonoRight,  ///< Mono to right only
	MonoBoth    ///< Mono to both channels
};

/// <summary>
/// HyperVoice (PCM DMA audio) channel state.
/// WonderSwan Color/SwanCrystal feature for streaming audio.
/// </summary>
struct WsApuHyperVoiceState {
	int16_t LeftOutput;           ///< Current left sample
	int16_t RightOutput;          ///< Current right sample

	bool Enabled;                 ///< HyperVoice enabled

	uint8_t LeftSample;           ///< Left sample buffer
	uint8_t RightSample;          ///< Right sample buffer
	bool UpdateRightValue;        ///< Update right on next sample

	uint8_t Divisor;              ///< Sample rate divisor
	uint8_t Timer;                ///< Sample timing counter
	uint8_t Input;                ///< Current input sample
	uint8_t Shift;                ///< Volume shift amount
	WsHyperVoiceChannelMode ChannelMode;   ///< Output routing mode
	WsHyperVoiceScalingMode ScalingMode;   ///< Sample scaling mode

	uint8_t ControlLow;           ///< Control register low byte
	uint8_t ControlHigh;          ///< Control register high byte
};

/// <summary>
/// Complete WonderSwan APU state.
/// 4 waveform channels + HyperVoice PCM.
/// </summary>
struct WsApuState {
	WsApuCh1State Ch1;            ///< Channel 1 (basic)
	WsApuCh2State Ch2;            ///< Channel 2 (PCM voice)
	WsApuCh3State Ch3;            ///< Channel 3 (sweep)
	WsApuCh4State Ch4;            ///< Channel 4 (noise)
	WsApuHyperVoiceState Voice;   ///< HyperVoice PCM channel

	uint16_t WaveTableAddress;    ///< Waveform data base address
	bool SpeakerEnabled;          ///< Internal speaker enabled
	uint8_t SpeakerVolume;        ///< Speaker volume level
	uint8_t InternalMasterVolume; ///< Internal master volume
	uint8_t MasterVolume;         ///< Master output volume
	bool HeadphoneEnabled;        ///< Headphone output enabled

	bool HoldChannels;            ///< Hold all channel outputs
	bool ForceOutput2;            ///< Force channel 2 output
	bool ForceOutput4;            ///< Force channel 4 output
	bool ForceOutputCh2Voice;     ///< Force channel 2 voice output

	uint8_t SoundTest;            ///< Sound test register
};

/// <summary>
/// Serial port state for link cable communication.
/// </summary>
struct WsSerialState {
	uint64_t SendClock;           ///< Clock when send started

	bool Enabled;                 ///< Serial port enabled
	bool HighSpeed;               ///< High speed mode (9600 vs 38400 baud)
	bool ReceiveOverflow;         ///< Receive buffer overflow

	bool HasReceiveData;          ///< Data available in receive buffer
	uint8_t ReceiveBuffer;        ///< Received data byte

	bool HasSendData;             ///< Data pending in send buffer
	uint8_t SendBuffer;           ///< Data byte to send
};

/// <summary>
/// EEPROM size configuration.
/// </summary>
enum class WsEepromSize : uint16_t {
	Size0 = 0,         ///< No EEPROM
	Size128 = 0x80,    ///< 128 bytes (1Kbit)
	Size1kb = 0x400,   ///< 1KB (8Kbit)
	Size2kb = 0x800    ///< 2KB (16Kbit)
};

/// <summary>
/// EEPROM (save data) state.
/// Used for both internal and cartridge EEPROM.
/// </summary>
struct WsEepromState {
	uint64_t CmdStartClock;       ///< Command start time
	WsEepromSize Size;            ///< EEPROM size
	uint16_t ReadBuffer;          ///< Read data buffer
	uint16_t WriteBuffer;         ///< Write data buffer
	uint16_t Command;             ///< Current command
	uint16_t Control;             ///< Control register
	bool WriteDisabled;           ///< Write protection enabled
	bool ReadDone;                ///< Read operation complete
	bool Idle;                    ///< EEPROM idle

	bool InternalEepromWriteProtected; ///< Internal EEPROM protected
};

/// <summary>
/// Cartridge state with ROM bank selection.
/// </summary>
struct WsCartState {
	uint8_t SelectedBanks[4];     ///< Selected ROM banks for each slot
};

/// <summary>
/// Complete WonderSwan emulation state.
/// </summary>
struct WsState {
	WsCpuState Cpu;                    ///< V30MZ CPU state
	WsPpuState Ppu;                    ///< PPU (video) state
	WsApuState Apu;                    ///< APU (audio) state
	WsMemoryManagerState MemoryManager; ///< Memory manager state
	WsControlManagerState ControlManager; ///< Controller state
	WsDmaControllerState DmaController;  ///< DMA controller state
	WsTimerState Timer;                ///< Timer state
	WsSerialState Serial;              ///< Serial port state
	WsEepromState InternalEeprom;      ///< Internal EEPROM state
	WsCartState Cart;                  ///< Cartridge state
	WsEepromState CartEeprom;          ///< Cartridge EEPROM state
	WsModel Model;                     ///< Console model (WS/WSC/SC)
};

/// <summary>
/// Lookup table for V30MZ parity flag calculation.
/// Parity flag is set when byte has even number of set bits.
/// </summary>
class WsCpuParityTable {
private:
	uint8_t _parityTable[0x100];   ///< Pre-computed parity for all byte values

public:
	/// <summary>
	/// Constructs parity table with pre-computed values.
	/// </summary>
	WsCpuParityTable() {
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
