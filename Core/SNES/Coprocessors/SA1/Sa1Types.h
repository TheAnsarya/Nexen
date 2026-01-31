#pragma once
#include "pch.h"
#include "SNES/SnesCpuTypes.h"

/// <summary>
/// SA-1 hardware math operation modes.
/// </summary>
enum class Sa1MathOp {
	/// <summary>16-bit × 16-bit multiplication.</summary>
	Mul = 0,
	/// <summary>16-bit ÷ 16-bit division with remainder.</summary>
	Div = 1,
	/// <summary>Cumulative 40-bit sum.</summary>
	Sum = 2
};

/// <summary>
/// SA-1 DMA source device selection.
/// </summary>
enum class Sa1DmaSrcDevice {
	/// <summary>Program ROM (cartridge ROM).</summary>
	PrgRom = 0,
	/// <summary>BW-RAM (battery-backed work RAM).</summary>
	BwRam = 1,
	/// <summary>SA-1 internal RAM (2KB).</summary>
	InternalRam = 2,
	/// <summary>Reserved/unused.</summary>
	Reserved = 3
};

/// <summary>
/// SA-1 DMA destination device selection.
/// </summary>
enum class Sa1DmaDestDevice {
	/// <summary>SA-1 internal RAM (2KB).</summary>
	InternalRam = 0,
	/// <summary>BW-RAM (battery-backed work RAM).</summary>
	BwRam = 1
};

/// <summary>
/// Complete state of the SA-1 coprocessor.
/// The SA-1 is a fast 65816 coprocessor running at 10.74 MHz with:
/// - Hardware math (multiply/divide/sum)
/// - DMA with character conversion
/// - Programmable timers
/// - Variable-length bit data decoder
/// - Memory mapping and protection
/// </summary>
struct Sa1State {
	// ===== Interrupt Vectors =====
	/// <summary>SA-1 reset vector address.</summary>
	uint16_t Sa1ResetVector;

	/// <summary>SA-1 IRQ vector address.</summary>
	uint16_t Sa1IrqVector;

	/// <summary>SA-1 NMI vector address.</summary>
	uint16_t Sa1NmiVector;

	// ===== SA-1 Interrupt Control =====
	/// <summary>SA-1 has pending IRQ from S-CPU.</summary>
	bool Sa1IrqRequested;

	/// <summary>Enables S-CPU to send IRQs to SA-1.</summary>
	bool Sa1IrqEnabled;

	/// <summary>SA-1 has pending NMI from S-CPU.</summary>
	bool Sa1NmiRequested;

	/// <summary>Enables S-CPU to send NMIs to SA-1.</summary>
	bool Sa1NmiEnabled;

	/// <summary>SA-1 is waiting (stopped by S-CPU).</summary>
	bool Sa1Wait;

	/// <summary>SA-1 is in reset state.</summary>
	bool Sa1Reset;

	/// <summary>DMA completion IRQ enabled.</summary>
	bool DmaIrqEnabled;

	/// <summary>Timer IRQ enabled.</summary>
	bool TimerIrqEnabled;

	// ===== Inter-CPU Messaging =====
	/// <summary>Message byte sent by S-CPU to SA-1.</summary>
	uint8_t Sa1MessageReceived;

	/// <summary>Message byte sent by SA-1 to S-CPU.</summary>
	uint8_t CpuMessageReceived;

	// ===== S-CPU Interrupt Vectors =====
	/// <summary>Custom IRQ vector for S-CPU.</summary>
	uint16_t CpuIrqVector;

	/// <summary>Custom NMI vector for S-CPU.</summary>
	uint16_t CpuNmiVector;

	/// <summary>Use custom IRQ vector instead of ROM vector.</summary>
	bool UseCpuIrqVector;

	/// <summary>Use custom NMI vector instead of ROM vector.</summary>
	bool UseCpuNmiVector;

	// ===== S-CPU Interrupt Control =====
	/// <summary>S-CPU has pending IRQ from SA-1.</summary>
	bool CpuIrqRequested;

	/// <summary>Enables SA-1 to send IRQs to S-CPU.</summary>
	bool CpuIrqEnabled;

	// ===== Character Conversion DMA =====
	/// <summary>Character conversion IRQ flag.</summary>
	bool CharConvIrqFlag;

	/// <summary>Character conversion IRQ enabled.</summary>
	bool CharConvIrqEnabled;

	/// <summary>Character conversion DMA is active.</summary>
	bool CharConvDmaActive;

	/// <summary>Character conversion bits per pixel (2, 4, or 8).</summary>
	uint8_t CharConvBpp;

	/// <summary>Character conversion tile format.</summary>
	uint8_t CharConvFormat;

