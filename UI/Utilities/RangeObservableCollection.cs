using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;

namespace Nexen.Utilities;

/// <summary>
/// An <see cref="ObservableCollection{T}"/> that supports batch operations
/// (add, insert, remove, replace) that fire a single <see cref="NotifyCollectionChangedAction.Reset"/>
/// notification instead of per-item events.
/// </summary>
public sealed class RangeObservableCollection<T> : ObservableCollection<T> {
	private bool _suppressNotification;

	/// <summary>
	/// Adds a range of items, firing a single Reset notification at the end.
	/// </summary>
	public void AddRange(IEnumerable<T> items) {
		ArgumentNullException.ThrowIfNull(items);

		_suppressNotification = true;
		try {
			foreach (var item in items) {
				Items.Add(item);
			}
		} finally {
			_suppressNotification = false;
		}

		OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
	}

	/// <summary>
	/// Inserts a range of items at the specified index, firing a single Reset notification.
	/// </summary>
	public void InsertRange(int index, IEnumerable<T> items) {
		ArgumentNullException.ThrowIfNull(items);
		if (index < 0 || index > Count) {
			throw new ArgumentOutOfRangeException(nameof(index));
		}

		_suppressNotification = true;
		try {
			int i = index;
			foreach (var item in items) {
				Items.Insert(i++, item);
			}
		} finally {
			_suppressNotification = false;
		}

		OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
	}

	/// <summary>
	/// Removes <paramref name="count"/> items starting at <paramref name="index"/>,
	/// firing a single Reset notification.
	/// </summary>
	public void RemoveRange(int index, int count) {
		if (index < 0 || count < 0 || index + count > Count) {
			throw new ArgumentOutOfRangeException(nameof(index));
		}

		if (count == 0) {
			return;
		}

		_suppressNotification = true;
		try {
			// Remove from end of range backward so indices stay valid
			for (int i = index + count - 1; i >= index; i--) {
				Items.RemoveAt(i);
			}
		} finally {
			_suppressNotification = false;
		}

		OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
	}

	/// <summary>
	/// Clears the collection and replaces it with the given items,
	/// firing a single Reset notification.
	/// </summary>
	public void ReplaceAll(IEnumerable<T> items) {
		ArgumentNullException.ThrowIfNull(items);

		_suppressNotification = true;
		try {
			Items.Clear();
			foreach (var item in items) {
				Items.Add(item);
			}
		} finally {
			_suppressNotification = false;
		}

		OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
	}

	/// <inheritdoc />
	protected override void OnCollectionChanged(NotifyCollectionChangedEventArgs e) {
		if (!_suppressNotification) {
			base.OnCollectionChanged(e);
		}
	}
}
