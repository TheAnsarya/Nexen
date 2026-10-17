# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-07T21:39:59.1669239Z
- frameStart: 0
- frameEnd: 600
- autoStopTimeoutSeconds: 45
- maxLines: 1200000
- romCount: 1
- regressionCount: 0
- missingNexenRefCount: 1
- policyFailureCount: 1
- anyErrors: False
- startupProfiles: logo-compat
- strictRequireNexenRefTraces: True
- failOnWramDiff: False
- failOnStartupDiff: False
- failOnMissingStartupMetrics: False
- minNexenStartupCheckpointCount: 1
- minNexenStartupDisplayTransitionCount: 0

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | NexenRef Startup Hash | Nexen Chkpt | Nexen Disp Tgl | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---:|---:|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 0830bdf1cc009bc4fe66d8d22196906567044f0a674c6e2b54874fe4b78249bc | missing | 0 | 1 |  | missingNexenRefTrace | policy-fail |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingNexenRefCount includes missing trace/startup-trace or skipped nexenRef run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
