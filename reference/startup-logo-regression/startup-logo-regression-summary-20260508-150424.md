# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T15:05:26.4773037Z
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
- strictRequireBothStartupDisplayTransitionEvents: True
- minNexenStartupCheckpointCount: 1
- minNexenStartupDisplayTransitionCount: 1
- minMesenStartupCheckpointCount: 1
- minMesenStartupDisplayTransitionCount: 1

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | Mesen Startup Hash | Nexen Chkpt | Nexen Disp Tgl | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---:|---:|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 2924388ee22a67603c68eb9d360b8a3230e32be77c9a9a728db4d56966c6fdfa | e2df0c6eb05bd67a382a158ced613ff9909cdcc2e95c9ebe88be8a411a7643cb | 601 | 1 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingMesenCount includes missing trace/startup-trace or skipped Mesen run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
