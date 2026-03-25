# Atari 2600 Debug and TAS Manual Test

Use this checklist to validate Atari 2600 debugger and TAS/movie functionality for release.

## Preconditions

- Release build available.
- Atari 2600 ROM and symbols loaded.
- A clean movie workspace path prepared.

## Stage 1: Debugger UI Validation

### 1.1 Disassembly and Breakpoint Flow

1. Open debugger and disassembly panel.
2. Set an execution breakpoint on a known game loop address.
3. Run until breakpoint and verify pause location.
4. Step one instruction and verify PC update.

Expected result:
- Breakpoint hit location is exact.
- Stepping always advances to the expected next instruction.

### 1.2 Memory and Event Consistency

1. Open memory viewer on TIA and RIOT ranges.
2. Trigger an in-game action that writes those registers.
3. Verify memory values update.
4. Open event viewer and confirm corresponding event entries appear.

Expected result:
- Memory viewer and event viewer agree on observed writes.

### 1.3 Trace Validation

1. Start trace logging.
2. Run 120 frames.
3. Stop trace logging.
4. Validate expected Atari 2600 tags and sequence ordering.

Expected result:
- Trace output is stable and ordered.

## Stage 2: TAS UI Validation

### 2.1 Controller Subtype Coverage

1. Load or create a movie with different Atari 2600 port types.
2. Select joystick port and confirm joystick layout.
3. Select paddle port and confirm paddle layout.
4. Select keypad, driving, and booster grip ports and confirm each layout.

Expected result:
- Button grids match the selected port subtype.

### 2.2 Paddle Editor and Console Switches

1. Select a paddle port frame and edit paddle position.
2. Verify value writes to the frame.
3. Select a non-paddle port frame and confirm paddle editor is hidden.
4. Toggle SELECT and RESET console switches.
5. Undo and redo the toggles.

Expected result:
- Paddle editor visibility and value writes are correct.
- Console switch toggles are undoable and frame-accurate.

### 2.3 Selected Port Isolation

1. Use dual-port movie input.
2. Select port 1 and toggle a button.
3. Verify only port 1 changed.
4. Repeat for port 2.

Expected result:
- Edit actions only affect the selected port.

## Stage 3: Movie Roundtrip Validation

### 3.1 Nexen Format

1. Save a movie with paddle positions and console switch commands.
2. Reload the movie.
3. Compare frame input state.

Expected result:
- No data loss for paddle and command fields.

### 3.2 BK2 Format

1. Export BK2 from the same movie.
2. Re-import BK2.
3. Compare frame input state.

Expected result:
- Joystick input, paddle positions, and console switches survive roundtrip.

## Pass Criteria

All Critical and High ranked steps in [Manual Step Priority Ranking](manual-step-priority-ranking.md) must pass.

## Evidence Logging

Record:
- Build identifier.
- Test ROM used.
- Step IDs executed.
- Pass or fail results.
- Issue links for any failures.
