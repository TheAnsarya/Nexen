# Harness Drift Comparator

The harness drift comparator detects digest regressions and benchmark shifts between a baseline and candidate artifact pack. It runs non-interactively and returns a machine-parseable report suitable for CI or local validation.

## Scripts

| Script | Purpose |
|---|---|
| `scripts/reference-validation-pack.ps1` | Generate an artifact pack (harness outputs + benchmark JSON) |
| `scripts/compare-reference-validation.ps1` | Compare two artifact packs and fail on drift |
| `scripts/test-reference-validation-compare.ps1` | Self-test suite for the comparator |

## Artifact Pack Contents

Each artifact pack lives in a directory and contains three files:

| File | Source |
|---|---|
| `atari-harness.txt` | Atari 2600 harness test output (summary/digest lines) |
| `genesis-harness.txt` | Genesis harness test output (summary/digest lines) |
| `atari-genesis-benchmarks.json` | Google Benchmark JSON for Atari and Genesis benchmarks |

A summary file (`reference-validation-summary.json`) is also written by the pack script.

## Baseline / Candidate Workflow

### Step 1 — Generate the Baseline

On a known-good commit, run the pack script to produce baseline artifacts:

```powershell
powershell -File scripts/reference-validation-pack.ps1 -OutputDir artifacts/reference-validation-baseline
```

Pass `-SkipBuild` if the Release x64 build is already up to date:

```powershell
powershell -File scripts/reference-validation-pack.ps1 -OutputDir artifacts/reference-validation-baseline -SkipBuild
```

### Step 2 — Make Changes and Build the Candidate

After applying code changes, build again and generate candidate artifacts:

```powershell
powershell -File scripts/reference-validation-pack.ps1 -OutputDir artifacts/reference-validation
```

### Step 3 — Compare

Run the comparator to detect drift:

```powershell
powershell -File scripts/compare-reference-validation.ps1 -BaselineDir artifacts/reference-validation-baseline -CandidateDir artifacts/reference-validation
```

The script exits with code `0` when no drift is detected and `1` when drift is found.

### Optional: JSON Report

Add `-ReportPath` to write a machine-readable drift report:

```powershell
powershell -File scripts/compare-reference-validation.ps1 -BaselineDir artifacts/reference-validation-baseline -CandidateDir artifacts/reference-validation -ReportPath artifacts/reference-validation/drift-report.json
```

### Optional: Allow Drift

Use `-AllowDrift` to log drift without failing (exit code remains `0`):

```powershell
powershell -File scripts/compare-reference-validation.ps1 -BaselineDir artifacts/reference-validation-baseline -CandidateDir artifacts/reference-validation -AllowDrift
```

## Machine-Parseable Output

### Digest Drift Lines

```text
[atari] digest-drift key=HARNESS_SUMMARY baseline=aabbccdd candidate=11223344
[genesis] digest-drift key=GEN_COMPAT_MATRIX_SUMMARY baseline=... candidate=...
```

### Summary Mismatch Lines

```text
[atari] summary-mismatch key=PERF_GATE_SUMMARY baseline='...' candidate='...'
```

### Benchmark Delta Lines

```text
[benchmarks] delta>5% name=BM_Genesis_DmaContention_CopyBurst baseline=123.4 candidate=145.6 delta=18.0%
```

### Schema Validation Lines

```text
[schema] missing-field label=candidate row=0 field=cpu_time
```

### JSON Report Schema

When `-ReportPath` is used, the report contains:

```json
{
	"baselineDir": "C:\\...\\baseline",
	"candidateDir": "C:\\...\\candidate",
	"schemaIssues": [],
	"drift": ["[atari] digest-drift key=..."],
	"allowDrift": false,
	"status": "drift"
}
```

`status` is `"pass"` when no drift or schema issues exist, `"drift"` otherwise.

## Recognized Summary Line Patterns

The comparator parses these machine-readable summary lines from harness output:

| Pattern | Platform |
|---|---|
| `HARNESS_SUMMARY PASS=<n> FAIL=<n> DIGEST=<hash>` | Atari 2600 |
| `ROM_SET_SUMMARY PASS=<n> FAIL=<n> DIGEST=<hash>` | Atari 2600 |
| `COMPAT_MATRIX_SUMMARY PASS=<n> FAIL=<n> DIGEST=<hash>` | Atari 2600 |
| `PERF_GATE_SUMMARY PASS=<n> FAIL=<n> BUDGET_US=<n> DIGEST=<hash>` | Atari 2600 |
| `TIMING_SPIKE SUMMARY STABLE=<bool> DIGEST=<hash>` | Atari 2600 |
| `GEN_COMPAT_MATRIX_SUMMARY PASS=<n> FAIL=<n> DIGEST=<hash>` | Genesis |
| `GEN_PERF_GATE_SUMMARY PASS=<n> FAIL=<n> BUDGET_US=<n> DIGEST=<hash>` | Genesis |

## Benchmark JSON Validation

The comparator validates required fields in the Google Benchmark JSON schema:

- Root: `context.json_schema_version`, `benchmarks[]`
- Per row: `name`, `run_name`, `run_type`, `repetitions`, `threads`, `iterations`, `real_time`, `cpu_time`, `time_unit`

Numeric fields (`repetitions`, `threads`, `iterations`, `real_time`, `cpu_time`) are validated as parseable numbers.

Any benchmark row with an absolute CPU-time delta greater than 5% between baseline and candidate is flagged as drift.

## CI Integration

### GitHub Actions Example

```yaml
steps:
  - name: Build Release x64
    run: msbuild Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m

  - name: Generate baseline artifacts
    run: powershell -File scripts/reference-validation-pack.ps1 -OutputDir artifacts/baseline -SkipBuild

  # (apply candidate changes here, or use a matrix with baseline/candidate commits)

  - name: Generate candidate artifacts
    run: powershell -File scripts/reference-validation-pack.ps1 -OutputDir artifacts/candidate -SkipBuild

  - name: Compare baseline and candidate
    run: >
      powershell -File scripts/compare-reference-validation.ps1
      -BaselineDir artifacts/baseline
      -CandidateDir artifacts/candidate
      -ReportPath artifacts/drift-report.json

  - name: Upload drift report
    if: always()
    uses: actions/upload-artifact@v4
    with:
      name: drift-report
      path: artifacts/drift-report.json
```

### Local Quick Check

```powershell
# Run the comparator self-test
powershell -File scripts/test-reference-validation-compare.ps1
```

## Related Documentation

- [Platform Parity Benchmark and Correctness Gates](../~docs/plans/platform-parity-benchmark-and-correctness-gates.md) — gate framework and command recipes
- [Atari 2600 TIA Timing Spike and Harness Plan](../~docs/plans/atari-2600-tia-timing-spike-harness.md) — Atari harness output contract
- [Genesis VDP Timing and DMA Milestone Plan](../~docs/plans/genesis-vdp-dma-milestones-and-harness.md) — Genesis harness output contract
- [Epic 16 Closure Matrix](../~docs/plans/platform-parity-epic16-closure-matrix.md) — issue-to-evidence traceability
