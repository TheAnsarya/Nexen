#pragma once
#include "pch.h"

/// <summary>
/// Abstract base class for AVI video compression codecs.
/// Provides interface for frame compression in various formats.
/// </summary>
/// <remarks>
/// Concrete implementations:
/// - RawCodec: Uncompressed RGB frames
/// - CamstudioCodec: Lossless RLE compression
/// - ZmbvCodec: ZMBV lossless compression (DosBox format)
///
/// Codecs are identified by FourCC codes (e.g., "ZMBV", "DIB", "CSCD").
/// Used by AviWriter to compress video frames before writing to file.
/// </remarks>
class BaseCodec {
public:
	/// <summary>
	/// Initialize codec with frame dimensions and compression settings.
	/// </summary>
	/// <param name="width">Frame width in pixels</param>
	/// <param name="height">Frame height in pixels</param>
	/// <param name="compressionLevel">Compression level (codec-specific, 0-100)</param>
	/// <returns>True if setup succeeded</returns>
	virtual bool SetupCompress(int width, int height, uint32_t compressionLevel) = 0;

	/// <summary>
	/// Compress single video frame.
	/// </summary>
	/// <param name="isKeyFrame">True for keyframe (full frame), false for delta frame</param>
	/// <param name="frameData">Input frame data (RGBA format)</param>
	/// <param name="compressedData">Output compressed data pointer (allocated by codec)</param>
	/// <returns>Size of compressed data in bytes</returns>
	/// <remarks>
	/// Codec allocates compressedData buffer - caller must not free it.
	/// Keyframes are self-contained, delta frames reference previous frame.
	/// </remarks>
	virtual int CompressFrame(bool isKeyFrame, uint8_t* frameData, uint8_t** compressedData) = 0;

	/// <summary>
	/// Get codec FourCC identifier.
	/// </summary>
	/// <returns>4-character codec ID (e.g., "ZMBV", "DIB", "CSCD")</returns>
	virtual const char* GetFourCC() = 0;

	/// <summary>Virtual destructor for proper cleanup</summary>
	virtual ~BaseCodec() {}
};