using System;
using System.Collections.Concurrent;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Debugger.Utilities;
public static class DebugWindowManager {
	private static int _debugWindowCounter = 0;
	private static ConcurrentDictionary<Window, bool> _openedWindows = new();
	private static ReaderWriterLockSlim _windowNotifLock = new();
	private static bool _loadingGame = false;
	private static int _closeInProgress = 0;

	public static T CreateDebugWindow<T>(Func<T> createWindow) where T : NexenWindow {
		int count = Interlocked.Increment(ref _debugWindowCounter);
		Log.Debug($"[DebugWindowManager] CreateDebugWindow start type={typeof(T).Name} count={count}");
		if (count == 1) {
			// Defer workspace loading until DebugApi.InitializeDebugger has completed
			// in DebuggerWindowViewModel. Loading here can occur too early and block.
			Log.Debug("[DebugWindowManager] First debug window creation - workspace load deferred to view model");
		}

		try {
			T wnd = createWindow();
			wnd.Closed += OnClosedHandler;
			_openedWindows.TryAdd(wnd, true);
			Log.Debug($"[DebugWindowManager] CreateDebugWindow success type={typeof(T).Name}");
			return wnd;
		} catch {
			Interlocked.Decrement(ref _debugWindowCounter);
			Log.Debug($"[DebugWindowManager] CreateDebugWindow failed type={typeof(T).Name} countRolledBack={_debugWindowCounter}");
			throw;
		}
	}

	private static void OnClosedHandler(object? sender, EventArgs e) {
		if (sender is Window window) {
			window.Closed -= OnClosedHandler;

			if (window.DataContext is IDisposable disposable) {
				disposable.Dispose();
			}

			CloseDebugWindow(window);
			ConfigManager.Config.Save();
		}
	}

	public static T OpenDebugWindow<T>(Func<T> createWindow) where T : NexenWindow {
		Log.Debug($"[DebugWindowManager] OpenDebugWindow start type={typeof(T).Name}");
		T wnd = CreateDebugWindow<T>(createWindow);
		wnd.Show();
		Log.Debug($"[DebugWindowManager] OpenDebugWindow shown type={typeof(T).Name}");
		return wnd;
	}

	public static T GetOrOpenDebugWindow<T>(Func<T> createWindow) where T : NexenWindow {
		foreach (Window wnd in _openedWindows.Keys) {
			if (wnd is T typedWnd) {
				wnd.BringToFront();
				return typedWnd;
			}
		}

		return OpenDebugWindow<T>(createWindow);
	}

	public static T? GetDebugWindow<T>(Func<T, bool> isMatch) where T : NexenWindow {
		foreach (Window wnd in _openedWindows.Keys) {
			if (wnd is T tWnd && isMatch(tWnd)) {
				return (T)wnd;
			}
		}

		return null;
	}

	private static void CloseDebugWindow(Window wnd) {
		//Remove window from list first, to ensure no more notifications are sent to it
		_openedWindows.TryRemove(wnd, out _);

		if (Interlocked.Decrement(ref _debugWindowCounter) == 0) {
			if (Interlocked.Exchange(ref _closeInProgress, 1) == 1) {
				return;
			}

			//Closed the last debug window, save the workspace and turn off the debugger
			//Run any jobs pending on the UI thread, to ensure the debugger
			//doesn't get restarted by a pending job from the window that was closed
			//Lock to prevent this from running while ProcessNotification is sending
			//notifications out to the debug windows
			_windowNotifLock.EnterWriteLock();
			try {
				Dispatcher.UIThread.RunJobs();
			} finally {
				_windowNotifLock.ExitWriteLock();
			}

			_ = Task.Run(() => {
				try {
					DebugWorkspaceManager.Save(true);

					// Ensure break-wait loops are released before shutting down debugger internals.
					if (DebugApi.IsDebuggerRunning()) {
						DebugApi.ResumeExecution();
					}

					DebugApi.ReleaseDebugger();
				} catch (Exception ex) {
					Log.Error(ex, "[DebugWindowManager] Failed to close debugger cleanly");
				} finally {
					Interlocked.Exchange(ref _closeInProgress, 0);
				}
			});
		}
	}

