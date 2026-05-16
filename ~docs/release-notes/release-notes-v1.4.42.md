# Nexen v1.4.42 Testing Release - Do Not Use For Validation

> ⚠️ Warning: This is a bad release for testing and should not be used yet. Please wait for v1.5.0 for the recommended validation target.

Nexen v1.4.42 is a test-only stabilization release focused on CI reliability, repository cleanup, release-pipeline hardening, and Linux/AppImage smoke-launch compatibility.

## 🚨 Important Notice

- This release is intentionally marked as test-discouraged.
- Use this only for CI artifact verification and packaging checks.
- For normal testing and reporting, target v1.5.0 instead.

## 🌟 Headline Improvements

| Area | Details |
|---|---|
| 🧹 Repository cleanup | Removed accidental generated `.lscache` artifacts and added ignore rule to prevent recurrence |
| ⚠️ Warning hygiene | Confirmed first-party Windows warning report is clean after vendor filtering |
| 🐧 Linux/AppImage smoke stability | Fixed DBus protocol version mismatch that caused runtime `TypeLoadException` during smoke launch |
| 📦 Release plumbing | Updated README download links to v1.4.42 and prepared release metadata for artifact publication |
| ✅ CI confidence | Revalidated full matrix workflow with all jobs passing before release publication |

## 🔧 Technical Changes

### 🐧 Linux/AppImage smoke-launch fix

- Removed direct `Tmds.DBus.Protocol` package override from `UI/UI.csproj`.
- Restored Avalonia-managed transitive resolution for DBus protocol compatibility.
- Verified Linux RID restore resolves `Tmds.DBus.Protocol/0.92.0` to match `Avalonia.FreeDesktop` expectations.

### 🧹 Repo hygiene updates

- Added `*.lscache` to `.gitignore`.
- Removed tracked `.lscache` files introduced by local tooling cache generation.

### 📖 Release documentation updates

- Updated `README.md` download and quick-start links from v1.4.41 to v1.4.42.
- Refreshed CI release body template used by the release job.

## ✅ Validation Snapshot

- `dotnet build UI/UI.csproj -c Release -nologo` passed (pre and post dependency fix).
- `dotnet restore UI/UI.csproj -r linux-x64 -nologo` passed with compatible DBus protocol resolution.
- Full matrix CI run passed: `Build Nexen` run `25966741153`.

## 📦 Expected Assets For v1.4.42

- Nexen-Windows-x64-v1.4.42.exe
- Nexen-Windows-x64-AoT-v1.4.42.exe
- Nexen-Linux-x64-v1.4.42.AppImage
- Nexen-Linux-ARM64-v1.4.42.AppImage
- Nexen-Linux-x64-v1.4.42.tar.gz
- Nexen-Linux-x64-gcc-v1.4.42.tar.gz
- Nexen-Linux-ARM64-v1.4.42.tar.gz
- Nexen-Linux-ARM64-gcc-v1.4.42.tar.gz
- Nexen-Linux-x64-AoT-v1.4.42.tar.gz
- Nexen-macOS-ARM64-v1.4.42.zip

## 🔗 Links

- Release tag: [v1.4.42](https://github.com/TheAnsarya/Nexen/releases/tag/v1.4.42)
- Compare: [v1.4.41...v1.4.42](https://github.com/TheAnsarya/Nexen/compare/v1.4.41...v1.4.42)
