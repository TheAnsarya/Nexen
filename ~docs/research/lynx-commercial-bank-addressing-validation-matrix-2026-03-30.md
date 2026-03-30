# Lynx Commercial Bank-Addressing Validation Matrix (2026-03-30)

## Scope

Follow-up artifact for #1105 based on the prior shift-register audit from #956.

Objective:

- Define a commercial-title validation set
- Track boot and in-game behavior against cart bank/page addressing logic
- Capture address-bit anomalies and map them to targeted regression tests

## Validation Matrix

Legend:

- `Pending`: Not executed yet
- `Pass`: Expected behavior observed
- `Fail`: Reproducible mismatch requiring anomaly note + test
- `N/A`: Not applicable for this title/scenario

| Title | Rotation | Cart Type/Notes | ROM Checksum | Boot Result | In-Game Result | Address-Bit Anomaly | Evidence Link/Artifact | Regression Test Status |
|---|---|---|---|---|---|---|---|---|
| California Games | none | Commercial, baseline gameplay + menu transitions | daddc150143a09ae792289ab0f1215fe0955f34bdc09fb595e8cf027340e5200 | Pending | Pending | Pending | ROM manifest (2026-03-30) | Not started |
| Chip's Challenge | none | Commercial, puzzle room transitions with frequent map changes | 5a55f01511e382297f86b073f71f7e1b504762d323a0c7e8ee2bbc1e6633ce45 | Pending | Pending | Pending | ROM manifest (2026-03-30) | Not started |
| Klax | right | Commercial, right-rotation title from manual test set | ab61cf96743fd1310b74ca9b340e1bfd19f5a9b7077e2e77f519f7964df707c1 | Pending | Pending | Pending | ROM manifest (2026-03-30) | Not started |
| Awesome Golf | right | Commercial, right-rotation + repeated scene transitions | aa1d5930f70673da746ecd905fc682ace4c1e1258d912f4e2ad8e22f790ad4a0 | Pending | Pending | Pending | ROM manifest (2026-03-30) | Not started |
| Warbirds | none | Commercial, action workload and rapid state transitions | b8d72524f55ff26fa52dad5251761a7298072efdc38554873da930bebc4eda0c | Pending | Pending | Pending | ROM manifest (2026-03-30) | Not started |
| Electrocop | none | Commercial, heavy scene/data transitions | 3718fdcc548fdcfcd78584abfa49f0848caed70249a3257bf77be324f3770246 | Pending | Pending | Pending | ROM manifest (2026-03-30) | Not started |
| Zarlor Mercenary | none | Commercial, shooter progression transitions | e723d5dacf3600210374ab3e27da667d36ecb555842679ddc99bddaecd06e8e1 | Pending | Pending | Pending | ROM manifest (2026-03-30) | Not started |
| Blue Lightning | none | Commercial, rapid streaming-style gameplay transitions | 148fcef35ad7985d2ba19a5fe7900ca7194e3267a7b4a3e422b07f9a471e7273 | Pending | Pending | Pending | ROM manifest (2026-03-30) | Not started |
| Ninja Gaiden | none | Commercial, scripted level progression checks | ab88a92efe52377d0de7f697291b70247a8ff68d4ffedfd3e57d4f29b04186b1 | Pending | Pending | Pending | ROM manifest (2026-03-30) | Not started |
| Gauntlet: The Third Encounter | none | Commercial, dungeon/room transition stress | 21a83624a636fde4d6f6a01363ebd7644dd5c3da19013525aae11ff0aeb2eb60 | Pending | Pending | Pending | ROM manifest (2026-03-30) | Not started |

## Evidence Capture Fields

For each executed row, capture and paste the following in issue comments or local run logs:

1. ROM file name and checksum (`sha256` preferred)
2. Boot mode (`HLE` or `Boot ROM`)
3. First verified gameplay milestone (title/menu/level)
4. Transition used to stress bank/page addressing
5. Observed result (`Pass`/`Fail`) and exact visible symptom
6. If `Fail`: suspected address-bit pattern and probable source path

Recommended quick template:

```text
Title:
ROM:
Checksum:
Boot mode:
Milestone reached:
Transition exercised:
Result:
Anomaly hypothesis:
Follow-up test:
```

## Execution Protocol

1. Select a title row and record ROM checksum in the matrix.
2. Validate both startup and at least one in-game transition that should touch cart paging.
3. Mark `Boot Result` and `In-Game Result` with `Pass`/`Fail`.
4. If mismatch is found, write explicit anomaly hypothesis in `Address-Bit Anomaly`.
5. Add targeted regression coverage in `Core.Tests/Lynx/LynxCartTests.cpp` (or closest suite) and update row test status.
6. Post row evidence in #1105, linking any new tests/commits.

## Closure Gate For #1105

#1105 can be closed when all conditions are satisfied:

1. All matrix rows have non-pending checksum and results.
2. Any `Fail` row has a documented anomaly hypothesis.
3. Each confirmed anomaly has a deterministic regression test.
4. New/updated tests pass in Release x64 validation.

## Linked Artifacts

- Prior audit: `~docs/research/lynx-cart-shift-register-addressing-audit-2026-03-30.md`
- ROM manifest: `~docs/research/lynx-commercial-bank-addressing-rom-manifest-2026-03-30.md`
- Issue tracker: #1105