	/// <summary>Character conversion tile width.</summary>
	uint8_t CharConvWidth;

	/// <summary>Character conversion tile counter.</summary>
	uint8_t CharConvCounter;

	// ===== BW-RAM Settings (S-CPU side) =====
	/// <summary>BW-RAM bank for S-CPU access.</summary>
	uint8_t CpuBwBank;

	/// <summary>S-CPU can write to BW-RAM.</summary>
	bool CpuBwWriteEnabled;

	// ===== BW-RAM Settings (SA-1 side) =====
	/// <summary>BW-RAM bank for SA-1 access.</summary>
	uint8_t Sa1BwBank;

	/// <summary>SA-1 BW-RAM access mode.</summary>
	uint8_t Sa1BwMode;

	/// <summary>SA-1 can write to BW-RAM.</summary>
	bool Sa1BwWriteEnabled;

	/// <summary>Write-protected area size in BW-RAM.</summary>
	uint8_t BwWriteProtectedArea;

	/// <summary>BW-RAM 2BPP bitmap mode enabled.</summary>
	bool BwRam2BppMode;

	// ===== Bitmap Registers =====
	/// <summary>Bitmap register 1 (8 bytes for BPP conversion).</summary>
	uint8_t BitmapRegister1[8];

	/// <summary>Bitmap register 2 (8 bytes for BPP conversion).</summary>
	uint8_t BitmapRegister2[8];

	// ===== I-RAM Write Protection =====
	/// <summary>S-CPU I-RAM write protection page.</summary>
	uint8_t CpuIRamWriteProtect;

	/// <summary>SA-1 I-RAM write protection page.</summary>
	uint8_t Sa1IRamWriteProtect;

	// ===== DMA Settings =====
	/// <summary>DMA source address (24-bit).</summary>
	uint32_t DmaSrcAddr;

	/// <summary>DMA destination address (24-bit).</summary>
	uint32_t DmaDestAddr;

	/// <summary>DMA transfer size in bytes.</summary>
	uint16_t DmaSize;

	/// <summary>DMA transfer enabled.</summary>
	bool DmaEnabled;

	/// <summary>DMA has priority over CPU.</summary>
	bool DmaPriority;

	/// <summary>DMA performs character conversion.</summary>
	bool DmaCharConv;

	/// <summary>Automatic character conversion mode.</summary>
	bool DmaCharConvAuto;

	/// <summary>DMA destination device.</summary>
	Sa1DmaDestDevice DmaDestDevice;

	/// <summary>DMA source device.</summary>
	Sa1DmaSrcDevice DmaSrcDevice;

	/// <summary>DMA transfer is running.</summary>
	bool DmaRunning;

	/// <summary>DMA IRQ flag (transfer complete).</summary>
	bool DmaIrqFlag;

	// ===== Timer Settings =====
	/// <summary>Horizontal (H-count) timer enabled.</summary>
	bool HorizontalTimerEnabled;

	/// <summary>Vertical (V-count) timer enabled.</summary>
	bool VerticalTimerEnabled;

	/// <summary>Use linear timer mode instead of H/V.</summary>
	bool UseLinearTimer;

	/// <summary>Horizontal timer compare value.</summary>
	uint16_t HTimer;

	/// <summary>Vertical timer compare value.</summary>
	uint16_t VTimer;

	/// <summary>Linear timer current value.</summary>
	uint32_t LinearTimerValue;

	// ===== Hardware Math =====
	/// <summary>Current math operation mode.</summary>
	Sa1MathOp MathOp;

	/// <summary>Multiplicand or dividend for math operations.</summary>
	uint16_t MultiplicandDividend;

	/// <summary>Multiplier or divisor for math operations.</summary>
	uint16_t MultiplierDivisor;

	/// <summary>Cycle when math operation started.</summary>
	uint64_t MathStartClock;

	/// <summary>Math operation result (40-bit for sum).</summary>
	uint64_t MathOpResult;

	/// <summary>Math overflow flag.</summary>
	uint8_t MathOverflow;

	// ===== Variable-Length Bit Data =====
	/// <summary>Auto-increment address after read.</summary>
	bool VarLenAutoInc;

	/// <summary>Bit count for variable-length data (0-15).</summary>
	uint8_t VarLenBitCount;

	/// <summary>Variable-length data source address.</summary>
	uint32_t VarLenAddress;

	/// <summary>Current bit position within byte.</summary>
	uint8_t VarLenCurrentBit;

	// ===== Memory Banking =====
	/// <summary>ROM bank mapping registers (4 banks).</summary>
	uint8_t Banks[4];
};
