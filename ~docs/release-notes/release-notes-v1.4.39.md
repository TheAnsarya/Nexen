# Nexen v1.4.39 Testing Release - Do Not Use For Validation

> ⚠️ Warning: This is a bad release for testing and should not be used as a validation baseline. Please wait for v1.5.0 for the recommended testing target.

Nexen v1.4.39 is a controlled stabilization checkpoint focused on high-signal Genesis runtime diagnostics, release traceability, and deterministic artifact generation while larger correctness work advances toward v1.5.0.

## 🌟 Release Title

Nexen v1.4.39 - Hold The Line Before 1.5.0

## 🧭 Scope Of This Build

| Area | Outcome |
|---|---|
| 🎮 Genesis triage | Added temporary Z80 handshake and window-gating diagnostics for startup-loop analysis |
| 🧠 Sonic evidence capture | Increased confidence in runtime traces around A11100/A11200 and A01FFD polling behavior |
| 🧪 Stability gates | Revalidated Release x64 and targeted Genesis startup tests |
| 🗂️ Workflow traceability | Issue-first tracking, prompt lineage, and session log completion for release work |
| 📦 Artifact readiness | Build workflow prepared to publish multi-platform v1.4.39 assets |

## 🔧 Fix And Triage Highlights

### 🎮 Genesis handshake instrumentation

- Added temporary logs for Z80 bus request and reset register reads/writes with PC context.
- Added temporary logs for Z80 window reads/writes to show gate decision (allow/blocked) plus state.
- Added transition-focused logging near Sonic loop-region PCs to ensure state changes are captured.

### 🔍 Runtime diagnosis quality

- Preserved forced checkpoint persistence strategy to keep deep traces landing in core logs.
- Correlated interrupt cadence and handshake state in a repeatable way for follow-up correctness work.

### 🧹 Low-risk stabilization posture

- Kept behavior-preserving diagnostics only in emulator-sensitive areas.
- Avoided broad speculative behavior changes during release prep.

## 🧪 Validation Summary

- Release build gate passed for Release x64.
- Targeted C++ gate passed: Core.Tests.exe --gtest_filter=GenesisExecutionStartupTests.* --gtest_brief=1 => 14/14.
- Runtime Sonic headless captures produced deep checkpointed logs for triage.

## 📦 Expected Release Assets (v1.4.39)

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

## 🔗 Links

- Release tag: https://github.com/TheAnsarya/Nexen/releases/tag/v1.4.39
- Compare: https://github.com/TheAnsarya/Nexen/compare/v1.4.38...v1.4.39
