using System;
using System.Text.Json;
using Nexen.Utilities;
using ReactiveUI;

namespace Nexen.Config; 
public class BaseConfig<T> : ReactiveObject where T : class {
	public T Clone() {
		if (this is T obj) {
			return JsonHelper.Clone<T>(obj);
		} else {
			throw new InvalidCastException();
		}
	}

	public bool IsIdentical(T other) {
		string a = JsonSerializer.Serialize(this, this.GetType(), NexenSerializerContext.Default);
		string b = JsonSerializer.Serialize(other, this.GetType(), NexenSerializerContext.Default);
		return a == b;
	}
}
