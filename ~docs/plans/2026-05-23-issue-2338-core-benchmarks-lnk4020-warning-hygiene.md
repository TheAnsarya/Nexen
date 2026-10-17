# Issue #2338 Core.Benchmarks LNK4020 Warning Hygiene (2026-05-23)

## Scope

- Address repeated linker `LNK4020` type-record warnings observed in `Core.Benchmarks` builds.
- Apply low-risk build-setting mitigation that avoids changing emulator runtime behavior.
- Confirm benchmark project still builds and benchmark executable remains usable.

## Approach

1. Adjust benchmark link configuration to avoid producing debug-info PDB artifacts for this release benchmark target.
2. Rebuild `Core.Benchmarks` and inspect output for `LNK4020` warnings.
3. Keep mitigation narrowly scoped to benchmark project configuration only.

## Acceptance Criteria

- `Core.Benchmarks` Release x64 build succeeds after configuration update.
- Build output no longer reports `LNK4020` warnings for benchmark link step.
- No changes are made to emulator core execution logic.
