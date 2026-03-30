# CI and Release Pipeline Fixes (v1.4.5 to v1.4.8)

## Scope

This document consolidates the CI and release hardening work completed during the v1.4.5 to v1.4.8 stabilization period.

Tracked by:

- #1066 Epic 22 CI/Release pipeline fixes documentation

## Problem and Fix Timeline

### 1. Windows CI toolset mismatch

- Symptom: warning report MSBuild step failed on the windows-2025-vs2026 runner.
- Root cause: Visual Studio 2026 build environment required explicit PlatformToolset value.
- Fix: added PlatformToolset v145 to warning report build step and made that diagnostic pass non-blocking.

### 2. Linux and AppImage ICU validation mismatch with single-file publish

- Symptom: validation expected standalone runtime config and ICU files beside output binary.
- Root cause: single-file publish mode bundles these assets and does not emit them as separate files in that layout.
- Fix: removed incorrect standalone-file expectation path and replaced with checks aligned to publish model.

### 3. AppImage matrix cancellation on one-arm failure

- Symptom: one matrix leg failure cancelled sibling leg.
- Root cause: matrix fail-fast behavior.
- Fix: disabled fail-fast for AppImage matrix and captured smoke logs with non-blocking collection path.

### 4. Release job skipped when one platform failed

- Symptom: no release publication when one platform leg failed.
- Root cause: release job depended on full upstream success.
- Fix: release condition changed to always run for version tags so available artifacts can still be published.

### 5. Windows runtime crash due missing native libraries in output flow

- Symptom: published executable crashed early at startup.
- Root cause: publish configuration did not ensure native runtime assets were available under that packaging mode.
- Fix: enabled native library extraction behavior for single-file path and aligned publish options accordingly.

### 6. Self-contained consistency across release surfaces

- Symptom: framework-dependent variability across target systems.
- Root cause: mixed publish strategy across jobs.
- Fix: standardized self-contained output strategy for release-facing jobs.

## Technical Notes

- App-local ICU strategy is enforced for deterministic globalization behavior.
- Publish settings now explicitly align restore, build, and publish expectations.
- Runtime dependency diagnostics are captured as artifacts for triage and auditability.

## Release Impact Summary

- v1.4.6 exposed pipeline and packaging weaknesses despite broad green status.
- v1.4.7 applied startup/runtime packaging corrections.
- v1.4.8 standardized self-contained release behavior across primary targets.

## Operational Checklist for Future Incidents

1. Verify toolset and runner compatibility first.
2. Validate checks against actual publish mode behavior.
3. Confirm matrix fail-fast settings match release policy.
4. Verify native runtime assets for packaged outputs.
5. Keep self-contained strategy consistent across release targets.
6. Keep runtime dependency reports attached to every significant build hardening change.
