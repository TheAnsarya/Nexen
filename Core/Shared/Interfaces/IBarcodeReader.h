#pragma once
#include "pch.h"

/// <summary>
/// Interface for barcode reader peripheral emulation.
/// </summary>
/// <remarks>
/// Supported systems:
/// - Famicom: Barcode Battler (Family Trainer series)
/// - Game Boy: Barcode Boy (Monster Maker Barcode Saga)
/// - SNES: Barcode Battler II
/// 
/// Barcode formats:
/// - EAN-13: European Article Number (13 digits)
/// - UPC-A: Universal Product Code (12 digits)
/// - Code 39: Alphanumeric barcodes
/// - JAN: Japanese Article Number
/// 
/// Hardware operation:
/// - Swipe barcode through reader slot
/// - Reader sends digit stream to console
/// - Game processes barcode data (unlock content, stat generation)
/// 
/// Emulation:
/// - User inputs barcode number via UI
/// - Interface simulates reader output timing
/// - Console receives digits as if from real hardware
/// 
/// Thread model:
/// - InputBarcode() called from UI thread
/// - Console polls barcode data on emulation thread
/// </remarks>
class IBarcodeReader {
public:
	/// <summary>
	/// Input barcode number for reading.
	/// </summary>
	/// <param name="barcode">Barcode number (up to 20 digits)</param>
	/// <param name="digitCount">Number of digits in barcode (8-20)</param>
	/// <remarks>
	/// Barcode encoding:
	/// - Stored as 64-bit integer (max ~18 digits)
	/// - digitCount specifies actual digit count (for leading zeros)
	/// - Reader simulates ~100ms scan time (digit-by-digit transmission)
	/// 
	/// Example barcodes:
	/// - EAN-13: 4902370517589 (13 digits)
	/// - UPC-A: 012345678905 (12 digits)
	/// - Barcode Battler: Custom 8-16 digit codes
	/// </remarks>
	virtual void InputBarcode(uint64_t barcode, uint32_t digitCount) = 0;
};
