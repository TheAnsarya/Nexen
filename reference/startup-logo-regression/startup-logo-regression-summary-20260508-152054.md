# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T15:25:51.7547679Z
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
- strictRequireBothStartupVdpRegisterWriteEvents: False
- strictRequireBothStartupTmssUnlockEvents: True
- strictRequireBothStartupZ80RuntimeToggleEvents: False
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
- minNexenRefStartupZ80RuntimeToggleCount: 0

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | NexenRef Startup Hash | N/M Chkpt | N/M Disp | N/M Pal | N/M VdpRegW | N/M Z80Tgl | N/M TmssUnlock | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---|---|---|---|---|---|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 63a48c49551108afbbe8133ec10dce244f84c89052c99e7f1263f29b18c87d68 | ce9a9f95ca8a51a2c12faa915e087e6d7ecd784f3e5de27185f08dc210ed58a0 | 601/413 | 1/1 | 601/413 | 6459/0 | 601/0 | 0/0 |  |  | pass |
| sonic-the-hedgehog-usa-europe | nexenref-startup | 3 | 0 | 0 | 22fa8e7e522a65c6a6fca4b5bcb77b194465655702247a7e39a4104b6b0f1fa2 | cb5a9db77f7fbd610699b5319023ee76eaa974e9f5fd30350c319dab3e6ba35d | 601/414 | 1/1 | 601/414 | 6459/0 | 601/0 | 0/0 |  |  | pass |
| sonic-the-hedgehog-usa-europe | strict-startup | 3 | 0 | 0 | a9e1612e19113fe0c530b0797c8d8bcee44d32fe179559a8c726c748a9444c9b | c4e7dfa68a6f7a3180b2289bc0f259884f8ec4e3ff9772201412ce7a1da02dbf | 61/413 | 1/1 | 61/413 | 6459/0 | 61/0 | 0/0 |  |  | pass |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingNexenRefCount includes missing trace/startup-trace or skipped nexenRef run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
