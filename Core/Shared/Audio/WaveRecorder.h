#include "pch.h"

/// <summary>
/// WAV file recorder for audio capture to disk.
/// Writes standard PCM WAV format with automatic header updates.
/// </summary>
/// <remarks>
/// WAV file format:
/// - RIFF header + fmt chunk + data chunk
/// - PCM 16-bit signed samples
/// - Mono or stereo support
/// - Sample rate configurable (typically 48000 Hz)
///
/// File lifecycle:
/// 1. Constructor creates file and writes initial header
/// 2. WriteSamples() appends sample data
/// 3. Destructor/CloseFile() updates header with final size
///
/// Thread safety:
/// - Not thread-safe, caller must synchronize
/// - Used via safe_ptr in SoundMixer for protection
/// </remarks>
class WaveRecorder {
private:
	std::ofstream _stream;
	uint32_t _streamSize;
	uint32_t _sampleRate;
	bool _isStereo;
	string _outputFile;

	void WriteHeader();
	void UpdateSizeValues();
	void CloseFile();

public:
	WaveRecorder(string outputFile, uint32_t sampleRate, bool isStereo);
	~WaveRecorder();

	bool WriteSamples(int16_t* samples, uint32_t sampleCount, uint32_t sampleRate, bool isStereo);
};