#pragma once
#include "pch.h"
#include "Utilities/SimpleLock.h"
#include "Shared/SettingTypes.h"

class Emulator;

/// <summary>
/// Base class for all video filters - handles PPU output to RGB conversion.
/// Provides NTSC filtering, color adjustments, and screenshot capture.
/// </summary>
/// <remarks>
/// **Video Pipeline:**
/// 1. Console PPU outputs native format (15-bit RGB, indexed palette, etc.)
/// 2. VideoFilter converts to 32-bit ARGB for display
/// 3. Applies color adjustments (brightness, contrast, saturation)
/// 4. Optionally applies NTSC filter for authentic CRT look
/// 5. Output sent to VideoRenderer for display
///
/// **Filter Types:**
/// - None: Direct color conversion only
/// - NTSC: Blargg's NTSC filter (composite/S-Video/RGB simulation)
/// - Scale2x/3x/4x: Pixel-art scaling algorithms
/// - HQ2x/3x/4x: High-quality magnification
/// - xBRZ: Edge-detection based upscaling
/// - Prescale: Simple integer scaling
///
/// **Color Space:**
/// - Internal: YIQ for NTSC color simulation
/// - Output: 32-bit ARGB (0xAARRGGBB)
/// - Supports hue/saturation adjustments
///
/// **Overscan:**
/// - Configurable per-console and per-game
/// - Crops edges to hide glitches (common in NES games)
/// - SetOverscan/GetOverscan manage cropping dimensions
///
/// **Screenshot:**
/// - TakeScreenshot() captures filtered output
/// - Supports PNG output to file or stream
///
/// **Threading:**
/// - _frameLock protects output buffer
/// - SendFrame() called from emulation thread
/// - Output buffer read from render thread
/// </remarks>
class BaseVideoFilter {
private:
	std::unique_ptr<uint32_t[]> _outputBuffer;
	double _yiqToRgbMatrix[6] = {};
	uint32_t _bufferSize = 0;
	SimpleLock _frameLock;
	OverscanDimensions _overscan = {};
	bool _isOddFrame = false;
	uint32_t _videoPhaseOffset = 0;

	void UpdateBufferSize();

protected:
	Emulator* _emu = nullptr;
	FrameInfo _baseFrameInfo = {};
	FrameInfo _frameInfo = {};
	void* _frameData = nullptr;
	uint16_t* _ppuOutputBuffer = nullptr;

	void InitConversionMatrix(double hueShift, double saturationShift);
	void ApplyColorOptions(uint8_t& r, uint8_t& g, uint8_t& b, double brightness, double contrast);
	void RgbToYiq(double r, double g, double b, double& y, double& i, double& q);
	void YiqToRgb(double y, double i, double q, double& r, double& g, double& b);

	virtual void ApplyFilter(uint16_t* ppuOutputBuffer) = 0;
	virtual void OnBeforeApplyFilter();
	[[nodiscard]] bool IsOddFrame();
	[[nodiscard]] uint32_t GetVideoPhaseOffset();
	[[nodiscard]] uint32_t GetBufferSize();

protected:
	virtual FrameInfo GetFrameInfo();

public:
	BaseVideoFilter(Emulator* emu);
	virtual ~BaseVideoFilter();

	template <typename T>
	bool NtscFilterOptionsChanged(T& ntscSetup, VideoConfig& cfg);
	template <typename T>
	static void InitNtscFilter(T& ntscSetup, VideoConfig& cfg);

	[[nodiscard]] uint32_t* GetOutputBuffer();
	FrameInfo SendFrame(uint16_t* ppuOutputBuffer, uint32_t frameNumber, uint32_t videoPhaseOffset, void* frameData, bool enableOverscan = true);
	void TakeScreenshot(const string& romName, VideoFilterType filterType);
	void TakeScreenshot(VideoFilterType filterType, string filename, std::stringstream* stream = nullptr);

	[[nodiscard]] virtual HudScaleFactors GetScaleFactor() { return {1.0, 1.0}; }
	[[nodiscard]] virtual OverscanDimensions GetOverscan();
	void SetOverscan(OverscanDimensions dimensions);
	[[nodiscard]] FrameInfo GetFrameInfo(uint16_t* ppuOutputBuffer, bool enableOverscan);

	void SetBaseFrameInfo(FrameInfo frameInfo);
};
