# Nexen Repository Rename Instructions

## Overview

This document provides step-by-step instructions to complete the Mesen2 → Nexen rename.

## Prerequisites

✅ All code changes committed to `nexen-rename` branch:

- Solution/project files renamed
- C# namespaces updated (580 files)
- Documentation updated
- Branding assets updated

## Steps to Complete

### 1. Push the nexen-rename Branch

```powershell
cd "c:\Users\me\source\repos\Mesen2"
git push -u origin nexen-rename
```

### 2. Merge to Master

```powershell
git checkout master
git merge nexen-rename -m "Merge nexen-rename: Complete Mesen to Nexen rebrand (#110)"
git push origin master
```

### 3. Rename GitHub Repository

1. Go to: <https://github.com/TheAnsarya/Mesen2/settings>
2. Scroll to "Repository name"
3. Change from `Mesen2` to `Nexen`
4. Click "Rename"

**Note:** GitHub will automatically redirect old URLs to the new location.

### 4. Update Local Repository

After renaming on GitHub:

```powershell
# Update remote URL
cd "c:\Users\me\source\repos\Mesen2"
git remote set-url origin https://github.com/TheAnsarya/Nexen.git

# Verify
git remote -v
```

### 5. Rename Local Folder

```powershell
# Close VS Code first!
cd "c:\Users\me\source\repos"
Rename-Item "Mesen2" "Nexen"
```

### 6. Update VS Code Workspace

Edit the workspace file (e.g., `GameInfo.code-workspace`) to update the folder path:

**Before:**

```json
{
	"path": "../Mesen2"
}
```

**After:**

```json
{
	"path": "../Nexen"
}
```

### 7. Close GitHub Issues

```powershell
cd "c:\Users\me\source\repos\Nexen"
gh issue close 110 --repo TheAnsarya/Nexen --comment "Completed: Nexen rebrand complete"
gh issue close 111 --repo TheAnsarya/Nexen --comment "Completed: Solution/project files renamed"
gh issue close 112 --repo TheAnsarya/Nexen --comment "Completed: Namespaces updated"
gh issue close 113 --repo TheAnsarya/Nexen --comment "Completed: Documentation updated"
gh issue close 114 --repo TheAnsarya/Nexen --comment "Completed: Branding assets updated"
gh issue close 115 --repo TheAnsarya/Nexen --comment "Completed: Repository renamed"
```

### 8. Update Upstream Remote

✅ **COMPLETED** - The upstream remote is correctly set to track the original Mesen2 repository:

```powershell
git remote -v
# Shows:
# origin     https://github.com/TheAnsarya/Nexen.git (fetch/push)
# upstream   https://github.com/SourMesen/Mesen2.git (fetch/push)
```

This allows syncing with upstream Mesen2 developments while maintaining the fork.

### 9. Update CI/CD & Documentation

✅ **COMPLETED** - The following have been updated:

1. **GitHub Actions Workflows** - Already use `TheAnsarya/Nexen` in all workflow references
2. **README.md** - Updated:
   - Fork attribution: `(TheAnsarya/Nexen)` ✅
   - Upstream link: `github.com/SourMesen/Mesen2` ✅
   - Fork link: `github.com/TheAnsarya/Nexen` ✅
3. **Copilot Instructions** - Updated to reference `TheAnsarya/Nexen` ✅
4. **Build Documentation** - Updated file paths and repository references ✅

## Verification

After completing all steps, verify:

1. ✅ `git remote -v` shows Nexen URL
2. ✅ `Nexen.sln` opens correctly in Visual Studio
3. ✅ Build succeeds: `dotnet build Nexen.sln`
4. ✅ GitHub shows new repository name
5. ✅ Old URLs redirect to new location

## Rollback (if needed)

If something goes wrong:

1. Rename repo back on GitHub: `Nexen` → `Mesen2`
2. Update remote URL: `git remote set-url origin https://github.com/TheAnsarya/Mesen2.git`
3. Rename local folder back: `Rename-Item "Nexen" "Mesen2"`

## Related Issues

- #110 - [Epic] Rename Mesen2 Fork to Nexen
- #111 - Solution/Project Rename
- #112 - Namespace Updates
- #113 - Documentation Updates
- #114 - Branding Assets Update
- #115 - Repository Rename
