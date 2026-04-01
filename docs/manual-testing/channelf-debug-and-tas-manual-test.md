# Channel F Debug and TAS Manual Test

Use this checklist to validate Fairchild Channel F debugger and TAS/movie functionality for release.

## Preconditions

- Release build available.
- Channel F ROM loaded.
- A clean movie workspace path prepared.

## Stage 1: Channel F Bring-Up Validation

### 1.1 ROM Open and Runtime Stability

1. Open a Channel F ROM from the File menu.
2. Verify emulator starts without crash.
3. Run for at least 300 frames.
4. Pause and resume execution twice.

Expected result:

- ROM opens cleanly and runtime remains stable.

### 1.2 Save State and Rewind Baseline

1. Create save state slot at a stable gameplay point.
2. Run 120 frames.
3. Load the saved slot and verify expected game state.
4. Use rewind for 60+ frames and resume forward execution.

Expected result:

- Save/load and rewind remain functional and deterministic enough for iterative testing.

## Stage 2: Debugger Validation

### 2.1 Disassembly and Breakpoint Flow

1. Open debugger and disassembly panel.
2. Set a breakpoint on a recurring code path.
3. Run until breakpoint is hit.
4. Step single instruction and verify PC progression.

Expected result:

- Breakpoint and step flow are stable and positionally correct.

### 2.2 Memory and Trace Agreement

1. Open memory viewer on Channel F relevant regions.
2. Trigger an in-game action known to update state.
3. Start trace logging for 120 frames.
4. Stop trace and compare observed memory changes with trace sequence.

Expected result:

- Memory and trace surfaces provide coherent observations.

## Stage 3: TAS and Movie Validation

### 3.1 TAS Open, Edit, and Replay

1. Start recording a short movie.
2. Open TAS editor and verify Channel F button layout appears correctly.
3. Edit input on at least 30 frames.
4. Replay from an earlier frame and verify edited input is applied.

Expected result:

- TAS editor remains stable and applies frame edits accurately.

### 3.2 Branching and Rerecord Loop

1. Create a branch from current movie timeline.
2. Apply alternate inputs for a short sequence.
3. Compare baseline and branch playback segments.
4. Confirm timeline operations do not crash.

Expected result:

- Branching and rerecording workflows complete without data loss.

### 3.3 Movie Roundtrip Baseline

1. Save Channel F movie to Nexen format.
2. Reload movie and compare key frame input states.
3. Replay a fixed window and verify expected control behavior.

Expected result:

- No unexpected movie data loss or replay drift in checked windows.

## Pass Criteria

All Critical and High ranked manual steps in [Manual Step Priority Ranking](manual-step-priority-ranking.md) must pass.

## Evidence Logging

Record:

- Build identifier.
- ROM used.
- Step IDs executed.
- Pass and fail outcomes.
- Issue links for any failures.
