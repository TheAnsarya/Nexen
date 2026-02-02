using BenchmarkDotNet.Running;
using Nexen.MovieConverter.Benchmarks;

// Run all benchmarks
BenchmarkSwitcher.FromAssembly(typeof(Program).Assembly).Run(args);
