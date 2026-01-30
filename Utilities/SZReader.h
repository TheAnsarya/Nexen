#pragma once
#include "pch.h"
#include "Utilities/ArchiveReader.h"
#include "SevenZip/7z.h"
#include "SevenZip/7zAlloc.h"
#include "SevenZip/7zCrc.h"
#include "SevenZip/7zTypes.h"
#include "SevenZip/7zMemBuffer.h"

/// <summary>
/// 7-Zip archive reader using LZMA SDK.
/// Concrete implementation of ArchiveReader for 7z format.
/// </summary>
/// <remarks>
/// Supports 7-Zip (.7z) archives with LZMA/LZMA2 compression.
/// Automatically detected by ArchiveReader::GetReader() via 7z magic bytes ("7z\xBC\xAF\x27\x1C").
///
/// Uses LZMA SDK components:
/// - CSzArEx: 7z archive structure
/// - CMemBufferInStream: Memory-based input stream
/// - CLookToRead: Buffered stream reader
///
/// Allocators: SzAlloc/SzFree for permanent data, SzAllocTemp/SzFreeTemp for temporary buffers.
/// Thread safety: Not thread-safe - use separate reader per thread.
/// </remarks>
class SZReader : public ArchiveReader {
private:
	CMemBufferInStream _memBufferStream;             ///< Memory buffer stream for archive data
	CLookToRead _lookStream;                         ///< Buffered stream reader
	CSzArEx _archive;                                ///< 7z archive structure
	ISzAlloc _allocImp{SzAlloc, SzFree};             ///< Permanent allocator
	ISzAlloc _allocTempImp{SzAllocTemp, SzFreeTemp}; ///< Temporary allocator

protected:
	/// <summary>Load and parse 7z archive from memory buffer</summary>
	bool InternalLoadArchive(void* buffer, size_t size);

	/// <summary>Get list of all files in 7z archive</summary>
	vector<string> InternalGetFileList();

public:
	/// <summary>Construct 7z reader</summary>
	SZReader();

	/// <summary>Destructor - closes archive and frees LZMA resources</summary>
	virtual ~SZReader();

	/// <summary>
	/// Extract file from 7z archive.
	/// </summary>
	/// <param name="filename">File path within archive</param>
	/// <param name="output">Output vector for decompressed data</param>
	/// <returns>True if file found and extracted</returns>
	bool ExtractFile(string filename, vector<uint8_t>& output);
};