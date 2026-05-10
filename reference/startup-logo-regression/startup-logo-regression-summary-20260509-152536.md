# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-09T15:26:43.8052525Z
- frameStart: 0
- frameEnd: 120
- autoStopTimeoutSeconds: 30
- maxLines: 250000
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
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 1 | 2 | e29485e093ee654cf1a1ae78a673bc5e1e7b941e7dbe4e84deb5fbb7d0c9eaf8 | d6b02a85b495c939e148b3b08134f6a357154e27c1dfb79fe31112e9b26e7306 | 600/117 | 1/0 | 1200/234 | 2695/465 | 1680/208 | 0/0 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingNexenRefCount includes missing trace/startup-trace or skipped nexenRef run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
