#pragma once
#include "pch.h"

/// <summary>
/// PNG image encoding/decoding utilities using LodePNG library.
/// Supports reading/writing PNG files with various bit depths and color formats.
/// </summary>
/// <remarks>
/// All methods are static - utility class for PNG operations.
/// Uses LodePNG for PNG codec implementation.
///
/// Common use cases:
/// - Screenshot capture (WritePNG from framebuffer)
/// - Texture loading (ReadPNG for sprite/tile data)
/// - Debug visualization (write intermediate rendering states)
///
/// Supported formats: 8/16/24/32 bits per pixel
/// Color types: Grayscale, RGB, RGBA
/// </remarks>
class PNGHelper {
private:
	/// <summary>Internal PNG decoder with optional RGBA32 conversion</summary>
	/// <param name="out_image">Decoded image data output</param>
	/// <param name="image_width">Image width in pixels (output)</param>
	/// <param name="image_height">Image height in pixels (output)</param>
	/// <param name="in_png">PNG file data input</param>
	/// <param name="in_size">Size of PNG data in bytes</param>
	/// <param name="convert_to_rgba32">Convert to RGBA32 format (default true)</param>
	/// <returns>LodePNG error code (0 = success)</returns>
	template <typename T>
	static int DecodePNG(vector<T>& out_image, unsigned long& image_width, unsigned long& image_height, const unsigned char* in_png, size_t in_size, bool convert_to_rgba32 = true);

public:
	/// <summary>
	/// Write PNG image to stream.
	/// </summary>
	/// <param name="stream">Output stream for PNG data</param>
	/// <param name="buffer">RGBA pixel data (width * height * 4 bytes)</param>
	/// <param name="xSize">Image width in pixels</param>
	/// <param name="ySize">Image height in pixels</param>
	/// <param name="bitsPerPixel">Bits per pixel (24=RGB, 32=RGBA, default 24)</param>
	/// <returns>True if write succeeded, false on error</returns>
	/// <remarks>
	/// Buffer format depends on bitsPerPixel:
	/// - 24bpp: RGB (3 bytes per pixel)
	/// - 32bpp: RGBA (4 bytes per pixel)
	/// Buffer size must be xSize * ySize * (bitsPerPixel/8) bytes.
	/// </remarks>
	static bool WritePNG(std::stringstream& stream, uint32_t* buffer, uint32_t xSize, uint32_t ySize, uint32_t bitsPerPixel = 24);

	/// <summary>
	/// Write PNG image to file.
	/// </summary>
	/// <param name="filename">Output filename path</param>
	/// <param name="buffer">RGBA pixel data</param>
	/// <param name="xSize">Image width in pixels</param>
	/// <param name="ySize">Image height in pixels</param>
	/// <param name="bitsPerPixel">Bits per pixel (24=RGB, 32=RGBA, default 24)</param>
	/// <returns>True if write succeeded, false on error</returns>
	static bool WritePNG(string filename, uint32_t* buffer, uint32_t xSize, uint32_t ySize, uint32_t bitsPerPixel = 24);

	/// <summary>
	/// Read PNG file and decode to raw pixel data.
	/// </summary>
	/// <param name="filename">PNG file path to read</param>
	/// <param name="pngData">Decoded pixel data (output, resized automatically)</param>
	/// <param name="pngWidth">Image width in pixels (output)</param>
	/// <param name="pngHeight">Image height in pixels (output)</param>
	/// <returns>True if read/decode succeeded, false on error</returns>
	static bool ReadPNG(string filename, vector<uint8_t>& pngData, uint32_t& pngWidth, uint32_t& pngHeight);

	/// <summary>
	/// Decode PNG data from memory buffer.
	/// </summary>
	/// <param name="input">PNG file data</param>
	/// <param name="output">Decoded pixel data (output)</param>
	/// <param name="pngWidth">Image width in pixels (output)</param>
	/// <param name="pngHeight">Image height in pixels (output)</param>
	/// <returns>True if decode succeeded, false on error</returns>
	/// <remarks>
	/// Template parameter T determines output pixel format.
	/// Automatically converts PNG to RGBA32 for processing.
	/// </remarks>
	template <typename T>
	static bool ReadPNG(vector<uint8_t> input, vector<T>& output, uint32_t& pngWidth, uint32_t& pngHeight);
};