#pragma once
#include "pch.h"

/// <summary>
/// State of the S-DD1 (Super Data Decompression 1) coprocessor.
/// The S-DD1 is a decompression chip used in Star Ocean and Street Fighter Alpha 2
/// to decompress graphics data on-the-fly during DMA transfers.
/// This allows larger games to fit on smaller ROM chips.
/// </summary>
struct Sdd1State {
	/// <summary>Bitmask indicating which DMA channels can use S-DD1 decompression.</summary>
	uint8_t AllowDmaProcessing;

	/// <summary>Bitmask indicating which DMA channels will process on next transfer.</summary>
	uint8_t ProcessNextDma;

	/// <summary>Selected ROM banks for S-DD1 memory mapping (4 banks: $C0-$CF, $D0-$DF, $E0-$EF, $F0-$FF).</summary>
	uint8_t SelectedBanks[4];

	/// <summary>DMA source addresses for each of the 8 DMA channels.</summary>
	uint32_t DmaAddress[8];

	/// <summary>DMA transfer lengths for each of the 8 DMA channels.</summary>
	uint16_t DmaLength[8];

	/// <summary>Flag indicating decompressor needs reinitialization.</summary>
	bool NeedInit;
};