# Channel F Benchmark Suite Matrix

## Scope

This document defines the required benchmark lanes for Channel F and the minimum evidence required to detect regressions.

## Benchmark Inventory

| Benchmark | Purpose | Category | CI Smoke | Local Full Run |
|---|---|---|---|---|
| `BM_ChannelFCore_RunFrame_NoInput` | Baseline frame execution without input mutation | Core runtime | Required | Required |
| `BM_ChannelFCore_RunFrame_WithInput` | Input-handling overhead under active panel/controller states | Input/runtime | Required | Required |
| `BM_ChannelFCore_OpcodeLookup_Defined` | Hot-path opcode metadata lookup for defined opcodes | Decode table | Optional | Required |
| `BM_ChannelFCore_OpcodeLookup_Unknown` | Fallback path for unknown opcode metadata lookups | Decode table | Optional | Required |
| `BM_ChannelFCore_BiosVariantDetect_Luxor` | BIOS hash-lookup path for Luxor variant selection | Startup/config | Optional | Required |
| `BM_ChannelFCore_SerializeRoundtrip` | Save-state serialization and restoration roundtrip cost | Persistence | Optional | Required |
| `BM_ChannelFCore_Reset` | Reset path and deterministic state re-initialization cost | Lifecycle | Optional | Required |
| `BM_ChannelFCore_OpcodeTableFullScan` | Full opcode-table scan throughput and branch behavior | Decode table | Optional | Required |

## CI Contract

- CI smoke benchmark filter must include: `BM_(NesCpu|SnesCpu|GbCpu|ChannelFCore)_.*`
- Smoke runs validate execution stability and catastrophic regressions only.
- Statistical performance sign-off is done in local full runs.

## Local Full-Run Protocol

1. Build release benchmark binary.
2. Execute Channel F subset with JSON output and at least 30 repetitions.
3. Compare median and p95 values against previous accepted baseline.
4. Treat any sustained regression above 5% as a blocker unless explicitly justified.

## Acceptance Criteria

- Every benchmark listed in this matrix executes successfully.
- CI smoke lane covers Channel F core benchmarks.
- Local full-run evidence is attached to the issue or session log when benchmark-related changes are made.
