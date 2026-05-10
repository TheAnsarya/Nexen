# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T19:13:52.9517062Z
- frameStart: 220
- frameEnd: 600
- autoStopTimeoutSeconds: 45
- maxLines: 1200000
- romCount: 1
- regressionCount: 0
- missingNexenRefCount: 0
- policyFailureCount: 0
- anyErrors: False
- startupProfiles: logo-compat
- strictRequireNexenRefTraces: False
- strictStartupParity: False
- failOnWramDiff: False
- failOnStartupDiff: False
- failOnMissingStartupMetrics: False
- strictRequireNexenRefStartupEvents: False
- strictRequireBothStartupCheckpointEvents: False
- strictRequireBothStartupDisplayTransitionEvents: False
- strictRequireBothStartupPaletteCheckpointEvents: False
- strictRequireBothStartupVdpSnapshotEvents: False
- strictRequireBothStartupVdpRegisterWriteEvents: False
- strictRequireBothStartupTmssUnlockEvents: False
- strictRequireBothStartupZ80RuntimeToggleEvents: False
- minNexenStartupCheckpointCount: 1
- minNexenStartupDisplayTransitionCount: 0
- minNexenStartupPaletteCheckpointCount: 1
- minNexenStartupVdpSnapshotCount: 1
- minNexenStartupVdpRegisterWriteCount: 1
- minNexenStartupTmssUnlockCount: 0
- minNexenStartupZ80RuntimeToggleCount: 0
- minNexenRefStartupCheckpointCount: 1
- minNexenRefStartupDisplayTransitionCount: 0
- minNexenRefStartupPaletteCheckpointCount: 1
- minNexenRefStartupVdpSnapshotCount: 1
- minNexenRefStartupVdpRegisterWriteCount: 1
- minNexenRefStartupTmssUnlockCount: 0
- minNexenRefStartupZ80RuntimeToggleCount: 0

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | NexenRef Startup Hash | N/M Chkpt | N/M Disp | N/M Pal | N/M VdpRegW | N/M Z80Tgl | N/M TmssUnlock | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---|---|---|---|---|---|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 4771f201269749968438e7e7a7236fa58eab29a523f1efb32b26a0924bd939ed | edb740f8330c8d57b17ffea8ea80c37a18bf27f51eb492ae356f4bdc60cd8b0e | 601/423 | 1/1 | 1202/846 | 9166/2441 | 1684/902 | 0/0 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingNexenRefCount includes missing trace/startup-trace or skipped nexenRef run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
