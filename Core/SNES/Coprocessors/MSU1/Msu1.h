#pragma once
#include "pch.h"
#include "Shared/Interfaces/IAudioProvider.h"
#include "Shared/Audio/PcmReader.h"
#include "Utilities/ISerializable.h"
#include "Utilities/VirtualFile.h"

class Spc;
class Emulator;

/// <summary>
/// MSU-1 (Media Streaming Unit) audio enhancement chip emulation.
/// Allows SNES ROMs to play CD-quality (44.1KHz 16-bit stereo) PCM audio
/// and access large data files stored alongside the ROM.
/// </summary>
/// <remarks>
/// MSU-1 is a modern enhancement specification designed by byuu (near)
/// for use with SNES emulators and flash cartridges. It provides:
/// - Streaming PCM audio from external .pcm files
/// - Large data file access from external .msu files
/// - Hardware-mixed audio output with volume control
/// 
/// Register map ($2000-$2007):
/// - $2000 (R): Status (bits: audio busy, data busy, track missing, etc.)
/// - $2000 (W): Data seek (low byte)
/// - $2001 (W): Data seek (mid byte)
/// - $2002 (W): Data seek (high byte)
/// - $2003 (W): Audio track (low byte)
/// - $2004 (W): Audio track (high byte)
/// - $2005 (W): Audio volume (0-255)
/// - $2006 (W): Audio control (bits: repeat, pause/resume)
/// - $2007 (R): Data read
/// </remarks>
class Msu1 final : public ISerializable, public IAudioProvider {
private:
	/// <summary>Reference to SPC for audio timing sync.</summary>
	Spc* _spc = nullptr;

	/// <summary>Reference to emulator for file paths and settings.</summary>
	Emulator* _emu = nullptr;

	/// <summary>PCM audio reader for track playback.</summary>
	PcmReader _pcmReader;

	/// <summary>Audio volume (0-255, scaled to 0-100%).</summary>
	uint8_t _volume = 100;

	/// <summary>Currently selected audio track number.</summary>
	uint16_t _trackSelect = 0;

	/// <summary>Temporary data pointer during seek operation.</summary>
	uint32_t _tmpDataPointer = 0;

	/// <summary>Current data file read position.</summary>
	uint32_t _dataPointer = 0;

	/// <summary>Base ROM name for locating associated MSU files.</summary>
	string _romName;

	/// <summary>Folder containing ROM and MSU files.</summary>
	string _romFolder;

	/// <summary>Path to current audio track .pcm file.</summary>
	string _trackPath;

	/// <summary>True to loop audio track when it ends.</summary>
	bool _repeat = false;

	/// <summary>True when audio playback is paused.</summary>
	bool _paused = false;

	/// <summary>Audio busy flag (always false - instant operations).</summary>
	bool _audioBusy = false;

	/// <summary>Data busy flag (always false - instant operations).</summary>
	bool _dataBusy = false;

	/// <summary>True when requested audio track file is not found.</summary>
	bool _trackMissing = false;

	/// <summary>Input stream for .msu data file.</summary>
	ifstream _dataFile;

	/// <summary>Size of currently loaded data file.</summary>
	uint32_t _dataSize;

	/// <summary>
	/// Loads an audio track file.
	/// </summary>
	/// <param name="startOffset">Offset to begin playback (default: 8 to skip header).</param>
	void LoadTrack(uint32_t startOffset = 8);

public:
	/// <summary>
	/// Creates a new MSU-1 instance.
	/// </summary>
	/// <param name="emu">Reference to emulator.</param>
	/// <param name="romFile">ROM file for path resolution.</param>
	/// <param name="spc">Reference to SPC for audio timing.</param>
	Msu1(Emulator* emu, VirtualFile& romFile, Spc* spc);

	/// <summary>Destructor - closes data file.</summary>
	~Msu1();

	/// <summary>
	/// Factory method to create MSU-1 if associated files exist.
	/// </summary>
	/// <param name="emu">Reference to emulator.</param>
	/// <param name="romFile">ROM file for path resolution.</param>
	/// <param name="spc">Reference to SPC for audio timing.</param>
	/// <returns>MSU-1 instance or nullptr if no MSU files found.</returns>
	static Msu1* Init(Emulator* emu, VirtualFile& romFile, Spc* spc);

	/// <summary>
	/// Writes to MSU-1 registers.
	/// </summary>
	/// <param name="addr">Register address ($2000-$2006).</param>
	/// <param name="value">Value to write.</param>
	void Write(uint16_t addr, uint8_t value);

	/// <summary>
	/// Reads from MSU-1 registers.
	/// </summary>
	/// <param name="addr">Register address ($2000 or $2007).</param>
	/// <returns>Status byte or data byte.</returns>
	uint8_t Read(uint16_t addr);

	/// <summary>
	/// Mixes MSU-1 PCM audio into the output buffer.
	/// </summary>
	/// <param name="buffer">Audio output buffer.</param>
	/// <param name="sampleCount">Number of samples to mix.</param>
	/// <param name="sampleRate">Target sample rate for resampling.</param>
	void MixAudio(int16_t* buffer, uint32_t sampleCount, uint32_t sampleRate) override;

	/// <summary>Serializes MSU-1 state for save states.</summary>
	void Serialize(Serializer& s) override;
};