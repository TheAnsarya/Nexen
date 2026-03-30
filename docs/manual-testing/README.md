# Manual Testing Hub

This section is the starting point for manual release validation in Nexen.

## Start Here

1. Read the ranked checklist in [Manual Step Priority Ranking](manual-step-priority-ranking.md).
2. Execute all release-blocking steps first.
3. Run the platform workflows below.
4. Record pass and fail evidence in the active session log.

## Platform Workflows

| Document | Purpose |
|---|---|
| [Manual Step Priority Ranking](manual-step-priority-ranking.md) | Rank each manual test by release impact and failure severity |
| [Atari 2600 Debug and TAS Manual Test](atari2600-debug-and-tas-manual-test.md) | Atari 2600 end-to-end debugger UI, TAS UI, and movie workflows |
| [Atari Lynx Debug and TAS Manual Test](lynx-debug-and-tas-manual-test.md) | Atari Lynx end-to-end ROM loading, debugger, TAS, save states, and audio |
| [Channel F Debug and TAS Manual Test](channelf-debug-and-tas-manual-test.md) | Fairchild Channel F end-to-end debugger, TAS, movie, and persistence workflows |

## Full Testing Tree

For the full developer-facing manual and automated testing catalog, use:

- [Developer Testing and Manual Validation Index](../../~docs/testing/README.md)

## Release Rule

Do not mark a platform release-ready unless every Critical step and every High step passes.
