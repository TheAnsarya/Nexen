#pragma once
#include "pch.h"
#include <thread>
#include "Utilities/AutoResetEvent.h"
#include "Utilities/SimpleLock.h"
#include "Utilities/Video/AviWriter.h"
#include "Utilities/Video/IVideoRecorder.h"

/// <summary>
/// AVI video recorder with threaded encoding and multiple codec support.
/// Implements IVideoRecorder for emulation video/audio capture to AVI format.
/// </summary>
/// <remarks>
/// Features:
/// - Threaded encoding (AviWriter runs on separate thread)
/// - Multiple codec support (Raw, Camstudio, ZMBV)
/// - Audio/video synchronization
/// - Configurable compression levels
/// 
/// Threading model:
/// - AddFrame() buffers frame and signals AviWriter thread
/// - AviWriter thread encodes and writes to disk
/// - AddSound() writes audio samples directly
/// 
/// Supported codecs (VideoCodec enum):
/// - Raw: Uncompressed (large files, fast)
/// - Camstudio: Lossless RLE
/// - ZMBV: Lossless (DosBox format, good compression)
/// 
/// Usage:
/// <code>
/// AviRecorder recorder(VideoCodec::ZMBV, 5);
/// recorder.Init("gameplay.avi");
/// recorder.StartRecording(256, 240, 32, 44100, 60.0);
/// // Record frames...
/// recorder.StopRecording();
/// </code>
/// </remarks>
class AviRecorder final : public IVideoRecorder {
private:
	std::thread _aviWriterThread;   ///< Background encoding thread

	unique_ptr<AviWriter> _aviWriter;  ///< AVI file writer

	string _outputFile;             ///< Output file path
	SimpleLock _lock;               ///< Thread synchronization lock
	AutoResetEvent _waitFrame;      ///< Frame ready signal

	atomic<bool> _stopFlag;         ///< Stop signal for thread
	atomic<bool> _framePending;     ///< Frame waiting for encoding

	bool _recording;                ///< Recording active flag
	std::unique_ptr<uint8_t[]> _frameBuffer;  ///< Frame data buffer
	uint32_t _frameBufferLength;    ///< Frame buffer size in bytes
	uint32_t _sampleRate;           ///< Audio sample rate

	double _fps;                    ///< Frames per second
	uint32_t _width;                ///< Video width
	uint32_t _height;               ///< Video height

	VideoCodec _codec;              ///< Video codec to use
	uint32_t _compressionLevel;     ///< Codec compression level (0-100)

public:
	/// <summary>
	/// Construct AVI recorder with codec settings.
	/// </summary>
	/// <param name="codec">Video codec (Raw, Camstudio, ZMBV)</param>
	/// <param name="compressionLevel">Compression level (0-100, codec-specific)</param>
	AviRecorder(VideoCodec codec, uint32_t compressionLevel);
	
	/// <summary>Destructor - stops recording and joins thread</summary>
	virtual ~AviRecorder();

	/// <summary>Initialize with output filename</summary>
	bool Init(string filename) override;
	
	/// <summary>Start recording with video/audio parameters</summary>
	bool StartRecording(uint32_t width, uint32_t height, uint32_t bpp, uint32_t audioSampleRate, double fps) override;
	
	/// <summary>Stop recording and finalize AVI file</summary>
	void StopRecording() override;

	/// <summary>
	/// Add video frame (buffered, signals encoding thread).
	/// </summary>
	/// <remarks>Copies frame to internal buffer, signals AviWriter thread for encoding.</remarks>
	bool AddFrame(void* frameBuffer, uint32_t width, uint32_t height, double fps) override;
	
	/// <summary>Add audio samples (written immediately)</summary>
	bool AddSound(int16_t* soundBuffer, uint32_t sampleCount, uint32_t sampleRate) override;

	/// <summary>Check if recording in progress</summary>
	bool IsRecording() override;
	
	/// <summary>Get output file path</summary>
	string GetOutputFile() override;
};