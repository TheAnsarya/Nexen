using Nexen.Utilities;
using Xunit;

namespace Nexen.Tests.Utilities;

public sealed class SpeedSliderPrototypeMapperTests {
	[Theory]
	[InlineData(0.0, SpeedSliderMode.Pause, 100)]
	[InlineData(1.0, SpeedSliderMode.EmulationSpeed, 25)]
	[InlineData(2.0, SpeedSliderMode.EmulationSpeed, 50)]
	[InlineData(3.0, SpeedSliderMode.EmulationSpeed, 100)]
	[InlineData(4.0, SpeedSliderMode.EmulationSpeed, 200)]
	[InlineData(5.0, SpeedSliderMode.EmulationSpeed, 300)]
	[InlineData(6.0, SpeedSliderMode.MaximumSpeed, 0)]
	public void FromSliderValue_MapsDiscreteSemantics(double sliderValue, SpeedSliderMode expectedMode, uint expectedSpeed) {
		SpeedSliderSelection selection = SpeedSliderPrototypeMapper.FromSliderValue(sliderValue);
		Assert.Equal(expectedMode, selection.Mode);
		Assert.Equal(expectedSpeed, selection.EmulationSpeed);
	}

	[Theory]
	[InlineData(0, 6)]
	[InlineData(1, 1)]
	[InlineData(25, 1)]
	[InlineData(40, 2)]
	[InlineData(100, 3)]
	[InlineData(180, 4)]
	[InlineData(300, 5)]
	[InlineData(4999, 5)]
	public void ToPosition_MapsToExpectedSliderStops(uint emulationSpeed, int expectedPosition) {
		int position = SpeedSliderPrototypeMapper.ToPosition(emulationSpeed);
		Assert.Equal(expectedPosition, position);
	}

	[Theory]
	[InlineData(2.49, 2)]
	[InlineData(2.5, 3)]
	[InlineData(3.49, 3)]
	[InlineData(3.5, 4)]
	public void FromSliderValue_RoundsToNearestDiscreteStop(double sliderValue, int expectedPosition) {
		SpeedSliderSelection selection = SpeedSliderPrototypeMapper.FromSliderValue(sliderValue);
		Assert.Equal(expectedPosition, selection.Position);
	}
}
