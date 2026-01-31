#pragma once
#include "pch.h"
#include "NES/Loaders/BaseLoader.h"

struct RomData;

/// <summary>
/// Famicom Disk System (FDS) and Quick Disk (QD) image loader.
/// Handles .fds and .qd disk image formats for the FDS expansion.
/// </summary>
/// <remarks>
/// **Famicom Disk System:**
/// - Disk-based storage system for Famicom (1986)
/// - Double-sided disks (65,500 bytes per side typical)
/// - Games can span multiple disk sides
/// - Requires FDS BIOS ROM to boot
///
/// **FDS Disk Format:**
/// - File system stored in GAP/block format
/// - Block types: Disk info, file count, file header, file data
/// - GAP1 (28300 bits), GAP2 (976 bits) between blocks
/// - CRC checks for data integrity
///
/// **Image Formats:**
/// - .fds: Raw disk data (65,500 bytes per side), optional 16-byte header
/// - .qd: Quick Disk format (65,536 bytes per side), used by some dumps
///
/// **Multi-Disk Games:**
/// - Disk images can contain multiple sides/disks
/// - Loader separates into individual disk sides
/// - Runtime supports disk swapping via UI
///
/// **Gap Addition:**
/// - Real disks have timing gaps between blocks
/// - AddGaps() reconstructs gap timing for accurate emulation
/// - Important for games that use timing-sensitive disk access
/// </remarks>
class FdsLoader : public BaseLoader {
private:
	/// <summary>Standard FDS disk side capacity (65,500 bytes).</summary>
	static constexpr size_t FdsDiskSideCapacity = 65500;

	/// <summary>Quick Disk side capacity (65,536 bytes).</summary>
	static constexpr size_t QdDiskSideCapacity = 65536;

	/// <summary>True if loading Quick Disk format instead of FDS.</summary>
	bool _useQdFormat = false;

private:
	/// <summary>
	/// Adds timing gaps between disk blocks for accurate emulation.
	/// </summary>
	/// <param name="diskSide">Output disk data with gaps.</param>
	/// <param name="readBuffer">Raw disk block data.</param>
	/// <param name="bufferSize">Size of raw data.</param>
	void AddGaps(vector<uint8_t>& diskSide, uint8_t* readBuffer, uint32_t bufferSize);

	/// <summary>Gets the capacity per disk side based on format.</summary>
	/// <returns>Bytes per disk side.</returns>
	int GetSideCapacity();

public:
	/// <summary>
	/// Constructs an FDS loader.
	/// </summary>
	/// <param name="useQdFormat">True for Quick Disk format, false for FDS.</param>
	FdsLoader(bool useQdFormat = false);

	/// <summary>
	/// Rebuilds an FDS file from modified disk data.
	/// Used when saving changes back to disk image.
	/// </summary>
	/// <param name="diskData">Per-side disk data.</param>
	/// <param name="needHeader">Include 16-byte FDS header.</param>
	/// <returns>Complete FDS file bytes.</returns>
	vector<uint8_t> RebuildFdsFile(vector<vector<uint8_t>> diskData, bool needHeader);

	/// <summary>
	/// Loads raw disk data from FDS image.
	/// Separates multi-side images into individual sides.
	/// </summary>
	/// <param name="romFile">Input FDS file bytes.</param>
	/// <param name="diskData">Output per-side disk data.</param>
	/// <param name="diskHeaders">Output per-side header data.</param>
	void LoadDiskData(vector<uint8_t>& romFile, vector<vector<uint8_t>>& diskData, vector<vector<uint8_t>>& diskHeaders);

	/// <summary>
	/// Loads an FDS ROM image.
	/// </summary>
	/// <param name="romData">Output ROM data structure.</param>
	/// <param name="romFile">Input FDS file bytes.</param>
	void LoadRom(RomData& romData, vector<uint8_t>& romFile);
};