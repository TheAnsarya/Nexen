# Third-Party Source Policy

## Goal

Nexen should consume third-party dependencies through package managers (NuGet, vcpkg, or system packages) instead of storing raw vendored source files in this repository.

## Current Policy

- Default: no new vendored third-party source.
- Exceptions: only components listed in [../configs/third-party-source-policy.json](../configs/third-party-source-policy.json).
- Guardrail: CI runs `scripts/audit-third-party-source.ps1 -Enforce` and fails when:
	- a listed component grows beyond policy caps, or
	- files are added under blocked vendor paths.

## Inventory

Current tracked inventory is generated at:

- [THIRD-PARTY-SOURCE-INVENTORY.md](THIRD-PARTY-SOURCE-INVENTORY.md)

Generate/update locally:

```powershell
./scripts/audit-third-party-source.ps1 -ReportPath docs/THIRD-PARTY-SOURCE-INVENTORY.md
```

Enforce policy locally:

```powershell
./scripts/audit-third-party-source.ps1 -Enforce -ReportPath docs/THIRD-PARTY-SOURCE-INVENTORY.md
```

## Migration Rule

When introducing or updating a third-party dependency:

1. Prefer package-backed consumption first (NuGet/vcpkg/system package).
2. If vendoring is unavoidable, add an explicit policy exception with hard caps and justification.
3. Keep vendored footprint bounded and schedule package migration work.
