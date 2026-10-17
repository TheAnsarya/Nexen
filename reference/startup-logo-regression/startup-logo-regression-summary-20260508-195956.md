# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T20:01:25.1730286Z
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
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 29266cad319469bc2cb20b3cd12d6ce337768906393616fae3dbfb343e9a8a77 | 2caaea5d2ca66029f448d18ea16b9193f580942dba53ef69fd2bdb085dd60302 | 601/432 | 1/1 | 1202/864 | 9166/2621 | 1684/947 | 0/0 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingNexenRefCount includes missing trace/startup-trace or skipped nexenRef run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
