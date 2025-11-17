using System.Collections.Concurrent;
using Diz.Core.Interfaces;
using Diz.Cpu._65816;
using Microsoft.Extensions.Logging;

namespace Diz.Import.Mesen;

/// <summary>
/// DiztinGUIsh importer for Mesen2 live trace streaming.
/// Integrates with DiztinGUIsh's SNES data model and follows the same patterns as BSNESTraceLogImporter.
/// </summary>
public class MesenTraceLogImporter : IDisposable
{
    private readonly ISnesData? _snesData;
    private readonly ILogger<MesenTraceLogImporter>? _logger;
    private readonly MesenLiveTraceClient _client;
    
    // Statistics tracking (matches BSNES pattern)
    private readonly object _statsLock = new();
    private ImportStats _currentStats;
    
    // Temporary comment storage (matches BSNES pattern)
    private readonly ConcurrentDictionary<int, string> _tracelogCommentsGenerated = new();
    
    // Cached values for performance (matches BSNES pattern)
    private readonly int _romSizeCached;
    private readonly RomMapMode _romMapModeCached;
    
    // Import settings
    public MesenImportSettings Settings { get; set; } = new();
    
    // Connection state
    public bool IsConnected => _client.IsConnected;
    public string ConnectionStats => _client.GetConnectionStats();
    public MesenHandshakeMessage? LastHandshake { get; private set; }

    // Statistics
    public ImportStats CurrentStats
    {
        get
        {
            lock (_statsLock)
            {
                return _currentStats;
            }
        }
    }

    public MesenTraceLogImporter(ISnesData? snesData, ILogger<MesenTraceLogImporter>? logger = null)
    {
        _snesData = snesData;
        _logger = logger;
        
        // Cache immutable values for performance
        _romSizeCached = _snesData?.GetRomSize() ?? 0;
        _romMapModeCached = _snesData?.RomMapMode ?? RomMapMode.LoRom;
        
        // Create client and wire up events
        _client = new MesenLiveTraceClient(logger);
        _client.HandshakeReceived += OnHandshakeReceived;
        _client.ExecTraceReceived += OnExecTraceReceived;
        _client.CdlUpdateReceived += OnCdlUpdateReceived;
        _client.CpuStateReceived += OnCpuStateReceived;
        _client.FrameReceived += OnFrameReceived;
        _client.ErrorReceived += OnErrorReceived;
        _client.Disconnected += OnDisconnected;
        
        InitStats();
        
        _logger?.LogDebug("MesenTraceLogImporter created for ROM: {RomSize} bytes, {MapMode}", 
            _romSizeCached, _romMapModeCached);
    }

    /// <summary>
    /// Connect to Mesen2 server and start receiving trace data.
    /// </summary>
    public async Task<bool> ConnectAsync(string host = "localhost", int port = 9998)
    {
        _logger?.LogInformation("Connecting to Mesen2 at {Host}:{Port}...", host, port);
        
        var success = await _client.ConnectAsync(host, port);
        if (success)
        {
            _logger?.LogInformation("Successfully connected to Mesen2 DiztinGUIsh server");
            InitStats(); // Reset stats for new connection
        }
        else
        {
            _logger?.LogError("Failed to connect to Mesen2 server");
        }
        
        return success;
    }

    /// <summary>
    /// Disconnect from server and stop receiving data.
    /// </summary>
    public void Disconnect()
    {
        _client.Disconnect();
    }

    /// <summary>
    /// Copy temporarily generated comments into main SNES data.
    /// Call this when import is complete (matches BSNES pattern).
    /// </summary>
    public void CopyTempGeneratedCommentsIntoMainSnesData()
    {
        if (_snesData?.Comments == null)
            return;

        var count = 0;
        foreach (var kvp in _tracelogCommentsGenerated)
        {
            _snesData.Comments[kvp.Key] = kvp.Value;
            count++;
        }
        
        _tracelogCommentsGenerated.Clear();
        _logger?.LogDebug("Copied {Count} trace comments into main SNES data", count);
    }

