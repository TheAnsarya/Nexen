using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.Windows;

namespace Nexen.Debugger.Labels;
/// <summary>
/// Unified entry point for importing and exporting Nexen label files.
/// </summary>
/// <remarks>
/// <para>
/// Supports two formats:
/// <list type="bullet">
///   <item><description><b>.nexen-labels</b> — New binary format with metadata, CRC32 integrity,
///     and full CodeLabelFlags preservation. This is the native format.</description></item>
///   <item><description><b>.mlb</b> — Legacy interop text format (MemoryType:Address:Label:Comment).
///     Kept for backward compatibility import. Does NOT preserve CodeLabelFlags.</description></item>
/// </list>
/// </para>
/// <para>
/// Export always writes the binary .nexen-labels format. Use <see cref="ExportLegacyMlb"/>
/// for explicit MLB export (interop with other MLB-compatible emulators).
/// </para>
/// </remarks>
public sealed class NexenLabelFile {
	/// <summary>
	/// Imports labels from a label file, auto-detecting format by extension.
	/// </summary>
	/// <param name="path">Path to the label file (.nexen-labels or .mlb).</param>
	/// <param name="showResult">If <c>true</c>, displays a message box with import statistics.</param>
	/// <remarks>
	/// <para>
	/// <b>.nexen-labels</b> files are read via <see cref="NexenLabelFormat"/> (binary).
	/// <b>.mlb</b> files are parsed as legacy text format.
	/// </para>
	/// <para>
	/// Import respects configuration settings for memory type filtering and comment-only labels.
	/// </para>
	/// </remarks>
	public static void Import(string path, bool showResult) {
		string ext = Path.GetExtension(path).ToLowerInvariant();

		if (ext == ".nexen-labels") {
			ImportBinary(path, showResult);
		} else {
			ImportLegacyMlb(path, showResult);
		}
	}

	/// <summary>
	/// Exports all labels to a .nexen-labels binary file.
	/// </summary>
	/// <param name="path">Path to write the label file to.</param>
	/// <param name="romInfo">ROM information for embedded metadata.</param>
	/// <param name="compress">If <c>true</c>, GZip-compress the label data.</param>
	/// <remarks>
	/// Always writes the binary .nexen-labels format. For legacy MLB export,
	/// use <see cref="ExportLegacyMlb"/>.
	/// </remarks>
	public static void Export(string path, RomInfo romInfo, bool compress = false) {
		List<CodeLabel> labels = LabelManager.GetAllLabels();
		NexenLabelFormat.Write(path, labels, romInfo, compress);
	}

	/// <summary>
	/// Imports labels from a legacy MLB text file.
	/// </summary>
	/// <param name="path">Path to the .mlb file.</param>
	/// <param name="showResult">If <c>true</c>, displays a message box with import statistics.</param>
	/// <remarks>
	/// <para>
	/// The MLB format is a simple text format: <c>MemoryType:Address[-EndAddress]:Label[:Comment]</c>
	/// </para>
	/// <para>
	/// <b>Warning:</b> MLB format does NOT preserve <see cref="CodeLabelFlags"/>.
	/// For full-fidelity round-trip, use .nexen-labels format.
	/// </para>
	/// </remarks>
	public static void ImportLegacyMlb(string path, bool showResult) {
		List<CodeLabel> labels = new(1000);

		int errorCount = 0;
		foreach (string row in File.ReadAllLines(path, Encoding.UTF8)) {
			CodeLabel? label = CodeLabel.FromString(row);
			if (label is null) {
				errorCount++;
			} else {
				if (ConfigManager.Config.Debug.Integration.IsMemoryTypeImportEnabled(label.MemoryType)) {
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
	/// Exports all labels to a legacy MLB text file.
	/// </summary>
	/// <param name="path">Path to write the .mlb file to.</param>
	/// <remarks>
	/// <para>
	/// This is for interop with other MLB-compatible emulators. The MLB format does NOT
	/// preserve <see cref="CodeLabelFlags"/>. For normal use, prefer <see cref="Export"/>.
	/// </para>
	/// <para>
	/// Labels are sorted by memory type then address for consistent output.
	/// </para>
	/// </remarks>
	public static void ExportLegacyMlb(string path) {
		List<CodeLabel> labels = LabelManager.GetAllLabels();

		labels.Sort((CodeLabel a, CodeLabel b) => {
			int result = a.MemoryType.CompareTo(b.MemoryType);
			return result != 0 ? result : a.Address.CompareTo(b.Address);
		});

		StringBuilder sb = new StringBuilder();
		foreach (CodeLabel label in labels) {
			sb.Append(label.ToString() + "\n");
		}

		FileHelper.WriteAllText(path, sb.ToString(), Encoding.UTF8);
	}

	/// <summary>
	/// Import from the binary .nexen-labels format.
	/// </summary>
	private static void ImportBinary(string path, bool showResult) {
		try {
			var (labels, header) = NexenLabelFormat.Read(path);

			// Apply memory type and comment filtering
			var filtered = new List<CodeLabel>(labels.Count);
			foreach (var label in labels) {
				if (ConfigManager.Config.Debug.Integration.IsMemoryTypeImportEnabled(label.MemoryType)) {
					if (label.Label.Length > 0 || ConfigManager.Config.Debug.Integration.ImportComments) {
						filtered.Add(label);
					}
				}
			}

			LabelManager.SetLabels(filtered);

			if (showResult) {
				NexenMsgBox.Show(null, "ImportLabels", MessageBoxButtons.OK, MessageBoxIcon.Question, filtered.Count.ToString(), "0");
			}
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"[NexenLabelFile] Binary import failed: {ex.Message}");
			if (showResult) {
				NexenMsgBox.Show(null, "ImportLabelsWithErrors", MessageBoxButtons.OK, MessageBoxIcon.Error, "0", "1");
			}
		}
	}
}
