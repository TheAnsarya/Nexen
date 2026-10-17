using System;
using System.ComponentModel;

namespace Nexen.Localization;

public sealed class LocalizationBindingSource : INotifyPropertyChanged {
	public static LocalizationBindingSource Instance { get; } = new();

	public event PropertyChangedEventHandler? PropertyChanged;

	private LocalizationBindingSource() {
	}

	public string this[string key] {
		get {
			string[] parts = key.Split('|', 2, StringSplitOptions.None);
			if (parts.Length != 2) {
				return key;
			}

			return ResourceHelper.GetViewLabel(parts[0], parts[1]);
		}
	}

	public void NotifyLanguageChanged() {
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Item[]"));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(string.Empty));
	}
}
