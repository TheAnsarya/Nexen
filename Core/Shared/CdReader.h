#pragma once
#include "pch.h"
#include "Utilities/VirtualFile.h"
#include "Shared/MessageManager.h"

/// <summary>Track format types for CD-ROM/CD-DA discs</summary>
enum class TrackFormat {
	Audio,      ///< CD-DA audio (2352 bytes/sector, RAW)
	Mode1_2352, ///< CD-ROM Mode 1 (2352 bytes/sector, with header)
	Mode1_2048  ///< CD-ROM Mode 1 (2048 bytes/sector, data only)
};

/// <summary>
/// CD-ROM disc position in MSF (Minutes:Seconds:Frames) format.
/// Standard CD-ROM addressing - 75 frames per second.
/// </summary>
struct DiscPosition {
	uint32_t Minutes; ///< Minutes (0-99)
	uint32_t Seconds; ///< Seconds (0-59)
	uint32_t Frames;  ///< Frames (0-74, 75 frames/second)

	/// <summary>Convert MSF to LBA (Logical Block Address)</summary>
	uint32_t ToLba() {
		return ((Minutes * 60) + Seconds) * 75 + Frames;
	}

	/// <summary>Format as "MM:SS:FF" string</summary>
	string ToString() {
		return GetValueString(Minutes) + ":" + GetValueString(Seconds) + ":" + GetValueString(Frames);
	}

	/// <summary>Convert LBA to MSF position</summary>
	static DiscPosition FromLba(uint32_t lba) {
		DiscPosition pos;
		pos.Minutes = lba / 75 / 60;
		pos.Seconds = lba / 75 % 60;
		pos.Frames = lba % 75;
		return pos;
	}

private:
	/// <summary>Format value with leading zero (e.g., "05" instead of "5")</summary>
	string GetValueString(uint32_t val) {
		return std::format("{:02}", val);
	}
};

/// <summary>CD-ROM track metadata</summary>
struct TrackInfo {
	uint32_t Size;        ///< Track size in bytes
	uint32_t SectorCount; ///< Number of sectors in track

	bool HasLeadIn;              ///< Lead-in pregap present
	DiscPosition LeadInPosition; ///< Lead-in start position (MSF)
	DiscPosition StartPosition;  ///< Track start position (MSF)
	DiscPosition EndPosition;    ///< Track end position (MSF)

	TrackFormat Format;  ///< Track format (audio/data)
	uint32_t FileIndex;  ///< Index into DiscInfo::Files array
	uint32_t FileOffset; ///< Byte offset within file

	uint32_t FirstSector; ///< First LBA sector
	uint32_t LastSector;  ///< Last LBA sector

	/// <summary>Get sector size for track format</summary>
	[[nodiscard]] uint32_t GetSectorSize() {
		switch (Format) {
			default:
			case TrackFormat::Audio:
				return 2352;
			case TrackFormat::Mode1_2352:
				return 2352;
			case TrackFormat::Mode1_2048:
				return 2048;
		}
	}
};

/// <summary>
/// Complete CD-ROM disc information (CUE/BIN format).
/// Supports multi-track audio + data discs.
/// </summary>
struct DiscInfo {
	static constexpr int SectorSize = 2352; ///< Standard CD-ROM sector size (RAW)

	vector<VirtualFile> Files;      ///< Track data files (BIN files)
	vector<TrackInfo> Tracks;       ///< Track metadata
	vector<uint8_t> SubCode;        ///< Raw subchannel data
	vector<uint8_t> DecodedSubCode; ///< Decoded subchannel Q data
	uint32_t DiscSize;              ///< Total disc size in bytes
	uint32_t DiscSectorCount;       ///< Total sector count
	DiscPosition EndPosition;       ///< Disc end position (MSF)

	/// <summary>
	/// Find track containing sector.
	/// </summary>
	/// <param name="sector">LBA sector number</param>
	/// <returns>Track index, or -1 if sector in pregap/invalid</returns>
	[[nodiscard]] int32_t GetTrack(uint32_t sector) {
		for (size_t i = 0; i < Tracks.size(); i++) {
			if (sector >= Tracks[i].FirstSector && sector <= Tracks[i].LastSector) {
				return (int32_t)i;
			}
		}
		return -1;
	}

	/// <summary>
	/// Get first sector of track.
	/// </summary>
	/// <param name="track">Track index</param>
	/// <returns>First LBA sector, or -1 if invalid</returns>
	/// <remarks>
	/// Special case: If track == Tracks.size(), returns last sector of last track.
	/// This handles games like "Tenshi no Uta 2" that specify playback end as
	/// one track beyond the last track (track 0x35 when last track is 0x34).
	/// Without this, playback would end at sector 0, causing immediate restart.
	/// </remarks>
	[[nodiscard]] int32_t GetTrackFirstSector(int32_t track) {
		if (track < Tracks.size()) {
			return Tracks[track].FirstSector;
		} else if (track > 0 && track == Tracks.size()) {
			// Tenshi no Uta 2 intro sets the end of the audio playback to track 0x35, but the last track is 0x34
			// The expected behavior is probably that audio should end at the of track 0x34
			// Without this code, the end gets set to sector 0, which immediately triggers an IRQ and restarts the
			// intro sequence early, making it impossible to start playing the game.
			return Tracks[track - 1].LastSector;
		}
		return -1;
	}

