using Avalonia.Media;
using Mesen.Config;
using Mesen.Interop;
using Mesen.Localization;
using Mesen.MovieConverter;
using Mesen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reactive.Linq;
using System.Threading.Tasks;

namespace Mesen.ViewModels
{
	public class MovieExportViewModel : ViewModelBase
	{
		[Reactive] public string SourcePath { get; set; } = "";
		[Reactive] public string ExportPath { get; set; } = "";
		[Reactive] public string SelectedFormat { get; set; } = "";
		[Reactive] public string StatusMessage { get; set; } = "";
		[Reactive] public IBrush StatusColor { get; set; } = Brushes.Gray;
		[Reactive] public bool CanExport { get; set; } = false;

		public List<string> AvailableFormats { get; } = new List<string>
		{
			"SNES9x (.smv)",
			"LSNES (.lsmv)",
			"FCEUX (.fm2)",
			"BizHawk (.bk2)"
		};

		public MovieExportViewModel()
		{
			SelectedFormat = AvailableFormats[0];
			
			// Update CanExport when paths change
			this.WhenAnyValue(x => x.SourcePath, x => x.ExportPath)
				.Subscribe(_ => UpdateCanExport());
		}

		public MovieExportViewModel(string sourcePath) : this()
		{
			SourcePath = sourcePath;
			
			// Set default export path based on source
			if(!string.IsNullOrEmpty(sourcePath))
			{
				string baseName = Path.GetFileNameWithoutExtension(sourcePath);
				ExportPath = Path.Combine(ConfigManager.MovieFolder, baseName + GetSelectedExtension());
			}
		}

		private void UpdateCanExport()
		{
			CanExport = !string.IsNullOrEmpty(SourcePath) && 
			            !string.IsNullOrEmpty(ExportPath) &&
			            File.Exists(SourcePath);
			
			if(!string.IsNullOrEmpty(SourcePath) && !File.Exists(SourcePath))
			{
				StatusMessage = ResourceHelper.GetMessage("MovieSourceNotFound");
				StatusColor = Brushes.Red;
			}
			else
			{
				StatusMessage = "";
			}
		}

		public string GetSelectedExtension()
		{
			return SelectedFormat switch
			{
				"SNES9x (.smv)" => ".smv",
				"LSNES (.lsmv)" => ".lsmv",
				"FCEUX (.fm2)" => ".fm2",
				"BizHawk (.bk2)" => ".bk2",
				_ => ".smv"
			};
		}

		public async Task<bool> Export()
		{
			if(!CanExport)
			{
				return false;
			}

			StatusMessage = ResourceHelper.GetMessage("MovieExporting");
			StatusColor = Brushes.Blue;

			try
			{
				// Use the MovieConverter library directly
				return await Task.Run(() => ConvertMovie(SourcePath, ExportPath));
			}
			catch(Exception ex)
			{
				StatusMessage = ex.Message;
				StatusColor = Brushes.Red;
				return false;
			}
		}

		private bool ConvertMovie(string inputPath, string outputPath)
		{
			// Detect formats
			var inputFormat = MovieConverterRegistry.DetectFormat(inputPath);
			var outputFormat = MovieConverterRegistry.DetectFormat(outputPath);

			if(inputFormat == MovieFormat.Unknown)
			{
				StatusMessage = $"Unknown input format: {Path.GetExtension(inputPath)}";
				StatusColor = Brushes.Red;
				return false;
			}

			if(outputFormat == MovieFormat.Unknown)
			{
				StatusMessage = $"Unknown output format: {Path.GetExtension(outputPath)}";
				StatusColor = Brushes.Red;
				return false;
			}

			var reader = MovieConverterRegistry.GetConverter(inputFormat);
			var writer = MovieConverterRegistry.GetConverter(outputFormat);

			if(reader == null || !reader.CanRead)
			{
				StatusMessage = $"Cannot read {inputFormat} format";
				StatusColor = Brushes.Red;
				return false;
			}

			if(writer == null || !writer.CanWrite)
			{
				StatusMessage = $"Cannot write {outputFormat} format";
				StatusColor = Brushes.Red;
				return false;
			}

			// Read input movie
			var movie = reader.Read(inputPath);

			// Write output movie
			writer.Write(movie, outputPath);

			StatusMessage = ResourceHelper.GetMessage("MovieExportSuccess");
			StatusColor = Brushes.Green;
			return true;
		}
	}
}
