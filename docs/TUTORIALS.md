# Nexen Tutorials

This page collects step-by-step tutorials for users and contributors.

## User Workflow Tutorials

| Tutorial | Description |
|---|---|
| [Save States Guide](Save-States.md) | Capture, browse, and restore save states using quick and designated slots |
| [Rewind Guide](Rewind.md) | Configure and use rewind for iterative gameplay analysis |
| [Movie System Guide](Movie-System.md) | Record, play back, and exchange movie files |
| [TAS Editor Manual](TAS-Editor-Manual.md) | End-to-end frame editing workflows in the TAS editor |
| [Debugging Guide](Debugging.md) | Disassembly, memory inspection, breakpoints, trace, and CDL workflows |
| [Video and Audio Guide](Video-Audio.md) | Configure rendering and audio output pipelines |

## Contributor Workflow Tutorials

### Tutorial 1: Add or Update User Documentation

1. Create or update content in the relevant guide under this folder.
2. Add screenshot tasks to [Screenshot Capture Checklist](screenshots/CAPTURE-CHECKLIST.md).
3. Update [Documentation Index](README.md) if a new document was added.
4. Validate markdown formatting rules before commit.

### Tutorial 2: Execute Issue-First Work

1. Create a GitHub issue before implementation.
2. Add a prompt-tracking comment immediately after issue creation.
3. Implement changes tied to the issue scope.
4. Commit with issue reference in the message.
5. Close the issue with a completion summary and commit hash.

Reference policy: [Developer Instructions](../.github/copilot-instructions.md).

### Tutorial 3: Plan and Run Future-Work Tracks

1. Review [Future Work Index](FUTURE-WORK.md) for active tracks.
2. Open the plan doc for the track you are executing.
3. Break work into milestones and measurable acceptance criteria.
4. Link new sub-issues and docs back into the relevant README index.
5. Record outcomes in [Session Logs](../~docs/session-logs/).

### Tutorial 4: Run a Platform-Parity Research Session

1. Open [Platform Parity Research Index](../~docs/research/platform-parity/README.md).
2. Pick one subsystem document from Atari 2600 or Genesis research trees.
3. Add source-linked findings and explicit implementation implications.
4. Update issue status for [#704](https://github.com/TheAnsarya/Nexen/issues/704), [#705](https://github.com/TheAnsarya/Nexen/issues/705), [#706](https://github.com/TheAnsarya/Nexen/issues/706), or [#707](https://github.com/TheAnsarya/Nexen/issues/707).
5. Add a new session log entry summarizing completed artifacts and next actions.

## Related References

| Document | Description |
|---|---|
| [Future Work Index](FUTURE-WORK.md) | Roadmap and execution tracks |
| [Developer Docs Index](../~docs/README.md) | Internal engineering documentation |
| [Q2 Future Work Program](../~docs/plans/future-work-program-2026-q2.md) | Milestones and acceptance criteria for current roadmap execution |
| [Q3 Platform Parity Research Program](../~docs/plans/platform-parity-research-program-2026-q3.md) | Multi-session research and execution cadence |
| [Platform Parity Research Index](../~docs/research/platform-parity/README.md) | Source-cited Atari and Genesis subsystem documentation tree |
