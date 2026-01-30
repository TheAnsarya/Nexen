#pragma once
#include "pch.h"
#include "Shared/SettingTypes.h"
#include "Shared/ControlDeviceState.h"

/// <summary>
/// Complete frame data packet containing rendered video, metadata, and input state.
/// Passed from emulation core to frontend for display and recording.
/// </summary>
/// <remarks>
/// Combines all information needed to display a frame and optionally record it:
/// - FrameBuffer: Rendered ARGB pixel data
/// - Data: Optional HD texture pack data
/// - Dimensions: Resolution and scaling factor
/// - FrameNumber: Monotonic counter for tracking
/// - VideoPhase: Interlaced video field (0/1) or progressive (0)
/// - InputData: Controller state for this frame (input display, movie recording)
///
/// Frame buffer format: 32-bit ARGB (0xAARRGGBB)
/// Width/Height: Native resolution * Scale (e.g., 256x240 → 512x480 at Scale=2.0)
/// </remarks>
struct RenderedFrame {
	/// <summary>Pointer to ARGB pixel data (caller owns memory)</summary>
	void* FrameBuffer = nullptr;

	/// <summary>Optional HD texture pack data (nullptr if not used)</summary>
	/// <remarks>Used by HD Pack feature for high-resolution replacements</remarks>
	void* Data = nullptr;

	/// <summary>Frame width in pixels (native resolution * scale)</summary>
	uint32_t Width = 256;

	/// <summary>Frame height in pixels (native resolution * scale)</summary>
	uint32_t Height = 240;

	/// <summary>Scaling factor applied to native resolution (1.0 = no scaling)</summary>
	/// <remarks>Common values: 1.0 (256x240), 2.0 (512x480), 3.0 (768x720)</remarks>
	double Scale = 1.0;

	/// <summary>Monotonic frame counter (increments every frame)</summary>
	/// <remarks>Used for movie sync, frame skip detection, performance metrics</remarks>
	uint32_t FrameNumber = 0;

	/// <summary>Video field indicator for interlaced systems (0=even, 1=odd, 0=progressive)</summary>
	/// <remarks>
	/// Most systems output progressive video (always 0).
	/// SNES interlaced mode (512x448) alternates between 0 and 1.
	/// </remarks>
	uint32_t VideoPhase = 0;

	/// <summary>Controller input state for this frame</summary>
	/// <remarks>
	/// Used for:
	/// - Input display overlay
	/// - Movie recording (.mmo files)
	/// - Network play synchronization
	/// Empty vector if input tracking is disabled.
	/// </remarks>
	vector<ControllerData> InputData;

	/// <summary>
	/// Default constructor - initializes to 256x240 resolution with null buffers.
	/// </summary>
	RenderedFrame() {}

	/// <summary>
	/// Construct rendered frame with basic parameters (no input data).
	/// </summary>
	/// <param name="buffer">Pointer to ARGB pixel data</param>
	/// <param name="width">Frame width in pixels</param>
	/// <param name="height">Frame height in pixels</param>
	/// <param name="scale">Scaling factor (default 1.0)</param>
	/// <param name="frameNumber">Frame counter value (default 0)</param>
	RenderedFrame(void* buffer, uint32_t width, uint32_t height, double scale = 1.0, uint32_t frameNumber = 0) : FrameBuffer(buffer),
	                                                                                                             Data(nullptr),
	                                                                                                             Width(width),
	                                                                                                             Height(height),
	                                                                                                             Scale(scale),
	                                                                                                             FrameNumber(frameNumber),
	                                                                                                             InputData({}) {}

	/// <summary>
	/// Construct rendered frame with full parameters including input data and video phase.
	/// </summary>
	/// <param name="buffer">Pointer to ARGB pixel data</param>
	/// <param name="width">Frame width in pixels</param>
	/// <param name="height">Frame height in pixels</param>
	/// <param name="scale">Scaling factor</param>
	/// <param name="frameNumber">Frame counter value</param>
	/// <param name="inputData">Controller input state for this frame</param>
	/// <param name="videoPhase">Interlaced field indicator (default 0=progressive)</param>
	RenderedFrame(void* buffer, uint32_t width, uint32_t height, double scale, uint32_t frameNumber, vector<ControllerData> inputData, uint32_t videoPhase = 0) : FrameBuffer(buffer),
	                                                                                                                                                              Data(nullptr),
	                                                                                                                                                              Width(width),
	                                                                                                                                                              Height(height),
	                                                                                                                                                              Scale(scale),
	                                                                                                                                                              FrameNumber(frameNumber),
	                                                                                                                                                              VideoPhase(videoPhase),
	                                                                                                                                                              InputData(inputData) {}
};
