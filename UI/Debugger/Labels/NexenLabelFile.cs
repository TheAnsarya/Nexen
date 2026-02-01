using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Utilities;
using Nexen.Windows;

namespace Nexen.Debugger.Labels {
	/// <summary>
	/// Handles import and export of Nexen native label files (.nexen-labels / .mlb).
	/// </summary>
	/// <remarks>
	/// <para>
	/// The Nexen label file format is a simple text format with one label per line:
	/// <c>MemoryType:Address[-EndAddress]:Label[:Comment]</c>
	/// </para>
	/// <para>
	/// Example lines:
	/// <code>
	/// NesPrgRom:8000:ResetVector:Entry point after reset
	/// SnesWorkRam:7E0000-7E00FF:SpriteBuffer
	/// </code>
	/// </para>
	/// <para>
	/// This format maintains backward compatibility with Mesen's MLB format while
	/// supporting all Nexen memory types and extended features.
	/// </para>
	/// </remarks>
	public class NexenLabelFile {
		/// <summary>
		/// Imports labels from a Nexen label file (.nexen-labels / .mlb).
		/// </summary>
		/// <param name="path">Path to the label file to import.</param>
		/// <param name="showResult">If <c>true</c>, displays a message box with import statistics.</param>
		/// <remarks>
		/// <para>
		/// The import respects the following configuration settings:
		/// <list type="bullet">
		/// <item><description>Memory type filtering via <c>IsMemoryTypeImportEnabled</c></description></item>
		/// <item><description>Comment-only labels via <c>ImportComments</c></description></item>
		/// </list>
		/// </para>
		/// <para>
		/// Invalid lines (malformed format) are counted and reported but don't stop the import.
		/// </para>
		/// </remarks>
		public static void Import(string path, bool showResult) {
			List<CodeLabel> labels = new(1000);

			int errorCount = 0;
			foreach (string row in File.ReadAllLines(path, Encoding.UTF8)) {
				CodeLabel? label = CodeLabel.FromString(row);
				if (label is null) {
					errorCount++;
				} else {
					// Check if this memory type is enabled for import
					if (ConfigManager.Config.Debug.Integration.IsMemoryTypeImportEnabled(label.MemoryType)) {
						// Include if has label name, or if comment-only labels are allowed
						if (label.Label.Length > 0 || ConfigManager.Config.Debug.Integration.ImportComments) {
							labels.Add(label);
						}
					}
				}
			}

			LabelManager.SetLabels(labels);

			if (showResult) {
				NexenMsgBox.Show(null, errorCount == 0 ? "ImportLabels" : "ImportLabelsWithErrors", MessageBoxButtons.OK, MessageBoxIcon.Question, labels.Count.ToString(), errorCount.ToString());
			}
		}

		/// <summary>
		/// Exports all labels to a Nexen label file (.nexen-labels / .mlb).
		/// </summary>
		/// <param name="path">Path to write the label file to.</param>
		/// <remarks>
		/// Labels are sorted by memory type first, then by address, for consistent output
		/// and easy diff comparison between exports.
		/// </remarks>
		public static void Export(string path) {
			List<CodeLabel> labels = LabelManager.GetAllLabels();

			// Sort by memory type, then by address for consistent output
			labels.Sort((CodeLabel a, CodeLabel b) => {
				int result = a.MemoryType.CompareTo(b.MemoryType);
				if (result == 0) {
					return a.Address.CompareTo(b.Address);
				} else {
					return result;
				}
			});

			StringBuilder sb = new StringBuilder();
			foreach (CodeLabel label in labels) {
				sb.Append(label.ToString() + "\n");
			}

			FileHelper.WriteAllText(path, sb.ToString(), Encoding.UTF8);
		}
	}
}
