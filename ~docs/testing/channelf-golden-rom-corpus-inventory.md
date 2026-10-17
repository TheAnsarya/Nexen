# Channel F Golden ROM Corpus Inventory

## Purpose

Track the canonical Channel F ROM and BIOS corpus used for deterministic validation, debugger checks, and regression reproduction.

## Reference Root

- `C:\~reference-roms\channel f\`

## BIOS Entries

| Display Name | File Name | SHA1 | MD5 | Status |
|---|---|---|---|---|
| Luxor Channel F BIOS SL90025 (1976) | `Luxor Channel F BIOS SL90025 (1976) (Luxor).bin` | `759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2` | `95d339631d867c8f1d15a5f2ec26069d` | Verified |

## Required Metadata Per Corpus Entry

| Field | Description |
|---|---|
| Platform | `ChannelF` |
| Region | BIOS/cart region tag if known |
| Mapper Profile | Mapper or board profile classification |
| SHA1 | Primary identity hash |
| MD5 | Secondary compatibility hash |
| Source | Provenance note (dump/source set) |
| Validation Status | `Verified`, `Pending`, or `Quarantined` |

## Validation Rules

1. Do not promote a ROM into golden corpus without checksum registration.
2. Use SHA1 as the canonical identity key; MD5 is a compatibility cross-check.
3. If hash mismatch occurs, mark the entry `Quarantined` and do not use for baseline evidence.
4. Keep corpus metadata synchronized with issue comments when new files are added or reclassified.

## Operational Use

- Reproduce regressions with fixed ROM/Bios identities.
- Anchor benchmark and memory captures to immutable corpus entries.
- Ensure TAS/movie playback and save-state checks run against known-good binaries.
