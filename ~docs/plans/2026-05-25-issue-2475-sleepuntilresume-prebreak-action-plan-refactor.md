# Issue #2475 SleepUntilResume Pre-Break Action Plan Refactor

## Scope

Refactor `Debugger::SleepUntilResume` to use the pre-break action-plan helper outcome instead of direct inline policy wiring for pre-break action decisions.

## Refactor Steps

1. Build action-plan context from resolved phase outcome.
2. Resolve action-plan outcome.
3. Use outcome flags to gate:
	- `GetMainDebugger()->OnBeforeBreak(sourceCpu)`
	- `_emu->OnBeforePause(false)`
	- `IgnoreBreakpoints`
	- `DrawPartialFrame()`
4. Preserve runtime bundle and downstream loop/post-loop behavior.

## Acceptance Criteria

- Pre-break action wiring in SleepUntilResume is helper-driven.
- Behavior remains unchanged for emitted and non-emitted paths.
- Targeted tests and Release build pass.
