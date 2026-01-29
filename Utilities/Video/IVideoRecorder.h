#pragma once
#include "pch.h"

/// <summary>
/// Abstract interface for video recording implementations.
/// Supports emulation video/audio capture to various formats (AVI, GIF, etc.).
/// </summary>
/// <remarks>
/// Implementations:
/// - AviRecorder: AVI video with multiple codec options
/// - GifRecorder: Animated GIF format
/// 
/// Common workflow:
/// <code>
/// recorder->Init("output.avi");
/// recorder->StartRecording(256, 240, 32, 44100, 60.0);
/// // Each frame:
/// recorder->AddFrame(pixels, 256, 240, 60.0);
/// recorder->AddSound(samples, count, 44100);
/// // When done:
/// recorder->StopRecording();
/// </code>
/// 
/// Thread safety: Implementations may use threading internally.
/// </remarks>
class IVideoRecorder {
public:
	/// <summary>Virtual destructor for proper cleanup</summary>
	virtual ~IVideoRecorder() = default;

	/// <summary>
	/// Initialize recorder with output filename.
	/// </summary>
	/// <param name="filename">Output file path</param>
	/// <returns>True if initialization succeeded</returns>
	virtual bool Init(string filename) = 0;

	/// <summary>
	/// Start recording with specified parameters.
	/// </summary>
	/// <param name="width">Video width in pixels</param>
	/// <param name="height">Video height in pixels</param>
	/// <param name="bpp">Bits per pixel (typically 32 for RGBA)</param>
	/// <param name="audioSampleRate">Audio sample rate (Hz, typically 44100 or 48000)</param>
	/// <param name="fps">Frames per second (60.0 for NTSC, 50.0 for PAL)</param>
	/// <returns>True if recording started successfully</returns>
	virtual bool StartRecording(uint32_t width, uint32_t height, uint32_t bpp, uint32_t audioSampleRate, double fps) = 0;
	
	/// <summary>Stop recording and finalize output file</summary>
	virtual void StopRecording() = 0;

	/// <summary>
	/// Add video frame to recording.
	/// </summary>
	/// <param name="frameBuffer">Pixel data (RGBA format)</param>
	/// <param name="width">Frame width in pixels</param>
	/// <param name="height">Frame height in pixels</param>
	/// <param name="fps">Current frame rate</param>
	/// <returns>True if frame added successfully</returns>
	virtual bool AddFrame(void* frameBuffer, uint32_t width, uint32_t height, double fps) = 0;
	
	/// <summary>
	/// Add audio samples to recording.
	/// </summary>
	/// <param name="soundBuffer">16-bit PCM audio samples (interleaved stereo)</param>
	/// <param name="sampleCount">Number of samples (not bytes)</param>
	/// <param name="sampleRate">Sample rate in Hz</param>
	/// <returns>True if audio added successfully</returns>
	virtual bool AddSound(int16_t* soundBuffer, uint32_t sampleCount, uint32_t sampleRate) = 0;

	/// <summary>Check if currently recording</summary>
	/// <returns>True if recording in progress</returns>
	virtual bool IsRecording() = 0;
	
	/// <summary>Get output file path</summary>
	/// <returns>Path to output file</returns>
	virtual string GetOutputFile() = 0;
};