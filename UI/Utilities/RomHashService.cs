using System;
using System.Diagnostics;
using Nexen.Interop;
using StreamHash.Core;

namespace Nexen.Utilities;

/// <summary>
/// Immutable record holding the four standard hash values for a ROM.
/// All hash values are uppercase hex strings.
/// </summary>
public record RomHashInfo(
	string Crc32,
	string Md5,
	string Sha1,
	string Sha256
) {
	/// <summary>
	/// CRC32 as a 32-bit unsigned integer.
	/// </summary>
	public uint Crc32Value => uint.Parse(Crc32, System.Globalization.NumberStyles.HexNumber);

	/// <summary>
	/// Returns an empty hash info with all values set to empty strings.
	/// </summary>
	public static RomHashInfo Empty { get; } = new("", "", "", "");

	/// <summary>
	/// Whether this hash info contains valid (non-empty) hash values.
	/// </summary>
	public bool IsValid => !string.IsNullOrEmpty(Crc32);
}

/// <summary>
/// Computes and caches ROM hash values using StreamHash's Basic Hashes API.
/// Provides CRC32, MD5, SHA-1, and SHA-256 in a single pass over the ROM data.
/// </summary>
public static class RomHashService {
	private static RomHashInfo? _cachedHashes;
	private static byte[]? _cachedRomData;

	/// <summary>
	/// Compute all four basic hashes (CRC32, MD5, SHA-1, SHA-256) for the given data
	/// in a single pass using StreamHash's batch streaming API.
	/// </summary>
	/// <param name="data">The data to hash</param>
	/// <returns>A <see cref="RomHashInfo"/> containing all four hash values as uppercase hex strings</returns>
	public static RomHashInfo ComputeBasicHashes(byte[] data) {
		if (data is null or { Length: 0 })
			return RomHashInfo.Empty;

		using var hasher = HashFacade.CreateBasicHashesStreaming();
		hasher.Update(data);
		var results = hasher.FinalizeAll();

		return new RomHashInfo(
			Crc32: results[HashAlgorithmNames.Crc32].ToUpperInvariant(),
			Md5: results[HashAlgorithmNames.Md5].ToUpperInvariant(),
			Sha1: results[HashAlgorithmNames.Sha1].ToUpperInvariant(),
			Sha256: results[HashAlgorithmNames.Sha256].ToUpperInvariant()
		);
	}

	/// <summary>
	/// Compute all four basic hashes for the currently loaded ROM's PRG data.
	/// Results are cached and reused if the ROM data hasn't changed.
	/// </summary>
	/// <param name="romInfo">ROM information containing console type</param>
	/// <returns>A <see cref="RomHashInfo"/> with all four hash values</returns>
	public static RomHashInfo ComputeRomHashes(RomInfo romInfo) {
		try {
			var cpuType = romInfo.ConsoleType.GetMainCpuType();
			var memType = cpuType.GetPrgRomMemoryType();
			byte[] romData = DebugApi.GetMemoryState(memType);

			if (romData is null or { Length: 0 })
				return RomHashInfo.Empty;

			// Return cached result if ROM data hasn't changed
			if (_cachedHashes is not null && _cachedRomData is not null &&
				romData.AsSpan().SequenceEqual(_cachedRomData)) {
				return _cachedHashes;
			}

			var hashes = ComputeBasicHashes(romData);
			_cachedRomData = romData;
			_cachedHashes = hashes;
			return hashes;
		} catch (Exception ex) {
			Debug.WriteLine($"[RomHashService] Hash computation failed: {ex.Message}");
			return RomHashInfo.Empty;
		}
	}

	/// <summary>
	/// Compute all four basic hashes for an arbitrary file.
	/// Useful for firmware verification, update integrity checks, etc.
	/// </summary>
	/// <param name="filePath">Path to the file to hash</param>
	/// <returns>A <see cref="RomHashInfo"/> with all four hash values, or empty if file cannot be read</returns>
	public static RomHashInfo ComputeFileHashes(string filePath) {
		try {
			byte[]? data = FileHelper.ReadAllBytes(filePath);
			if (data is null or { Length: 0 })
				return RomHashInfo.Empty;

			return ComputeBasicHashes(data);
		} catch (Exception ex) {
			Debug.WriteLine($"[RomHashService] File hash computation failed: {ex.Message}");
			return RomHashInfo.Empty;
		}
	}

	/// <summary>
	/// Invalidate the cached ROM hashes. Call when a new ROM is loaded.
	/// </summary>
	public static void InvalidateCache() {
		_cachedHashes = null;
		_cachedRomData = null;
	}
}
