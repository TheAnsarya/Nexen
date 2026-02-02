# Video Rendering Subsystem Documentation

This document covers Nexen's video rendering, filtering, and recording subsystem.

---

## Architecture Overview

```text
Video Rendering Pipeline
════════════════════════

┌─────────────────────────────────────────────────────────────────┐
│					Per-Console PPU							  │
│   (NES PPU, SNES PPU, GB PPU, etc.)							│
└─────────────────────────────────────────────────────────────────┘
			  │
			  │ Raw framebuffer (native resolution)
			  ▼
┌─────────────────────────────────────────────────────────────────┐
│					 VideoDecoder								│
│   (Decode, color conversion, aspect ratio)					  │
└─────────────────────────────────────────────────────────────────┘
			  │
			  │ Decoded frame
			  ▼
┌─────────────────────────────────────────────────────────────────┐
│				   Video Filters								 │
│   (NTSC, Scale, HQ2x, Scanlines, Rotate)					   │
└─────────────────────────────────────────────────────────────────┘
			  │
			  │ Filtered frame
			  ▼
┌─────────────────────────────────────────────────────────────────┐
│					VideoRenderer								│
│   (HUD overlays, rendering coordination)						│
└─────────────────────────────────────────────────────────────────┘
			  │
	┌─────────┼─────────┬──────────────┐
	│		 │		 │			  │
	▼		 ▼		 ▼			  ▼
┌───────┐ ┌───────┐ ┌─────────┐ ┌───────────┐
│DebugH │ │SystemH│ │ InputH  │ │  AVI	  │
│  UD   │ │  UD   │ │   UD	│ │ Recorder  │
└───────┘ └───────┘ └─────────┘ └───────────┘
			  │
			  ▼
┌─────────────────────────────────────────────────────────────────┐
│				   IRenderingDevice							  │
│   (OpenGL, Direct3D, SDL - platform display)				   │
└─────────────────────────────────────────────────────────────────┘
```

## Directory Structure

```text
Core/Shared/Video/
├── VideoRenderer.h/cpp	  - Render coordinator, HUD management
├── VideoDecoder.h/cpp	   - Frame decoding, format conversion
├── SoftwareRenderer.h/cpp   - CPU-based rendering backend
│
├── Filters
│   ├── BaseVideoFilter.h/cpp   - Filter base class
│   ├── ScaleFilter.h/cpp	   - Integer scaling
│   ├── RotateFilter.h/cpp	  - Screen rotation
│   ├── ScanlineFilter.h		- Scanline overlay
│   └── GenericNtscFilter.h	 - NTSC simulation
│
├── HUD Components
│   ├── DebugHud.h/cpp		  - Debugger overlays
│   ├── SystemHud.h/cpp		 - FPS, messages
│   └── DebugStats.h/cpp		- Performance stats
│
└── Drawing
	├── DrawCommand.h		   - Base draw command
	├── DrawPixelCommand.h	  - Single pixel
	├── DrawLineCommand.h	   - Lines
	├── DrawRectangleCommand.h  - Rectangles
	├── DrawStringCommand.h/cpp - Text rendering
	└── DrawScreenBufferCommand.h - Buffer blits
```

---

## Core Components

### VideoRenderer (VideoRenderer.h)

Central rendering coordinator.

**Key Responsibilities:**

- Dedicated render thread
- HUD overlay management
- Video recording coordination
- Frame rate limiting

**Rendering Thread:**

```cpp
void RenderThread() {
	while (!_stopFlag) {
		_waitForRender.Wait();
		
		// 1. Get filtered frame
		// 2. Apply HUD overlays
		// 3. Send to display device
		// 4. Record if enabled
	}
}
```

**HUD Layers (render order):**

1. **DebugHud** - Debugger information
2. **SystemHud** - FPS, messages, warnings
3. **InputHud** - Controller display
4. **ScriptHud** - Lua script drawings

### VideoDecoder (VideoDecoder.h)

Frame processing and filter application.

**Processing Pipeline:**

```cpp
void DecodeFrame(uint16_t* rawFrame, uint32_t width, uint32_t height) {
	// 1. Convert palette (if needed)
	// 2. Apply video filter
	// 3. Handle overscan
	// 4. Pass to renderer
}
```

**Features:**

- Palette conversion (NES/GB specific)
- Overscan cropping
- Aspect ratio correction

---

## Video Filters

### Available Filters

| Filter | Type | Description |
| -------- | ------ | ------------- |
| **None** | Passthrough | No filtering |
| **NTSC** | Color | Authentic CRT simulation |
| **Scale2x-4x** | Integer | Sharp pixel scaling |
| **HQ2x-4x** | Interpolation | Smooth upscaling |
| **xBRZ** | Interpolation | High-quality scaling |
| **Scanlines** | Effect | CRT scanline overlay |
| **Prescale** | Integer | Pre-scale before other filters |

### NTSC Filter (GenericNtscFilter.h)

Simulates NTSC composite video artifacts.

**Effects:**

- Color fringing
- Dot crawl
- Signal blurring
- Chroma/luma separation

**Configuration:**

```cpp
struct NtscFilterSettings {
	double Hue = 0.0;
	double Saturation = 0.0;
	double Contrast = 0.0;
	double Brightness = 0.0;
	double Sharpness = 0.0;
	bool MergeFields = false;
};
```

### Scale Filters (ScaleFilter.h)

Integer scaling algorithms.

**Algorithms:**

- **Scale2x/3x/4x**: Pattern-based edge detection
- **HQ2x/3x/4x**: Sophisticated color blending
- **xBRZ**: Modern high-quality scaler

### Scanline Filter (ScanlineFilter.h)

