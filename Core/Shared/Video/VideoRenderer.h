#pragma once
#include "pch.h"
#include <thread>
#include "Shared/SettingTypes.h"
#include "Shared/RenderedFrame.h"
#include "Shared/Interfaces/IRenderingDevice.h"
#include "Utilities/AutoResetEvent.h"
#include "Utilities/SimpleLock.h"
#include "Utilities/safe_ptr.h"

class IRenderingDevice;
class Emulator;
class SystemHud;
class DebugHud;
class InputHud;

class IVideoRecorder;
enum class VideoCodec;

/// <summary>
/// AVI recording configuration options.
/// </summary>
struct RecordAviOptions {
	VideoCodec Codec;          ///< Video compression codec (Raw/ZMBV/Camstudio)
	uint32_t CompressionLevel; ///< Codec-specific compression level (0-9)
	bool RecordSystemHud;      ///< Include system HUD (FPS/messages) in recording
	bool RecordInputHud;       ///< Include controller input display in recording
};

/// <summary>
/// Video rendering coordinator and HUD manager.
/// Manages rendering thread, HUD overlays, and video recording.
/// </summary>
/// <remarks>
/// Architecture:
/// - Dedicated render thread for non-blocking display updates
/// - Three HUD layers: Debugger HUD, System HUD (FPS/messages), Input HUD (controller display)
/// - Optional AVI recording with configurable codecs
///
/// Rendering pipeline:
/// 1. Receive filtered frame from VideoDecoder
/// 2. Apply HUD overlays (if enabled)
/// 3. Send to IRenderingDevice (OpenGL/D3D/SDL)
/// 4. Optionally encode frame to AVI recorder
///
/// HUD layers (rendered in order):
/// - DebugHud: Debugger information (memory viewer, CPU state, etc.)
/// - SystemHud: FPS counter, OSD messages, warnings
/// - InputHud: Controller button display (for recording/streaming)
/// - ScriptHud: Lua script drawings (arbitrary graphics)
///
/// Thread safety:
/// - _frameLock protects frame buffer access
/// - _hudLock protects HUD layer updates
/// - AutoResetEvent for efficient frame notifications
///
/// Video recording:
/// - safe_ptr<IVideoRecorder> for async recorder management
/// - Records post-filter, pre-HUD or post-HUD frames
/// - Supports multiple codecs: Raw, ZMBV, Camstudio LZSS
/// </remarks>
class VideoRenderer {
private:
	Emulator* _emu;

	AutoResetEvent _waitForRender;
	unique_ptr<std::thread> _renderThread;
	IRenderingDevice* _renderer = nullptr;
	atomic<bool> _stopFlag;
	SimpleLock _stopStartLock;

	uint32_t _rendererWidth = 512;
	uint32_t _rendererHeight = 480;

	unique_ptr<DebugHud> _rendererHud;
	unique_ptr<SystemHud> _systemHud;
	unique_ptr<InputHud> _inputHud;
	SimpleLock _hudLock;

	RenderSurfaceInfo _aviRecorderSurface = {};
	RecordAviOptions _recorderOptions = {};
	unique_ptr<DebugHud> _aviDebugHud;    ///< Persistent HUD for AVI recording (avoids per-frame alloc)
	unique_ptr<InputHud> _aviInputHud;    ///< Persistent input HUD for AVI recording (avoids per-frame alloc)

	RenderSurfaceInfo _emuHudSurface = {};
	RenderSurfaceInfo _scriptHudSurface = {};
	bool _needScriptHudClear = false;
	uint32_t _scriptHudScale = 2;
	uint32_t _lastScriptHudFrameNumber = 0;
	bool _needRedraw = true;

	RenderedFrame _lastFrame;
	SimpleLock _frameLock;

	safe_ptr<IVideoRecorder> _recorder;

	void RenderThread();
	bool DrawScriptHud(RenderedFrame& frame);

	FrameInfo GetEmuHudSize(FrameInfo baseFrameSize);

	void ProcessAviRecording(RenderedFrame& frame);

public:
	VideoRenderer(Emulator* emu);
	~VideoRenderer();

	FrameInfo GetRendererSize();
	void SetRendererSize(uint32_t width, uint32_t height);

	void SetScriptHudScale(uint32_t scale) { _scriptHudScale = scale; }
	std::pair<FrameInfo, OverscanDimensions> GetScriptHudSize();

	void StartThread();
	void StopThread();

	void UpdateFrame(RenderedFrame& frame);
	void ClearFrame();
	void RegisterRenderingDevice(IRenderingDevice* renderer);
	void UnregisterRenderingDevice(IRenderingDevice* renderer);

	void StartRecording(string filename, RecordAviOptions options);
	void AddRecordingSound(int16_t* soundBuffer, uint32_t sampleCount, uint32_t sampleRate);
	void StopRecording();
	bool IsRecording();
};
