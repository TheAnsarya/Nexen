# Nexen v1.4.37 🚧 Testing Release: Archive Stack Migration and Integration Checkpoint

> ⚠️ **WARNING - TESTING RELEASE ONLY**
> This is a **bad release for testing** and is **not intended for production use**.
> Please wait for **v1.5.0** for the target stabilization milestone.
> **38 commits since v1.4.36** | **Cross-platform artifacts built by GitHub Actions**

Nexen v1.4.37 is a broad integration checkpoint that combines archive-system modernization, Epic 21 settings/onboarding UX progress, testing and benchmark infrastructure updates, and release-process hardening. This tag is intentionally published as a high-churn validation build to verify packaging, runtime dependencies, and platform artifact generation end-to-end.

## 🔥 What Changed In v1.4.37

| Area | Summary |
|---|---|
| 📦 Archive stack | Removed vendored SevenZip source and native SZReader flow from active UI extraction path; moved archive listing/extraction to managed package tooling |
| 🧭 Settings and onboarding | Continued Epic 21 work: routing/deeplink behavior, responsive layout refinements, scrollability/touch-target improvements, and setup flow progression |
| 🧪 Quality and observability | Added benchmark/test strategy artifacts, regression fixtures, telemetry harness checkpoints, and validation docs |
| 🎨 Visual consistency | Updated key logo/icon assets with consistent interior fill treatment |
| 🛠️ Release governance | Added mandatory release-notes-per-release policy and aligned docs/changelog/release metadata for v1.4.37 |

## 📦 Archive Stack Migration Details

### ✅ Removed

- Vendored SevenZip project and related solution wiring.
- Native Utilities/SZReader integration path.

### ✅ Added and Refined

- Managed archive handling via SharpCompress in the UI stack.
- Archive enumeration and extraction flow updates in UI archive helper/loading paths.
- Archive-selection and temp-extract behavior alignment for ROM loading UX.

### ✅ Docs and Pipeline Alignment

- Archive stack documentation added/updated to reflect package/framework direction.
- CI warning filtering and release workflow expectations aligned with the no-vendored-source architecture.

## 🧭 Epic 21 Progress Included In This Tag

v1.4.37 includes merged Epic 21 progress items such as:

- Settings responsiveness and settings-tab breakpoint improvements.
- Scroll-container and touch-target upgrades for usability.
- Onboarding setup flow evolution (profile, storage, cancellation/resume, customization simplification).
- Input routing improvements toward active system configuration behavior.

## 🧪 Validation And Build Status

- ✅ Local release build and test gates were executed during release prep.
- ✅ GitHub Actions release workflow completed successfully for this tag.
- ✅ Cross-platform artifact matrix completed (Windows, Linux, Linux AoT, AppImage, macOS ARM64).

## 📥 Release Assets

Assets for tag v1.4.37 are attached on the release page:

- [Nexen v1.4.37 release](https://github.com/TheAnsarya/Nexen/releases/tag/v1.4.37)

Expected asset set includes:

- `Nexen-Windows-x64-v1.4.37.exe`
- `Nexen-Windows-x64-AoT-v1.4.37.exe`
- `Nexen-Linux-x64-v1.4.37.AppImage`
- `Nexen-Linux-ARM64-v1.4.37.AppImage`
- `Nexen-Linux-x64-v1.4.37.tar.gz`
- `Nexen-Linux-x64-gcc-v1.4.37.tar.gz`
- `Nexen-Linux-ARM64-v1.4.37.tar.gz`
- `Nexen-Linux-ARM64-gcc-v1.4.37.tar.gz`
- `Nexen-Linux-x64-AoT-v1.4.37.tar.gz`
- `Nexen-macOS-ARM64-v1.4.37.zip`

## 🔁 Compare

- [Compare v1.4.36...v1.4.37](https://github.com/TheAnsarya/Nexen/compare/v1.4.36...v1.4.37)
