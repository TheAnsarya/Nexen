using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Running;

namespace Mesen.Benchmarks;

public class Program {
	public static void Main(string[] args) {
		// Run with category filter to avoid baseline conflicts
		var config = DefaultConfig.Instance
			.WithOptions(ConfigOptions.JoinSummary);

		BenchmarkRunner.Run<PansyExportBenchmarks>(config, args);
	}
}
