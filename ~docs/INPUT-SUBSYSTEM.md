# Input and Controller Subsystem Documentation

This document covers Nexen's input handling and controller emulation subsystem.

---

## Architecture Overview

```text
Input Architecture
══════════════════

┌─────────────────────────────────────────────────────────────────┐
│                     KeyManager                                 │
│   (Physical input → key codes, handles keyboard/gamepad)      │
└─────────────────────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  BaseControlManager                           │
│   (Per-console manager, owns all controllers)                │
└─────────────────────────────────────────────────────────────────┘
              │
    ┌─────────┼─────────┬──────────────┬──────────────┐
    │        │       │            │           │
    ▼        ▼       ▼            ▼           ▼
┌───────┐ ┌───────┐ ┌─────────┐ ┌───────────┐ ┌───────────┐
│Port 1 │ │Port 2 │ │ Multitap│ │  System   │ │  Input  │
│ Ctrl  │ │ Ctrl  │ │ Adapter │ │  Devices  │ │  Recorder │
└───────┘ └───────┘ └─────────┘ └───────────┘ └───────────┘
    │
    └── BaseControlDevice
        ├── StandardController
        ├── Zapper (Light Gun)
        ├── Mouse
        ├── Keyboard
        └── ControllerHub (Multitap)
```

## Directory Structure

```text
Core/Shared/
├── Input Core
│   ├── BaseControlManager.h/cpp   - Console input manager
│   ├── BaseControlDevice.h/cpp - Controller base class
│   ├── ControlDeviceState.h       - Input state structure
│   ├── ControllerHub.h         - Multitap adapter base
│   └── IControllerHub.h           - Hub interface
│
├── Key Management
│   ├── KeyManager.h/cpp           - Physical input handling
│   ├── KeyDefinitions.h           - Key code definitions
│   └── ShortcutKeyHandler.h/cpp   - Hotkey handling
│
├── Input Display
│   └── InputHud.h/cpp           - On-screen input display
│
└── Platform-Specific (Core/{Platform}/Input/)
    ├── NES/  - NES controllers
    ├── SNES/ - SNES controllers
    ├── GB/   - Game Boy input
    ├── GBA/  - GBA input
    ├── SMS/  - SMS/GG controllers
    ├── PCE/  - PC Engine controllers
    └── WS/   - WonderSwan input
```

---

## Core Components

### BaseControlManager (BaseControlManager.h)

Abstract base class for console-specific input managers.

**Key Responsibilities:**

- Owns and manages all connected controllers
- Coordinates input polling
- Handles input recording/playback
- Detects lag frames

**Controller Management:**

```cpp
class BaseControlManager : public ISerializable {
protected:
    vector<shared_ptr<BaseControlDevice>> _controlDevices; // Controllers
    vector<shared_ptr<BaseControlDevice>> _systemDevices;  // Power, reset
    
    // Input providers/recorders for movies and netplay
    vector<IInputRecorder*> _inputRecorders;
    vector<IInputProvider*> _inputProviders;
    
    uint32_t _pollCounter = 0;  // Total input polls
    uint32_t _lagCounter = 0;    // Lag frame counter
    bool _wasInputRead = false;   // Input polled this frame
};
```

**Lag Frame Detection:**

```cpp
// A frame is "lag" if no input was polled
void ProcessEndOfFrame() {
    if (!_wasInputRead) {
        _lagCounter++;
    }
    _wasInputRead = false;
}
```

**Platform Implementations:**

- `NesControlManager` - NES/Famicom
- `SnesControlManager` - SNES/Super Famicom
- `GameboyControlManager` - Game Boy family
- `GbaControlManager` - Game Boy Advance
- `SmsControlManager` - SMS/Game Gear
- `PceControlManager` - PC Engine
- `WsControlManager` - WonderSwan

### BaseControlDevice (BaseControlDevice.h)

Base class for all input devices.

**Key Responsibilities:**

- Input state management
- Key mapping
- Hardware I/O emulation
- Serialization

**State Structure:**

```cpp
struct ControlDeviceState {
    vector<uint8_t> State;    // Button state bits
    MouseMovement Movement;   // Mouse delta (if applicable)
};
```

**Features:**

- Button state tracking (bit flags)
- Turbo/autofire support
- Coordinate input (light gun, mouse)
- Key mapping configuration

### KeyManager (KeyManager.h)

Handles physical input devices.

**Features:**

- Keyboard input
- Gamepad/joystick input
- Key binding management
- Hotkey detection

---

## Controller Types

### Standard Controllers

