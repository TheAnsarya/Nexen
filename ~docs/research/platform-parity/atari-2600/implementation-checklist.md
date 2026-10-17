# Atari 2600 Implementation Checklist

Issue linkage: [#704](https://github.com/TheAnsarya/Nexen/issues/704)

## Session Checklist

| Step | Goal | Exit Criteria |
|---|---|---|
| S1 | CPU loop skeleton | Deterministic instruction stepping and cycle accounting |
| S2 | TIA timing skeleton | Stable scanline progression and WSYNC handling |
| S3 | RIOT timer/I/O skeleton | Timer and basic input path validated by focused tests |
| S4 | Mirror decode correctness | Address mirror tests pass for TIA and RIOT |
| S5 | Minimal bankswitch set | Baseline mapper set boots reference ROM subset |
| S6 | Harness hardening | Per-cycle traces and replay checks integrated in CI |

## Quality Gates

- Accuracy first: no performance-only shortcuts in timing paths.
- Determinism: repeated runs produce identical trace summaries.
- Issue tracking: each checklist step tied to a GitHub sub-issue.

## Suggested Follow-up Issues

- Dedicated RIOT timer edge-case suite.
- Mapper-by-mapper compatibility matrix publication.
- Per-scanline trace diff tooling for Atari core bring-up.