	public static bool HasOpenedDebugWindows() {
		return _debugWindowCounter > 0;
	}

	public static void CloseAllWindows() {
		//Iterate on a copy of the list since it will change during iteration
		foreach (Window window in _openedWindows.Keys) {
			window.Close();
		}
	}

	public static void ProcessNotification(NotificationEventArgs e) {
		if (_openedWindows.Count == 0) {
			return;
		}

		switch (e.NotificationType) {
			case ConsoleNotificationType.ExecuteShortcut:
			case ConsoleNotificationType.ReleaseShortcut:
			case ConsoleNotificationType.RefreshSoftwareRenderer:
			case ConsoleNotificationType.CheatsChanged:
			case ConsoleNotificationType.ConfigChanged:
			case ConsoleNotificationType.MissingFirmware:
			case ConsoleNotificationType.RequestConfigChange:
			case ConsoleNotificationType.ResolutionChanged:
				//These notifications are never used by debugger windows, don't process them at all here.
				//In particular, ExecuteShortcut/ReleaseShortcut/RefreshSoftwareRenderer can be
				//sent by a thread other than the emulation thread, which could cause deadlocks before.
				return;

			case ConsoleNotificationType.BeforeGameLoad:
				//Suspend all other events until game load is done (or cancelled)
				_loadingGame = true;

				//Run any pending UI calls (and wait for them to complete)
				if (Dispatcher.UIThread.CheckAccess()) {
					Dispatcher.UIThread.RunJobs();
				} else {
					Dispatcher.UIThread.InvokeAsync(() => Dispatcher.UIThread.RunJobs()).Wait();
				}

				break;

			case ConsoleNotificationType.GameLoaded:
			case ConsoleNotificationType.GameLoadFailed:
				//Load operation is done, allow notifications to be sent to windows
				_loadingGame = false;
				break;
		}

		if (!_loadingGame) {
#if DEBUG
			//Allow multiple threads to send notifications to avoid deadlocks if this ever occurs,
			//but throw an exception in debug builds to be able to fix the source of the problem.
			if (_windowNotifLock.CurrentReadCount > 0) {
				throw new Exception("Multiple threads tried to send debugger notifications at the same time");
			}
#endif

			if (_windowNotifLock.TryEnterReadLock(100)) {
				try {
					foreach (Window window in _openedWindows.Keys) {
						if (window is INotificationHandler handler) {
							handler.ProcessNotification(e);
						}
					}
				} finally {
					_windowNotifLock.ExitReadLock();
				}
			} else {
				//Unlikely scenario, but couldn't grab the lock, ignore this notification
				//This should only happen if this code ends up running while the last
				//remaining debug tool is closing and the debugger is trying to shut down.
				return;
			}
		}

		switch (e.NotificationType) {
			case ConsoleNotificationType.GameLoaded:
				GameLoadedEventParams evtParams = Marshal.PtrToStructure<GameLoadedEventParams>(e.Parameter);
				if (!evtParams.IsPowerCycle) {
					//When reloading or loading another rom, reload workspace
					Dispatcher.UIThread.Post(() => DebugWorkspaceManager.Load());
				} else {
					//On power cycle, send the active breakpoints & labels to
					//the core, without reloading the workspace from the disk
					Dispatcher.UIThread.Post(() => {
						BreakpointManager.SetBreakpoints();
						LabelManager.RefreshLabels(false);
					});
				}

				break;

			case ConsoleNotificationType.EmulationStopped:
				Dispatcher.UIThread.Post(() => DebugWindowManager.CloseAllWindows());
				break;
		}
	}

	public static bool IsDebugWindow(Window wnd) {
		return _openedWindows.ContainsKey(wnd);
	}
}