| Platform | Controller | Buttons |
| ---------- | ------------ | --------- |
| NES | NES Controller | A, B, Select, Start, D-Pad |
| NES | Four Score | 4-player adapter |
| SNES | SNES Controller | A, B, X, Y, L, R, Select, Start, D-Pad |
| SNES | Multitap | 4-player adapter |
| GB/GBC | Game Boy | A, B, Select, Start, D-Pad |
| GBA | GBA | A, B, L, R, Select, Start, D-Pad |
| SMS | Master System | 1, 2, D-Pad |
| PCE | TurboGrafx | I, II, Select, Run, D-Pad |
| WS | WonderSwan | A, B, X1-4, Y1-4, Start |

### Special Controllers

| Type | Platform | Description |
| ------ | ---------- | ------------- |
| **Zapper** | NES | Light gun |
| **Super Scope** | SNES | Light gun |
| **SNES Mouse** | SNES | 2-button mouse |
| **Power Pad** | NES | Dance mat |
| **Arkanoid Paddle** | NES | Analog paddle |
| **Keyboard** | NES/SNES | Full keyboard |

---

## Input Flow

### Frame Input Cycle

```text
1. Frame Start
   └── Input providers set state (movies, netplay)

2. Controller Polling
   └── Game reads controller port
       └── SetInputReadFlag() called
           └── _wasInputRead = true

3. Input Recording
   └── Input recorders capture state

4. Frame End
   └── ProcessEndOfFrame()
       └── Check if lag frame (_wasInputRead?)
```

### Input Provider Interface

```cpp
class IInputProvider {
public:
    // Called to set controller input for frame
    virtual bool SetInput(BaseControlDevice* device) = 0;
};
```

**Providers:**

- `MovieManager` - Movie playback
- `RewindManager` - Rewind input
- `NetplayClient` - Network play

### Input Recorder Interface

```cpp
class IInputRecorder {
public:
    // Called to record controller input
    virtual void RecordInput(vector<shared_ptr<BaseControlDevice>>& devices) = 0;
};
```

**Recorders:**

- `MovieRecorder` - Movie recording
- `NetplayServer` - Network broadcast

---

## Turbo/Autofire

Turbo toggles button state at configurable rate.

**Configuration:**

```cpp
// In EmuSettings
uint32_t TurboSpeed = 2;  // Toggle every N frames
```

**Implementation:**

- Button held + turbo enabled → auto-toggle
- Per-button turbo settings
- Configurable toggle rate

---

## Light Gun Support

### Zapper (NES)

**Input:**

- Trigger button
- Screen coordinates (X, Y)

**Detection:**

```cpp
// Check if pointing at bright pixel
bool IsLightDetected(uint16_t x, uint16_t y) {
    // Compare against current PPU output
    return GetBrightness(x, y) > threshold;
}
```

### Super Scope (SNES)

**Input:**

- Fire, Cursor, Turbo, Pause buttons
- Screen coordinates

---

## Input Display (HUD)

### InputHud (InputHud.h)

On-screen input display for TAS and streaming.

**Features:**

- Controller overlay
- Button press visualization
- Configurable position/size
- Per-controller display

**Usage:**

```cpp
// Render input state to screen
inputHud->Draw(frameBuffer, controlDevices);
```

---

## Serialization

Controllers save/load state for save states.

**Serialized Data:**

- Button state
- Strobe state
- Device-specific state (light gun position, etc.)

```cpp
void Serialize(Serializer& s) override {
    SV(_strobe);
    SVArray(_state.State.data(), _state.State.size());
    // Device-specific fields...
}
```

---

## Latency Considerations

### Input Latency Sources

1. **Physical Input** - USB/Bluetooth polling (~1-8ms)
2. **Frame Timing** - Waiting for frame start (~0-16ms)
3. **Emulation** - Processing until input read (~0-16ms)
4. **Display** - V-sync, monitor latency (~1-50ms)

### Minimizing Latency

- Poll input close to frame start
- Use run-ahead feature
- Disable V-sync (causes tearing)
- Use low-latency monitors

---

## Configuration

### Controller Mapping

```json
{
    "Port1": {
        "Type": "SnesController",
        "Mappings": {
            "A": "Keyboard.Z",
            "B": "Keyboard.X",
            "Start": "Keyboard.Enter"
        }
    }
}
```

### Per-Game Settings

Controller configurations can be saved per-game for different mapping preferences.

---

## Related Documentation

- [MOVIE-TAS.md](MOVIE-TAS.md) - Movie/TAS recording
- [ARCHITECTURE-OVERVIEW.md](ARCHITECTURE-OVERVIEW.md) - Overall structure
- [NES-CORE.md](NES-CORE.md) - NES-specific controllers
- [SNES-CORE.md](SNES-CORE.md) - SNES-specific controllers
