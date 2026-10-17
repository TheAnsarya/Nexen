# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T15:40:44.0795301Z
- frameStart: 0
- frameEnd: 600
- autoStopTimeoutSeconds: 45
- maxLines: 1200000
- romCount: 3
- regressionCount: 0
- missingNexenRefCount: 0
- policyFailureCount: 0
- anyErrors: False
- startupProfiles: logo-compat,nexenref-startup,strict-startup
- strictRequireNexenRefTraces: True
- strictStartupParity: True
- failOnWramDiff: False
- failOnStartupDiff: False
- failOnMissingStartupMetrics: False
- strictRequireNexenRefStartupEvents: False
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
- minNexenStartupZ80RuntimeToggleCount: 0
- minNexenRefStartupCheckpointCount: 1
- minNexenRefStartupDisplayTransitionCount: 1
- minNexenRefStartupPaletteCheckpointCount: 1
- minNexenRefStartupVdpSnapshotCount: 1
- minNexenRefStartupVdpRegisterWriteCount: 1
- minNexenRefStartupTmssUnlockCount: 0
- minNexenRefStartupZ80RuntimeToggleCount: 1

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | NexenRef Startup Hash | N/M Chkpt | N/M Disp | N/M Pal | N/M VdpRegW | N/M Z80Tgl | N/M TmssUnlock | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---|---|---|---|---|---|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 0690916919e8cabc24dce866864a07ad62a611841878ea2dae4df45dd33e7735 | 82555261ab772120c5e4235daf3d7e0bea69d6fb3e4ffbafc2c06f29c17b2280 | 601/404 | 1/1 | 1202/808 | 9165/2061 | 1684/807 | 0/0 |  |  | pass |
| sonic-the-hedgehog-usa-europe | nexenref-startup | 3 | 0 | 0 | e1842381cc43cbe272e2757f9274c7b2164e2124e58a3f03ae5dc28f27a1362f | 0a71f4e95a5f41f5834c079c5487c0ebeba810e2719f453086b810d026ff4db8 | 601/406 | 1/1 | 1202/812 | 9165/2101 | 1684/817 | 0/0 |  |  | pass |
| sonic-the-hedgehog-usa-europe | strict-startup | 3 | 0 | 0 | 4d30a1a8549a66430f26cdc61b7da25b809877a98f6fd992b793aec2eae376d4 | 8d7e933bf8dc81c17a5b92a13aa5125aabceefcc784641d492c2b68ba88d398f | 61/414 | 1/1 | 122/828 | 9165/2261 | 1144/857 | 0/0 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingNexenRefCount includes missing trace/startup-trace or skipped nexenRef run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
