using System;
using System.Collections;
using System.Reactive.Disposables;
using Avalonia.Controls;
using Avalonia.Interactivity;

namespace Nexen.Utilities;

/// <summary>
/// Base UserControl class that provides automatic disposal tracking using <see cref="CompositeDisposable"/>.
/// Resources are automatically disposed when the control is unloaded.
/// </summary>
public class NexenUserControl : UserControl, IDisposable {
	/// <summary>Collection of tracked disposables using System.Reactive pattern.</summary>
	private readonly CompositeDisposable _disposables = [];

	/// <summary>
	/// Gets whether this control has been disposed.
	/// </summary>
	public bool Disposed { get; private set; } = false;

	protected override void OnUnloaded(RoutedEventArgs e) {
		base.OnUnloaded(e);
		Dispose();
	}

	/// <summary>
	/// Disposes all tracked resources and marks the control as disposed.
	/// </summary>
	public void Dispose() {
		if (Disposed) {
			return;
		}

		Disposed = true;
		_disposables.Dispose();
		GC.SuppressFinalize(this);
	}

	/// <summary>
	/// Adds a disposable object to be tracked and disposed with this control.
	/// </summary>
	/// <typeparam name="T">The type of disposable.</typeparam>
	/// <param name="obj">The disposable object to track.</param>
	/// <returns>The same object for fluent chaining.</returns>
	public T AddDisposable<T>(T obj) where T : IDisposable {
		_disposables.Add(obj);
		return obj;
	}

	/// <summary>
	/// Adds multiple disposable objects from a collection.
	/// </summary>
	/// <typeparam name="T">The collection type.</typeparam>
	/// <param name="disposables">Collection of disposables to track.</param>
	/// <returns>The same collection for fluent chaining.</returns>
	public T AddDisposables<T>(T disposables) where T : IEnumerable {
		foreach (object obj in disposables) {
			if (obj is IDisposable disposable) {
				_disposables.Add(disposable);
			}
		}

		return disposables;
	}
}
