# Issue #1298 Pansy Dependency Decoupling Plan

## Issue

- GitHub: https://github.com/TheAnsarya/Nexen/issues/1298
- Symptom: Nexen build path expects sibling `pansy` repository to exist.
- User-impact: fresh contributors can fail restore/build due to hard project reference path.

## Root Cause

`UI/UI.csproj` and `Tests/Nexen.Tests.csproj` had unconditional:

- `ProjectReference Include="..\\..\\pansy\\src\\Pansy.Core\\Pansy.Core.csproj"`

This creates a hard external repository dependency.

## Fix Implemented

1. Added conditional Pansy reference strategy in both projects:
	- local `ProjectReference` when sibling project exists,
	- fallback `PackageReference` to `Pansy.Core` `1.0.0` when sibling project is absent.
2. Added `COMPILING.md` documentation section describing:
	- automatic local-vs-package behavior,
	- local package feed workflow via `dotnet pack` for `Pansy.Core`.

## Validation Plan

- Build Nexen with sibling pansy present (current workspace baseline).
- Ensure managed tests pass.
- Verify Release x64 solution build passes.

## NuGet Publishing Follow-Up (Proposed)

The fallback path is ready, but project-level package publishing automation should be tracked as a follow-up:

- publish `Pansy.Core` package for each stable Pansy release,
- keep Nexen `PansyCorePackageVersion` aligned to known-good Pansy versions,
- optionally add a CI sanity check that package restore works without sibling source.

## Risk

Low-to-medium. Build-system and restore behavior change only; runtime integration remains unchanged.
