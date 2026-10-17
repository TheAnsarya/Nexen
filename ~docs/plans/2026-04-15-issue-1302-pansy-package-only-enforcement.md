# Issue #1302 Pansy Package-Only Enforcement Plan (2026-04-15)

## Context

Issue #1298 initially added conditional fallback between sibling source and NuGet package for `Pansy.Core`.
Now that `Pansy.Core` is published to NuGet, Nexen should enforce package-only consumption and remove sibling repository assumptions from CI and build docs.

## Goals

1. Remove local sibling `ProjectReference` fallback from managed projects.
2. Ensure CI no longer clones sibling `pansy` repository.
3. Keep package version centralized and easy to upgrade.
4. Add explicit package-only validation coverage and documentation updates.

## Implementation Steps

1. Project graph updates
	- Remove `PansyCoreProjectPath` and `UseLocalPansyProject` logic from `UI/UI.csproj`.
	- Remove conditional sibling `ProjectReference` from `UI/UI.csproj`.
	- Remove equivalent fallback logic from `Tests/Nexen.Tests.csproj`.
	- Keep package version in `Directory.Build.props` via `PansyCorePackageVersion`.

2. CI updates
	- Remove all `Checkout Pansy` steps from:
		- `.github/workflows/build.yml`
		- `.github/workflows/accuracy-tests.yml`
	- Add package-only managed validation workflow that runs restore/build/test without sibling checkout.

3. Documentation updates
	- Update `COMPILING.md` to state sibling `pansy` is no longer required.
	- Add source-build note in root `README.md`.
	- Add testing/benchmark checkpoint doc and link it from `~docs/README.md`.

## Validation Plan

1. Local validation
	- `dotnet test Tests/Nexen.Tests.csproj -c Release --nologo -v q`
	- `MSBuild Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m`

2. CI validation
	- Run package-only managed workflow on GitHub Actions.
	- Verify no steps clone sibling `pansy`.

## Risks

1. NuGet indexing delay after `Pansy.Core` publish may cause temporary restore failures.
2. Developer machines with stale credential/source overrides may use invalid feed settings.

## Risk Mitigations

1. Document cache-clear restore path in `COMPILING.md`.
2. Keep package version centralized for quick patch-level changes.
3. Add CI package-only lane to catch regressions in dependency resolution.
