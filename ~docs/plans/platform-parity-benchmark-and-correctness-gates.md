# Platform Parity Benchmark and Correctness Gates

Issue linkage: [#703](https://github.com/TheAnsarya/Nexen/issues/703)

Parent issue: [#673](https://github.com/TheAnsarya/Nexen/issues/673)

## Goal

Define per-phase benchmark and correctness gates for Atari 2600 and Genesis rollout, and provide reusable issue templates for gate-driven implementation tracking.

## Gate Framework

| Gate | Required Evidence | Pass Condition |
|---|---|---|
| Correctness Gate | Deterministic harness summary and targeted tests | All required checks pass, no digest drift |
| Build Gate | Release x64 build output | Build succeeds with no new warnings relevant to changes |
| Performance Gate | Before/after benchmark snapshot | No unexplained regression in tracked metrics |
| Regression Gate | Re-run prior phase checkpoint suite | Previously passing checkpoints remain green |

## Phase Matrix

| Platform Phase | Correctness Harness | Benchmark Focus |
|---|---|---|
| Atari CPU/RIOT skeleton | `Atari2600Bringup*` | Frame-step baseline overhead |
| Atari TIA timing | `Atari2600TiaTiming*` | Scanline/timing checkpoint overhead |
| Atari mapper expansions | Mapper-specific regression suite | Bank-switch dispatch cost |
| Genesis CPU boundary | `GenesisM68k*` | Core stepping overhead |
| Genesis VDP/DMA | `GenesisVdp*` | DMA path and rendering cadence |
| Genesis Z80/audio staging | `GenesisZ80*`, `GenesisAudio*` | Audio pipeline and sync overhead |

## Benchmark Capture Template

```text
Phase:
Baseline Commit:
Candidate Commit:
Benchmark Command:
Key Metrics (before):
Key Metrics (after):
Result: PASS/FAIL
Notes:
```

## Correctness Capture Template

```text
Harness Command:
Checkpoint Summary:
Digest:
Result: PASS/FAIL
Notes:
```

## Roadmap Integration Targets

- [User Future Work Index](../../docs/FUTURE-WORK.md)
- [Q3 Platform Parity Program](platform-parity-research-program-2026-q3.md)
- [Platform Parity Research Index](../research/platform-parity/README.md)

## Issue Template Integration

A reusable GitHub issue template is defined at:

- `.github/ISSUE_TEMPLATE/platform-parity-phase-gate.yml`

Use this template for each new implementation phase to enforce gate evidence and traceability.

## Focused Atari + Genesis Command Set (Issue #748)

Use this set for fast local or CI parity checks specific to Atari 2600 and Genesis.

Quick-run correctness sweep:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="Atari2600*:Genesis*" --gtest_brief=1
```

Atari edge-case depth suite (includes deterministic edge coverage):

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="Atari2600DeterministicEdgeCaseTests.*" --gtest_brief=1
```

Genesis combined interaction suite (DMA/Z80/interrupt + deterministic replay):

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="GenesisCombinedInteractionTests.*" --gtest_brief=1
```

Focused benchmark sweep:

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_(Atari2600|Genesis)" --benchmark_repetitions=3
```

Structured benchmark artifact (CI-friendly):

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_(Atari2600|Genesis)" --benchmark_repetitions=3 --benchmark_out=bench-atari-genesis-focused.json --benchmark_out_format=json
```

Expected artifacts and usage:

- `Core.Tests.exe` output: correctness gate status for focused suites.
- `bench-atari-genesis-focused.json`: machine-readable benchmark baseline/candidate comparison input.
- Quick-run mode: use correctness sweep plus one benchmark pass for iteration.
- Full-run mode: run focused suites, then full platform matrix and full benchmark set.

## Reference Validation Workflow (Issue #745)

Use this workflow when validating against known-good emulator behavior and hardware expectations.

### Atari 2600 Checklist

- Confirm ROM identity before running checks: filename, mapper expectation, and checksum.
- Run Atari harness suites and save the full console output.
- Compare failure-context lines (`ROM_FAIL_CONTEXT`, `COMPAT_FAIL_CONTEXT`, `PERF_FAIL_CONTEXT`) against the current reference output.
- Compare deterministic digests across repeated runs; any drift is a fail.
- Compare timing-focused outputs (scanline/frame checkpoint contexts and timing digests) against the stored reference snapshot.

### Genesis Checklist

- Confirm ROM identity before running checks: filename, title-class expectation, and checksum.
- Run Genesis harness suites and save the full console output.
- Compare failure-context lines (`GEN_COMPAT_FAIL_CONTEXT`, `GEN_PERF_FAIL_CONTEXT`) against the current reference output.
- Compare deterministic digests across repeated runs; any drift is a fail.
- Compare render/audio/timing digest contexts against the stored reference snapshot.

### Reproducible Command Recipes

Build Release x64:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m
```

Atari harness validation:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="Atari2600TimingSpikeHarnessTests.*:Atari2600CompatibilityMatrixTests.*:Atari2600PerformanceGateTests.*" --gtest_brief=1
```

Genesis harness validation:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="GenesisCompatibilityHarnessTests.*:GenesisPerformanceGateTests.*:GenesisCombinedInteractionTests.*" --gtest_brief=1
```

Debugger responsiveness benchmark pack:

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_Debugger_BreakpointPath_UnorderedSetSparse|BM_Debugger_TraceRowFormatting_StdFormat|BM_Debugger_TraceRowFormatting_Append|BM_Debugger_RequestDispatch_Direct|BM_Debugger_RequestDispatch_StdFunction" --benchmark_repetitions=3 --benchmark_report_aggregates_only=true --benchmark_out=bench-debugger-hotpath.json --benchmark_out_format=json
```

Gap follow-up issues created from this workflow:

- [#751](https://github.com/TheAnsarya/Nexen/issues/751): automate Atari/Genesis reference-trace artifact generation for CI diffs.
- [#752](https://github.com/TheAnsarya/Nexen/issues/752): build Atari/Genesis reference ROM manifest schema and seed set.

### Plain-Language Explanation for #751 and #752

- #751 means: run the same focused tests and benchmarks every time and save the outputs to files in a predictable place.
- #752 means: keep a small example checklist of ROM entries used for regression checks so everyone runs the same test set.
- These are test quality tools. They do not add new emulation behavior directly.
- They help emulator work by catching regressions faster and making comparisons consistent.

### #751 Artifact Pack Command

```powershell
.\scripts\reference-validation-pack.ps1 -SkipBuild
```

Expected outputs:

- `artifacts/reference-validation/atari-harness.txt`
- `artifacts/reference-validation/genesis-harness.txt`
- `artifacts/reference-validation/atari-genesis-benchmarks.json`
- `artifacts/reference-validation/reference-validation-summary.json`

### #752 Example ROM Checklist File

- File: `~docs/testing/reference-rom-validation-manifest.example.json`
- Purpose: example ROM entries for standardized regression checks.
- Important: this file uses placeholders until real ROM hashes are filled in.

## Genesis-Only Fast CI Command Pack (Issue #765)

Use these commands for quick Genesis-only validation locally or in CI.

Genesis focused tests:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="GenesisCompatibilityHarnessTests.*:GenesisPerformanceGateTests.*:GenesisCombinedInteractionTests.*" --gtest_brief=1
```

Genesis debugger smoke tests:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="*Debugger*" --gtest_brief=1
```

Genesis-focused benchmark artifact output:

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_Debugger_BreakpointPath_UnorderedSetSparse|BM_Debugger_TraceRowFormatting_StdFormat|BM_Debugger_TraceRowFormatting_Append|BM_Debugger_RequestDispatch_Direct|BM_Debugger_RequestDispatch_StdFunction" --benchmark_repetitions=3 --benchmark_report_aggregates_only=true --benchmark_out=bench-genesis-fast-ci.json --benchmark_out_format=json
```

Artifact output path:

- `bench-genesis-fast-ci.json` at repository root.
- Upload this file in CI for benchmark comparison and trend review.

## Genesis Title Regression Pack (Issue #760)

Use this small title pack for quick Genesis regression checks:

- Sonic class (`sonic-*.bin` behavior path)
- Jurassic class (`jurassic-*.bin` behavior path)
- Generic class (all other names)

Quick-run command:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="GenesisCompatibilityHarnessTests.*:GenesisPerformanceGateTests.*" --gtest_brief=1
```

Machine-parseable summary lines produced by the harness:

- `GEN_COMPAT_RESULT <title> <PASS|FAIL> PASS=<n> FAIL=<n> DIGEST=<hash>`
- `GEN_COMPAT_MATRIX_SUMMARY PASS=<n> FAIL=<n> DIGEST=<hash>`
- `GEN_PERF_RESULT <title> <PASS|FAIL> CLASS=<class> ELAPSED_US=<n> BUDGET_US=<n> DIGEST=<hash>`
- `GEN_PERF_GATE_SUMMARY PASS=<n> FAIL=<n> BUDGET_US=<n> DIGEST=<hash>`

## Genesis Benchmark Artifact Validation Checks (Issue #768)

Use this command to generate a Genesis benchmark JSON artifact:

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_Genesis_AudioCadence_AlternatingYmPsg|BM_Genesis_AudioBusWriteAndMix|BM_Genesis_DmaContention_CopyBurst|BM_Genesis_DmaContention_FillBurst|BM_Genesis_PerformanceGate_Corpus" --benchmark_repetitions=3 --benchmark_report_aggregates_only=true --benchmark_out=bench-genesis-artifact-check.json --benchmark_out_format=json
```

Use this command to validate that the artifact exists and contains benchmark data:

```powershell
$artifact = Get-Content .\bench-genesis-artifact-check.json -Raw | ConvertFrom-Json
if (-not $artifact.benchmarks -or $artifact.benchmarks.Count -eq 0) { throw "No benchmark rows found in artifact." }
Write-Output ("artifact-ok benchmarks={0}" -f $artifact.benchmarks.Count)
```

Expected output files:

- `bench-genesis-artifact-check.json` at repository root.
- Optional CI copy in your artifacts folder (example: `artifacts/benchmarks/bench-genesis-artifact-check.json`).

## Genesis Benchmark Trend Guard Commands (Issue #774)

Use this process to compare baseline and candidate benchmark artifacts.

Baseline capture command:

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_Genesis_AudioCadence_AlternatingYmPsg|BM_Genesis_AudioBusWriteAndMix|BM_Genesis_DmaContention_CopyBurst|BM_Genesis_DmaContention_FillBurst|BM_Genesis_PerformanceGate_Corpus" --benchmark_repetitions=3 --benchmark_report_aggregates_only=true --benchmark_out=bench-genesis-baseline.json --benchmark_out_format=json
```

Candidate capture command:

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_Genesis_AudioCadence_AlternatingYmPsg|BM_Genesis_AudioBusWriteAndMix|BM_Genesis_DmaContention_CopyBurst|BM_Genesis_DmaContention_FillBurst|BM_Genesis_PerformanceGate_Corpus" --benchmark_repetitions=3 --benchmark_report_aggregates_only=true --benchmark_out=bench-genesis-candidate.json --benchmark_out_format=json
```

Comparison command:

```powershell
$baseline = Get-Content .\bench-genesis-baseline.json -Raw | ConvertFrom-Json
$candidate = Get-Content .\bench-genesis-candidate.json -Raw | ConvertFrom-Json

$baselineRows = @{}
foreach ($row in $baseline.benchmarks) {
	$baselineRows[$row.name] = $row
}

foreach ($cand in $candidate.benchmarks) {
	if ($baselineRows.ContainsKey($cand.name)) {
		$base = $baselineRows[$cand.name]
		$delta = [math]::Round((($cand.cpu_time - $base.cpu_time) / $base.cpu_time) * 100.0, 2)
		Write-Output ("{0} | baseline={1} | candidate={2} | delta={3}%" -f $cand.name, $base.cpu_time, $cand.cpu_time, $delta)
	}
}
```

Trend guard rule:

- Investigate any benchmark with `delta > 5%` slowdown.
- Keep baseline and candidate JSON files as CI artifacts for review.

## Reference Drift Comparator Script (Issues #787 and #798)

Use this script to compare baseline and candidate artifact packs and fail on digest drift or large benchmark deltas.

Command:

```powershell
powershell -File scripts/compare-reference-validation.ps1 -BaselineDir artifacts/reference-validation-baseline -CandidateDir artifacts/reference-validation
```

Behavior:

- Compares Atari and Genesis summary/digest lines from harness outputs.
- Validates benchmark JSON schema fields required by CI comparisons (`context.json_schema_version`, `benchmarks[*].name`, `run_name`, `run_type`, `repetitions`, `threads`, `iterations`, `real_time`, `cpu_time`, `time_unit`).
- Compares benchmark rows and flags absolute CPU-time delta above 5%.
- Returns exit code `1` on drift by default.
- Optional: write a machine-readable report with `-ReportPath artifacts/reference-validation/drift-report.json`.

Quick script self-test command:

```powershell
powershell -File scripts/test-reference-validation-compare.ps1
```

## Epic 16 Tracking Docs

- Closure matrix: [platform-parity-epic16-closure-matrix.md](platform-parity-epic16-closure-matrix.md)
- Weekly cadence and triage: [platform-parity-epic16-weekly-cadence.md](platform-parity-epic16-weekly-cadence.md)
