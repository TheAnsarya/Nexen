# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T14:55:18.7812489Z
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
- strictRequireNexenRefStartupEvents: True
- minNexenStartupCheckpointCount: 1
- minNexenStartupDisplayTransitionCount: 0
- minNexenRefStartupCheckpointCount: 1
- minNexenRefStartupDisplayTransitionCount: 0

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | NexenRef Startup Hash | Nexen Chkpt | Nexen Disp Tgl | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---:|---:|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 7bd1a71d620d71e9ac7da090654dcf9471d4e1c07749eda4315b977eaf789d22 | e86e6bb25255ba47982a72d130e40d3eb0c1f7d36b5adc7ebd6c924a8906127f | 0 | 1 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingNexenRefCount includes missing trace/startup-trace or skipped nexenRef run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
