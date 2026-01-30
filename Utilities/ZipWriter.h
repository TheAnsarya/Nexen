#pragma once
#include "pch.h"
#include "miniz.h"

/// <summary>
/// ZIP archive writer using miniz library for compression.
/// Creates compressed ZIP files with multiple entries.
/// </summary>
/// <remarks>
/// Common usage:
/// <code>
/// ZipWriter zip;
/// zip.Initialize("output.zip");
/// zip.AddFile(data, "file1.dat");
/// zip.AddFile("path/to/file2.txt", "file2.txt");
/// zip.Save();
/// </code>
///
/// Supports adding files from:
/// - Filesystem paths (reads and compresses file)
/// - Memory vectors (compresses raw data)
/// - Stringstreams (compresses stream data)
///
/// All files stored with deflate compression.
/// ZIP format compatible with standard ZIP tools.
/// </remarks>
class ZipWriter {
private:
	mz_zip_archive _zipArchive; ///< Miniz ZIP archive structure
	string _zipFilename;        ///< Output ZIP file path

public:
	/// <summary>Construct ZipWriter (call Initialize before adding files)</summary>
	ZipWriter();

	/// <summary>Destructor - saves and closes archive if not already saved</summary>
	~ZipWriter();

	/// <summary>
	/// Initialize ZIP archive for writing.
	/// </summary>
	/// <param name="filename">Output ZIP file path</param>
	/// <returns>True if initialization succeeded</returns>
	bool Initialize(string filename);

	/// <summary>
	/// Finalize and save ZIP archive to disk.
	/// </summary>
	/// <returns>True if save succeeded</returns>
	/// <remarks>Called automatically by destructor if not called manually</remarks>
	bool Save();

	/// <summary>
	/// Add file from filesystem to ZIP.
	/// </summary>
	/// <param name="filepath">Source file path to read</param>
	/// <param name="zipFilename">Path within ZIP archive</param>
	void AddFile(string filepath, string zipFilename);

	/// <summary>
	/// Add file from memory vector to ZIP.
	/// </summary>
	/// <param name="fileData">File data to compress</param>
	/// <param name="zipFilename">Path within ZIP archive</param>
	void AddFile(vector<uint8_t>& fileData, string zipFilename);

	/// <summary>
	/// Add file from stringstream to ZIP.
	/// </summary>
	/// <param name="filestream">Stream containing file data</param>
	/// <param name="zipFilename">Path within ZIP archive</param>
	void AddFile(std::stringstream& filestream, string zipFilename);
};