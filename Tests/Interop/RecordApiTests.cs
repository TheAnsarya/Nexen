using System.Runtime.InteropServices;
using System.Text;
using Nexen.Config;
using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Interop;

/// <summary>
/// Tests for recording API structs and enums: <see cref="RecordMovieOptions"/>,
/// <see cref="RecordAviOptions"/>, <see cref="RecordMovieFrom"/>, <see cref="VideoCodec"/>.
/// Validates construction, marshaling sizes, field encoding, and enum values.
/// Tracks: #922 (Screenshot and recording integration tests)
/// </summary>
public class RecordApiTests {
	#region VideoCodec Enum

	[Fact]
	public void VideoCodec_HasExpectedValues() {
		Assert.Equal(0, (int)VideoCodec.None);
		Assert.Equal(1, (int)VideoCodec.ZMBV);
		Assert.Equal(2, (int)VideoCodec.CSCD);
		Assert.Equal(3, (int)VideoCodec.GIF);
	}

	[Fact]
	public void VideoCodec_HasFourMembers() {
		var values = Enum.GetValues<VideoCodec>();
		Assert.Equal(4, values.Length);
	}

	#endregion

	#region RecordMovieFrom Enum

	[Fact]
	public void RecordMovieFrom_HasExpectedValues() {
		Assert.Equal(0, (int)RecordMovieFrom.StartWithoutSaveData);
		Assert.Equal(1, (int)RecordMovieFrom.StartWithSaveData);
		Assert.Equal(2, (int)RecordMovieFrom.CurrentState);
	}

	[Fact]
	public void RecordMovieFrom_HasThreeMembers() {
		var values = Enum.GetValues<RecordMovieFrom>();
		Assert.Equal(3, values.Length);
	}

	#endregion

	#region RecordAviOptions Struct

	[Fact]
	public void RecordAviOptions_DefaultCodecIsNone() {
		RecordAviOptions options = default;
		Assert.Equal(VideoCodec.None, options.Codec);
	}

	[Fact]
	public void RecordAviOptions_DefaultCompressionLevelIsZero() {
		RecordAviOptions options = default;
		Assert.Equal(0u, options.CompressionLevel);
	}

	[Fact]
	public void RecordAviOptions_DefaultHudFlagsAreFalse() {
		RecordAviOptions options = default;
		Assert.False(options.RecordSystemHud);
		Assert.False(options.RecordInputHud);
	}

	[Fact]
	public void RecordAviOptions_CanSetAllFields() {
		var options = new RecordAviOptions {
			Codec = VideoCodec.ZMBV,
			CompressionLevel = 9,
			RecordSystemHud = true,
			RecordInputHud = true
		};

		Assert.Equal(VideoCodec.ZMBV, options.Codec);
		Assert.Equal(9u, options.CompressionLevel);
		Assert.True(options.RecordSystemHud);
		Assert.True(options.RecordInputHud);
	}

	[Fact]
	public void RecordAviOptions_GifCodecValue() {
		var options = new RecordAviOptions { Codec = VideoCodec.GIF };
		Assert.Equal(VideoCodec.GIF, options.Codec);
	}

	#endregion

	#region RecordMovieOptions Struct

	[Fact]
	public void RecordMovieOptions_FilenameIsEncoded() {
		var options = new RecordMovieOptions("test.nmv", "Author", "Desc", RecordMovieFrom.StartWithoutSaveData);
		string decoded = Encoding.UTF8.GetString(options.Filename, 0, Array.IndexOf<byte>(options.Filename, 0));
		Assert.Equal("test.nmv", decoded);
	}

	[Fact]
	public void RecordMovieOptions_AuthorIsEncoded() {
		var options = new RecordMovieOptions("file.nmv", "TestAuthor", "Desc", RecordMovieFrom.StartWithSaveData);
		string decoded = Encoding.UTF8.GetString(options.Author, 0, Array.IndexOf<byte>(options.Author, 0));
		Assert.Equal("TestAuthor", decoded);
	}

	[Fact]
	public void RecordMovieOptions_DescriptionIsEncoded() {
		var options = new RecordMovieOptions("file.nmv", "Author", "A test description", RecordMovieFrom.CurrentState);
		string decoded = Encoding.UTF8.GetString(options.Description, 0, Array.IndexOf<byte>(options.Description, 0));
		Assert.Equal("A test description", decoded);
	}

	[Fact]
	public void RecordMovieOptions_RecordFromIsSet() {
		var options = new RecordMovieOptions("file.nmv", "Author", "Desc", RecordMovieFrom.CurrentState);
		Assert.Equal(RecordMovieFrom.CurrentState, options.RecordFrom);
	}

	[Fact]
	public void RecordMovieOptions_FilenameArraySize() {
		var options = new RecordMovieOptions("f.nmv", "A", "D", RecordMovieFrom.StartWithoutSaveData);
		Assert.Equal(2000, options.Filename.Length);
	}

	[Fact]
	public void RecordMovieOptions_AuthorArraySize() {
		var options = new RecordMovieOptions("f.nmv", "A", "D", RecordMovieFrom.StartWithoutSaveData);
		Assert.Equal(250, options.Author.Length);
	}

