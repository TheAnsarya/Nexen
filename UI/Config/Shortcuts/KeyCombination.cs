using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using Nexen.Interop;

namespace Nexen.Config.Shortcuts;
public sealed class KeyCombination {
	public UInt16 Key1 { get; set; }
	public UInt16 Key2 { get; set; }
	public UInt16 Key3 { get; set; }

	public bool IsEmpty { get { return Key1 == 0 && Key2 == 0 && Key3 == 0; } }

	public override string ToString() {
		if (IsEmpty) {
			return "";
		} else {
			return GetKeyNames();
		}
	}

	public KeyCombination() {
	}

	public KeyCombination(List<UInt16>? keyCodes = null) {
		if (keyCodes is not null) {
			Key1 = keyCodes.Count > 0 ? keyCodes[0] : (UInt16)0;
			Key2 = keyCodes.Count > 1 ? keyCodes[1] : (UInt16)0;
			Key3 = keyCodes.Count > 2 ? keyCodes[2] : (UInt16)0;
		} else {
			Key1 = 0;
			Key2 = 0;
			Key3 = 0;
		}
	}

	private string GetKeyNames() {
		UInt16[] scanCodes = [Key1, Key2, Key3];
		List<string> keyNames = new List<string>(3);
		foreach (var scanCode in scanCodes) {
			string keyName = InputApi.GetKeyName(scanCode);
			if (!string.IsNullOrWhiteSpace(keyName)) {
				keyNames.Add(keyName);
			}
		}

		if (keyNames.Count > 1) {
			//Merge left/right ctrl/alt/shift for key combinations
			for (int i = 0; i < keyNames.Count; i++) {
				keyNames[i] = keyNames[i] switch {
					"Left Ctrl" or "Right Ctrl" => "Ctrl",
					"Left Alt" or "Right Alt" => "Alt",
					"Left Shift" or "Right Shift" => "Shift",
					_ => keyNames[i]
				};
			}
		}

		keyNames.Sort((string a, string b) => {
			if (a == b) {
				return 0;
			}

			if (a.Contains("Ctrl")) {
				return -1;
			} else if (b.Contains("Ctrl")) {
				return 1;
			}

			if (a.Contains("Alt")) {
				return -1;
			} else if (b.Contains("Alt")) {
				return 1;
			}

			if (a.Contains("Shift")) {
				return -1;
			} else if (b.Contains("Shift")) {
				return 1;
			}

			return a.CompareTo(b);
		});

		return string.Join("+", keyNames);
	}

	public InteropKeyCombination ToInterop() {
		return new InteropKeyCombination() {
			Key1 = Key1,
			Key2 = Key2,
			Key3 = Key3
		};
	}
}
