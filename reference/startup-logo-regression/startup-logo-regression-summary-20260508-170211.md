# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T17:03:23.8973182Z
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
- strictStartupParity: True
- failOnWramDiff: False
- failOnStartupDiff: False
- failOnMissingStartupMetrics: False
- strictRequireMesenStartupEvents: False
- strictRequireBothStartupCheckpointEvents: True
- strictRequireBothStartupDisplayTransitionEvents: True
- strictRequireBothStartupPaletteCheckpointEvents: True
- strictRequireBothStartupVdpSnapshotEvents: True
- strictRequireBothStartupVdpRegisterWriteEvents: True
- strictRequireBothStartupTmssUnlockEvents: True
- strictRequireBothStartupZ80RuntimeToggleEvents: True
- minNexenStartupCheckpointCount: 1
- minNexenStartupDisplayTransitionCount: 1
- minNexenStartupPaletteCheckpointCount: 1
- minNexenStartupVdpSnapshotCount: 1
- minNexenStartupVdpRegisterWriteCount: 1
- minNexenStartupTmssUnlockCount: 0
- minNexenStartupZ80RuntimeToggleCount: 1
- minMesenStartupCheckpointCount: 1
- minMesenStartupDisplayTransitionCount: 1
- minMesenStartupPaletteCheckpointCount: 1
- minMesenStartupVdpSnapshotCount: 1
- minMesenStartupVdpRegisterWriteCount: 1
- minMesenStartupTmssUnlockCount: 0
- minMesenStartupZ80RuntimeToggleCount: 1

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | Mesen Startup Hash | N/M Chkpt | N/M Disp | N/M Pal | N/M VdpRegW | N/M Z80Tgl | N/M TmssUnlock | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---|---|---|---|---|---|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 345dad9c32d2eeea1037cc5cb4e63cfbcf6e35249341aa8c290bab42b622f070 | 1ae794eaeafd9913fc620f1095adfc7dfeb20ca35e51578099678dd6f29dbf49 | 601/418 | 1/1 | 1202/836 | 9165/2341 | 1684/877 | 0/0 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingMesenCount includes missing trace/startup-trace or skipped Mesen run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
