using System.Runtime.CompilerServices;
using ReactiveUI.Builder;

namespace Nexen.Tests;

internal static class TestInitializer {
	[ModuleInitializer]
	internal static void Initialize() {
		// Initialize ReactiveUI for test context (no Avalonia platform)
		RxAppBuilder.CreateReactiveUIBuilder()
			.WithCoreServices()
			.BuildApp();
	}
}
