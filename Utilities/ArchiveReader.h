#pragma once
#include "pch.h"

/// <summary>
/// Abstract base class for reading compressed archive formats (ZIP, 7z, etc.).
/// Provides unified interface for loading ROMs and data files from archives.
/// </summary>
/// <remarks>
/// Concrete implementations:
/// - ZipReader: ZIP archive support
/// - SevenZipReader: 7z archive support
/// 
/// Factory pattern: Use GetReader() to auto-detect format and create appropriate reader.
/// 
/// Common workflow:
/// <code>
/// auto reader = ArchiveReader::GetReader("game.zip");
/// if (reader) {
///     auto files = reader->GetFileList({".nes", ".sfc"});
///     vector&lt;uint8_t&gt; rom;
///     reader->ExtractFile(files[0], rom);
/// }
/// </code>
/// 
/// Thread safety: Not thread-safe - use separate reader per thread.
/// </remarks>
class ArchiveReader {
protected:
	bool _initialized = false;  ///< Archive successfully loaded flag
	uint8_t* _buffer = nullptr; ///< Internal archive data buffer
	
	/// <summary>Derived class implements format-specific archive loading</summary>
	/// <param name="buffer">Archive data buffer</param>
	/// <param name="size">Buffer size in bytes</param>
	/// <returns>True if archive loaded successfully</returns>
	virtual bool InternalLoadArchive(void* buffer, size_t size) = 0;
	
	/// <summary>Derived class implements file listing</summary>
	/// <returns>List of all files in archive</returns>
	virtual vector<string> InternalGetFileList() = 0;

public:
	/// <summary>Virtual destructor - cleans up buffer and resources</summary>
	virtual ~ArchiveReader();

	/// <summary>Load archive from memory buffer</summary>
	/// <param name="buffer">Archive data</param>
	/// <param name="size">Buffer size in bytes</param>
	/// <returns>True if loaded successfully</returns>
	bool LoadArchive(void* buffer, size_t size);
	
	/// <summary>Load archive from vector</summary>
	bool LoadArchive(vector<uint8_t>& data);
	
	/// <summary>Load archive from file path</summary>
	bool LoadArchive(string filename);
	
	/// <summary>Load archive from input stream</summary>
	bool LoadArchive(std::istream& in);

	/// <summary>
	/// Extract file to stream without allocating vector.
	/// </summary>
	/// <param name="filename">File path within archive</param>
	/// <param name="stream">Output stream for file data</param>
	/// <returns>True if file found and extracted</returns>
	bool GetStream(string filename, std::stringstream& stream);

	/// <summary>
	/// Get list of files with optional extension filtering.
	/// </summary>
	/// <param name="extensions">Extension filter (e.g., {".nes", ".sfc"}), empty = all files</param>
	/// <returns>List of matching file paths</returns>
	vector<string> GetFileList(std::initializer_list<string> extensions = {});
	
	/// <summary>Check if specific file exists in archive</summary>
	/// <param name="filename">File path to check</param>
	/// <returns>True if file exists</returns>
	bool CheckFile(string filename);

	/// <summary>
	/// Extract file from archive to vector.
	/// </summary>
	/// <param name="filename">File path within archive</param>
	/// <param name="output">Output vector for file data (resized automatically)</param>
	/// <returns>True if file found and extracted</returns>
	/// <remarks>Pure virtual - must be implemented by derived classes</remarks>
	virtual bool ExtractFile(string filename, vector<uint8_t>& output) = 0;

	/// <summary>
	/// Factory method: auto-detect archive format and create reader.
	/// </summary>
	/// <param name="in">Input stream positioned at archive start</param>
	/// <returns>ArchiveReader instance or nullptr if format not supported</returns>
	/// <remarks>Detects ZIP, 7z, and other supported formats by magic bytes</remarks>
	static unique_ptr<ArchiveReader> GetReader(std::istream& in);
	
	/// <summary>Factory method: create reader from file path</summary>
	static unique_ptr<ArchiveReader> GetReader(string filepath);
};