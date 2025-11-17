using Diz.Controllers.interfaces;
using Diz.Core.model;
using Microsoft.Extensions.Logging;

namespace Diz.Import.Mesen;

/// <summary>
/// Extension methods for ProjectController to add Mesen2 live streaming support.
/// Integrates with DiztinGUIsh's existing controller architecture.
/// </summary>
public static class ProjectControllerMesenExtensions
{
    /// <summary>
    /// Start live trace import from Mesen2.
    /// </summary>
    /// <param name="controller">The project controller</param>
    /// <param name="host">Mesen2 server host (default: localhost)</param>
    /// <param name="port">Mesen2 server port (default: 9998)</param>
    /// <param name="settings">Import settings (optional)</param>
    /// <returns>The importer instance for monitoring/control</returns>
    public static async Task<MesenTraceLogImporter?> StartMesenLiveImportAsync(
        this IProjectController controller,
        string host = "localhost", 
        int port = 9998,
        MesenImportSettings? settings = null)
    {
        if (controller.Project?.Data?.GetSnesApi() == null)
        {
            throw new InvalidOperationException("No SNES project loaded");
        }

        var logger = GetLoggerForController(controller);
        var importer = new MesenTraceLogImporter(controller.Project.Data.GetSnesApi(), logger);
        
        if (settings != null)
        {
            importer.Settings = settings;
        }

        var connected = await importer.ConnectAsync(host, port);
        if (!connected)
        {
            importer.Dispose();
            return null;
        }

        // Mark project as changed when we receive data
        var originalStats = importer.CurrentStats;
        
        // TODO: Set up periodic check for statistics changes and call MarkChanged()
        // This would require adding a timer or background task
        
        return importer;
    }

    /// <summary>
    /// Import Mesen2 trace log data (following existing pattern).
    /// </summary>
    /// <param name="controller">The project controller</param>
    /// <param name="host">Mesen2 server host</param>
    /// <param name="port">Mesen2 server port</param>
    /// <param name="settings">Import settings</param>
    /// <returns>Number of ROM bytes modified</returns>
    public static async Task<long> ImportMesenTraceLiveAsync(
        this IProjectController controller, 
        string host = "localhost",
        int port = 9998, 
        MesenImportSettings? settings = null)
    {
        using var importer = await controller.StartMesenLiveImportAsync(host, port, settings);
        if (importer == null)
        {
            return 0;
        }

        // Wait for user to stop or some condition
        // In a real implementation, this would integrate with the UI
        // For now, just wait a bit and return statistics
        await Task.Delay(1000);
        
        // Copy generated comments into main data
        importer.CopyTempGeneratedCommentsIntoMainSnesData();
        
        var bytesModified = importer.CurrentStats.NumRomBytesModified;
        if (bytesModified > 0)
        {
            controller.MarkChanged();
        }

        return bytesModified;
    }

    /// <summary>
    /// Get logger instance for the controller (helper method).
    /// </summary>
    private static ILogger<MesenTraceLogImporter>? GetLoggerForController(IProjectController controller)
    {
        // In a real implementation, this would get the logger from DI container
        // For now, return null (which means no logging)
        return null;
    }
}

/// <summary>
/// Interface extension for future integration with IProjectController.
/// This would be added to the main DiztinGUIsh interfaces if this integration is merged.
/// </summary>
public interface IMesenTraceLogImporters
{
    /// <summary>
    /// Start live trace import from Mesen2 server.
    /// </summary>
    Task<MesenTraceLogImporter?> StartMesenLiveImportAsync(
        string host = "localhost", 
        int port = 9998, 
        MesenImportSettings? settings = null);

    /// <summary>
    /// Import from Mesen2 for a specified duration.
    /// </summary>
    Task<long> ImportMesenTraceLiveAsync(
        string host = "localhost",
        int port = 9998, 
        MesenImportSettings? settings = null);

    /// <summary>
    /// Get list of active Mesen2 connections.
    /// </summary>
    IEnumerable<MesenTraceLogImporter> GetActiveMesenImporters();

    /// <summary>
    /// Stop all Mesen2 live imports.
    /// </summary>
    void StopAllMesenImports();
}

/// <summary>
/// Concrete implementation of Mesen2 importing functionality.
/// This would be integrated into the main ProjectController if this integration is merged.
/// </summary>
public class MesenTraceLogImporterManager : IMesenTraceLogImporters, IDisposable
{
    private readonly IProjectController _controller;
    private readonly ILogger<MesenTraceLogImporter>? _logger;
    private readonly List<MesenTraceLogImporter> _activeImporters = new();
    private readonly object _importersLock = new();

    public MesenTraceLogImporterManager(IProjectController controller, ILogger<MesenTraceLogImporter>? logger = null)
    {
        _controller = controller;
        _logger = logger;
    }

    public async Task<MesenTraceLogImporter?> StartMesenLiveImportAsync(
        string host = "localhost", 
        int port = 9998, 
        MesenImportSettings? settings = null)
    {
        if (_controller.Project?.Data?.GetSnesApi() == null)
        {
            throw new InvalidOperationException("No SNES project loaded");
        }

        var importer = new MesenTraceLogImporter(_controller.Project.Data.GetSnesApi(), _logger);
        
        if (settings != null)
        {
            importer.Settings = settings;
        }

        var connected = await importer.ConnectAsync(host, port);
        if (!connected)
        {
            importer.Dispose();
            return null;
        }

        lock (_importersLock)
        {
            _activeImporters.Add(importer);
        }

        return importer;
    }

    public async Task<long> ImportMesenTraceLiveAsync(
        string host = "localhost",
        int port = 9998, 
        MesenImportSettings? settings = null)
    {
        using var importer = await StartMesenLiveImportAsync(host, port, settings);
        if (importer == null)
        {
            return 0;
        }

        try
        {
            // In real implementation, this would have UI integration
            // For now, just wait a moment
            await Task.Delay(1000);
            
            return importer.CurrentStats.NumRomBytesModified;
        }
        finally
        {
            importer.CopyTempGeneratedCommentsIntoMainSnesData();
            
            lock (_importersLock)
            {
                _activeImporters.Remove(importer);
            }
            
            if (importer.CurrentStats.NumRomBytesModified > 0)
            {
                _controller.MarkChanged();
            }
        }
    }

    public IEnumerable<MesenTraceLogImporter> GetActiveMesenImporters()
    {
        lock (_importersLock)
        {
            return _activeImporters.ToList();
        }
    }

    public void StopAllMesenImports()
    {
        List<MesenTraceLogImporter> toStop;
        
        lock (_importersLock)
        {
            toStop = _activeImporters.ToList();
            _activeImporters.Clear();
        }

        foreach (var importer in toStop)
        {
            try
            {
                importer.CopyTempGeneratedCommentsIntoMainSnesData();
                importer.Dispose();
            }
            catch (Exception ex)
            {
                _logger?.LogError(ex, "Error stopping Mesen2 importer");
            }
        }
    }

    public void Dispose()
    {
        StopAllMesenImports();
        GC.SuppressFinalize(this);
    }
}