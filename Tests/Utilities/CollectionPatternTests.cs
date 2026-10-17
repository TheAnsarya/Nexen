using System.Collections.Frozen;
using System.Text;
using Xunit;

namespace Nexen.Tests.Utilities;

/// <summary>
/// Tests verifying correctness of collection and string patterns used
/// in performance optimizations (#590-#593, #592).
/// </summary>
public class CollectionPatternTests {
	// ===== FrozenDictionary preserves semantics =====

	[Fact]
	public void FrozenDictionary_RetainsAllEntries() {
		var source = new Dictionary<int, string> {
			[1] = "one", [2] = "two", [3] = "three"
		};
		var frozen = source.ToFrozenDictionary();

		Assert.Equal(source.Count, frozen.Count);
		foreach (var kvp in source) {
			Assert.True(frozen.ContainsKey(kvp.Key));
			Assert.Equal(kvp.Value, frozen[kvp.Key]);
		}
	}

	[Fact]
	public void FrozenDictionary_TryGetValue_MatchesDictionary() {
		var source = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase) {
			["Alpha"] = 1, ["Beta"] = 2, ["Gamma"] = 3
		};
		var frozen = source.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase);

		// Existing keys (case-insensitive)
		Assert.True(frozen.TryGetValue("alpha", out int v1));
		Assert.Equal(1, v1);
		Assert.True(frozen.TryGetValue("BETA", out int v2));
		Assert.Equal(2, v2);

		// Missing key
		Assert.False(frozen.TryGetValue("Delta", out _));
	}

	[Fact]
	public void FrozenSet_ContainsAll_OriginalElements() {
		var source = new HashSet<int> { 10, 20, 30, 40, 50 };
		var frozen = source.ToFrozenSet();

		Assert.Equal(source.Count, frozen.Count);
		foreach (int item in source) {
			Assert.Contains(item, (IReadOnlySet<int>)frozen);
		}
		Assert.DoesNotContain(99, (IReadOnlySet<int>)frozen);
	}

	[Fact]
	public void FrozenSet_CaseInsensitive_MatchesHashSet() {
		var source = new HashSet<string>(StringComparer.OrdinalIgnoreCase) {
			"NES", "SNES", "GB", "GBA"
		};
		var frozen = source.ToFrozenSet(StringComparer.OrdinalIgnoreCase);

		Assert.Contains("nes", (IReadOnlySet<string>)frozen);
		Assert.Contains("Snes", (IReadOnlySet<string>)frozen);
		Assert.Contains("GB", (IReadOnlySet<string>)frozen);
		Assert.DoesNotContain("Genesis", (IReadOnlySet<string>)frozen);
	}

	// ===== TryGetValue replaces ContainsKey+indexer =====

	[Fact]
	public void TryGetValue_ReturnsCorrectValue_ForExistingKey() {
		var dict = new Dictionary<string, int> {
			["key1"] = 100, ["key2"] = 200
		};

		Assert.True(dict.TryGetValue("key1", out int value));
		Assert.Equal(100, value);
	}

	[Fact]
	public void TryGetValue_ReturnsFalse_ForMissingKey() {
		var dict = new Dictionary<string, int> { ["key1"] = 100 };

		Assert.False(dict.TryGetValue("missing", out int value));
		Assert.Equal(0, value);
	}

	[Fact]
	public void TryGetValue_EquivalentTo_ContainsKeyPlusIndexer() {
		var dict = new Dictionary<ulong, string>();
		for (int i = 0; i < 100; i++) {
			dict[(ulong)i] = $"Label_{i}";
		}

		// Verify both patterns produce identical results
		for (ulong key = 0; key < 120; key++) {
			bool containsResult = dict.ContainsKey(key);
			bool tryGetResult = dict.TryGetValue(key, out string? tryValue);

			Assert.Equal(containsResult, tryGetResult);
			if (containsResult) {
				Assert.Equal(dict[key], tryValue);
			}
		}
	}

	// ===== TryAdd replaces ContainsKey+Add =====

	[Fact]
	public void TryAdd_AddsNew_ReturnsTrue() {
		var dict = new Dictionary<string, int>();

		Assert.True(dict.TryAdd("key", 42));
		Assert.Equal(42, dict["key"]);
	}

	[Fact]
	public void TryAdd_ExistingKey_ReturnsFalse_KeepsOriginal() {
		var dict = new Dictionary<string, int> { ["key"] = 1 };

		Assert.False(dict.TryAdd("key", 999));
		Assert.Equal(1, dict["key"]);
	}

	[Fact]
	public void TryAdd_EquivalentTo_ContainsKeyGuard() {
		var original = new Dictionary<string, int>();
		var tryAddDict = new Dictionary<string, int>();

		var entries = new[] {
			("a", 1), ("b", 2), ("a", 3), ("c", 4), ("b", 5)
		};

		foreach (var (key, value) in entries) {
			if (!original.ContainsKey(key)) {
				original.Add(key, value);
			}
			tryAddDict.TryAdd(key, value);
		}

		Assert.Equal(original.Count, tryAddDict.Count);
		foreach (var kvp in original) {
			Assert.Equal(kvp.Value, tryAddDict[kvp.Key]);
		}
	}

	// ===== StringBuilder replaces string concat =====

	[Fact]
	public void StringBuilder_ProducesIdenticalOutput_ToConcat() {
		short[] byteCode = [0x4c, 0x00, 0x80, -2, 0xa9, 0xff, -2];

		// String concat version (old pattern)
		string concatResult = "";
		foreach (short s in byteCode) {
			if (s >= 0) {
				concatResult += s.ToString("X2") + " ";
			} else if (s == -2) {
				concatResult += Environment.NewLine;
			}
		}

		// StringBuilder version (new pattern)
		var sb = new StringBuilder();
		foreach (short s in byteCode) {
			if (s >= 0) {
				sb.Append(s.ToString("X2")).Append(' ');
			} else if (s == -2) {
				sb.Append(Environment.NewLine);
			}
		}

		Assert.Equal(concatResult, sb.ToString());
	}

	[Fact]
	public void StringBuilder_HandlesEmptyInput() {
		short[] byteCode = [];
		var sb = new StringBuilder();
		foreach (short s in byteCode) {
			if (s >= 0) sb.Append(s.ToString("X2")).Append(' ');
		}
		Assert.Equal("", sb.ToString());
	}

	// ===== ToArray vs ToList semantics =====

	[Fact]
	public void ToArray_PreservesElementOrder_SameAsToList() {
		var source = Enumerable.Range(0, 100).Select(i => $"Item_{i}");

		var list = source.ToList();
		var array = source.ToArray();

		Assert.Equal(list.Count, array.Length);
		for (int i = 0; i < list.Count; i++) {
			Assert.Equal(list[i], array[i]);
		}
	}
}
