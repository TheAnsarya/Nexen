using System.Runtime.InteropServices;
using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Interop;

/// <summary>
/// Tests for <see cref="SaveStateInfo"/> model class and <see cref="SaveStateOrigin"/> enum.
/// Validates badge colors, labels, timestamp formatting, and interop marshaling.
/// Tracks: #921 (SaveState EmuApi integration tests)
/// </summary>
public class SaveStateInfoTests {
	#region SaveStateOrigin Enum

	[Fact]
	public void SaveStateOrigin_HasExpectedValues() {
		Assert.Equal((byte)0, (byte)SaveStateOrigin.Auto);
		Assert.Equal((byte)1, (byte)SaveStateOrigin.Save);
		Assert.Equal((byte)2, (byte)SaveStateOrigin.Recent);
		Assert.Equal((byte)3, (byte)SaveStateOrigin.Lua);
	}

	[Fact]
	public void SaveStateOrigin_HasFourMembers() {
		var values = Enum.GetValues<SaveStateOrigin>();
		Assert.Equal(4, values.Length);
	}

	#endregion

	#region Default Constructor

	[Fact]
	public void DefaultConstructor_SetsEmptyDefaults() {
		var info = new SaveStateInfo();

		Assert.Equal("", info.Filepath);
		Assert.Equal("", info.RomName);
		Assert.Equal(default(DateTime), info.Timestamp);
		Assert.Equal(0u, info.FileSize);
		Assert.Equal(SaveStateOrigin.Auto, info.Origin);
		Assert.False(info.IsPaused);
	}

	#endregion

	#region InteropSaveStateInfo Constructor

	[Fact]
	public void InteropConstructor_MarshalsSaveOrigin() {
		var interop = CreateInteropInfo(
			filepath: @"C:\saves\game_2026-01-15_14-30-00.nexen-save",
			romName: "TestRom",
			timestamp: 1736956200, // 2025-01-15T14:30:00 UTC
			fileSize: 65536,
			origin: SaveStateOrigin.Save);

		var info = new SaveStateInfo(interop);

		Assert.Equal(SaveStateOrigin.Save, info.Origin);
	}

	[Fact]
	public void InteropConstructor_MarshalsFilepath() {
		var interop = CreateInteropInfo(
			filepath: @"C:\saves\game.nexen-save",
			romName: "TestRom",
			timestamp: 1736956200,
			fileSize: 1024,
			origin: SaveStateOrigin.Auto);

		var info = new SaveStateInfo(interop);

		Assert.Equal(@"C:\saves\game.nexen-save", info.Filepath);
	}

	[Fact]
	public void InteropConstructor_MarshalsRomName() {
		var interop = CreateInteropInfo(
			filepath: @"C:\saves\game.nexen-save",
			romName: "Super Mario Bros",
			timestamp: 1736956200,
			fileSize: 2048,
			origin: SaveStateOrigin.Recent);

		var info = new SaveStateInfo(interop);

		Assert.Equal("Super Mario Bros", info.RomName);
	}

	[Fact]
	public void InteropConstructor_MarshalsFileSize() {
		var interop = CreateInteropInfo(
			filepath: @"C:\saves\game.nexen-save",
			romName: "TestRom",
			timestamp: 1736956200,
			fileSize: 131072,
			origin: SaveStateOrigin.Auto);

		var info = new SaveStateInfo(interop);

		Assert.Equal(131072u, info.FileSize);
	}

	[Fact]
	public void InteropConstructor_ConvertsUnixTimestampToLocalDateTime() {
		// 2025-01-15T00:00:00 UTC
		long unixTimestamp = 1736899200;
		var interop = CreateInteropInfo(
			filepath: @"C:\saves\game.nexen-save",
			romName: "TestRom",
			timestamp: unixTimestamp,
			fileSize: 1024,
			origin: SaveStateOrigin.Auto);

		var info = new SaveStateInfo(interop);

		// The timestamp is converted to local time via DateTimeOffset.FromUnixTimeSeconds
		var expected = DateTimeOffset.FromUnixTimeSeconds(unixTimestamp).LocalDateTime;
		Assert.Equal(expected, info.Timestamp);
	}

