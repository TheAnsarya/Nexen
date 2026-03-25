# Issue #169 Approval Sheet (Initial Icon Set)

Select one option per icon:

- Keep = keep current PNG
- B2 = use Batch 2 ColorBold SVG candidate
- B3 = use Batch 3 ColorBadge SVG candidate

Reference catalogs:

- Batch 2: ../../~docs/plans/icon-replacement-catalog-169-batch2-colorbold.md
- Batch 3: ../../~docs/plans/icon-replacement-catalog-169-batch3-colorbadge.md

## Decision Table

| Icon | Keep | B2 | B3 | Decision |
|---|---|---|---|---|
| Help.png | [ ] | [x] | [ ] | B2 (white ? contrast) |
| Warning.png | [ ] | [x] | [ ] | B2 |
| Error.png | [ ] | [x] | [ ] | B2 |
| Record.png | [ ] | [x] | [ ] | B2 (keep current Batch 2 record) |
| MediaPlay.png | [ ] | [x] | [ ] | B2 (green triangle) |
| MediaPause.png | [ ] | [x] | [ ] | B2 (yellow ||) |
| MediaStop.png | [ ] | [x] | [ ] | B2 (red square) |
| Close.png | [ ] | [x] | [ ] | B2 (red X, bolder/larger) |
| Settings.png | [ ] | [x] | [ ] | B2 |
| Find.png | [ ] | [x] | [ ] | B2 |
| CheatCode.png | [ ] | [x] | [ ] | B2 (pass2 high-usage) |
| Enum.png | [ ] | [x] | [ ] | B2 (pass2 high-usage) |
| Edit.png | [ ] | [x] | [ ] | B2 (pass2 high-usage) |
| Drive.png | [ ] | [x] | [ ] | B2 (pass2 high-usage) |
| Function.png | [ ] | [x] | [ ] | B2 (pass2 high-usage) |
| NextArrow.png | [ ] | [x] | [ ] | B2 (pass2 high-usage) |
| PreviousArrow.png | [ ] | [x] | [ ] | B2 (pass2 high-usage) |
| NesIcon.png | [ ] | [x] | [ ] | B2 (pass3 system/platform) |
| SnesIcon.png | [ ] | [x] | [ ] | B2 (pass3 system/platform) |
| GameboyIcon.png | [ ] | [x] | [ ] | B2 (pass3 system/platform) |
| GbaIcon.png | [ ] | [x] | [ ] | B2 (pass3 system/platform) |
| PceIcon.png | [ ] | [x] | [ ] | B2 (pass3 system/platform) |
| SmsIcon.png | [ ] | [x] | [ ] | B2 (pass3 system/platform) |
| WsIcon.png | [ ] | [x] | [ ] | B2 (pass3 system/platform) |
| LynxIcon.png | [ ] | [x] | [ ] | B2 (pass3 system/platform) |
| EditLabel.png | [ ] | [x] | [ ] | B2 (pass3 utility) |
| Add.png | [ ] | [x] | [ ] | B2 (pass3 utility) |
| Copy.png | [ ] | [x] | [ ] | B2 (pass3 utility) |
| Paste.png | [ ] | [x] | [ ] | B2 (pass3 utility) |
| Reload.png | [ ] | [x] | [ ] | B2 (pass3 utility) |
| SaveFloppy.png | [ ] | [x] | [ ] | B2 (pass3 utility) |
| Undo.png | [ ] | [x] | [ ] | B2 (pass3 utility) |

## Notes

- Implementation status (2026-03-18): all currently approved B2 icons in the decision table are implemented in production as SVG files and verified in sync with proposed ColorBold assets.

- I will only apply approved rows.
- First replacement commit will be limited to this initial set for easy rollback/review.
- Help B2 uses a slightly smaller white ? on blue.
- Second pass continues the same ColorBold aesthetic for the next highest-usage icons from inventory.
- Third pass extends the same ColorBold aesthetic to system/platform icons and frequent utility icons.
- Third-pass system/platform SVGs were redesigned after feedback to replace generic controller glyphs.
- Third-pass system/platform SVGs were refined again to use system/logo-centric marks (SNES no-text 4-color logo mark, DMG-style Game Boy, purple GBA, and revised NES/PCE/SMS/WS designs).
- Third-pass system/platform SVGs were refined again after additional feedback with research-driven shape tuning: SNES no-background mark with corrected color order, slightly larger Game Boy, and improved NES/GBA/PCE/SMS/WonderSwan silhouettes.
- Third-pass system/platform SVGs received a micro-pass for geometry fidelity: SNES blue/red cutout treatment and additional NES/GBA/PCE/SMS/WonderSwan silhouette tuning.
