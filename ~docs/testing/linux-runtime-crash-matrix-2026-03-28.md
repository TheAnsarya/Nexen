# Linux Runtime Crash Reproduction Matrix (2026-03-28)

## Purpose

This matrix tracks distro-specific crash signatures and stabilization status for Epic 22 / Issue #1049.

## Reproduction Matrix

| Distro / Environment | Runtime Context | Observed Signature | Current Status | Notes |
| ---------- | ---------- | ---------- | ---------- | ---------- |
| KUbuntu 25.10 | .NET 10 runtime, Linux native launch | SIGSEGV in `locale_get_default_76` from system ICU 76 (`libicuuc.so.76`) | Mitigated by app-local ICU strategy | Original regression thread: #1039 |
| KUbuntu 25.10 (post app-local ICU attempt) | App-local ICU configured to 72.1 token | Startup abort: failed to load `libicudata.so.72.1` | Fixed by exact token/version alignment (`72.1.0.3`) | Token mismatch root cause identified |
| Linux Mint 22.3 (Xfce) Live | AppImage run | Skia native mismatch (`libSkiaSharp` 88.1 vs expected range 116/117) | Mitigated by dependency/native packaging checks | Validate in CI artifact checks and runtime smoke |
| AnduinOS 1.4.1 Live | AppImage run | Same Skia mismatch startup failure pattern | Mitigated pathway in progress | Requires repeated artifact validation |

## Evidence Sources

1. GitHub issue #1039 user reports and follow-up comments.
2. CI runtime checks in [build.yml](../../.github/workflows/build.yml).
3. Validation pack in [~docs/testing/epic-22-validation-pack-2026-03-28.md](epic-22-validation-pack-2026-03-28.md).

## Mitigation Checklist

1. Ensure app-local ICU token and native assets match exactly (`72.1.0.3`).
2. Verify Linux publish outputs include ICU, Skia, and HarfBuzz native assets.
3. Block CI on missing runtime dependencies.
4. Keep benchmark/test checkpoints for each runtime package or CI change.

## Remaining Risks

1. Cross-distro runtime loader behavior can still vary with system libraries and launch environment.
2. AppImage launch context may differ from plain publish binary behavior on some distros.
3. Additional Linux desktop stacks may expose library-loader edge cases not yet covered.
