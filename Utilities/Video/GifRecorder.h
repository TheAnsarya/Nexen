#pragma once
#include "pch.h"
#include "Utilities/Video/IVideoRecorder.h"

struct GifWriter;

/// <summary>
/// Animated GIF recorder for emulation capture.
/// Implements IVideoRecorder for lightweight animated GIF output.
/// </summary>
/// <remarks>
/// GIF format features:
/// - Animated GIF output (256 colors max per frame)
/// - Lossless compression
/// - Widely compatible format
/// - Smaller file size than uncompressed video
///
/// Limitations:
/// - 256 color palette limit (quantized from true color)
/// - No audio support (AddSound() is no-op)
/// - Larger than video codecs for long recordings
///
/// Use cases:
/// - Short gameplay clips for sharing
/// - Social media posts
/// - Bug reports with visual reproduction
///
/// Common workflow:
/// <code>
/// GifRecorder recorder;
/// recorder.Init("clip.gif");
/// recorder.StartRecording(256, 240, 32, 0, 60.0);
/// // Record frames...
/// recorder.StopRecording();  // Finalizes GIF
/// </code>
///
/// Frame delay calculated from FPS for smooth playback.
/// </remarks>
class GifRecorder final : public IVideoRecorder {
private:
	std::unique_ptr<GifWriter> _gif; ///< GIF writer instance
	bool _recording = false;         ///< Recording active flag
	uint32_t _frameCounter = 0;      ///< Frame count
	string _outputFile;              ///< Output file path
	uint32_t _width = 0;             ///< GIF width
	uint32_t _height = 0;            ///< GIF height
	double _fps = 0;                 ///< Frames per second

public:
	/// <summary>Construct GIF recorder</summary>
	GifRecorder();

	/// <summary>Destructor - stops recording and finalizes GIF</summary>
	virtual ~GifRecorder();

	/// <summary>Initialize with output filename</summary>
	bool Init(string filename) override;

	/// <summary>Start recording with video parameters</summary>
	/// <remarks>audioSampleRate ignored - GIF has no audio support</remarks>
	bool StartRecording(uint32_t width, uint32_t height, uint32_t bpp, uint32_t audioSampleRate, double fps) override;

	/// <summary>Stop recording and finalize GIF file</summary>
	void StopRecording() override;

	/// <summary>
	/// Add video frame to GIF.
	/// </summary>
	/// <remarks>
	/// Frame data is quantized to 256 colors using palette generation.
	/// Frame delay calculated from FPS for accurate timing.
	/// </remarks>
	bool AddFrame(void* frameBuffer, uint32_t width, uint32_t height, double fps) override;

	/// <summary>
	/// No-op - GIF format does not support audio.
	/// </summary>
	/// <returns>Always returns true</returns>
	bool AddSound(int16_t* soundBuffer, uint32_t sampleCount, uint32_t sampleRate) override;

	/// <summary>Check if recording in progress</summary>
	bool IsRecording() override;

	/// <summary>Get output file path</summary>
	string GetOutputFile() override;
};