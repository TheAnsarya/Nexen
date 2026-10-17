# Genesis Startup Logo Regression Summary

- generatedUtc: 2026-05-08T15:20:27.4959396Z
- frameStart: 0
- frameEnd: 600
- autoStopTimeoutSeconds: 45
- maxLines: 1200000
- romCount: 3
- regressionCount: 0
- missingNexenRefCount: 0
- policyFailureCount: 3
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
- minNexenStartupZ80RuntimeToggleCount: 1
- minNexenRefStartupCheckpointCount: 1
- minNexenRefStartupDisplayTransitionCount: 1
- minNexenRefStartupPaletteCheckpointCount: 1
- minNexenRefStartupVdpSnapshotCount: 1
- minNexenRefStartupVdpRegisterWriteCount: 1
- minNexenRefStartupTmssUnlockCount: 0
- minNexenRefStartupZ80RuntimeToggleCount: 1

| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | NexenRef Startup Hash | N/M Chkpt | N/M Disp | N/M Pal | N/M VdpRegW | N/M Z80Tgl | N/M TmssUnlock | Regressions | Policy Failures | Verdict |
|---|---|---:|---:|---:|---|---|---|---|---|---|---|---|---|---|---|
| sonic-the-hedgehog-usa-europe | logo-compat | 3 | 0 | 0 | 4de2b066757aba7db6be4047fb9e25212dea4347c7f3f0d2f5203c672838b5c8 | 5b626a7c14021cb77d93f677dcedbca50e9febdac345ca4c93a31464fe320633 | 601/415 | 1/1 | 601/415 | 6459/0 | 601/0 | 0/0 |  | dualStartupVdpRegisterWrite.nexenRef,dualStartupZ80RuntimeToggle.nexenRef | policy-fail |
| sonic-the-hedgehog-usa-europe | nexenref-startup | 3 | 0 | 0 | 305e194cc872ecce753ced41964b9208658503dace5f6f38cee50dca3fb3fd18 | 34ea80baf5cb70b5ee4b83cbead0c16926d46369199ca59c24d5aa86c8258375 | 601/415 | 1/1 | 601/415 | 6459/0 | 601/0 | 0/0 |  | dualStartupVdpRegisterWrite.nexenRef,dualStartupZ80RuntimeToggle.nexenRef | policy-fail |
| sonic-the-hedgehog-usa-europe | strict-startup | 3 | 0 | 0 | af0b088bca3ee9e4489d3f18e536be82bdd9acc0c12082cf1d8449a99b990910 | f41889b354aafb538a0b750d661f98fa9615ef228abf83ad3aa0a94dc134f218 | 61/416 | 1/1 | 61/416 | 6459/0 | 61/0 | 0/0 |  | dualStartupVdpRegisterWrite.nexenRef,dualStartupZ80RuntimeToggle.nexenRef | policy-fail |

## Notes

- compareExitCode=3 indicates first-difference detection from the compare script.
- missingNexenRefCount includes missing trace/startup-trace or skipped nexenRef run.
- policy failures are controlled by strict gate switches and do not imply infrastructure errors.
- Use -UpdateBaseline to refresh baseline hashes and startup counters.
