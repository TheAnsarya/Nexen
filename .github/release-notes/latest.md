# Nexen v1.4.39 - Hold The Line Before 1.5.0

> ⚠️ Warning: This is a bad release for testing and should not be used as a validation baseline. Please wait for v1.5.0 for the recommended testing target.

Nexen v1.4.39 is an intentionally caution-labeled stabilization release that captures deep Genesis runtime diagnostics, hardens release traceability, and preserves deterministic build output while active emulator fixes continue toward v1.5.0.

## 🚨 Important Notice

- This release is intentionally marked as test-discouraged.
- Use this only for narrow regression comparison and CI artifact verification.
- For normal testing and reporting, target v1.5.0 instead.

## 🌟 Headline Improvements

| Area | Details |
|---|---|
| 🎮 Genesis Runtime Triage | Added temporary high-signal diagnostics for Z80 bus request/reset handshake and Z80 window access gating |
| 🧠 Sonic Startup Investigation | Captured deeper loop-region evidence around 68k PC polling behavior and mailbox reads |
| 🗂️ Release Traceability | Added issue-first tracking, prompt lineage, and session-log coverage for release-path work |
| 🧪 Stability Gates | Revalidated Release x64 build and targeted Genesis startup suite while preserving runtime behavior |
| 📦 Artifact Pipeline | Release workflow remains configured to generate and attach multi-platform binaries for v1.4.39 |

## 🔍 Detailed Notes

### 🎮 Genesis Z80 handshake diagnostics

- Added temporary logging around A11100 and A11200 read/write pathways with PC and state context.
- Added temporary logging for A00000-A0FFFF Z80 window accesses indicating whether reads/writes were allowed or blocked.
- Added transition-focused logging around the Sonic loop region to avoid losing critical state changes behind coarse throttling.

### 🧭 Runtime evidence quality upgrades

- Preserved periodic persistence checkpoints so deep runtime traces are consistently flushed to core logs.
- Improved triage confidence by correlating interrupt cadence, handshake transitions, and mailbox polling behavior.

### 🧹 Cautious stabilization pass

- Kept changes intentionally low risk and diagnostics-focused in emulator-sensitive paths.
- Avoided speculative behavior changes so evidence remains clean for follow-up correctness fixes.

## ✅ Validation Snapshot

- Release build passed: Nexen.sln (Release x64).
- Targeted Genesis startup tests passed: GenesisExecutionStartupTests.* (14/14).
- Runtime capture succeeded with forced frame checkpoint persistence and deep core trace output.

## 📦 Expected Assets For v1.4.39

- Nexen-Windows-x64-v1.4.39.exe
- Nexen-Windows-x64-AoT-v1.4.39.exe
- Nexen-Linux-x64-v1.4.39.AppImage
- Nexen-Linux-ARM64-v1.4.39.AppImage
- Nexen-Linux-x64-v1.4.39.tar.gz
- Nexen-Linux-x64-gcc-v1.4.39.tar.gz
- Nexen-Linux-ARM64-v1.4.39.tar.gz
- Nexen-Linux-ARM64-gcc-v1.4.39.tar.gz
- Nexen-Linux-x64-AoT-v1.4.39.tar.gz
- Nexen-macOS-ARM64-v1.4.39.zip

## 🔗 Project Links

- Changelog: https://github.com/TheAnsarya/Nexen/blob/master/CHANGELOG.md
- Compare: https://github.com/TheAnsarya/Nexen/compare/v1.4.38...v1.4.39
