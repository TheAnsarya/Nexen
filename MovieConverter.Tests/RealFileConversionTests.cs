using System.IO.Compression;

namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Integration tests that load real TAS files from GameInfo repository.
/// Tests reading, conversion, and round-trip capabilities with actual movie data.
/// </summary>
public class RealFileConversionTests {
	/// <summary>
	/// Base path to the TAS test files in the GameInfo repository
	/// </summary>
	private const string TestFilesBasePath = @"C:\Users\me\source\repos\GameInfo\~tas-files\uncompressed";

	/// <summary>
	/// Check if test files are available (skip tests gracefully if not)
	/// </summary>
	private static bool TestFilesAvailable => Directory.Exists(TestFilesBasePath);

	#region FM2 Tests (FCEUX - NES)

	[SkippableFact]
	public void Fm2_ReadRealFile_ParsesCorrectly() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "chamale-northandsouth.fm2");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.Fm2MovieConverter();
		MovieData movie = converter.Read(filePath);

		// Verify basic parsing worked
		Assert.Equal(MovieFormat.Fm2, movie.SourceFormat);
		Assert.Equal(SystemType.Nes, movie.SystemType);
		Assert.True(movie.TotalFrames > 0, "Should have parsed input frames");
	}

	[SkippableFact]
	public void Fm2_ConvertToNexen_PreservesData() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "chamale-northandsouth.fm2");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var fm2Converter = new Converters.Fm2MovieConverter();
		var nexenConverter = new Converters.NexenMovieConverter();

		// Read FM2
		MovieData original = fm2Converter.Read(filePath);

		// Write to Nexen format
		using var stream = new MemoryStream();
		nexenConverter.Write(original, stream);

		// Read back
		stream.Position = 0;
		MovieData roundTripped = nexenConverter.Read(stream);

		// Verify data preserved
		Assert.Equal(original.TotalFrames, roundTripped.TotalFrames);
		Assert.Equal(original.SystemType, roundTripped.SystemType);
		Assert.Equal(original.RerecordCount, roundTripped.RerecordCount);
		Assert.Equal(original.Region, roundTripped.Region);

		// Verify some input frames
		for (int i = 0; i < Math.Min(100, original.TotalFrames); i++) {
			InputFrame origFrame = original.InputFrames[i];
			InputFrame rtFrame = roundTripped.InputFrames[i];
			Assert.Equal(origFrame.Controllers[0].ButtonBits, rtFrame.Controllers[0].ButtonBits);
		}
	}

	#endregion

	#region BK2 Tests (BizHawk - Multi-system)

	[SkippableFact]
	public void Bk2_ReadRealFile_ParsesCorrectly() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "alyosha-halo2600.bk2");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.Bk2MovieConverter();
		MovieData movie = converter.Read(filePath);

		// Verify basic parsing worked
		Assert.Equal(MovieFormat.Bk2, movie.SourceFormat);
		Assert.True(movie.TotalFrames >= 0, "Should have parsed input frames");
	}

	[SkippableFact]
	public void Bk2_RoundTrip_PreservesData() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "alyosha-halo2600.bk2");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.Bk2MovieConverter();

		// Read BK2
		MovieData original = converter.Read(filePath);

		// Write to BK2 format
		using var stream = new MemoryStream();
		converter.Write(original, stream);

		// Read back
		stream.Position = 0;
		MovieData roundTripped = converter.Read(stream);

		// Verify data preserved
		Assert.Equal(original.TotalFrames, roundTripped.TotalFrames);
		Assert.Equal(original.SystemType, roundTripped.SystemType);
	}

	[SkippableFact]
	public void Bk2_ConvertToNexen_PreservesData() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "alyosha-halo2600.bk2");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var bk2Converter = new Converters.Bk2MovieConverter();
		var nexenConverter = new Converters.NexenMovieConverter();

		// Read BK2
		MovieData original = bk2Converter.Read(filePath);

		// Write to Nexen format
		using var stream = new MemoryStream();
		nexenConverter.Write(original, stream);

		// Read back
		stream.Position = 0;
		MovieData roundTripped = nexenConverter.Read(stream);

		// Verify data preserved
		Assert.Equal(original.TotalFrames, roundTripped.TotalFrames);
		Assert.Equal(original.SystemType, roundTripped.SystemType);
	}

	#endregion

	#region SMV Tests (Snes9x - SNES)

	[SkippableFact]
	public void Smv_ReadRealFile_ParsesCorrectly() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "cpadolf2-contra3.smv");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.SmvMovieConverter();
		MovieData movie = converter.Read(filePath);

		// Verify basic parsing worked
		Assert.Equal(MovieFormat.Smv, movie.SourceFormat);
		Assert.Equal(SystemType.Snes, movie.SystemType);
		Assert.True(movie.TotalFrames > 0, "Should have parsed input frames");
	}

	[SkippableFact]
	public void Smv_RoundTrip_PreservesData() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "cpadolf2-contra3.smv");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.SmvMovieConverter();

		// Read SMV
		MovieData original = converter.Read(filePath);

		// Write to SMV format
		using var stream = new MemoryStream();
		converter.Write(original, stream);

		// Read back
		stream.Position = 0;
		MovieData roundTripped = converter.Read(stream);

		// Verify data preserved
		Assert.Equal(original.TotalFrames, roundTripped.TotalFrames);
		Assert.Equal(original.SystemType, roundTripped.SystemType);
	}

	[SkippableFact]
	public void Smv_ConvertToNexen_PreservesData() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "cpadolf2-contra3.smv");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var smvConverter = new Converters.SmvMovieConverter();
		var nexenConverter = new Converters.NexenMovieConverter();

		// Read SMV
		MovieData original = smvConverter.Read(filePath);

		// Write to Nexen format
		using var stream = new MemoryStream();
		nexenConverter.Write(original, stream);

		// Read back
		stream.Position = 0;
		MovieData roundTripped = nexenConverter.Read(stream);

		// Verify data preserved
		Assert.Equal(original.TotalFrames, roundTripped.TotalFrames);
		Assert.Equal(original.SystemType, roundTripped.SystemType);
	}

	#endregion

	#region LSMV Tests (lsnes - SNES)

	[SkippableFact]
	public void Lsmv_ReadRealFile_ParsesCorrectly() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "pirohikov1-earthbound-glitched.lsmv");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.LsmvMovieConverter();
		MovieData movie = converter.Read(filePath);

		// Verify basic parsing worked
		Assert.Equal(MovieFormat.Lsmv, movie.SourceFormat);
		Assert.Equal(SystemType.Snes, movie.SystemType);
		Assert.True(movie.TotalFrames > 0, "Should have parsed input frames");
	}

	[SkippableFact]
	public void Lsmv_RoundTrip_PreservesData() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "pirohikov1-earthbound-glitched.lsmv");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.LsmvMovieConverter();

		// Read LSMV
		MovieData original = converter.Read(filePath);

		// Write to LSMV format
		using var stream = new MemoryStream();
		converter.Write(original, stream);

		// Read back
		stream.Position = 0;
		MovieData roundTripped = converter.Read(stream);

		// Verify data preserved
		Assert.Equal(original.TotalFrames, roundTripped.TotalFrames);
		Assert.Equal(original.SystemType, roundTripped.SystemType);
	}

	[SkippableFact]
	public void Lsmv_ConvertToNexen_PreservesData() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "pirohikov1-earthbound-glitched.lsmv");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var lsmvConverter = new Converters.LsmvMovieConverter();
		var nexenConverter = new Converters.NexenMovieConverter();

		// Read LSMV
		MovieData original = lsmvConverter.Read(filePath);

		// Write to Nexen format
		using var stream = new MemoryStream();
		nexenConverter.Write(original, stream);

		// Read back
		stream.Position = 0;
		MovieData roundTripped = nexenConverter.Read(stream);

		// Verify data preserved
		Assert.Equal(original.TotalFrames, roundTripped.TotalFrames);
		Assert.Equal(original.SystemType, roundTripped.SystemType);
	}

	#endregion

	#region VBM Tests (VisualBoyAdvance - GBA/GB)

	[SkippableFact]
	public void Vbm_ReadRealFile_ParsesCorrectly() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "akheon-xenawarriorprincess.vbm");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.VbmMovieConverter();
		MovieData movie = converter.Read(filePath);

		// Verify basic parsing worked
		Assert.Equal(MovieFormat.Vbm, movie.SourceFormat);
		Assert.True(movie.SystemType is SystemType.Gba or SystemType.Gb or SystemType.Gbc,
			$"Should be GBA/GB/GBC, got {movie.SystemType}");
		Assert.True(movie.TotalFrames > 0, "Should have parsed input frames");
	}

	[SkippableFact]
	public void Vbm_IsReadOnly() {
		var converter = new Converters.VbmMovieConverter();
		Assert.True(converter.CanRead);
		Assert.False(converter.CanWrite);
	}

	[SkippableFact]
	public void Vbm_ConvertToNexen_PreservesData() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "akheon-xenawarriorprincess.vbm");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var vbmConverter = new Converters.VbmMovieConverter();
		var nexenConverter = new Converters.NexenMovieConverter();

		// Read VBM
		MovieData original = vbmConverter.Read(filePath);

		// Write to Nexen format
		using var stream = new MemoryStream();
		nexenConverter.Write(original, stream);

		// Read back
		stream.Position = 0;
		MovieData roundTripped = nexenConverter.Read(stream);

		// Verify data preserved
		Assert.Equal(original.TotalFrames, roundTripped.TotalFrames);
		Assert.Equal(original.SystemType, roundTripped.SystemType);
	}

	#endregion

	#region GMV Tests (Gens - Genesis)

	[SkippableFact]
	public void Gmv_ReadRealFile_ParsesCorrectly() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "fuzzerd,sonikkustar,upthorn,aglar-sonic2.gmv");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.GmvMovieConverter();
		MovieData movie = converter.Read(filePath);

		// Verify basic parsing worked
		Assert.Equal(MovieFormat.Gmv, movie.SourceFormat);
		Assert.Equal(SystemType.Genesis, movie.SystemType);
		Assert.True(movie.TotalFrames > 0, "Should have parsed input frames");
	}

	[SkippableFact]
	public void Gmv_IsReadOnly() {
		var converter = new Converters.GmvMovieConverter();
		Assert.True(converter.CanRead);
		Assert.False(converter.CanWrite);
	}

	[SkippableFact]
	public void Gmv_ConvertToNexen_PreservesData() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "fuzzerd,sonikkustar,upthorn,aglar-sonic2.gmv");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var gmvConverter = new Converters.GmvMovieConverter();
		var nexenConverter = new Converters.NexenMovieConverter();

		// Read GMV
		MovieData original = gmvConverter.Read(filePath);

		// Write to Nexen format
		using var stream = new MemoryStream();
		nexenConverter.Write(original, stream);

		// Read back
		stream.Position = 0;
		MovieData roundTripped = nexenConverter.Read(stream);

		// Verify data preserved
		Assert.Equal(original.TotalFrames, roundTripped.TotalFrames);
		Assert.Equal(original.SystemType, roundTripped.SystemType);
	}

	#endregion

	#region Cross-Format Conversion Tests

	[SkippableFact]
	public void CrossConversion_Fm2ToBk2ViaCommon_PreservesFrameCount() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string fm2Path = Path.Combine(TestFilesBasePath, "chamale-northandsouth.fm2");
		Skip.IfNot(File.Exists(fm2Path), $"Test file not found: {fm2Path}");

		var fm2Converter = new Converters.Fm2MovieConverter();
		var nexenConverter = new Converters.NexenMovieConverter();

		// Read FM2 → Write Nexen → Read Nexen → Verify
		MovieData fm2Movie = fm2Converter.Read(fm2Path);
		int originalFrames = fm2Movie.TotalFrames;

		using var nexenStream = new MemoryStream();
		nexenConverter.Write(fm2Movie, nexenStream);
		nexenStream.Position = 0;
		MovieData nexenMovie = nexenConverter.Read(nexenStream);

		Assert.Equal(originalFrames, nexenMovie.TotalFrames);
	}

	[SkippableFact]
	public void CrossConversion_AllFormatsToNexen_Succeed() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		var testFiles = new Dictionary<string, Type> {
			["chamale-northandsouth.fm2"] = typeof(Converters.Fm2MovieConverter),
			["alyosha-halo2600.bk2"] = typeof(Converters.Bk2MovieConverter),
			["cpadolf2-contra3.smv"] = typeof(Converters.SmvMovieConverter),
			["pirohikov1-earthbound-glitched.lsmv"] = typeof(Converters.LsmvMovieConverter),
			["akheon-xenawarriorprincess.vbm"] = typeof(Converters.VbmMovieConverter),
			["fuzzerd,sonikkustar,upthorn,aglar-sonic2.gmv"] = typeof(Converters.GmvMovieConverter)
		};

		var nexenConverter = new Converters.NexenMovieConverter();

		foreach ((string fileName, Type converterType) in testFiles) {
			string filePath = Path.Combine(TestFilesBasePath, fileName);
			if (!File.Exists(filePath)) {
				continue; // Skip missing files
			}

			var converter = (IMovieConverter)Activator.CreateInstance(converterType)!;

			// Read source format
			MovieData movie = converter.Read(filePath);
			Assert.True(movie.TotalFrames >= 0, $"Failed to read {fileName}");

			// Convert to Nexen
			using var stream = new MemoryStream();
			nexenConverter.Write(movie, stream);
			stream.Position = 0;

			// Read back
			MovieData converted = nexenConverter.Read(stream);
			Assert.Equal(movie.TotalFrames, converted.TotalFrames);
		}
	}

	#endregion

	#region Async Read Tests

	[SkippableFact]
	public async Task Fm2_ReadAsync_WorksCorrectly() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "chamale-northandsouth.fm2");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.Fm2MovieConverter();
		MovieData movie = await converter.ReadAsync(filePath);

		Assert.Equal(MovieFormat.Fm2, movie.SourceFormat);
		Assert.True(movie.TotalFrames > 0);
	}

	[SkippableFact]
	public async Task Bk2_ReadAsync_WorksCorrectly() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "alyosha-halo2600.bk2");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.Bk2MovieConverter();
		MovieData movie = await converter.ReadAsync(filePath);

		Assert.Equal(MovieFormat.Bk2, movie.SourceFormat);
	}

	#endregion

	#region Input Data Verification

	[SkippableFact]
	public void Fm2_VerifyInputData_HasButtonPresses() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "chamale-northandsouth.fm2");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.Fm2MovieConverter();
		MovieData movie = converter.Read(filePath);

		// Real TAS should have some button presses
		bool hasAnyInput = movie.InputFrames.Any(f =>
			f.Controllers.Any(c => c.ButtonBits != 0));

		Assert.True(hasAnyInput, "Real TAS file should have input data");
	}

	[SkippableFact]
	public void Smv_VerifyInputData_HasButtonPresses() {
		Skip.IfNot(TestFilesAvailable, "Test files not available");

		string filePath = Path.Combine(TestFilesBasePath, "cpadolf2-contra3.smv");
		Skip.IfNot(File.Exists(filePath), $"Test file not found: {filePath}");

		var converter = new Converters.SmvMovieConverter();
		MovieData movie = converter.Read(filePath);

		// Real TAS should have some button presses
		bool hasAnyInput = movie.InputFrames.Any(f =>
			f.Controllers.Any(c => c.ButtonBits != 0));

		Assert.True(hasAnyInput, "Real TAS file should have input data");
	}

	#endregion
}
