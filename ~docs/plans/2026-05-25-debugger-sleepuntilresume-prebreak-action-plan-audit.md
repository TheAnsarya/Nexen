# Debugger SleepUntilResume Pre-Break Action Plan Audit (2026-05-25)

## Context

Epic #2323 continues the coordinator-thinning program for SleepUntilResume. After extracting guard, phase, runtime-bundle, loop/post-bundle, and coordinator-entry helpers, the remaining inline wiring still contained direct pre-break action choreography inside Debugger::SleepUntilResume.

## Problem

SleepUntilResume still directly coordinated these deterministic pre-break steps:

- OnBeforeBreak invocation
- OnBeforePause invocation
- IgnoreBreakpoints gating
- DrawPartialFrame gating

These are policy decisions derived from phase outcomes and should be represented as explicit helper outcomes instead of inline control wiring.

## Proposed Slice Trio

1. #2471: add typed pre-break action plan context/outcome helpers in DebuggerDispatchUtils.
2. #2474: add regression tests for emitted and non-emitted action-plan behavior.
3. #2475: refactor Debugger::SleepUntilResume to consume the new helper outcome.

## Acceptance

- Deterministic pre-break action decisions are modeled as helper context/outcome.
- SleepUntilResume uses helper outcomes for pre-break action dispatch.
- Existing behavior remains unchanged for emitted and non-emitted notification paths.
- Targeted DebuggerDispatchUtilsTests and Release build pass.
