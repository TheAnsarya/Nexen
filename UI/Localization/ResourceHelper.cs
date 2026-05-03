using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Xml;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Localization;
class ResourceHelper {
	private static XmlDocument _resources = new XmlDocument();
	private const string DefaultLanguageCode = "en";

	private static Dictionary<Enum, string> _enumLabelCache = new();
	private static Dictionary<string, string> _viewLabelCache = new();
	private static Dictionary<string, string> _messageCache = new();
	private static HashSet<string> _missingMessageIds = new();
	private static HashSet<string> _missingEnumIds = new();
	private static HashSet<string> _missingViewKeys = new();

	public static void LoadResources(string? preferredLanguageCode = null) {
		try {
			Assembly assembly = typeof(ResourceHelper).Assembly;
			string normalizedCode = NormalizeLanguageCode(preferredLanguageCode);
			string resourceName = $"Nexen.Localization.resources.{normalizedCode}.xml";

			_messageCache.Clear();
			_enumLabelCache.Clear();
			_viewLabelCache.Clear();
			_missingMessageIds.Clear();
			_missingEnumIds.Clear();
			_missingViewKeys.Clear();

			Stream? stream = assembly.GetManifestResourceStream(resourceName);
			if (stream is null && normalizedCode != DefaultLanguageCode) {
				Log.Warn($"ResourceHelper: Missing language resource '{resourceName}', falling back to '{DefaultLanguageCode}'.");
				resourceName = $"Nexen.Localization.resources.{DefaultLanguageCode}.xml";
				stream = assembly.GetManifestResourceStream(resourceName);
			}

			if (stream is null) {
				throw new InvalidOperationException($"ResourceHelper: Could not locate localization resource '{resourceName}'.");
			}

			using (stream)
			using (StreamReader reader = new StreamReader(stream)) {
				_resources.LoadXml(reader.ReadToEnd());
			}

			foreach (XmlNode node in _resources.SelectNodes("/Resources/Messages/Message")!) {
				_messageCache[node.Attributes!["ID"]!.Value] = node.InnerText;
			}

#pragma warning disable IL2026 // Members annotated with 'RequiresUnreferencedCodeAttribute' require dynamic access otherwise can break functionality when trimming application code
			Dictionary<string, Type> enumTypes = typeof(ResourceHelper).Assembly.GetTypes().Where(t => t.IsEnum).ToDictionary(t => t.Name);
#pragma warning restore IL2026 // Members annotated with 'RequiresUnreferencedCodeAttribute' require dynamic access otherwise can break functionality when trimming application code

			foreach (XmlNode node in _resources.SelectNodes("/Resources/Enums/Enum")!) {
				string enumName = node.Attributes!["ID"]!.Value;
				if (enumTypes.TryGetValue(enumName, out Type? enumType)) {
					foreach (XmlNode enumNode in node.ChildNodes) {
						if (Enum.TryParse(enumType, enumNode.Attributes!["ID"]!.Value, out object? value)) {
							_enumLabelCache[(Enum)value!] = enumNode.InnerText;
						}
					}
				} else {
					throw new Exception("Unknown enum type: " + enumName);
				}
			}

			foreach (XmlNode node in _resources.SelectNodes("/Resources/Forms/Form")!) {
				string viewName = node.Attributes!["ID"]!.Value;
				foreach (XmlNode formNode in node.ChildNodes) {
					if (formNode is XmlElement elem) {
						_viewLabelCache[viewName + "_" + elem.Attributes!["ID"]!.Value] = elem.InnerText;
					}
				}
			}
		} catch (Exception ex) {
			Debug.WriteLine($"ResourceHelper: Failed to load localization resources: {ex.Message}");
		}
	}

	public static string[] GetAvailableLanguageCodes() {
		const string prefix = "Nexen.Localization.resources.";
		const string suffix = ".xml";

		return typeof(ResourceHelper).Assembly
			.GetManifestResourceNames()
			.Where(name => name.StartsWith(prefix, StringComparison.Ordinal) && name.EndsWith(suffix, StringComparison.Ordinal))
			.Select(name => name[prefix.Length..^suffix.Length])
			.Distinct(StringComparer.OrdinalIgnoreCase)
			.OrderBy(code => code, StringComparer.OrdinalIgnoreCase)
			.ToArray();
	}

	public static string[] GetAvailableLanguageDisplayNames() {
		return GetAvailableLanguageCodes().Select(GetLanguageDisplayName).ToArray();
	}

	public static string GetLanguageDisplayName(string languageCode) {
		try {
			CultureInfo culture = CultureInfo.GetCultureInfo(languageCode);
			return culture.EnglishName;
		} catch (CultureNotFoundException) {
			return languageCode;
		}
	}

	private static string NormalizeLanguageCode(string? languageCode) {
		if (string.IsNullOrWhiteSpace(languageCode)) {
			return DefaultLanguageCode;
		}

		return languageCode.Trim().ToLowerInvariant();
	}

	public static string GetMessage(string id, params object[] args) {
		if (_messageCache.TryGetValue(id, out string? text)) {
			return string.Format(text, args);
		} else {
			if (_missingMessageIds.Add(id)) {
				Log.Warn($"ResourceHelper: Missing message key '{id}'.");
			}

			return id;
		}
	}

	public static string GetEnumText(Enum e) {
		if (_enumLabelCache.TryGetValue(e, out string? text)) {
			return text;
		} else {
			string fallback = e.ToString();
			if (_missingEnumIds.Add(fallback)) {
				Log.Warn($"ResourceHelper: Missing enum localization for '{fallback}'.");
			}

			return fallback;
		}
	}

	public static Enum[] GetEnumValues(Type t) {
		List<Enum> values = [];
		XmlNode? node = _resources.SelectSingleNode("/Resources/Enums/Enum[@ID='" + t.Name + "']");
		if (node?.Attributes!["ID"]!.Value == t.Name) {
			foreach (XmlNode enumNode in node.ChildNodes) {
				if (Enum.TryParse(t, enumNode.Attributes!["ID"]!.Value, out object? value) && value is not null) {
					values.Add((Enum)value);
				}
			}
		}

		return values.ToArray();
	}

	public static string GetViewLabel(string view, string control) {
		string key = view + "_" + control;
		if (_viewLabelCache.TryGetValue(key, out string? text)) {
			return text;
		} else {
			if (_missingViewKeys.Add(key)) {
				Log.Warn($"ResourceHelper: Missing view label for '{view}.{control}'.");
			}

			return control;
		}
	}
}
