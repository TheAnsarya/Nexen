# Lynx Commercial Bank-Addressing Validation Matrix (2026-03-30)

## Scope

Follow-up artifact for #1105 based on the prior shift-register audit from #956.

Objective:

- Define a commercial-title validation set
- Track boot and in-game behavior against cart bank/page addressing logic
- Capture address-bit anomalies and map them to targeted regression tests

## Validation Matrix

| Title | Cart Type/Notes | Boot Result | In-Game Result | Address-Bit Anomaly | Repro Notes | Regression Test Status |
|---|---|---|---|---|---|---|
| TODO: Title 01 | Bank-switched commercial cart | Pending | Pending | Pending | Pending | Not started |
| TODO: Title 02 | Bank-switched commercial cart | Pending | Pending | Pending | Pending | Not started |
| TODO: Title 03 | Bank-switched commercial cart | Pending | Pending | Pending | Pending | Not started |
| TODO: Title 04 | Bank-switched commercial cart | Pending | Pending | Pending | Pending | Not started |
| TODO: Title 05 | Bank-switched commercial cart | Pending | Pending | Pending | Pending | Not started |
| TODO: Title 06 | Bank-switched commercial cart | Pending | Pending | Pending | Pending | Not started |
| TODO: Title 07 | Bank-switched commercial cart | Pending | Pending | Pending | Pending | Not started |
| TODO: Title 08 | Bank-switched commercial cart | Pending | Pending | Pending | Pending | Not started |
| TODO: Title 09 | Bank-switched commercial cart | Pending | Pending | Pending | Pending | Not started |
| TODO: Title 10 | Bank-switched commercial cart | Pending | Pending | Pending | Pending | Not started |

## Execution Protocol

1. Select a title and record ROM identifier/checksum in local notes.
2. Verify boot path and initial input responsiveness.
3. Exercise at least one transition expected to trigger bank/page changes.
4. Capture observed vs expected behavior and map any mismatch to address-bit hypotheses.
5. Add or update targeted tests in Core test suites for confirmed anomalies.

## Linked Artifacts

- Prior audit: `~docs/research/lynx-cart-shift-register-addressing-audit-2026-03-30.md`
- Issue tracker: #1105
