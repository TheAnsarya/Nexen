# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T15:00:46.8319562Z
- frameStart: 0
- frameEnd: 600
- autoStopTimeoutSeconds: 45
- maxLines: 1200000
- romCount: 1
- regressionCount: 0
- missingNexenRefCount: 0
- policyFailureCount: 0
- anyErrors: False
- startupProfiles: logo-compat
- strictRequireNexenRefTraces: True
- failOnWramDiff: False
- failOnStartupDiff: False
- failOnMissingStartupMetrics: False
- strictRequireNexenRefStartupEvents: False
- strictRequireBothStartupCheckpointEvents: True
- minNexenStartupCheckpointCount: 1
- minNexenStartupDisplayTransitionCount: 0
- minNexenRefStartupCheckpointCount: 1
- minNexenRefStartupDisplayTransitionCount: 0

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | NexenRef Startup Hash | Nexen Chkpt | Nexen Disp Tgl | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---:|---:|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 2c4d4b10f430ca30edd1eba5d1e899cc2326dfc7a000726118dba5dcf00f9741 | 53dea00c4adc9c35e50d09561b4333de9161de44d7ce741262de64a51cc02752 | 601 | 1 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingNexenRefCount includes missing trace/startup-trace or skipped nexenRef run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
