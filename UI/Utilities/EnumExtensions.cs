using System;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;

namespace Nexen.Utilities;

public static class EnumExtensions {
	[RequiresUnreferencedCode("Uses reflection to access enum member attributes")]
	public static T? GetAttribute<T>(this Enum val) where T : Attribute {
		return val.GetType().GetMember(val.ToString())[0].GetCustomAttribute<T>();
	}
}
