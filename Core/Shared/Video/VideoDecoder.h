#pragma once
#include "pch.h"
#include "Utilities/SimpleLock.h"
#include "Utilities/AutoResetEvent.h"
#include "Shared/SettingTypes.h"
#include "Shared/RenderedFrame.h"

class BaseVideoFilter;
class ScaleFilter;
class RotateFilter;
class IRenderingDevice;
class Emulator;

/// <summary>
/// Asynchronous video frame decoder and filter processor.
/// Applies video filters (NTSC, scanlines, scale, rotation) on dedicated thread.
/// </summary>
/// <remarks>
/// Architecture:
/// - Dedicated decode thread for parallel processing
/// - Frame queue with AutoResetEvent synchronization
/// - Filter pipeline: BaseVideoFilter → ScaleFilter → RotateFilter
///
/// Supported filters:
/// - NTSC composite video simulation (Generic/SNES/NES)
/// - Scanlines (horizontal/vertical)
/// - Scaling (2x/3x/4x/etc. with various algorithms)
/// - Rotation (90/180/270 degrees)
///
/// Thread safety:
/// - _stopStartLock protects thread lifecycle
/// - Atomic flags for frame change notifications
/// - AutoResetEvent for efficient wait/notify
///
/// Performance:
/// - Asynchronous decoding doesn't block emulation thread
/// - Filters can be swapped at runtime (hot-reload)
/// - Synchronous mode available for debugging/screenshots
/// </remarks>
class VideoDecoder {
private:
	Emulator* _emu;

	ConsoleType _consoleType = ConsoleType::Snes;

	unique_ptr<thread> _decodeThread;

	SimpleLock _stopStartLock;
	AutoResetEvent _waitForFrame;

	atomic<bool> _frameChanged;
	atomic<bool> _stopFlag;
	uint32_t _frameCount = 0;
	bool _forceFilterUpdate = false;

	double _lastAspectRatio = 0.0;

	FrameInfo _baseFrameSize = {};
	FrameInfo _lastFrameSize = {};
	RenderedFrame _frame = {};

	VideoFilterType _videoFilterType = VideoFilterType::None;
	unique_ptr<BaseVideoFilter> _videoFilter;
	unique_ptr<ScaleFilter> _scaleFilter;
	unique_ptr<RotateFilter> _rotateFilter;

	void UpdateVideoFilter();

	void DecodeThread();

public:
	VideoDecoder(Emulator* console);
	~VideoDecoder();

	void Init();

	void DecodeFrame(bool synchronous = false);
	void TakeScreenshot();
	void TakeScreenshot(std::stringstream& stream);

	void ForceFilterUpdate() { _forceFilterUpdate = true; }

	uint32_t GetFrameCount();
	FrameInfo GetBaseFrameInfo(bool removeOverscan);
	FrameInfo GetFrameInfo();
	[[nodiscard]] double GetLastFrameScale() { return _frame.Scale; }

	void UpdateFrame(RenderedFrame frame, bool sync, bool forRewind);

	void WaitForAsyncFrameDecode();

	bool IsRunning();
	void StartThread();
	void StopThread();
};
