# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T19:25:05.0948052Z
- frameStart: 220
- frameEnd: 600
- autoStopTimeoutSeconds: 45
- maxLines: 1200000
- romCount: 1
- regressionCount: 0
- missingMesenCount: 0
- policyFailureCount: 0
- anyErrors: False
- startupProfiles: logo-compat
- strictRequireMesenTraces: False
- strictStartupParity: False
- failOnWramDiff: False
- failOnStartupDiff: False
- failOnMissingStartupMetrics: False
- strictRequireMesenStartupEvents: False
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
- minMesenStartupCheckpointCount: 1
- minMesenStartupDisplayTransitionCount: 0
- minMesenStartupPaletteCheckpointCount: 1
- minMesenStartupVdpSnapshotCount: 1
- minMesenStartupVdpRegisterWriteCount: 1
- minMesenStartupTmssUnlockCount: 0
- minMesenStartupZ80RuntimeToggleCount: 0

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | Mesen Startup Hash | N/M Chkpt | N/M Disp | N/M Pal | N/M VdpRegW | N/M Z80Tgl | N/M TmssUnlock | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---|---|---|---|---|---|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | f018323e8254e26feb09a7b577f77314adc360daeff63d4cadc963ed2d27fc7f | 30ba4511a73a204d52510831d6b63d03469ce723561c845b29efa8f14f4c0d64 | 601/427 | 1/1 | 1202/854 | 9166/2521 | 1684/922 | 0/0 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingMesenCount includes missing trace/startup-trace or skipped Mesen run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
