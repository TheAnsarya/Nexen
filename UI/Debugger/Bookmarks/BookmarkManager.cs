using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Nexen.Debugger.Utilities;
using Nexen.Interop;

namespace Nexen.Debugger;

/// <summary>
/// Manages all debugger bookmarks for quick address navigation.
/// </summary>
public static class BookmarkManager {
	public static event EventHandler? BookmarksChanged;

	private static List<Bookmark> _bookmarks = [];
	private static ReadOnlyCollection<Bookmark>? _cachedBookmarks = null;

	private static void InvalidateCache() {
		_cachedBookmarks = null;
	}

	public static ReadOnlyCollection<Bookmark> Bookmarks {
		get {
			return _cachedBookmarks ??= _bookmarks.ToList().AsReadOnly();
		}
	}

	public static List<Bookmark> GetBookmarks(CpuType cpuType) {
		List<Bookmark> bookmarks = [];
		foreach (Bookmark bm in _bookmarks) {
			if (bm.CpuType == cpuType) {
				bookmarks.Add(bm);
			}
		}
		return bookmarks;
	}

	public static void RefreshBookmarks(Bookmark? bm = null) {
		InvalidateCache();
		BookmarksChanged?.Invoke(bm, EventArgs.Empty);
	}

	public static void ClearBookmarks() {
		_bookmarks = new();
		InvalidateCache();
		RefreshBookmarks();
	}

	public static void AddBookmarks(List<Bookmark> bookmarks) {
		_bookmarks.AddRange(bookmarks);
		RefreshBookmarks();
	}

	public static void AddBookmark(Bookmark bm) {
		if (!_bookmarks.Contains(bm)) {
			_bookmarks.Add(bm);
			DebugWorkspaceManager.AutoSave();
		}
		RefreshBookmarks(bm);
	}

	public static void RemoveBookmark(Bookmark bm) {
		if (_bookmarks.Remove(bm)) {
			DebugWorkspaceManager.AutoSave();
		}
		RefreshBookmarks(bm);
	}

	public static void RemoveBookmarks(IEnumerable<Bookmark> bookmarks) {
		foreach (Bookmark bm in bookmarks) {
			_bookmarks.Remove(bm);
		}
		RefreshBookmarks(null);
	}

	public static Bookmark? GetMatchingBookmark(uint address, MemoryType memType, CpuType cpuType) {
		foreach (Bookmark bm in _bookmarks) {
			if (bm.Address == address && bm.MemoryType == memType && bm.CpuType == cpuType) {
				return bm;
			}
		}
		return null;
	}

	public static void ToggleBookmark(uint address, MemoryType memType, CpuType cpuType, string name = "") {
		Bookmark? existing = GetMatchingBookmark(address, memType, cpuType);
		if (existing != null) {
			RemoveBookmark(existing);
		} else {
			AddBookmark(new Bookmark(cpuType, memType, address, name));
		}
	}
}
