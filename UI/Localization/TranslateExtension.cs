using System;
using Avalonia.Data;
using Avalonia.Markup.Xaml;

namespace Nexen.Localization;
public sealed class TranslateExtension : MarkupExtension {
	public TranslateExtension(string key) {
		this.Key = key;
	}

	public string Key { get; set; }

	public override object ProvideValue(IServiceProvider serviceProvider) {
		Type? contextType = null;
		if (serviceProvider.GetType().GenericTypeArguments.Length > 0) {
			contextType = serviceProvider.GetType().GenericTypeArguments[0];
		}

		string bindingKey = $"{contextType?.Name ?? "unknown"}|{this.Key}";

#pragma warning disable IL3050
		return new Binding($"[{bindingKey}]") {
			Mode = BindingMode.OneWay,
			Source = LocalizationBindingSource.Instance
		};
#pragma warning restore IL3050
	}
}
