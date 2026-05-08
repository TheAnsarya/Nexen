# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T15:42:53.7021690Z
- frameStart: 0
- frameEnd: 600
- autoStopTimeoutSeconds: 45
- maxLines: 1200000
- romCount: 1
- regressionCount: 0
- missingMesenCount: 0
- policyFailureCount: 0
- anyErrors: False
- startupProfiles: strict-startup
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
| sonic-the-hedgehog-usa-europe | strict-startup | 3 | 0 | 0 | b1154d6f831abfc4709e55e7acfb2eb10ccbe3d2b37aa8971a473c2e9ae4134c | 6c749c9e124877e46d50684f7c297d35aa7b6b7331a24d3e528061dfe82c01d1 | 61/416 | 1/1 | 122/832 | 9165/2301 | 1144/867 | 0/0 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingMesenCount includes missing trace/startup-trace or skipped Mesen run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
