# Epic 25 Architecture Simplification and Redundancy Reduction

## Goal

Reduce architectural redundancy and switch-heavy dispatch in core debugger/utility paths while preserving emulation correctness and improving maintainability.

## Scope Themes

1. Debugger dispatch unification
	- Replace large per-CPU switch dispatch with polymorphism where behavior is CPU-specific but interface-stable.
2. ExpressionEvaluator consolidation
	- Consolidate repeated token/value mapping patterns using traits/strategy registration.
3. NotificationManager lifecycle simplification
	- Reduce listener cleanup overhead and improve lifecycle semantics.
4. Debugger state centralization
	- Reduce duplicated per-debugger state patterns where safe.

## Modernization Standards

- Use C++23 language features where zero-cost and readability-positive.
- Prefer polymorphic interfaces over central switch fanout for extensibility.
- Preserve emulator accuracy and observable behavior.
- Keep changes incremental, test-backed, and benchmarked when hot paths are affected.

## Success Metrics

- Fewer central switch-case dispatch blocks in `Core/Debugger/Debugger.cpp`.
- Reduced duplicate method implementations for shared debugger workflows.
- No regressions in existing test suites.
- Clean Release x64 build and no new warnings in touched code.

## Planned Execution

- [25.1] Architecture inventory and phased simplification roadmap.
- [25.2] SaveRom polymorphic dispatch unification (implemented in this session).
- [25.3] NotificationManager listener lifecycle/container simplification.

## Risks and Mitigations

- Risk: behavior drift from dispatch rewrites.
	- Mitigation: keep API signatures stable and run focused + release validation each step.
- Risk: hot-path overhead.
	- Mitigation: avoid extra allocations and keep virtual calls off tight CPU instruction loops.
