#pragma once
#include "pch.h"
#include <string>
#include <unordered_map>

// Advanced configuration system for DiztinGUIsh integration
// Provides runtime configuration of streaming behavior, performance settings, and debugging options
class DiztinguishConfig
{
public:
	// Connection Settings
	struct ConnectionConfig {
		uint16_t defaultPort = 9998;
		uint16_t maxPort = 9999;
		uint32_t connectionTimeoutMs = 10000;  // 10 seconds
		uint32_t heartbeatIntervalMs = 5000;   // 5 seconds
		uint32_t maxReconnectAttempts = 3;
		bool autoReconnect = true;
		bool enableKeepAlive = true;
	};

	// Performance Settings
	struct PerformanceConfig {
		size_t socketBufferSize = 64 * 1024;  // 64KB
		size_t maxTraceBufferSize = 1024;     // 1024 trace entries
		uint32_t flushIntervalMs = 16;        // ~60 FPS
		bool enableCompression = false;        // Future: zlib compression
		bool enableBatching = true;
		uint32_t maxBatchSize = 100;
		bool adaptiveBandwidth = true;
	};

	// Debug & Monitoring
	struct DebugConfig {
		bool enableStatistics = true;
		bool enableBandwidthMonitoring = true;
		bool enableConnectionHealth = true;
		bool logMessages = false;
		bool logTraces = false;
		bool verboseLogging = false;
		std::string logFilePath = "";
	};

	// Data Filtering
	struct FilterConfig {
		bool enableCpuTraces = true;
		bool enableMemoryAccess = true;
		bool enableCdlUpdates = true;
		bool enableBreakpoints = true;
		bool enableLabels = true;
		
		// Address range filtering
		uint32_t minAddress = 0x000000;
		uint32_t maxAddress = 0xFFFFFF;
		
		// Trace filtering
		bool filterByOpcode = false;
		std::unordered_map<uint8_t, bool> opcodeFilter;
		
		bool filterByBank = false;
		std::unordered_map<uint8_t, bool> bankFilter;
	};

	// UI Integration
	struct UIConfig {
		bool enableStatusWindow = true;
		bool showBandwidthGraph = true;
		bool showConnectionStatus = true;
		bool showStatistics = true;
		uint32_t statusUpdateIntervalMs = 1000;  // 1 second
		bool minimizeToTray = false;
		bool showNotifications = true;
	};

private:
	ConnectionConfig _connection;
	PerformanceConfig _performance;
	DebugConfig _debug;
	FilterConfig _filter;
	UIConfig _ui;
	
	std::string _configFilePath;
	bool _configLoaded = false;

public:
	DiztinguishConfig();
	~DiztinguishConfig();

	// Configuration access
	const ConnectionConfig& GetConnection() const { return _connection; }
	const PerformanceConfig& GetPerformance() const { return _performance; }
	const DebugConfig& GetDebug() const { return _debug; }
	const FilterConfig& GetFilter() const { return _filter; }
	const UIConfig& GetUI() const { return _ui; }

	ConnectionConfig& GetConnection() { return _connection; }
	PerformanceConfig& GetPerformance() { return _performance; }
	DebugConfig& GetDebug() { return _debug; }
	FilterConfig& GetFilter() { return _filter; }
	UIConfig& GetUI() { return _ui; }

	// Configuration management
	bool LoadFromFile(const std::string& filePath);
	bool SaveToFile(const std::string& filePath);
	void LoadDefaults();
	void ResetToDefaults();
	
	// Runtime configuration
	void ApplyPerformanceProfile(const std::string& profile); // "fast", "balanced", "quality"
	void EnableDebugMode(bool enable);
	void OptimizeForBandwidth(uint32_t maxBandwidthKBps);
	
	// Validation
	bool ValidateConfig() const;
	std::vector<std::string> GetConfigErrors() const;
	
	// Hot-reload support
	void WatchConfigFile(bool enable);
	bool HasConfigChanged() const;
	void ReloadIfChanged();
};

// Global configuration instance
extern DiztinguishConfig g_DiztinguishConfig;

// Helper macros for easy access
#define DZCONFIG DiztinguishConfig
#define DZ_CONNECTION_CONFIG() g_DiztinguishConfig.GetConnection()
#define DZ_PERFORMANCE_CONFIG() g_DiztinguishConfig.GetPerformance()
#define DZ_DEBUG_CONFIG() g_DiztinguishConfig.GetDebug()
#define DZ_FILTER_CONFIG() g_DiztinguishConfig.GetFilter()
#define DZ_UI_CONFIG() g_DiztinguishConfig.GetUI()