	/// <summary>
	/// Get last sector of track.
	/// </summary>
	/// <param name="track">Track index</param>
	/// <returns>Last LBA sector, or -1 if invalid</returns>
	[[nodiscard]] int32_t GetTrackLastSector(int32_t track) {
		if (track < Tracks.size()) {
			return Tracks[track].LastSector;
		}
		return -1;
	}

	/// <summary>
	/// Read 2048-byte data sector.
	/// </summary>
	/// <typeparam name="T">Output container type (vector, deque, etc.)</typeparam>
	/// <param name="sector">LBA sector number</param>
	/// <param name="outData">Output data container (2048 bytes appended)</param>
	/// <remarks>
	/// Automatically skips 16-byte Mode1_2352 header if present.
	/// Fills with zeros if sector in pregap or invalid.
	/// </remarks>
	template <typename T>
	void ReadDataSector(uint32_t sector, T& outData) {
		constexpr int Mode1_2352_SectorHeaderSize = 16;

		int32_t track = GetTrack(sector);
		if (track < 0) {
			// TODO support reading pregap when it's available
			LogDebug("Invalid sector/track (or inside pregap)");
			outData.insert(outData.end(), 2048, 0);
		} else {
			TrackInfo& trk = Tracks[track];
			uint32_t sectorSize = trk.GetSectorSize();
			uint32_t sectorHeaderSize = trk.Format == TrackFormat::Mode1_2352 ? Mode1_2352_SectorHeaderSize : 0;
			uint32_t byteOffset = trk.FileOffset + (sector - trk.FirstSector) * sectorSize;
			if (!Files[trk.FileIndex].ReadChunk(outData, byteOffset + sectorHeaderSize, 2048)) {
				LogDebug("Invalid read offsets");
			}
		}
	}

	/// <summary>
	/// Read single CD-DA audio sample (16-bit signed).
	/// </summary>
	/// <param name="sector">LBA sector number</param>
	/// <param name="sample">Sample index within sector (0-587, 588 samples/sector)</param>
	/// <param name="byteOffset">Channel offset (0=left, 2=right)</param>
	/// <returns>16-bit audio sample, or 0 if invalid</returns>
	int16_t ReadAudioSample(uint32_t sector, uint32_t sample, uint32_t byteOffset) {
		int32_t track = GetTrack(sector);
		if (track < 0) {
			LogDebug("Invalid sector/track");
			return 0;
		}

		uint32_t fileIndex = Tracks[track].FileIndex;
		uint32_t startByte = Tracks[track].FileOffset + (sector - Tracks[track].FirstSector) * DiscInfo::SectorSize;
		return (int16_t)(Files[fileIndex].ReadByte(startByte + sample * 4 + byteOffset) | (Files[fileIndex].ReadByte(startByte + sample * 4 + 1 + byteOffset) << 8));
	}

	/// <summary>Read left channel audio sample</summary>
	int16_t ReadLeftSample(uint32_t sector, uint32_t sample) {
		return ReadAudioSample(sector, sample, 0);
	}

	/// <summary>Read right channel audio sample</summary>
	int16_t ReadRightSample(uint32_t sector, uint32_t sample) {
		return ReadAudioSample(sector, sample, 2);
	}

	/// <summary>
	/// Get subchannel Q data for sector.
	/// </summary>
	/// <param name="sector">LBA sector number</param>
	/// <param name="out">Output deque (10 bytes appended)</param>
	/// <remarks>
	/// Subchannel Q contains track info, timecode, CRC.
	/// Used for CD playback control and copy protection.
	/// </remarks>
	void GetSubCodeQ(uint32_t sector, std::deque<uint8_t>& out) {
		uint32_t startPos = sector * 96 + 12;
		uint32_t endPos = startPos + 10;
		if (endPos <= DecodedSubCode.size()) {
			out.insert(out.end(), DecodedSubCode.begin() + startPos, DecodedSubCode.begin() + endPos);
		}
	}
};

/// <summary>
/// CD-ROM CUE/BIN file parser for PC Engine CD, Sega CD, PlayStation.
/// </summary>
class CdReader {
	/// <summary>Load .sub subchannel file (if present)</summary>
	static void LoadSubcodeFile(VirtualFile& cueFile, DiscInfo& disc);

public:
	/// <summary>
	/// Load CUE sheet and associated BIN files.
	/// </summary>
	/// <param name="file">CUE sheet file</param>
	/// <param name="disc">Output disc info</param>
	/// <returns>True if loaded successfully</returns>
	static bool LoadCue(VirtualFile& file, DiscInfo& disc);

	/// <summary>
	/// Convert binary value to BCD (Binary Coded Decimal).
	/// </summary>
	/// <param name="value">Binary value (0-99)</param>
	/// <returns>BCD representation (e.g., 42 → 0x42)</returns>
	/// <remarks>
	/// Used for CD-ROM timecodes in subchannel data.
	/// Each nibble represents a decimal digit.
	/// </remarks>
	static uint8_t ToBcd(uint8_t value) {
		uint8_t div = value / 10;
		uint8_t rem = value % 10;
		return (div << 4) | rem;
	}

	/// <summary>
	/// Convert BCD (Binary Coded Decimal) to binary.
	/// </summary>
	/// <param name="value">BCD value (e.g., 0x42)</param>
	/// <returns>Binary representation (e.g., 0x42 → 42)</returns>
	static uint8_t FromBcd(uint8_t value) {
		return ((value >> 4) & 0x0F) * 10 + (value & 0x0F);
	}
};
