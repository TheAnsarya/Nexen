using System.CommandLine;
using Microsoft.Extensions.Logging;
using Diz.Import.Mesen;

namespace Mesen.Demo;

/// <summary>
/// Standalone demo application to test Mesen2 live streaming client.
/// Tests the TCP protocol without DiztinGUIsh dependencies.
/// </summary>
public class Program
{
    public static async Task<int> Main(string[] args)
    {
        var hostOption = new Option<string>(
            "--host",
            description: "Mesen2 server hostname or IP address",
            getDefaultValue: () => "localhost");

        var portOption = new Option<int>(
            "--port", 
            description: "Mesen2 server port number",
            getDefaultValue: () => 9998);

        var durationOption = new Option<int>(
            "--duration",
            description: "How long to capture in seconds (0 = infinite)",
            getDefaultValue: () => 10);

        var verboseOption = new Option<bool>(
            "--verbose",
            description: "Enable verbose logging including individual messages");

        var rootCommand = new RootCommand("Mesen2 DiztinGUIsh Live Trace Demo")
        {
            hostOption,
            portOption,
            durationOption,
            verboseOption
        };

        rootCommand.SetHandler(async (host, port, duration, verbose) =>
        {
            await RunDemo(host, port, duration, verbose);
        }, hostOption, portOption, durationOption, verboseOption);

        return await rootCommand.InvokeAsync(args);
    }

    private static async Task RunDemo(string host, int port, int duration, bool verbose)
    {
        // Set up logging
        using var loggerFactory = LoggerFactory.Create(builder =>
        {
            builder.AddConsole().SetMinimumLevel(verbose ? LogLevel.Debug : LogLevel.Information);
        });
        
        var logger = loggerFactory.CreateLogger<Program>();
        var clientLogger = loggerFactory.CreateLogger<MesenLiveTraceClient>();

        Console.WriteLine($"🎮 Mesen2 DiztinGUIsh Live Trace Demo");
        Console.WriteLine($"📡 Connecting to {host}:{port}");
        Console.WriteLine($"⏱️  Duration: {(duration == 0 ? "infinite (Ctrl+C to stop)" : $"{duration} seconds")}");
        Console.WriteLine($"🔍 Verbose: {verbose}");
        Console.WriteLine();

        // Create client with event handlers
        using var client = new MesenLiveTraceClient(clientLogger);
        
        var stats = new DemoStats();
        
        // Wire up events
        client.HandshakeReceived += (_, handshake) =>
        {
            Console.WriteLine($"✅ Connected! {handshake}");
            stats.HandshakeReceived = handshake;
        };

        client.ExecTraceReceived += (_, trace) =>
        {
            stats.ExecTraceCount++;
            if (verbose && stats.ExecTraceCount <= 10) // Show first 10 traces
            {
                Console.WriteLine($"🔄 TRACE #{stats.ExecTraceCount}: {trace}");
            }
        };

        client.CdlUpdateReceived += (_, cdl) =>
        {
            stats.CdlUpdateCount++;
            if (verbose && stats.CdlUpdateCount <= 10) // Show first 10 CDL updates
            {
                Console.WriteLine($"📝 CDL #{stats.CdlUpdateCount}: {cdl}");
            }
        };

        client.FrameReceived += (_, frame) =>
        {
            if (frame.IsStart)
            {
                stats.FrameCount++;
                if (verbose || stats.FrameCount % 60 == 0) // Show every 60 frames or all if verbose
                {
                    Console.WriteLine($"🖼️  {frame}");
                }
            }
        };

        client.ErrorReceived += (_, error) =>
        {
            Console.WriteLine($"❌ Server Error: {error}");
        };

        client.Disconnected += (_, _) =>
        {
            Console.WriteLine($"⚠️  Disconnected from server");
        };

        // Set up cancellation
        using var cts = new CancellationTokenSource();
        if (duration > 0)
        {
            cts.CancelAfter(TimeSpan.FromSeconds(duration));
        }
        
        Console.CancelKeyPress += (_, e) =>
        {
            e.Cancel = true;
            cts.Cancel();
        };

        try
        {
            // Connect
            var connected = await client.ConnectAsync(host, port);
            if (!connected)
            {
                Console.WriteLine($"❌ Failed to connect to {host}:{port}");
                Console.WriteLine($"💡 Make sure Mesen2 is running with DiztinGUIsh server enabled");
                Console.WriteLine($"   In Mesen2: Load a SNES ROM, open Script Window, run:");
                Console.WriteLine($"   emu.startDiztinguishServer({port})");
                return;
            }

            // Wait for specified duration
            var startTime = DateTime.Now;
            var lastStatsTime = DateTime.Now;

            while (!cts.Token.IsCancellationRequested)
            {
                await Task.Delay(1000, cts.Token);

                // Update statistics every second
                var now = DateTime.Now;
                if ((now - lastStatsTime).TotalSeconds >= 1)
                {
                    UpdateStats(client, stats, now - startTime);
                    lastStatsTime = now;
                }
            }
        }
        catch (OperationCanceledException)
        {
            // Expected when duration expires or Ctrl+C
        }
        catch (Exception ex)
        {
            Console.WriteLine($"❌ Error: {ex.Message}");
            logger.LogError(ex, "Demo error");
        }
        finally
        {
            if (client.IsConnected)
            {
                Console.WriteLine($"🔌 Disconnecting...");
                client.Disconnect();
            }

            // Final statistics
            Console.WriteLine();
            Console.WriteLine($"📊 Final Statistics:");
            PrintFinalStats(stats);
        }
    }

