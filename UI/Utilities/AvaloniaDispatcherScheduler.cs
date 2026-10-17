using System;
using System.Reactive.Concurrency;
using System.Reactive.Disposables;
using Avalonia.Threading;

namespace Nexen.Utilities;

/// <summary>
/// Reactive Extensions scheduler that dispatches work to Avalonia's UI thread.
/// Replaces the scheduler previously provided by the ReactiveUI.Avalonia bridge package,
/// which has no Avalonia 12-compatible release.
/// </summary>
internal sealed class AvaloniaDispatcherScheduler : LocalScheduler {
	public static readonly AvaloniaDispatcherScheduler Instance = new();

	private AvaloniaDispatcherScheduler() { }

	public override IDisposable Schedule<TState>(TState state, TimeSpan dueTime, Func<IScheduler, TState, IDisposable> action) {
		IDisposable innerDisposable = Disposable.Empty;
		bool cancelled = false;

		void Execute() {
			if (!cancelled) {
				innerDisposable = action(this, state);
			}
		}

		if (dueTime <= TimeSpan.Zero) {
			if (Dispatcher.UIThread.CheckAccess()) {
				return action(this, state);
			}

			Dispatcher.UIThread.Post(Execute, DispatcherPriority.Normal);
		} else {
			DispatcherTimer.RunOnce(Execute, dueTime, DispatcherPriority.Normal);
		}

		return Disposable.Create(() => {
			cancelled = true;
			innerDisposable.Dispose();
		});
	}
}
