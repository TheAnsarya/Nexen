# 🌿 Nexen Branch Strategy

> **Document Version:** 1.0.0
> **Last Updated:** January 27, 2026
> **Status:** Active

## 📋 Overview

This document describes the branch strategy used for Nexen development, specifically for the pansy integration and modernization efforts.

## 🌳 Branch Hierarchy

```text
main (sour's upstream)
 │
 └─► pansy-export (our fork's main integration branch)
	  │
	  └─► modernization (completed modernization work)
```

## 📍 Branch Descriptions

### `main`

- **Purpose:** Tracks upstream Nexen (sour's repository)
- **Owner:** Upstream (sour)
- **Merge Policy:** We pull from upstream, never push
- **Status:** Read-only from our perspective

### `pansy-export`

- **Purpose:** Main development branch for pansy integration
- **Base:** `main`
- **Features:**
	- Pansy metadata export functionality
	- Integration with Peony disassembler
	- Background auto-save of debug labels
- **Tag:** `v2.0.0-pansy-phase3`
- **Status:** Stable, production-ready

### `modernization`

- **Purpose:** .NET 10 and dependency modernization
- **Base:** `pansy-export`
- **Features:**
	- .NET 8.0 → .NET 10.0 migration
	- Avalonia 11.3.1 → 11.3.9 update
	- Lua 5.4.4 → 5.4.8 update
	- System.IO.Hashing integration
	- K&R code formatting
	- 24 unit tests for PansyExporter
- **Status:** ✅ Complete, ready to merge

## 🔄 Workflow

### Feature Development

1. **Create feature branch** from `pansy-export`:

   ```bash
   git checkout pansy-export
   git checkout -b feature/my-feature
   ```

2. **Develop and commit** with conventional commits:

   ```bash
   git commit -m "feat: add new feature"
   git commit -m "fix: correct bug"
   git commit -m "docs: update documentation"
   ```

3. **Merge back** to `pansy-export`:

   ```bash
   git checkout pansy-export
   git merge feature/my-feature
   git branch -d feature/my-feature
   ```

### Modernization Branch (Completed)

The modernization branch followed this workflow:

1. **Created** from `pansy-export` at tag `v2.0.0-pansy-phase3`
2. **Implemented** all modernization phases
3. **Verified** build success and test passage
4. **Documented** all changes in `~docs/modernization/`
5. **Tagged** completion point
6. **Merged** back to `pansy-export`

## 🏷️ Tagging Strategy

### Tag Format

```text
v{major}.{minor}.{patch}-{feature}[-{phase}]
```

### Current Tags

| Tag | Branch | Description |
| ----- | -------- | ------------- |
| `v2.0.0-pansy-phase3` | pansy-export | Initial pansy integration complete |
| `v2.0.0-modernization` | modernization | Modernization complete |

### When to Tag

- After completing a major feature
- Before merging a significant branch
- At stable checkpoints for easy rollback

## 📊 Commit Convention

### Format

```xml
<type>: <description>

[optional body]

[optional footer]
```

### Types

| Type | Description |
| ------ | ------------- |
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `style` | Formatting, no code change |
| `refactor` | Code change, no new feature or bug fix |
| `perf` | Performance improvement |
| `test` | Adding or fixing tests |
| `chore` | Build process, auxiliary tools |

### Examples

```bash
feat: add pansy export menu option
fix: correct CRC32 calculation for labels
docs: update modernization roadmap
style: apply K&R formatting to Debugger folder
refactor: use System.IO.Hashing for CRC32
test: add PansyExporter unit tests
chore: update NuGet packages
```

## 🔀 Merge Checklist

Before merging a feature branch:

- [ ] All tests pass (`dotnet test`)
- [ ] Build succeeds (`dotnet build`)
- [ ] Documentation updated
- [ ] Session log created
- [ ] Commit messages follow convention
- [ ] Branch is up-to-date with base

## 🚀 Merging Modernization

### Pre-Merge Steps

1. ✅ Verify all phases complete
2. ✅ Run full test suite
3. ✅ Update all documentation
4. ✅ Create session log
5. ⏳ Create `v2.0.0-modernization` tag
6. ⏳ Merge to `pansy-export`
7. ⏳ Push all changes

### Merge Commands

```bash
# Tag the modernization branch
git checkout modernization
git tag -a v2.0.0-modernization -m "Modernization complete: .NET 10, Avalonia 11.3.9, Lua 5.4.8"

# Merge to pansy-export
git checkout pansy-export
git merge modernization

# Push everything
git push origin pansy-export
git push origin modernization
git push origin --tags
```

## 📝 Branch Cleanup

After successful merge:

- Keep `modernization` branch for reference
- Feature branches can be deleted after merge
- Never delete `main` or `pansy-export`

## 🔮 Future Branches

Potential future branches:

- `feature/performance` - Performance optimizations
- `feature/new-systems` - Additional console support
- `feature/ui-improvements` - UI/UX enhancements
- `bugfix/*` - Bug fix branches

