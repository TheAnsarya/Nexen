# Input Mapping Coverage Checkpoint (2026-03-30)

## Scope

Evidence checkpoint for #1073 focused on per-system controller button mapping decode coverage and regression status.

## Automated Validation

Command:

```powershell
dotnet test Tests/Nexen.Tests.csproj -c Release --filter "FullyQualifiedName~InputApiDecoderTests" --nologo -v m
```

Result:

- Total: 50
- Passed: 50
- Failed: 0
- Skipped: 0

## Covered Mapping Groups

The current `InputApiDecoderTests` class includes explicit mapping assertions for:

- Lynx (`LynxController`)
- SNES (`SnesController`)
- NES/Famicom (`NesController`, `FamicomController`, `FamicomControllerP2`)
- Game Boy and GBA (`GameboyController`, `GbaController`)
- SMS/Coleco (`SmsController`, `ColecoVisionController`)
- PC Engine (`PceController`, `PceAvenuePad6` including extra 6-button lanes)
- WonderSwan (`WsController`, `WsControllerVertical`)
- Atari 2600 (`Joystick`, `Driving`, `BoosterGrip`, `Keypad`)

## UI and Consistency Signal

Input mapping configuration flow still routes through the shared configuration surfaces (`InputComboBox` setup flow plus per-system config models), while decode mapping assertions remain centralized in `InputApiDecoderTests` for consistency.

## Follow-Up

- Keep this checkpoint linked from Epic 23 validation matrix and testing index.
- Extend with explicit conflict/default behavior coverage when corresponding UI conflict-handling tests are added.
