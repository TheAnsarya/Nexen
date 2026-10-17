using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.Windows;

namespace Nexen.Debugger.Integration;

public sealed class BassLabelFile {
	public static void Import(string path, bool showResult, CpuType cpuType) {
		List<CodeLabel> labels = new List<CodeLabel>(1000);

		MemoryType memType = cpuType.ToMemoryType();
		int errorCount = 0;
		foreach (string row in File.ReadAllLines(path, Encoding.UTF8)) {
			string lineData = row.Trim();
			int splitIndex = lineData.IndexOf(' ');
			if (splitIndex < 0) {
				errorCount++;
				continue;
			}

			UInt32 address;

			if (!UInt32.TryParse(lineData.Substring(0, splitIndex), NumberStyles.HexNumber, null, out address)) {
				errorCount++;
				continue;
			}

			AddressInfo absAddress = DebugApi.GetAbsoluteAddress(new AddressInfo() { Address = (int)address, Type = memType });

			if (absAddress.Address >= 0) {
				CodeLabel label = new CodeLabel();
				label.Address = (UInt32)absAddress.Address;
				label.MemoryType = absAddress.Type;
				label.Comment = "";

				string labelName = LabelManager.InvalidLabelRegex.Replace(lineData.Substring(splitIndex + 1), "_");
				if (string.IsNullOrEmpty(labelName) || !LabelManager.LabelRegex.IsMatch(labelName)) {
					errorCount++;
				} else {
					label.Label = labelName;
					if (ConfigManager.Config.Debug.Integration.IsMemoryTypeImportEnabled(label.MemoryType)) {
						labels.Add(label);
					}
				}
			}
		}

		LabelManager.SetLabels(labels);

		if (showResult) {
			if (errorCount > 0) {
				NexenMsgBox.Show(null, "ImportLabelsWithErrors", MessageBoxButtons.OK, MessageBoxIcon.Warning, labels.Count.ToString(), errorCount.ToString());
			} else {
				NexenMsgBox.Show(null, "ImportLabels", MessageBoxButtons.OK, MessageBoxIcon.Info, labels.Count.ToString());
			}
		}
	}
}
