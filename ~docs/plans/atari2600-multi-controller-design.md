# Atari 2600 Multi-Controller Design Document

## Epic 23 (#856) — Controller Support Matrix

### Overview

The Atari 2600 supports multiple controller types through its 9-pin D-sub connector.
While all controllers share the same physical port, they use different electrical signaling:

| Controller | Model | Signal Type | TIA Pins Used | SWCHA Usage |
|---|---|---|---|---|
| Joystick | CX40 | Digital switches | INPT4/5 (fire) | Directions (nibble) |
| Paddle | CX30 | Analog pot + button | INPT0-3 (position), INPT4/5 (fire) | None |
| Keypad | CX50 | Matrix scanning | INPT0-3 (columns) | Rows (output) |
| Driving | CX20 | Quadrature encoder | INPT4/5 (fire) | Gray code bits |
| Booster Grip | — | Digital + analog | INPT0-3 (extra triggers) | Directions (nibble) |

### Hardware Interface Details

#### 9-Pin Connector Pinout

```
Pin 1: Up (joystick) / Row select (keypad) / Gray code A (driving)
Pin 2: Down / Row select / Gray code B
Pin 3: Left / Row select
Pin 4: Right / Row select
Pin 5: Pot 0 (paddle) / Column read (keypad) / Trigger (booster grip)
Pin 6: Fire button
Pin 7: +5V
Pin 8: Ground
Pin 9: Pot 1 (paddle) / Column read (keypad) / Booster (booster grip)
```

#### TIA Input Ports

- **INPT0** ($08): Paddle 0 / Keypad column 0 / Booster Grip trigger P0
- **INPT1** ($09): Paddle 1 / Keypad column 1 / Booster Grip booster P0
- **INPT2** ($0a): Paddle 2 / Keypad column 2 / Booster Grip trigger P1
- **INPT3** ($0b): Paddle 3 / Keypad column 3 / Booster Grip booster P1
- **INPT4** ($0c): Fire button P0
- **INPT5** ($0d): Fire button P1

#### VBLANK Bit 7 — Input Latch/Dump Control

- Bit 7 of VBLANK controls whether INPT0-3 are in "dumped" or "latched" mode
- Dumped mode (bit 7 = 1): Grounds the capacitor, resetting charge
- Latched mode (bit 7 = 0): Capacitor charges through potentiometer
- Bit 7 of INPT0-3: Set when capacitor charges past threshold

### Controller Implementations

#### 1. Paddle Controller (CX30) — #857

Each port supports 2 paddles (4 total across both ports).

**Inputs:**

- Analog position: 0-255 (pot resistance → charge time → INPT0-3 bit 7)
- Fire button: 1 per paddle (P0 paddles share INPT4, P1 paddles share INPT5)

**Implementation:**

- Mouse X movement maps to paddle position (like Arkanoid pattern)
- `HasCoordinates()` returns true for mouse-based analog
- Position stored as uint8_t [0, 255]
- TIA must track charge timing with cycle counter
- Simplified approach: Map position directly to INPT threshold

**TAS Key Names:** `Pf` (Position byte + fire bit)

**State:** `_paddlePositions[4]` in ControlManager, read by TIA

#### 2. Keypad Controller (CX50) — #858

