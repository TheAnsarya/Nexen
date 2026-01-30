#pragma once
#include "pch.h"
#include "miniz.h"
#include "ArchiveReader.h"

/// <summary>
/// ZIP archive reader using miniz library for decompression.
/// Concrete implementation of ArchiveReader for ZIP format.
/// </summary>
/// <remarks>
/// Supports standard ZIP archive operations:
/// - List files in archive
/// - Extract individual files
/// - Check file existence
///
/// Automatically detected by ArchiveReader::GetReader() via ZIP magic bytes.
/// Uses miniz for deflate decompression.
/// Thread safety: Not thread-safe - use separate reader per thread.
/// </remarks>
class ZipReader : public ArchiveReader {
private:
	mz_zip_archive _zipArchive; ///< Miniz ZIP archive structure

protected:
	/// <summary>Load and parse ZIP archive from memory buffer</summary>
	bool InternalLoadArchive(void* buffer, size_t size);

	/// <summary>Get list of all files in ZIP archive</summary>
	vector<string> InternalGetFileList();

public:
	/// <summary>Construct ZIP reader</summary>
	ZipReader();

	/// <summary>Destructor - closes ZIP archive and frees resources</summary>
	virtual ~ZipReader();

	/// <summary>
	/// Extract file from ZIP archive.
	/// </summary>
	/// <param name="filename">File path within archive</param>
	/// <param name="output">Output vector for decompressed data</param>
	/// <returns>True if file found and extracted</returns>
	bool ExtractFile(string filename, vector<uint8_t>& output);
};