	[Theory]
	[InlineData(SaveStateOrigin.Auto)]
	[InlineData(SaveStateOrigin.Save)]
	[InlineData(SaveStateOrigin.Recent)]
	[InlineData(SaveStateOrigin.Lua)]
	public void InteropConstructor_PreservesAllOriginValues(SaveStateOrigin origin) {
		var interop = CreateInteropInfo(
			filepath: "test.nexen-save",
			romName: "Rom",
			timestamp: 1000000,
			fileSize: 512,
			origin: origin);

		var info = new SaveStateInfo(interop);

		Assert.Equal(origin, info.Origin);
	}

	#endregion

	#region GetBadgeColor

	[Theory]
	[InlineData(SaveStateOrigin.Auto, "4a90d9")]
	[InlineData(SaveStateOrigin.Save, "5cb85c")]
	[InlineData(SaveStateOrigin.Recent, "d9534f")]
	[InlineData(SaveStateOrigin.Lua, "f0ad4e")]
	public void GetBadgeColor_ReturnsCorrectHexColor(SaveStateOrigin origin, string expectedColor) {
		var info = new SaveStateInfo { Origin = origin };
		Assert.Equal(expectedColor, info.GetBadgeColor());
	}

	[Fact]
	public void GetBadgeColor_InvalidOrigin_ReturnsGrayFallback() {
		var info = new SaveStateInfo { Origin = (SaveStateOrigin)255 };
		Assert.Equal("777777", info.GetBadgeColor());
	}

	[Fact]
	public void GetBadgeColor_ColorsAreLowercase() {
		foreach (var origin in Enum.GetValues<SaveStateOrigin>()) {
			var info = new SaveStateInfo { Origin = origin };
			string color = info.GetBadgeColor();
			Assert.Equal(color.ToLowerInvariant(), color);
		}
	}

	#endregion

	#region GetBadgeLabel

	[Theory]
	[InlineData(SaveStateOrigin.Auto, "Auto")]
	[InlineData(SaveStateOrigin.Save, "Save")]
	[InlineData(SaveStateOrigin.Recent, "Recent")]
	[InlineData(SaveStateOrigin.Lua, "Lua")]
	public void GetBadgeLabel_ReturnsCorrectLabel(SaveStateOrigin origin, string expectedLabel) {
		var info = new SaveStateInfo { Origin = origin };
		Assert.Equal(expectedLabel, info.GetBadgeLabel());
	}

	[Fact]
	public void GetBadgeLabel_InvalidOrigin_ReturnsUnknown() {
		var info = new SaveStateInfo { Origin = (SaveStateOrigin)99 };
		Assert.Equal("Unknown", info.GetBadgeLabel());
	}

	#endregion

	#region GetFriendlyTimestamp

	[Fact]
	public void GetFriendlyTimestamp_Today_PrefixedWithToday() {
		var now = DateTime.Now;
		var info = new SaveStateInfo { Timestamp = now };

		string result = info.GetFriendlyTimestamp();

		Assert.StartsWith("Today ", result);
		Assert.Contains(now.ToString("M/d/yyyy"), result);
	}

	[Fact]
	public void GetFriendlyTimestamp_Yesterday_PrefixedWithYesterday() {
		var yesterday = DateTime.Now.AddDays(-1);
		var info = new SaveStateInfo { Timestamp = yesterday };

		string result = info.GetFriendlyTimestamp();

		Assert.StartsWith("Yesterday ", result);
		Assert.Contains(yesterday.ToString("M/d/yyyy"), result);
	}

	[Fact]
	public void GetFriendlyTimestamp_OlderDate_JustDateAndTime() {
		var oldDate = DateTime.Now.AddDays(-5);
		var info = new SaveStateInfo { Timestamp = oldDate };

		string result = info.GetFriendlyTimestamp();

		Assert.DoesNotContain("Today", result);
		Assert.DoesNotContain("Yesterday", result);
		Assert.Contains(oldDate.ToString("M/d/yyyy"), result);
	}

	[Fact]
	public void GetFriendlyTimestamp_IncludesTimeComponent() {
		var timestamp = new DateTime(2026, 3, 15, 14, 30, 0);
		var info = new SaveStateInfo { Timestamp = timestamp };

		string result = info.GetFriendlyTimestamp();

		// Should contain the time in h:mm tt format
		Assert.Contains("2:30 PM", result);
	}

