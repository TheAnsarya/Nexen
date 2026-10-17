# Nexen v1.4.40 Testing Release - Do Not Use For Validation

> ⚠️ Warning: This is a bad release for testing and should not be used yet. Please wait for v1.5.0 (coming in June) for the recommended validation target.

Nexen v1.4.40 is a high-churn integration checkpoint that bundles major Genesis parity work, startup-compatibility hardening, naming cleanup, and release-pipeline stabilization. This release is intentionally published for artifact and CI verification, not for end-user validation.

## 🌟 Release Title

Nexen v1.4.40 - Thunder Before Bloom 1.5.0

## 🧭 Scope Of This Build

| Area | Outcome |
|---|---|
| 🎮 Genesis hardware parity | Large multi-issue parity wave across TMSS, controller protocol, DMA/FIFO timing, byte-lane behavior, and startup sequencing |
| 🧪 Regression depth | Expanded Genesis startup and VDP parity tests, including mixed control/data sequencing edge cases |
| 🧹 Repository consistency | Continued Mesen-to-Nexen naming normalization and workflow/script cleanup batches |
| 📦 Release infrastructure | Release metadata, README version links, and workflow-driven artifact production prepared for v1.4.40 |
| 🛡️ Stabilization pass | Low-risk warning cleanup in active test surfaces and coarse-seek guard initialization in audio code |

## 🔥 Major Change Themes Since v1.4.39

### 🎮 Genesis parity and startup convergence

- Continued aggressive Genesis parity batches across issues, including startup/logo timing, TMSS behavior, bridge/MMU windows, and bus arbitration semantics.
- Added and expanded deterministic startup compatibility gates and stress coverage to catch ordering-sensitive regressions early.
- Hardened VDP behavior around control/data pending sequencing and DMA/FIFO status windows.
- Finalized 3-button versus 6-button controller protocol gating and associated parity tests.

### 🧪 Test expansion and reliability

- Increased targeted Genesis test coverage for startup transitions, delayed unlock states, mixed command ordering, and read/write lane interactions.
- Added parity-focused tests for stale high-byte latch behavior when full-word writes occur on VDP data/control paths.
- Preserved green targeted regression runs for active Genesis suites after each stabilization pass.

### 🧰 Tooling, scripts, and diagnostics

- Extended startup diagnostics and trace-capture workflows used for cross-emulator parity triage.
- Continued script and artifact naming consistency passes to reduce ambiguity during automation and manual triage.
- Improved release-oriented workflow hygiene for repeatable build and artifact publication.

### 🧹 Warning and stability adjustments in this release prep

- Removed signed/unsigned assertion mismatches in Genesis VDP parity tests.
- Fixed a potential uninitialized local warning path in `seek_to_sample_coarse` by initializing the probe struct.
- Eliminated prior corrupted-PDB linker warning storms via clean rebuild and output stabilization.

## ✅ Validation Summary For v1.4.40 Prep

- Release build gate passed locally for Release x64.
- Core targeted Genesis regression filter passed:
	- `GenesisControlManagerTests.*`
	- `GenesisVdpDmaStartupLatencyTests.*`
	- `GenesisVdpReadPortParityTests.*`
- Result: 50/50 tests passed.

## 📦 Expected Release Assets (v1.4.40)

- Nexen-Windows-x64-v1.4.40.exe
- Nexen-Windows-x64-AoT-v1.4.40.exe
- Nexen-Linux-x64-v1.4.40.AppImage
- Nexen-Linux-ARM64-v1.4.40.AppImage
- Nexen-Linux-x64-v1.4.40.tar.gz
- Nexen-Linux-x64-gcc-v1.4.40.tar.gz
- Nexen-Linux-ARM64-v1.4.40.tar.gz
- Nexen-Linux-ARM64-gcc-v1.4.40.tar.gz
- Nexen-Linux-x64-AoT-v1.4.40.tar.gz
- Nexen-macOS-ARM64-v1.4.40.zip

## 📣 Notes For Testers

- This build is intentionally labeled as a bad testing release.
- Use it only to verify release artifact plumbing and packaging paths.
- For meaningful gameplay or parity validation, wait for v1.5.0 in June.

## 🔗 Links

- Release tag: [v1.4.40](https://github.com/TheAnsarya/Nexen/releases/tag/v1.4.40)
- Compare: [v1.4.39...v1.4.40](https://github.com/TheAnsarya/Nexen/compare/v1.4.39...v1.4.40)
