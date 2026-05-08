# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T15:00:46.8319562Z
- frameStart: 0
- frameEnd: 600
- autoStopTimeoutSeconds: 45
- maxLines: 1200000
- romCount: 1
- regressionCount: 0
- missingMesenCount: 0
- policyFailureCount: 0
- anyErrors: False
- startupProfiles: logo-compat
- strictRequireMesenTraces: True
- failOnWramDiff: False
- failOnStartupDiff: False
- failOnMissingStartupMetrics: False
- strictRequireMesenStartupEvents: False
- strictRequireBothStartupCheckpointEvents: True
- minNexenStartupCheckpointCount: 1
- minNexenStartupDisplayTransitionCount: 0
- minMesenStartupCheckpointCount: 1
- minMesenStartupDisplayTransitionCount: 0

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | Mesen Startup Hash | Nexen Chkpt | Nexen Disp Tgl | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---:|---:|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 2c4d4b10f430ca30edd1eba5d1e899cc2326dfc7a000726118dba5dcf00f9741 | 53dea00c4adc9c35e50d09561b4333de9161de44d7ce741262de64a51cc02752 | 601 | 1 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingMesenCount includes missing trace/startup-trace or skipped Mesen run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
