# Performance Guide

## Purpose

This page centralizes performance strategy and optimization references so user-facing feature pages can stay focused on workflow and usage.

## Performance Principles

- Accuracy first: optimization must preserve emulator correctness.
- Measure before and after: use benchmarks and repeatable test runs.
- Keep hot paths lean: CPU, memory handlers, rendering loops, and audio pipelines.
- Use modern C++/C# features where they are correctness-safe and measurable.

## Where Performance Work Lives

- Ongoing optimization notes: [CHANGELOG](../CHANGELOG.md)
- Profiling workflow: [~docs/PROFILING-GUIDE.md](../~docs/PROFILING-GUIDE.md)
- Build/test verification: [~docs/BUILD-AND-RUN.md](../~docs/BUILD-AND-RUN.md)
- Debugger-specific performance: [DEBUGGER-PERFORMANCE.md](DEBUGGER-PERFORMANCE.md)

## Benchmark and Validation Workflow

1. Build release configuration.
2. Run targeted and full test suites.
3. Run benchmark suites with repetitions.
4. Compare baseline and candidate results.
5. Keep only semantics-preserving wins.

## Genesis Benchmark Commands

Run a focused Genesis benchmark command (verified):

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_Genesis_StepFrameScaffold_OneScanline" --benchmark_min_time=0.01s --benchmark_repetitions=1
```

Run Genesis-only benchmark families (broader filter):

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_Genesis" --benchmark_min_time=0.01s --benchmark_repetitions=1
```

Run Genesis parity test suites before/after performance work:

```powershell
dotnet test Tests\Nexen.Tests.csproj -c Release --filter "FullyQualifiedName~TasEditorViewModelBranchAndLayoutTests|FullyQualifiedName~GenesisTasLayoutTests|FullyQualifiedName~TasInputOverrideMappingParityTests|FullyQualifiedName~InputApiDecoderTests"
```

## Related Links

- [Documentation Index](README.md)
- [Systems Documentation](systems/README.md)
- [Architecture Overview](../~docs/ARCHITECTURE-OVERVIEW.md)