    /// <summary>
    /// Handle handshake message from server.
    /// </summary>
    private void OnHandshakeReceived(object? sender, MesenHandshakeMessage handshake)
    {
        LastHandshake = handshake;
        _logger?.LogInformation("Handshake received: {Handshake}", handshake);
        
        // Validate protocol compatibility
        if (handshake.ProtocolVersion != 1)
        {
            _logger?.LogWarning("Protocol version mismatch: Expected 1, got {Version}", handshake.ProtocolVersion);
        }
        
        // Validate ROM if we have one loaded
        if (_romSizeCached > 0 && handshake.RomSize != _romSizeCached)
        {
            _logger?.LogWarning("ROM size mismatch: DiztinGUIsh has {DizSize} bytes, Mesen2 reports {MesenSize} bytes", 
                _romSizeCached, handshake.RomSize);
        }
    }

    /// <summary>
    /// Handle CPU execution trace from Mesen2.
    /// </summary>
    private void OnExecTraceReceived(object? sender, MesenExecTraceMessage trace)
    {
        // Convert Mesen2 address to ROM offset
        var romOffset = ConvertSnesAddressToRomOffset(trace.PC);
        if (romOffset < 0 || romOffset >= _romSizeCached)
        {
            // Address is not in ROM (probably RAM/registers), skip
            return;
        }

        try
        {
            lock (_statsLock)
            {
                _currentStats.NumRomBytesAnalyzed++;
            }

            // Update SNES data with trace information
            var updated = UpdateSnesDataFromTrace(romOffset, trace);
            if (updated)
            {
                lock (_statsLock)
                {
                    _currentStats.NumRomBytesModified++;
                }
            }

            // Add trace comment if enabled
            if (Settings.AddTraceComments)
            {
                var comment = $"TRACE: {trace}";
                _tracelogCommentsGenerated.TryAdd((int)trace.PC, comment);
                
                lock (_statsLock)
                {
                    _currentStats.NumCommentsMarked++;
                }
            }
        }
        catch (Exception ex)
        {
            _logger?.LogError(ex, "Error processing execution trace: {Trace}", trace);
        }
    }

    /// <summary>
    /// Handle CDL update from Mesen2.
    /// </summary>
    private void OnCdlUpdateReceived(object? sender, MesenCdlUpdateMessage cdl)
    {
        // Convert Mesen2 address to ROM offset
        var romOffset = ConvertSnesAddressToRomOffset(cdl.Address);
        if (romOffset < 0 || romOffset >= _romSizeCached)
        {
            return; // Address not in ROM
        }

        try
        {
            // Update DiztinGUIsh CDL data
            var updated = UpdateSnesDataFromCdl(romOffset, cdl);
            if (updated)
            {
                lock (_statsLock)
                {
                    _currentStats.NumMarksModified++;
                }
            }
        }
        catch (Exception ex)
        {
            _logger?.LogError(ex, "Error processing CDL update: {Cdl}", cdl);
        }
    }

    /// <summary>
    /// Handle CPU state snapshot from Mesen2.
    /// </summary>
    private void OnCpuStateReceived(object? sender, MesenCpuStateMessage cpuState)
    {
        // This could be used for more sophisticated analysis
        // For now, just log it if debugging is enabled
        if (Settings.LogCpuStates)
        {
            _logger?.LogDebug("CPU State: {CpuState}", cpuState);
        }
    }

    /// <summary>
    /// Handle frame boundary markers.
    /// </summary>
    private void OnFrameReceived(object? sender, MesenFrameMessage frame)
    {
        if (Settings.LogFrameMarkers)
        {
            _logger?.LogDebug("Frame: {Frame}", frame);
        }
    }

    /// <summary>
    /// Handle error messages from server.
    /// </summary>
    private void OnErrorReceived(object? sender, MesenErrorMessage error)
    {
        _logger?.LogError("Mesen2 server error: {Error}", error);
    }

    /// <summary>
    /// Handle disconnection from server.
    /// </summary>
    private void OnDisconnected(object? sender, EventArgs e)
    {
        _logger?.LogInformation("Disconnected from Mesen2 server");
    }

    /// <summary>
    /// Convert SNES address to ROM file offset.
    /// </summary>
    private int ConvertSnesAddressToRomOffset(uint snesAddress)
    {
        // Use DiztinGUIsh's existing SNES address conversion logic
        return _snesData?.ConvertSnesToPc((int)snesAddress) ?? -1;
    }

