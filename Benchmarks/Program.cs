using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Running;

namespace Nexen.Benchmarks;

public class Program {
	public static void Main(string[] args) {
		var config = DefaultConfig.Instance
			.WithOptions(ConfigOptions.JoinSummary);

		// Run all benchmarks from this assembly
		BenchmarkSwitcher.FromAssembly(typeof(Program).Assembly).Run(args, config);
	}
}
