#pragma once

#include "pch.h"
#include "Shared/SettingTypes.h"

struct RenderedFrame;

/// <summary>
/// Render surface for HUD overlays and script drawings.
/// Manages ARGB pixel buffer with automatic resizing and dirty tracking.
/// </summary>
/// <remarks>
/// Used for:
/// - Emulator HUD (FPS counter, messages, warnings)
/// - Script HUD (Lua drawings, debug visualizations)
/// - Input display overlay (controller button indicators)
///
/// Pixel format: 32-bit ARGB (0xAARRGGBB)
/// Dirty flag optimizes rendering (skip unchanged surfaces)
/// Automatic memory management in destructor
/// </remarks>
struct RenderSurfaceInfo {
	std::unique_ptr<uint32_t[]> Buffer; ///< ARGB pixel data (managed by unique_ptr)
	uint32_t Width = 0;                 ///< Surface width in pixels
	uint32_t Height = 0;                ///< Surface height in pixels
	bool IsDirty = true;                ///< True if surface changed since last render

	/// <summary>
	/// Resize surface if dimensions changed.
	/// </summary>
	/// <param name="width">New width in pixels</param>
	/// <param name="height">New height in pixels</param>
	/// <returns>True if buffer was reallocated</returns>
	/// <remarks>
	/// Deletes old buffer and allocates new one if size differs.
	/// Clears buffer to transparent black after reallocation.
	/// </remarks>
	bool UpdateSize(uint32_t width, uint32_t height) {
		if (Width != width || Height != height) {
			Buffer = std::make_unique<uint32_t[]>(height * width);
			Width = width;
			Height = height;
			Clear();
			return true;
		}
		return false;
	}

	/// <summary>
	/// Clear surface to transparent black (0x00000000).
	/// </summary>
	void Clear() {
		memset(Buffer.get(), 0, Width * Height * sizeof(uint32_t));
		IsDirty = true;
	}

	/// <summary>
	/// Destructor - frees pixel buffer.
	/// </summary>
	~RenderSurfaceInfo() = default;
};

/// <summary>
/// Interface for platform-specific rendering backends.
/// Implemented by OpenGL, Direct3D, SDL, and Vulkan renderers.
/// </summary>
/// <remarks>
/// Implementations:
/// - OpenGL: Cross-platform (Windows/Linux/macOS)
/// - Direct3D 11: Windows native, best performance on Windows
/// - SDL Renderer: Software/hardware fallback
/// - Vulkan: High-performance modern API (planned)
///
/// Rendering pipeline:
/// 1. UpdateFrame() receives filtered video frame from VideoDecoder
/// 2. Render() composites frame + HUD layers + script overlays
/// 3. Backend swaps buffers and presents to screen
///
/// Thread model:
/// - All methods called from render thread (VideoRenderer)
/// - OnRendererThreadStarted() for thread-local initialization (OpenGL contexts)
/// - UpdateFrame() may be called without subsequent Render() (frame skipping)
///
/// HUD composition order:
/// 1. Emulated video frame (base layer)
/// 2. Emulator HUD (FPS, messages) - emuHud parameter
/// 3. Script HUD (Lua drawings) - scriptHud parameter
/// </remarks>
class IRenderingDevice {
public:
	virtual ~IRenderingDevice() {}

	/// <summary>
	/// Update video frame buffer for next render.
	/// </summary>
	/// <param name="frame">Filtered video frame from VideoDecoder</param>
	/// <remarks>
	/// Frame buffer lifetime:
	/// - Caller owns memory (VideoDecoder)
	/// - Must copy/upload to GPU immediately (buffer may be reused)
	/// - Called before every Render() or during frame skip
	/// </remarks>
	virtual void UpdateFrame(RenderedFrame& frame) = 0;

	/// <summary>
	/// Clear frame buffer to black (used when paused/stopped).
	/// </summary>
	virtual void ClearFrame() = 0;

	/// <summary>
	/// Composite and render frame with HUD overlays.
	/// </summary>
	/// <param name="emuHud">Emulator HUD surface (FPS, messages)</param>
	/// <param name="scriptHud">Lua script drawing surface</param>
	/// <remarks>
	/// Rendering order:
	/// 1. Draw video frame (from last UpdateFrame)
	/// 2. Blend emuHud on top (if dirty)
	/// 3. Blend scriptHud on top (if dirty)
	/// 4. Present to screen
	/// </remarks>
	virtual void Render(RenderSurfaceInfo& emuHud, RenderSurfaceInfo& scriptHud) = 0;

	/// <summary>
	/// Reset renderer state (clear caches, reinitialize).
	/// </summary>
	virtual void Reset() = 0;

	/// <summary>
	/// Callback when render thread starts (for thread-local initialization).
	/// </summary>
	/// <remarks>
	/// Used by OpenGL for context creation (contexts are thread-local).
	/// Optional - default implementation does nothing.
	/// </remarks>
	virtual void OnRendererThreadStarted() {}

	/// <summary>
	/// Enter/exit exclusive fullscreen mode.
	/// </summary>
	/// <param name="fullscreen">True to enter fullscreen, false for windowed</param>
	/// <param name="windowHandle">Platform window handle (HWND on Windows, etc.)</param>
	virtual void SetExclusiveFullscreenMode(bool fullscreen, void* windowHandle) = 0;
};