Simulates CRT scanlines.

**Configuration:**

```cpp
struct ScanlineSettings {
	uint8_t Intensity = 50;	// 0-100%
	bool OddLines = true;	  // Darken odd or even lines
};
```

### Rotation Filter (RotateFilter.h)

Screen rotation for vertical games.

**Options:**

- 0° (normal)
- 90° (clockwise)
- 180° (upside down)
- 270° (counter-clockwise)

---

## HUD System

### SystemHud (SystemHud.h)

On-screen display for system information.

**Components:**

- FPS counter
- OSD messages (game load, state save, etc.)
- Warning indicators
- Turbo mode indicator

**Configuration:**

```cpp
struct SystemHudSettings {
	bool ShowFps = true;
	bool ShowMessages = true;
	Position FpsPosition = TopRight;
	uint32_t MessageDuration = 3000; // ms
};
```

### DebugHud (DebugHud.h)

Debugger visualization overlays.

**Features:**

- Memory viewer overlay
- CPU state display
- Breakpoint indicators
- Watch values

### InputHud (InputHud.h)

Controller input visualization.

**Features:**

- Button press display
- Per-controller overlays
- Configurable position/opacity
- Ideal for streaming/recording

---

## Video Recording

### Supported Formats

| Format | Extension | Codec | Notes |
| -------- | ----------- | ------- | ------- |
| **AVI** | .avi | Raw/ZMBV | Lossless, large files |
| **GIF** | .gif | LZW | Animated, limited colors |

### Recording Options

```cpp
struct RecordAviOptions {
	VideoCodec Codec;		   // Raw, ZMBV, Camstudio
	uint32_t CompressionLevel;  // 0-9
	bool RecordSystemHud;	   // Include FPS/messages
	bool RecordInputHud;		// Include controller display
};
```

### Codecs

| Codec | Description | Size | Speed |
| ------- | ------------- | ------ | ------- |
| **Raw** | Uncompressed | Huge | Fast |
| **ZMBV** | Zip Motion Block | Small | Medium |
| **Camstudio** | LZSS compression | Medium | Fast |

### Recording API

```cpp
// Start recording
RecordAviOptions options;
options.Codec = VideoCodec::ZMBV;
options.CompressionLevel = 6;
videoRenderer->StartRecording("gameplay.avi", options);

// Stop recording
videoRenderer->StopRecording();
```

---

## Screenshots

### Screenshot Features

- PNG format (lossless)
- Optional HUD inclusion
- Filename patterns (date, ROM name)
- Configurable output folder

### API

```cpp
// Take screenshot
void TakeScreenshot(string filename, bool includeHud = false);

// Auto-named screenshot
void TakeScreenshot();  // Uses configured pattern
```

---

## Resolution and Aspect Ratio

### Native Resolutions

| System | Resolution | Aspect Ratio |
| -------- | ------------ | -------------- |
| NES | 256×240 | 4:3 (with overscan) |
| SNES | 256×224 / 512×448 | 4:3 (with overscan) |
| Game Boy | 160×144 | ~10:9 |
| GBA | 240×160 | 3:2 |
| SMS | 256×192 | 4:3 |
| PC Engine | 256×224 | 4:3 |
| WonderSwan | 224×144 | ~14:9 |

### Aspect Ratio Options

```cpp
enum class AspectRatio {
	NoStretching,	// Native pixels
	Auto,			// System default
	NTSC,			// 8:7 pixel aspect
	PAL,			 // Different aspect
	Standard,		// 4:3
	Widescreen	   // 16:9
};
```

### Overscan Handling

```cpp
struct OverscanSettings {
	uint8_t Top = 8;	  // Pixels to crop
	uint8_t Bottom = 8;
	uint8_t Left = 0;
	uint8_t Right = 0;
};
```

---

## Thread Safety

### Frame Buffer Access

```cpp
// Protected frame access
SimpleLock _frameLock;

void UpdateFrame(RenderedFrame& frame) {
	auto lock = _frameLock.AcquireSafe();
	// Update frame buffer
}
```

### HUD Updates

```cpp
// Protected HUD access
SimpleLock _hudLock;

void DrawHudElement(/* ... */) {
	auto lock = _hudLock.AcquireSafe();
	// Update HUD
}
```

---

## Performance Considerations

### V-Sync

**Enabled:**

- Smooth display
- No tearing
- Adds latency (~16ms)

**Disabled:**

- Lower latency
- May cause tearing
- Higher CPU usage

### Frame Skipping

```cpp
struct FrameSkipSettings {
	uint8_t FrameSkip = 0;	  // Skip N frames
	bool AutoFrameSkip = false;  // Dynamic skipping
};
```

### Rendering Backend

| Backend | Platform | Features |
| --------- | ---------- | ---------- |
| OpenGL | Cross-platform | Shaders, hardware scaling |
| Direct3D | Windows | Low latency, Windows native |
| SDL | Cross-platform | Simple, portable |

---

## Shader Support

### Custom Shaders

GLSL shaders for post-processing effects.

**Built-in Shaders:**

- CRT simulation
- LCD simulation
- Retro effects

**Shader API:**

```cpp
void LoadShader(string filename);
void SetShaderParameter(string name, float value);
```

---

## Related Documentation

- [ARCHITECTURE-OVERVIEW.md](ARCHITECTURE-OVERVIEW.md) - Overall structure
- [NES-CORE.md](NES-CORE.md) - NES PPU details
- [SNES-CORE.md](SNES-CORE.md) - SNES PPU details
- [GB-GBA-CORE.md](GB-GBA-CORE.md) - GB/GBA PPU details
- [DEBUGGER.md](DEBUGGER.md) - Debugger HUD features