    private static void UpdateStats(MesenLiveTraceClient client, DemoStats stats, TimeSpan elapsed)
    {
        var elapsedSeconds = elapsed.TotalSeconds;
        var execPerSec = elapsedSeconds > 0 ? stats.ExecTraceCount / elapsedSeconds : 0;
        var cdlPerSec = elapsedSeconds > 0 ? stats.CdlUpdateCount / elapsedSeconds : 0;
        var fps = elapsedSeconds > 0 ? stats.FrameCount / elapsedSeconds : 0;

        // Clear previous line and print current stats
        Console.Write($"\r⏱️  {elapsed:mm\\:ss} | " +
                     $"📊 Exec: {stats.ExecTraceCount:N0} ({execPerSec:F1}/sec) | " +
                     $"📝 CDL: {stats.CdlUpdateCount:N0} ({cdlPerSec:F1}/sec) | " +
                     $"🖼️  Frames: {stats.FrameCount:N0} ({fps:F1} FPS) | " +
                     $"📡 {client.BytesReceived:N0} bytes");
    }

    private static void PrintFinalStats(DemoStats stats)
    {
        Console.WriteLine($"   Handshake: {(stats.HandshakeReceived.HasValue ? "✅" : "❌")}");
        
        if (stats.HandshakeReceived.HasValue)
        {
            var hs = stats.HandshakeReceived.Value;
            Console.WriteLine($"   Protocol: v{hs.ProtocolVersion}");
            Console.WriteLine($"   Mesen2: v{hs.EmulatorVersionMajor}.{hs.EmulatorVersionMinor}.{hs.EmulatorVersionPatch}");
            Console.WriteLine($"   ROM: {hs.RomSize:N0} bytes (CRC32: 0x{hs.RomCrc32:X8})");
        }
        
        Console.WriteLine($"   Messages:");
        Console.WriteLine($"     - Exec Traces: {stats.ExecTraceCount:N0}");
        Console.WriteLine($"     - CDL Updates: {stats.CdlUpdateCount:N0}");
        Console.WriteLine($"     - Frame Markers: {stats.FrameCount:N0}");
        Console.WriteLine($"   Performance:");
        
        if (stats.ExecTraceCount > 0)
        {
            var bytesPerTrace = (stats.ExecTraceCount > 0) ? (14 + 5) : 0; // 14 bytes payload + 5 bytes header
            var totalTraceBytes = stats.ExecTraceCount * bytesPerTrace;
            Console.WriteLine($"     - Estimated execution trace bandwidth: {totalTraceBytes:N0} bytes");
            
            // Estimate CPU frequency based on trace rate
            // Typical SNES runs at ~1.79 MHz for CPU instructions
            if (stats.ExecTraceCount > 100) // Only estimate if we have reasonable data
            {
                Console.WriteLine($"     - CPU instruction rate: ~{stats.ExecTraceCount:N0} instructions captured");
                Console.WriteLine($"     - (SNES CPU typically runs at ~1,790,000 instructions/sec)");
            }
        }

        Console.WriteLine();
        Console.WriteLine($"💡 To use with DiztinGUIsh:");
        Console.WriteLine($"   1. Load this ROM in DiztinGUIsh");
        Console.WriteLine($"   2. Use Diz.Import.Mesen library to connect");
        Console.WriteLine($"   3. Live trace data will update your disassembly in real-time!");
    }
}

/// <summary>
/// Demo statistics tracking.
/// </summary>
internal class DemoStats
{
    public MesenHandshakeMessage? HandshakeReceived { get; set; }
    public long ExecTraceCount { get; set; }
    public long CdlUpdateCount { get; set; }
    public long FrameCount { get; set; }
}