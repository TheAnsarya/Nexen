using System;
using System.Collections;
using System.Reactive.Disposables;
using ReactiveUI;

namespace Nexen.ViewModels;

/// <summary>
/// Base class for all ViewModels in the application.
/// Inherits from <see cref="ReactiveObject"/> for reactive property support.
/// </summary>
public class ViewModelBase : ReactiveObject {
}

/// <summary>
/// Base class for ViewModels that need to manage disposable resources.
/// Provides automatic disposal tracking and cleanup using <see cref="CompositeDisposable"/>.
/// </summary>
/// <remarks>
/// Use <see cref="AddDisposable{T}"/> to register disposables that should be
/// cleaned up when the ViewModel is disposed. Override <see cref="DisposeView"/>
/// for custom cleanup logic.
/// </remarks>
public class DisposableViewModel : ViewModelBase, IDisposable {
	/// <summary>Collection of tracked disposables using System.Reactive pattern.</summary>
	private readonly CompositeDisposable _disposables = [];

	/// <summary>
	/// Gets whether this ViewModel has been disposed.
	/// </summary>
	public bool Disposed { get; private set; } = false;

	/// <summary>
	/// Disposes all tracked resources and marks the ViewModel as disposed.
	/// </summary>
	public void Dispose() {
		if (Disposed) {
			return;
		}

		Disposed = true;
		_disposables.Dispose();
		DisposeView();
		GC.SuppressFinalize(this);
	}

	/// <summary>
	/// Called during disposal. Override to add custom cleanup logic.
	/// </summary>
	protected virtual void DisposeView() { }

	/// <summary>
	/// Adds a disposable object to be tracked and disposed with this ViewModel.
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