    /// <summary>
    /// Update DiztinGUIsh SNES data based on execution trace.
    /// </summary>
    private bool UpdateSnesDataFromTrace(int romOffset, MesenExecTraceMessage trace)
    {
        if (_snesData == null)
            return false;

        var updated = false;

        // Mark as opcode (executed instruction)
        var currentFlag = _snesData.GetFlag(romOffset);
        if (currentFlag != FlagType.Opcode)
        {
            _snesData.SetFlag(romOffset, FlagType.Opcode);
            updated = true;
        }

        // Update M flag if changed
        var currentMFlag = _snesData.GetMFlag(romOffset);
        if (currentMFlag != trace.MFlag)
        {
            _snesData.SetMFlag(romOffset, trace.MFlag);
            updated = true;
            
            lock (_statsLock)
            {
                _currentStats.NumMFlagsModified++;
            }
        }

        // Update X flag if changed
        var currentXFlag = _snesData.GetXFlag(romOffset);
        if (currentXFlag != trace.XFlag)
        {
            _snesData.SetXFlag(romOffset, trace.XFlag);
            updated = true;
            
            lock (_statsLock)
            {
                _currentStats.NumXFlagsModified++;
            }
        }

        // Update Data Bank if changed
        var currentDb = _snesData.GetDataBank(romOffset);
        if (currentDb != trace.DataBank)
        {
            _snesData.SetDataBank(romOffset, trace.DataBank);
            updated = true;
            
            lock (_statsLock)
            {
                _currentStats.NumDbModified++;
            }
        }

        // Update Direct Page if changed
        var currentDp = _snesData.GetDirectPage(romOffset);
        if (currentDp != trace.DirectPage)
        {
            _snesData.SetDirectPage(romOffset, trace.DirectPage);
            updated = true;
            
            lock (_statsLock)
            {
                _currentStats.NumDpModified++;
            }
        }

        return updated;
    }

    /// <summary>
    /// Update DiztinGUIsh SNES data based on CDL information.
    /// </summary>
    private bool UpdateSnesDataFromCdl(int romOffset, MesenCdlUpdateMessage cdl)
    {
        if (_snesData == null)
            return false;

        // Convert Mesen2 CDL flags to DiztinGUIsh flag types
        var newFlag = FlagType.Unreached;
        
        if (cdl.IsCode)
        {
            newFlag = FlagType.Opcode;
        }
        else if (cdl.IsData || cdl.IsIndirectData || cdl.IsIndexedData)
        {
            newFlag = FlagType.Data8Bit; // Default to 8-bit data
        }

        // Only update if flag has changed
        var currentFlag = _snesData.GetFlag(romOffset);
        if (currentFlag != newFlag)
        {
            _snesData.SetFlag(romOffset, newFlag);
            return true;
        }

        return false;
    }

    /// <summary>
    /// Initialize statistics tracking.
    /// </summary>
    private void InitStats()
    {
        lock (_statsLock)
        {
            _currentStats = new ImportStats();
        }
    }

    public void Dispose()
    {
        _client?.Dispose();
        GC.SuppressFinalize(this);
    }
}

/// <summary>
/// Import statistics (matches BSNES pattern).
/// </summary>
public struct ImportStats
{
    public long NumRomBytesAnalyzed { get; set; }
    public long NumRomBytesModified { get; set; }
    public long NumXFlagsModified { get; set; }
    public long NumMFlagsModified { get; set; }
    public long NumDbModified { get; set; }
    public long NumDpModified { get; set; }
    public long NumMarksModified { get; set; }
    public int NumCommentsMarked { get; set; }
    
    public override string ToString() => 
        $"Analyzed: {NumRomBytesAnalyzed:N0}, Modified: {NumRomBytesModified:N0}, " +
        $"Flags: M={NumMFlagsModified:N0} X={NumXFlagsModified:N0} DB={NumDbModified:N0} DP={NumDpModified:N0}, " +
        $"Marks: {NumMarksModified:N0}, Comments: {NumCommentsMarked:N0}";
}

/// <summary>
/// Settings for Mesen2 import behavior.
/// </summary>
public class MesenImportSettings
{
    /// <summary>
    /// Add trace comments to ROM bytes showing execution details.
    /// </summary>
    public bool AddTraceComments { get; set; } = true;

    /// <summary>
    /// Log CPU state changes for debugging.
    /// </summary>
    public bool LogCpuStates { get; set; } = false;

    /// <summary>
    /// Log frame markers for debugging.
    /// </summary>
    public bool LogFrameMarkers { get; set; } = false;

    /// <summary>
    /// Timeout for network operations (milliseconds).
    /// </summary>
    public int NetworkTimeoutMs { get; set; } = 5000;
}