	[Fact]
	public void GetFriendlyTimestamp_MorningTime_FormatsCorrectly() {
		var timestamp = new DateTime(2026, 3, 15, 9, 5, 0);
		var info = new SaveStateInfo { Timestamp = timestamp };

		string result = info.GetFriendlyTimestamp();

		Assert.Contains("9:05 AM", result);
	}

	#endregion

	#region InteropSaveStateInfo Layout

	[Fact]
	public void InteropSaveStateInfo_FilepathField_Has2000ByteCapacity() {
		var interop = new InteropSaveStateInfo();
		interop.Filepath = new byte[2000];
		Assert.Equal(2000, interop.Filepath.Length);
	}

	[Fact]
	public void InteropSaveStateInfo_RomNameField_Has256ByteCapacity() {
		var interop = new InteropSaveStateInfo();
		interop.RomName = new byte[256];
		Assert.Equal(256, interop.RomName.Length);
	}

	#endregion

	#region Property Setters

	[Fact]
	public void Properties_AreSettable() {
		var info = new SaveStateInfo {
			Filepath = @"C:\test\save.nexen-save",
			RomName = "Test ROM",
			Timestamp = new DateTime(2026, 6, 15),
			FileSize = 42000,
			Origin = SaveStateOrigin.Lua,
			IsPaused = true
		};

		Assert.Equal(@"C:\test\save.nexen-save", info.Filepath);
		Assert.Equal("Test ROM", info.RomName);
		Assert.Equal(new DateTime(2026, 6, 15), info.Timestamp);
		Assert.Equal(42000u, info.FileSize);
		Assert.Equal(SaveStateOrigin.Lua, info.Origin);
		Assert.True(info.IsPaused);
	}

	#endregion

	#region IsPaused Marshaling

	[Fact]
	public void InteropConstructor_IsPausedTrue_WhenByteNonZero() {
		var interop = CreateInteropInfo(
			filepath: @"C:\saves\game.nexen-save",
			romName: "TestRom",
			timestamp: 1736956200,
			fileSize: 65536,
			origin: SaveStateOrigin.Save);
		interop.IsPaused = 1;

		var info = new SaveStateInfo(interop);

		Assert.True(info.IsPaused);
	}

	[Fact]
	public void InteropConstructor_IsPausedFalse_WhenByteZero() {
		var interop = CreateInteropInfo(
			filepath: @"C:\saves\game.nexen-save",
			romName: "TestRom",
			timestamp: 1736956200,
			fileSize: 65536,
			origin: SaveStateOrigin.Save);
		interop.IsPaused = 0;

		var info = new SaveStateInfo(interop);

		Assert.False(info.IsPaused);
	}

	[Fact]
	public void InteropConstructor_IsPausedTrue_WhenByteIs255() {
		var interop = CreateInteropInfo(
			filepath: @"C:\saves\game.nexen-save",
			romName: "TestRom",
			timestamp: 1736956200,
			fileSize: 65536,
			origin: SaveStateOrigin.Save);
		interop.IsPaused = 0xff;

		var info = new SaveStateInfo(interop);

		Assert.True(info.IsPaused);
	}

	[Fact]
	public void InteropSaveStateInfo_StructLayout_IncludesIsPaused() {
		// Verify the struct has the expected size with IsPaused field
		int size = Marshal.SizeOf<InteropSaveStateInfo>();
		// 2000 + 256 + 8 + 4 + 1 (origin) + 1 (isPaused) + 2 padding = 2272
		Assert.Equal(2272, size);
	}

	#endregion

	#region Helpers

	private static InteropSaveStateInfo CreateInteropInfo(
		string filepath, string romName, long timestamp, uint fileSize, SaveStateOrigin origin) {
		var interop = new InteropSaveStateInfo {
			Filepath = new byte[2000],
			RomName = new byte[256],
			Timestamp = timestamp,
			FileSize = fileSize,
			Origin = origin
		};

		var pathBytes = System.Text.Encoding.UTF8.GetBytes(filepath);
		Array.Copy(pathBytes, interop.Filepath, Math.Min(pathBytes.Length, 1999));

		var nameBytes = System.Text.Encoding.UTF8.GetBytes(romName);
		Array.Copy(nameBytes, interop.RomName, Math.Min(nameBytes.Length, 255));

		return interop;
	}

	#endregion
}
