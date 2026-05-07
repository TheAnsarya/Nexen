# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-07T21:38:14.4788011Z
- frameStart: 0
- frameEnd: 600
- autoStopTimeoutSeconds: 45
- maxLines: 1200000
- romCount: 2
- regressionCount: 0
- missingMesenCount: 2
- policyFailureCount: 0
- anyErrors: False
- startupProfiles: logo-compat,mesen
- strictRequireMesenTraces: False
- failOnWramDiff: False
- failOnStartupDiff: False
- failOnMissingStartupMetrics: False
- minNexenStartupCheckpointCount: 1
- minNexenStartupDisplayTransitionCount: 0

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | Mesen Startup Hash | Nexen Chkpt | Nexen Disp Tgl | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---:|---:|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 1b4e57018dc15a5e7a44457e0d074c19bebb7b381fd06be3b02707be6da5b347 | missing | 0 | 1 |  |  | pass |
| sonic-the-hedgehog-usa-europe | mesen | 3 | 0 | 0 | d539c780cc0271e8d5f00fac5b5caefe46754a86c10372c64964576baf46bb45 | missing | 0 | 1 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingMesenCount includes missing trace/startup-trace or skipped Mesen run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
