#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"

// Based on bsnes' code (by byuu)
// original implementation: neviksti
// optimized implementation: talarubi

class Spc7110;

/// <summary>
/// SPC7110 hardware decompression engine emulation.
/// Implements context-based adaptive arithmetic coding for graphics data.
/// Supports 1bpp, 2bpp, and 4bpp planar tile decompression.
/// </summary>
/// <remarks>
/// The SPC7110 decompressor uses a probabilistic model to encode tile graphics
/// data with high compression ratios. Key features:
/// 
/// - Context modeling: Predictions based on previously decoded pixels
/// - Adaptive probability: Model evolves as data is decoded
/// - Move-to-front coding: Efficient encoding of recently-used colors
/// - Planar output: Decompresses directly to SNES tile format
/// 
/// The decompressor maintains 75 context states (5 context types × 15 contexts)
/// with probability values and next-state transitions for MPS/LPS outcomes.
/// 
/// Original implementation by neviksti, optimized by talarubi for bsnes.
/// </remarks>
class Spc7110Decomp : public ISerializable {
private:
	/// <summary>Symbol indices for probability states.</summary>
	enum : uint32_t { MPS = 0,   ///< More Probable Symbol
		              LPS = 1 }; ///< Less Probable Symbol

	/// <summary>Probability constants for arithmetic coding.</summary>
	enum : uint32_t { One = 0xAA,  ///< Probability ~2/3
		              Half = 0x55, ///< Probability 1/3
		              Max = 0xFF }; ///< Maximum probability

	/// <summary>
	/// State transition entry for probability evolution.
	/// </summary>
	struct ModelState {
		/// <summary>Probability of the more probable symbol (0-255).</summary>
		uint8_t probability;

		/// <summary>Next state indices after outputting {MPS, LPS}.</summary>
		uint8_t next[2];
	};

	/// <summary>Probability evolution table (53 states).</summary>
	static ModelState evolution[53];

	/// <summary>
	/// Decoder context state.
	/// </summary>
	struct Context {
		/// <summary>Current model state index (0-52).</summary>
		uint8_t prediction;

		/// <summary>If 1, swap MPS and LPS roles.</summary>
		uint8_t swap;
	};

	/// <summary>Context array [contextType][contextIndex].</summary>
	/// <remarks>Not all 75 contexts exist; this layout simplifies code.</remarks>
	Context _context[5][15];

	/// <summary>Reference to parent SPC7110 for ROM access.</summary>
	Spc7110* _spc;

	/// <summary>Bits per pixel mode (1, 2, or 4).</summary>
	uint32_t _bpp;

	/// <summary>Current read offset in SPC7110 data ROM.</summary>
	uint32_t _offset;

	/// <summary>Bits remaining in current input byte.</summary>
	uint32_t _bits;

	/// <summary>Arithmetic coding range (8-bit, but Max+1 = 256).</summary>
	uint16_t _range;

	/// <summary>Input data from SPC7110 data ROM.</summary>
	uint16_t _input;

	/// <summary>Current output byte being built.</summary>
	uint8_t _output;

	/// <summary>Decoded pixel buffer (up to 64 bits for 4bpp 8x1 row).</summary>
	uint64_t _pixels;

	/// <summary>Most Recently Used list for move-to-front coding.</summary>
	uint64_t _colormap;

	/// <summary>Decoded tile data after calling Decode().</summary>
	uint32_t _result;

private:
	/// <summary>
	/// Reads next byte from compressed data stream.
	/// </summary>
	/// <returns>Data byte from data ROM.</returns>
	uint8_t ReadByte();

	/// <summary>
	/// Converts interleaved pixel data to planar format.
	/// </summary>
	/// <param name="data">Interleaved pixel data.</param>
	/// <param name="bits">Bits per pixel.</param>
	/// <returns>Planar tile data.</returns>
	uint32_t Deinterleave(uint64_t data, uint32_t bits);

	/// <summary>
	/// Updates MRU list with newly used color.
	/// </summary>
	/// <param name="list">Current MRU list.</param>
	/// <param name="nibble">Color index to move to front.</param>
	/// <returns>Updated MRU list.</returns>
	uint64_t MoveToFront(uint64_t list, uint32_t nibble);

public:
	/// <summary>
	/// Creates a new SPC7110 decompressor.
	/// </summary>
	/// <param name="spc">Reference to parent SPC7110 for ROM access.</param>
	Spc7110Decomp(Spc7110* spc);

	/// <summary>Destructor.</summary>
	virtual ~Spc7110Decomp();

	/// <summary>
	/// Initializes decompressor for a new data stream.
	/// </summary>
	/// <param name="mode">Bits per pixel mode (0=1bpp, 1=2bpp, 2=4bpp).</param>
	/// <param name="origin">Starting offset in data ROM.</param>
	void Initialize(uint32_t mode, uint32_t origin);

	/// <summary>
	/// Decodes the next tile row from compressed stream.
	/// </summary>
	/// <remarks>
	/// Call GetResult() after Decode() to retrieve decompressed data.
	/// </remarks>
	void Decode();

	/// <summary>
	/// Gets the most recently decoded tile data.
	/// </summary>
	/// <returns>Planar tile row data.</returns>
	uint32_t GetResult();

	/// <summary>
	/// Gets the current bits-per-pixel mode.
	/// </summary>
	/// <returns>BPP value (1, 2, or 4).</returns>
	uint8_t GetBpp();

	/// <summary>Serializes decompressor state for save states.</summary>
	void Serialize(Serializer& s) override;
};