12-button keypad (0-9, *, #) in a 4×3 matrix.

**Inputs:**

- 12 discrete buttons via column/row scanning:
  - Rows selected via SWCHA output (DDR set to output)
  - Columns read from INPT0-3

**Matrix Layout:**

```
         Col0(INPT0)  Col1(INPT1)  Col2(INPT2)
Row0:       1            2            3
Row1:       4            5            6
Row2:       7            8            9
Row3:       *            0            #
```

**Implementation:**

- 12 buttons mapped to CustomKeys[0..11]
- SWCHA row select drives which row is active
- INPT0-3 return column state for active row
- ControlManager must expose current row state for TIA to read

**TAS Key Names:** `123456789s0p` (s=star, p=pound)

#### 3. Driving Controller (CX20) — #859

Continuous rotation via 2-bit gray code.

**Inputs:**

- Rotation: Gray code sequence on SWCHA bits 1:0 (per port nibble)
  - Sequence: 00 → 01 → 11 → 10 → 00 (clockwise)
  - Sequence: 00 → 10 → 11 → 01 → 00 (counter-clockwise)
- Fire button: INPT4/INPT5

**Implementation:**

- Track rotation counter internally
- Mouse X or Left/Right keys increment/decrement counter
- Counter mod 4 → gray code value
- Gray code placed into SWCHA nibble bits 1:0
- Bits 3:2 of nibble are unused (set high/released)

**TAS Key Names:** `LRf` (left-rotate, right-rotate, fire)

#### 4. Booster Grip — #860

Standard joystick plus 2 extra buttons.

**Inputs:**

- Directions: Same as joystick (SWCHA nibble)
- Fire: INPT4/INPT5 (same as joystick)
- Trigger: INPT1 (P0) / INPT3 (P1)
- Booster: INPT0 (P0) / INPT2 (P1)

**Implementation:**

- Extends joystick: directions + fire via same mechanism
- 2 additional buttons read via INPT0-3

**TAS Key Names:** `UDLRftb` (up/down/left/right/fire/trigger/booster)

### ControllerType Enum Additions

```cpp
// C++ (SettingTypes.h)
Atari2600Joystick,    // existing
Atari2600Paddle,      // new
Atari2600Keypad,      // new
Atari2600DrivingController, // new
Atari2600BoosterGrip, // new
```

```csharp
// C# (InputConfig.cs) — must match C++ enum values
Atari2600Joystick,
Atari2600Paddle,
Atari2600Keypad,
Atari2600DrivingController,
Atari2600BoosterGrip,
```

### ControlManager Changes

- Add `_inpt[4]` state array (uint8_t) for INPT0-3
- Add `SetInptState(index, value)` method
- Expand `CreateControllerDevice()` switch for new types
- Expand `UpdateInputState()` to handle each controller type
- Wire INPT0-3 through to TIA ReadRegister (replace hardcoded 0x80)

### UI Changes

- New `Atari2600InputConfigViewModel` with `AvailableControllerTypesP12`
- New `Atari2600InputConfigView.axaml` (modeled on SMS pattern)
- New `Atari2600ConfigViewModel` wrapping input + existing config
- Add `ConfigWindowTab.Atari2600` enum entry
- Add tab in `ConfigWindow.axaml` (after Lynx, before OtherConsoles)
- Add localization entries for tab and controller type names

### TAS/Movie Impact

- Movie header must record controller type per port
- Different controllers have different key counts/names
- BaseControlDevice handles this via `GetKeyNames()` per subclass
- Movie files should be backward compatible (existing Joystick movies unchanged)

### File Inventory

**New Files:**

- `Core/Atari2600/Atari2600Paddle.h`
- `Core/Atari2600/Atari2600Keypad.h`
- `Core/Atari2600/Atari2600DrivingController.h`
- `Core/Atari2600/Atari2600BoosterGrip.h`
- `UI/ViewModels/Atari2600InputConfigViewModel.cs`
- `UI/ViewModels/Atari2600ConfigViewModel.cs`
- `UI/Views/Atari2600InputConfigView.axaml`
- `UI/Views/Atari2600InputConfigView.axaml.cs`
- `UI/Views/Atari2600ConfigView.axaml`
- `UI/Views/Atari2600ConfigView.axaml.cs`

**Modified Files:**

- `Core/Shared/SettingTypes.h` — Add 4 enum entries
- `Core/Atari2600/Atari2600Console.cpp` — ControlManager expansion + TIA INPT wiring
- `UI/Config/InputConfig.cs` — Add 4 enum entries + ControllerTypeExtensions
- `UI/Config/Atari2600Config.cs` — Default to Joystick (unchanged)
- `UI/ViewModels/ConfigViewModel.cs` — Add Atari2600 tab
- `UI/Windows/ConfigWindow.axaml` — Add Atari2600 tab item
- `UI/Localization/resources.en.xml` — Add localization entries

### Issues

- #856 — Epic 23: Atari 2600 Multi-Controller Support
- #857 — [23.1] Paddle Controller (CX30)
- #858 — [23.2] Keypad Controller (CX50)
- #859 — [23.3] Driving Controller (CX20)
- #860 — [23.4] Booster Grip Controller
- #861 — [23.5] Atari 2600 Input Config UI
- #862 — [23.6] TAS/Movie Serialization
