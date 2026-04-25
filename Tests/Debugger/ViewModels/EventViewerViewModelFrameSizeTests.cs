using System.Reflection;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Debugger.ViewModels;

public class EventViewerViewModelFrameSizeTests {
	[Theory]
	[InlineData(0u, 0u, 1u, 1u)]
	[InlineData(0u, 240u, 1u, 240u)]
	[InlineData(320u, 0u, 320u, 1u)]
	[InlineData(320u, 240u, 320u, 240u)]
	public void NormalizeFrameSize_EnsuresMinimumDimensions(uint inputWidth, uint inputHeight, uint expectedWidth, uint expectedHeight) {
		MethodInfo? method = typeof(EventViewerViewModel).GetMethod("NormalizeFrameSize", BindingFlags.NonPublic | BindingFlags.Static);
		Assert.NotNull(method);

		FrameInfo input = new FrameInfo { Width = inputWidth, Height = inputHeight };
		object? result = method!.Invoke(null, [input]);
		Assert.NotNull(result);

		FrameInfo normalized = (FrameInfo)result;
		Assert.Equal(expectedWidth, normalized.Width);
		Assert.Equal(expectedHeight, normalized.Height);
	}
}