	[Fact]
	public void RecordMovieOptions_DescriptionArraySize() {
		var options = new RecordMovieOptions("f.nmv", "A", "D", RecordMovieFrom.StartWithoutSaveData);
		Assert.Equal(10000, options.Description.Length);
	}

	[Fact]
	public void RecordMovieOptions_FilenameArrayIsNullTerminated() {
		var options = new RecordMovieOptions("f.nmv", "A", "D", RecordMovieFrom.StartWithoutSaveData);
		// Last byte must be null-terminated
		Assert.Equal(0, options.Filename[1999]);
	}

	[Fact]
	public void RecordMovieOptions_AuthorArrayIsNullTerminated() {
		var options = new RecordMovieOptions("f.nmv", "A", "D", RecordMovieFrom.StartWithoutSaveData);
		Assert.Equal(0, options.Author[249]);
	}

	[Fact]
	public void RecordMovieOptions_DescriptionArrayIsNullTerminated() {
		var options = new RecordMovieOptions("f.nmv", "A", "D", RecordMovieFrom.StartWithoutSaveData);
		Assert.Equal(0, options.Description[9999]);
	}

	[Fact]
	public void RecordMovieOptions_DescriptionStripsCarriageReturns() {
		var options = new RecordMovieOptions("f.nmv", "A", "Line1\r\nLine2\r\n", RecordMovieFrom.StartWithoutSaveData);
		string decoded = Encoding.UTF8.GetString(options.Description, 0, Array.IndexOf<byte>(options.Description, 0));
		Assert.Equal("Line1\nLine2\n", decoded);
		Assert.DoesNotContain("\r", decoded);
	}

	[Fact]
	public void RecordMovieOptions_LongFilenameIsTruncated() {
		string longName = new string('x', 3000) + ".nmv";
		var options = new RecordMovieOptions(longName, "A", "D", RecordMovieFrom.StartWithoutSaveData);
		Assert.Equal(2000, options.Filename.Length);
		Assert.Equal(0, options.Filename[1999]);
	}

	[Fact]
	public void RecordMovieOptions_LongAuthorIsTruncated() {
		string longAuthor = new string('a', 500);
		var options = new RecordMovieOptions("f.nmv", longAuthor, "D", RecordMovieFrom.StartWithoutSaveData);
		Assert.Equal(250, options.Author.Length);
		Assert.Equal(0, options.Author[249]);
	}

	[Fact]
	public void RecordMovieOptions_LongDescriptionIsTruncated() {
		string longDesc = new string('d', 20000);
		var options = new RecordMovieOptions("f.nmv", "A", longDesc, RecordMovieFrom.StartWithoutSaveData);
		Assert.Equal(10000, options.Description.Length);
		Assert.Equal(0, options.Description[9999]);
	}

	[Fact]
	public void RecordMovieOptions_EmptyStringsProduceNullArrays() {
		var options = new RecordMovieOptions("", "", "", RecordMovieFrom.StartWithoutSaveData);
		Assert.Equal(2000, options.Filename.Length);
		Assert.Equal(250, options.Author.Length);
		Assert.Equal(10000, options.Description.Length);
		// First byte should be null for empty strings
		Assert.Equal(0, options.Filename[0]);
		Assert.Equal(0, options.Author[0]);
		Assert.Equal(0, options.Description[0]);
	}

	[Fact]
	public void RecordMovieOptions_UnicodeAuthorPreserved() {
		var options = new RecordMovieOptions("f.nmv", "日本語テスト", "D", RecordMovieFrom.StartWithoutSaveData);
		string decoded = Encoding.UTF8.GetString(options.Author, 0, Array.IndexOf<byte>(options.Author, 0));
		Assert.Equal("日本語テスト", decoded);
	}

	[Fact]
	public void RecordMovieOptions_AllRecordFromValues() {
		var opt1 = new RecordMovieOptions("f.nmv", "A", "D", RecordMovieFrom.StartWithoutSaveData);
		var opt2 = new RecordMovieOptions("f.nmv", "A", "D", RecordMovieFrom.StartWithSaveData);
		var opt3 = new RecordMovieOptions("f.nmv", "A", "D", RecordMovieFrom.CurrentState);

		Assert.Equal(RecordMovieFrom.StartWithoutSaveData, opt1.RecordFrom);
		Assert.Equal(RecordMovieFrom.StartWithSaveData, opt2.RecordFrom);
		Assert.Equal(RecordMovieFrom.CurrentState, opt3.RecordFrom);
	}

	#endregion

	#region VideoRecordConfig Defaults

	[Fact]
	public void VideoRecordConfig_DefaultCodecIsCscd() {
		var config = new VideoRecordConfig();
		Assert.Equal(VideoCodec.CSCD, config.Codec);
	}

	[Fact]
	public void VideoRecordConfig_DefaultCompressionLevel() {
		var config = new VideoRecordConfig();
		Assert.Equal(6u, config.CompressionLevel);
	}

	[Fact]
	public void VideoRecordConfig_DefaultHudFlagsAreFalse() {
		var config = new VideoRecordConfig();
		Assert.False(config.RecordSystemHud);
		Assert.False(config.RecordInputHud);
	}

	#endregion
}
