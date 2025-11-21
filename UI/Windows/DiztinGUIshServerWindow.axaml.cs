using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using System;
using System.ComponentModel;
using Avalonia.Data;
using Mesen.Interop;
using Mesen.Utilities;
using System.Threading.Tasks;

namespace Mesen.Windows
{
	public class DiztinGUIshServerWindow : MesenWindow
	{
		private DispatcherTimer _updateTimer;
		private bool _serverRunning = false;

		public DiztinGUIshServerWindow()
		{
			InitializeComponent();
			
			// Start update timer to refresh status every 500ms
			_updateTimer = new DispatcherTimer(TimeSpan.FromMilliseconds(500), DispatcherPriority.Normal, (s, e) => UpdateStatus());
			_updateTimer.Start();
			
			// Update initial status
			UpdateStatus();
		}

		private void InitializeComponent()
		{
			AvaloniaXamlLoader.Load(this);
		}

		private void UpdateStatus()
		{
			try 
			{
				bool serverRunning = DiztinguishApi.IsServerRunning();
				bool clientConnected = DiztinguishApi.IsClientConnected();
				
				// Update server status
				var lblStatus = this.GetControl<TextBlock>("lblStatus");
				var btnStartStop = this.GetControl<Button>("btnStartStop");
				
				if (serverRunning) 
				{
					bool emulationRunning = !EmuApi.IsPaused() && EmuApi.IsRunning();
					lblStatus.Text = $"Server running on port {DiztinguishApi.GetServerPort()} - Emulation {(emulationRunning ? "RUNNING" : "PAUSED")}";
					lblStatus.Foreground = emulationRunning ? Avalonia.Media.Brushes.Green : Avalonia.Media.Brushes.Orange;
					btnStartStop.Content = "Stop Server";
				} 
				else 
				{
					lblStatus.Text = "Server stopped";
					lblStatus.Foreground = Avalonia.Media.Brushes.Red;
					btnStartStop.Content = "Start Server";
				}
				
				_serverRunning = serverRunning;

				// Update connection status
				var lblClientConnected = this.GetControl<TextBlock>("lblClientConnected");
				if (clientConnected) 
				{
					lblClientConnected.Text = "Yes";
					lblClientConnected.Foreground = Avalonia.Media.Brushes.Green;
				} 
				else 
				{
					lblClientConnected.Text = "No";
					lblClientConnected.Foreground = Avalonia.Media.Brushes.Gray;
				}

				// Update connection duration
				var lblConnectionDuration = this.GetControl<TextBlock>("lblConnectionDuration");
				if (clientConnected) 
				{
					var duration = DiztinguishApi.GetConnectionDuration();
					lblConnectionDuration.Text = $"{duration.Minutes:D2}:{duration.Seconds:D2}";
				} 
				else 
				{
					lblConnectionDuration.Text = "--";
				}

				// Update bandwidth
				var lblBandwidth = this.GetControl<TextBlock>("lblBandwidth");
				if (clientConnected) 
				{
					double bandwidthKBps = DiztinguishApi.GetBandwidthKBps();
					lblBandwidth.Text = $"{bandwidthKBps:F1} KB/s";
				} 
				else 
				{
					lblBandwidth.Text = "--";
				}

				// Update statistics
				this.GetControl<TextBlock>("lblMessagesSent").Text = DiztinguishApi.GetMessagesSent().ToString("N0");
				this.GetControl<TextBlock>("lblMessagesReceived").Text = DiztinguishApi.GetMessagesReceived().ToString("N0");
				this.GetControl<TextBlock>("lblBytesSent").Text = DiztinguishApi.GetBytesSent().ToString("N0");
				this.GetControl<TextBlock>("lblBytesReceived").Text = DiztinguishApi.GetBytesReceived().ToString("N0");

				// Add diagnostic info for troubleshooting
				if (serverRunning && clientConnected)
				{
					uint currentFrame = DiztinguishApi.GetCurrentFrame();
					uint traceBufferSize = DiztinguishApi.GetTraceBufferSize();
					bool configReceived = DiztinguishApi.IsConfigReceived();
					bool emulationRunning = !EmuApi.IsPaused() && EmuApi.IsRunning();
					
					string diagnostics = $"Frame: {currentFrame}, Buffer: {traceBufferSize}, Config: {(configReceived ? "OK" : "MISSING")}, Emu: {(emulationRunning ? "RUN" : "PAUSE")}";
					
					// Update the bandwidth label to show diagnostics when no data is flowing
					var lblBandwidth = this.GetControl<TextBlock>("lblBandwidth");
					if (DiztinguishApi.GetBytesSent() == 0 && clientConnected)
					{
						lblBandwidth.Text = diagnostics;
					}
				}
			}
			catch (Exception ex)
			{
				// Handle any API access errors gracefully
				try {
					var lblStatus = this.GetControl<TextBlock>("lblStatus");
					lblStatus.Text = $"Error accessing DiztinGUIsh server: {ex.Message}";
					lblStatus.Foreground = Avalonia.Media.Brushes.Red;
				} catch {
					// If we can't even access the UI controls, silently fail
				}
			}
		}

		private async void StartStop_OnClick(object? sender, RoutedEventArgs e)
		{
			try 
			{
				if (_serverRunning) 
				{
					// Stop server
					DiztinguishApi.StopServer();
				} 
				else 
				{
					// Start server
					var txtPort = this.GetControl<TextBox>("txtPort");
					if (ushort.TryParse(txtPort.Text, out ushort port)) 
					{
						bool success = DiztinguishApi.StartServer(port);
						if (!success) 
						{
							await MesenMsgBox.Show(this, $"Failed to start server on port {port}. Port may be in use.", MessageBoxButtons.OK, MessageBoxIcon.Error);
						}
					} 
					else 
					{
						await MesenMsgBox.Show(this, "Invalid port number. Please enter a valid port (1-65535).", MessageBoxButtons.OK, MessageBoxIcon.Warning);
						return;
					}
				}
				
				// Update status immediately
				UpdateStatus();
			}
			catch (Exception ex)
			{
				await MesenMsgBox.Show(this, $"Error controlling server: {ex.Message}", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
		}

		private void Reset_OnClick(object? sender, RoutedEventArgs e)
		{
			// Note: The C++ bridge doesn't currently have a reset statistics method,
			// so we'll just refresh the display. Statistics reset when server restarts.
			UpdateStatus();
		}

		private void Close_OnClick(object? sender, RoutedEventArgs e)
		{
			Close();
		}

		protected override void OnClosing(WindowClosingEventArgs e)
		{
			_updateTimer?.Stop();
			base.OnClosing(e);
		}
	}
}