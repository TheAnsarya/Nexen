#pragma once
#include "pch.h"
#include <sstream>

/// <summary>
/// Unified file abstraction supporting filesystem files, archive entries, and memory buffers.
/// Handles ROM loading from disk, ZIP/7z archives, or in-memory data.
/// </summary>
/// <remarks>
/// Supports multiple data sources:
/// - Filesystem files (ROMs, patches, etc.)
/// - Archive entries (file.zip#inner.nes)
/// - Memory buffers (downloaded data, generated content)
/// - Streams (network, pipes, etc.)
/// 
/// Features:
/// - Automatic archive detection and extraction
/// - Lazy loading (data loaded on first access)
/// - Chunked reading for large files (256KB chunks)
/// - SHA1/CRC32 hashing
/// - IPS/BPS patch application
/// 
/// Common usage:
/// <code>
/// VirtualFile rom("game.zip#game.nes");  // Archive entry
/// if (rom.IsValid()) {
///     auto& data = rom.GetData();  // Decompress and return data
/// }
/// </code>
/// </remarks>
class VirtualFile {
private:
	constexpr static int ChunkSize = 256 * 1024;  ///< Chunk size for large file streaming (256KB)

	string _path = "";                ///< Filesystem path or archive path
	string _innerFile = "";           ///< Inner file name (if archive)
	int32_t _innerFileIndex = -1;     ///< Index within archive (-1 if not archive)
	vector<uint8_t> _data;            ///< File data (loaded on demand)
	int64_t _fileSize = -1;           ///< File size in bytes (-1 if not loaded)
	vector<vector<uint8_t>> _chunks;  ///< Chunked data for large files
	bool _useChunks = false;          ///< Chunked mode enabled flag

	/// <summary>Read stream data into vector</summary>
	void FromStream(std::istream& input, vector<uint8_t>& output);

	/// <summary>Lazy load file data from disk/archive</summary>
	void LoadFile();

public:
	/// <summary>Standard ROM file extensions (.nes, .sfc, .gb, etc.)</summary>
	static const std::initializer_list<string> RomExtensions;

	/// <summary>Construct empty invalid VirtualFile</summary>
	VirtualFile();
	
	/// <summary>Construct from archive entry (e.g., "game.zip", "game.nes")</summary>
	VirtualFile(const string& archivePath, const string innerFile);
	
	/// <summary>Construct from filesystem path or archive notation (path#innerfile)</summary>
	VirtualFile(const string& file);
	
	/// <summary>Construct from memory buffer</summary>
	/// <param name="buffer">Data buffer</param>
	/// <param name="bufferSize">Buffer size in bytes</param>
	/// <param name="fileName">Virtual filename (for extension detection)</param>
	VirtualFile(const void* buffer, size_t bufferSize, string fileName = "noname");
	
	/// <summary>Construct from input stream</summary>
	VirtualFile(std::istream& input, string filePath);

	/// <summary>Implicit conversion to filepath string</summary>
	operator std::string() const;

	/// <summary>Check if file successfully loaded/valid</summary>
	bool IsValid();
	
	/// <summary>Check if source is archive (ZIP/7z)</summary>
	bool IsArchive();
	
	/// <summary>Get full file path (including archive notation if applicable)</summary>
	string GetFilePath();
	
	/// <summary>Get folder path containing file/archive</summary>
	string GetFolderPath();
	
	/// <summary>Get filename without path</summary>
	string GetFileName();
	
	/// <summary>Get file extension (.nes, .sfc, etc.)</summary>
	string GetFileExtension();
	
	/// <summary>Calculate SHA1 hash of file data</summary>
	string GetSha1Hash();
	
	/// <summary>Calculate CRC32 checksum of file data</summary>
	uint32_t GetCrc32();

	/// <summary>Get file size in bytes</summary>
	size_t GetSize();
	
	/// <summary>
	/// Check if file matches any of given signatures (magic bytes).
	/// </summary>
	/// <param name="signatures">List of signature strings to check</param>
	/// <param name="loadArchives">If true, load archive contents before checking</param>
	/// <returns>True if file starts with any signature</returns>
	bool CheckFileSignature(vector<string> signatures, bool loadArchives = false);
	
	/// <summary>Enable chunked reading mode for large files (>256KB)</summary>
	void InitChunks();

	/// <summary>Get reference to file data (loads file if not loaded)</summary>
	/// <returns>Reference to internal data vector</returns>
	vector<uint8_t>& GetData();

	/// <summary>Read file data into vector</summary>
	bool ReadFile(vector<uint8_t>& out);
	
	/// <summary>Read file data into stringstream</summary>
	bool ReadFile(std::stringstream& out);
	
	/// <summary>Read file data into preallocated buffer with size validation</summary>
	/// <param name="out">Output buffer (must be expectedSize bytes)</param>
	/// <param name="expectedSize">Expected file size (returns false if mismatch)</param>
	bool ReadFile(uint8_t* out, uint32_t expectedSize);

	/// <summary>Read single byte at offset (chunked mode compatible)</summary>
	uint8_t ReadByte(uint32_t offset);

	/// <summary>Apply IPS/BPS patch to this file</summary>
	/// <param name="patch">VirtualFile containing patch data</param>
	/// <returns>True if patch applied successfully</returns>
	bool ApplyPatch(VirtualFile& patch);

	template <typename T>
	bool ReadChunk(T& container, int start, int length) {
		InitChunks();
		if (start < 0 || start + length > GetSize()) {
			// Out of bounds
			return false;
		}

		for (int i = start, end = start + length; i < end; i++) {
			container.push_back(ReadByte(i));
		}

		return true;
	}
};