# Nexen v1.4.41 Testing Release - Do Not Use For Validation

> ⚠️ Warning: This is a bad release for testing and should not be used yet. Please wait for v1.5.0 (coming in June) for the recommended validation target.

Nexen v1.4.41 is a modernization and stabilization checkpoint focused on current .NET tooling alignment, package freshness, targeted SNES confidence checks, and Pansy workflow safety while Genesis parity triage continues.

## 🌟 Release Title

Nexen v1.4.41 - Modern Forge, Testing Only

## 🧭 Scope Of This Build

| Area | Outcome |
|---|---|
| 🧰 Toolchain modernization | SDK pin aligned to latest available .NET 10 preview in-repo and C# 14 surfaces retained |
| 📦 Managed dependency freshness | Updated core Avalonia, ReactiveUI, logging, test, compression, DBus, and related managed packages |
| 🎮 SNES confidence check | Targeted SNES suite rerun to confirm no immediate regressions from ongoing work |
| 🌼 Pansy and metadata safety | CDL/label-focused coverage rerun to guard metadata tracking paths |
| 🛡️ Release plumbing | README links advanced to v1.4.41 and release-note artifacts prepared |

## 🚀 Major Changes In v1.4.41

### 🧰 Modern .NET and C# baseline

- Updated global SDK resolution to a current .NET 10 preview available in this environment.
- Preserved modern project baseline with `net10.0`, nullable enabled, implicit usings enabled, and C# 14 usage where configured.

### 📦 Managed package updates

- Updated Avalonia runtime stack to latest 12.0.x patch level used by this repo.
- Updated ReactiveUI and source generators to latest compatible releases.
- Updated Microsoft.Extensions.Logging packages to current 10.0.x patch.
- Updated SharpCompress and Tmds.DBus.Protocol.
- Updated test infrastructure packages (Microsoft.NET.Test.Sdk and coverlet collector).
- Updated System.IO.Hashing references to latest 10.0.x patch.
- Aligned SkiaSharp native Linux asset version to avoid downgrade conflicts introduced by newer Avalonia dependencies.

### 🎮 Validation highlights

- Managed UI build passed after dependency modernization.
- Icon rasterizer utility build passed with updated SVG tooling.
- Targeted SNES/CDL/label-native test filter passed: 216/216.

## ✅ Validation Summary For v1.4.41 Prep

- `dotnet build UI/UI.csproj -c Release` passed.
- `dotnet build scripts/IconRasterizer/IconRasterizer.csproj -c Release` passed.
- `Core.Tests.shadow.exe --gtest_filter="Snes*:*Cdl*:*Label*"` passed (216/216).

## 📊 Progress Report (Live Release State)

| Track | Status | Notes |
|---|---|---|
| 🧰 Modernization updates | ✅ Completed | .NET 10 SDK pin + package refresh landed and pushed |
| 🧪 Targeted validation | ✅ Completed | SNES/CDL/label validation passed (216/216) |
| 🚀 Release metadata | ✅ Completed | v1.4.41 tag + release notes updated |
| 🏗️ Artifact pipeline | 🔄 In Progress | CI reruns active to finish full cross-platform asset publish |
| 🎯 Final release verification | 🔄 In Progress | Waiting for full matrix completion + asset confirmation |

## 📦 Expected Release Assets (v1.4.41)

- Nexen-Windows-x64-v1.4.41.exe
- Nexen-Windows-x64-AoT-v1.4.41.exe
- Nexen-Linux-x64-v1.4.41.AppImage
- Nexen-Linux-ARM64-v1.4.41.AppImage
- Nexen-Linux-x64-v1.4.41.tar.gz
- Nexen-Linux-x64-gcc-v1.4.41.tar.gz
- Nexen-Linux-ARM64-v1.4.41.tar.gz
- Nexen-Linux-ARM64-gcc-v1.4.41.tar.gz
- Nexen-Linux-x64-AoT-v1.4.41.tar.gz
- Nexen-macOS-ARM64-v1.4.41.zip

## 📣 Notes For Testers

- This build is intentionally labeled as a bad testing release.
- Use it only to verify release artifact plumbing and packaging paths.
- For meaningful gameplay and long-run validation, wait for v1.5.0 in June.

## 🔗 Links

- Release tag: [v1.4.41](https://github.com/TheAnsarya/Nexen/releases/tag/v1.4.41)
- Compare: [v1.4.40...v1.4.41](https://github.com/TheAnsarya/Nexen/compare/v1.4.40...v1.4.41)
