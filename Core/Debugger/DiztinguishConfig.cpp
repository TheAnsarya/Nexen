#include "pch.h"
#include "DiztinguishConfig.h"
#include <fstream>
#include <json/json.h>  // Note: May need to include JSON library for config files
#include <iostream>

// Global configuration instance
DiztinguishConfig g_DiztinguishConfig;

DiztinguishConfig::DiztinguishConfig()
{
	LoadDefaults();
}

DiztinguishConfig::~DiztinguishConfig()
{
	// Auto-save configuration on exit
	if (_configLoaded && !_configFilePath.empty()) {
		SaveToFile(_configFilePath);
	}
}

void DiztinguishConfig::LoadDefaults()
{
	// Connection defaults (already set in struct definitions)
	_connection = ConnectionConfig{};
	
	// Performance defaults  
	_performance = PerformanceConfig{};
	
	// Debug defaults
	_debug = DebugConfig{};
	
	// Filter defaults
	_filter = FilterConfig{};
	
	// UI defaults
	_ui = UIConfig{};
	
	_configLoaded = true;
}

void DiztinguishConfig::ResetToDefaults()
{
	LoadDefaults();
}

bool DiztinguishConfig::LoadFromFile(const std::string& filePath)
{
	try {
		std::ifstream configFile(filePath);
		if (!configFile.is_open()) {
			std::cout << "DiztinguishConfig: Could not open config file: " << filePath << std::endl;
			return false;
		}
		
		// For now, implement basic key=value parsing
		// TODO: Implement full JSON parsing when JSON library is available
		std::string line;
		while (std::getline(configFile, line)) {
			if (line.empty() || line[0] == '#') continue;  // Skip comments
			
			size_t pos = line.find('=');
			if (pos != std::string::npos) {
				std::string key = line.substr(0, pos);
				std::string value = line.substr(pos + 1);
				
				// Basic configuration parsing
				if (key == "defaultPort") {
					_connection.defaultPort = static_cast<uint16_t>(std::stoi(value));
				} else if (key == "socketBufferSize") {
					_performance.socketBufferSize = std::stoul(value);
				} else if (key == "enableStatistics") {
					_debug.enableStatistics = (value == "true" || value == "1");
				} else if (key == "enableCpuTraces") {
					_filter.enableCpuTraces = (value == "true" || value == "1");
				} else if (key == "enableStatusWindow") {
					_ui.enableStatusWindow = (value == "true" || value == "1");
				}
				// Add more configuration options as needed
			}
		}
		
		_configFilePath = filePath;
		_configLoaded = true;
		
		std::cout << "DiztinguishConfig: Loaded configuration from " << filePath << std::endl;
		return true;
		
	} catch (const std::exception& e) {
		std::cout << "DiztinguishConfig: Error loading config file: " << e.what() << std::endl;
		return false;
	}
}

bool DiztinguishConfig::SaveToFile(const std::string& filePath)
{
	try {
		std::ofstream configFile(filePath);
		if (!configFile.is_open()) {
			std::cout << "DiztinguishConfig: Could not create config file: " << filePath << std::endl;
			return false;
		}
		
		// Write header
		configFile << "# DiztinGUIsh Integration Configuration\n";
		configFile << "# Auto-generated on " << __DATE__ << " at " << __TIME__ << "\n\n";
		
		// Connection settings
		configFile << "[Connection]\n";
		configFile << "defaultPort=" << _connection.defaultPort << "\n";
		configFile << "maxPort=" << _connection.maxPort << "\n"; 
		configFile << "connectionTimeoutMs=" << _connection.connectionTimeoutMs << "\n";
		configFile << "heartbeatIntervalMs=" << _connection.heartbeatIntervalMs << "\n";
		configFile << "maxReconnectAttempts=" << _connection.maxReconnectAttempts << "\n";
		configFile << "autoReconnect=" << (_connection.autoReconnect ? "true" : "false") << "\n";
		configFile << "enableKeepAlive=" << (_connection.enableKeepAlive ? "true" : "false") << "\n\n";
		
		// Performance settings
		configFile << "[Performance]\n";
		configFile << "socketBufferSize=" << _performance.socketBufferSize << "\n";
		configFile << "maxTraceBufferSize=" << _performance.maxTraceBufferSize << "\n";
		configFile << "flushIntervalMs=" << _performance.flushIntervalMs << "\n";
		configFile << "enableCompression=" << (_performance.enableCompression ? "true" : "false") << "\n";
		configFile << "enableBatching=" << (_performance.enableBatching ? "true" : "false") << "\n";
		configFile << "maxBatchSize=" << _performance.maxBatchSize << "\n";
		configFile << "adaptiveBandwidth=" << (_performance.adaptiveBandwidth ? "true" : "false") << "\n\n";
		
		// Debug settings  
		configFile << "[Debug]\n";
		configFile << "enableStatistics=" << (_debug.enableStatistics ? "true" : "false") << "\n";
		configFile << "enableBandwidthMonitoring=" << (_debug.enableBandwidthMonitoring ? "true" : "false") << "\n";
		configFile << "enableConnectionHealth=" << (_debug.enableConnectionHealth ? "true" : "false") << "\n";
		configFile << "logMessages=" << (_debug.logMessages ? "true" : "false") << "\n";
		configFile << "logTraces=" << (_debug.logTraces ? "true" : "false") << "\n";
		configFile << "verboseLogging=" << (_debug.verboseLogging ? "true" : "false") << "\n";
		if (!_debug.logFilePath.empty()) {
			configFile << "logFilePath=" << _debug.logFilePath << "\n";
		}
		configFile << "\n";
		
		// Filter settings
		configFile << "[Filter]\n";
		configFile << "enableCpuTraces=" << (_filter.enableCpuTraces ? "true" : "false") << "\n";
		configFile << "enableMemoryAccess=" << (_filter.enableMemoryAccess ? "true" : "false") << "\n";
		configFile << "enableCdlUpdates=" << (_filter.enableCdlUpdates ? "true" : "false") << "\n";
		configFile << "enableBreakpoints=" << (_filter.enableBreakpoints ? "true" : "false") << "\n";
		configFile << "enableLabels=" << (_filter.enableLabels ? "true" : "false") << "\n";
		configFile << "minAddress=0x" << std::hex << _filter.minAddress << "\n";
		configFile << "maxAddress=0x" << std::hex << _filter.maxAddress << "\n";
		configFile << std::dec;  // Reset to decimal
		configFile << "\n";
		
		// UI settings
		configFile << "[UI]\n";
		configFile << "enableStatusWindow=" << (_ui.enableStatusWindow ? "true" : "false") << "\n";
		configFile << "showBandwidthGraph=" << (_ui.showBandwidthGraph ? "true" : "false") << "\n";
		configFile << "showConnectionStatus=" << (_ui.showConnectionStatus ? "true" : "false") << "\n";
		configFile << "showStatistics=" << (_ui.showStatistics ? "true" : "false") << "\n";
		configFile << "statusUpdateIntervalMs=" << _ui.statusUpdateIntervalMs << "\n";
		configFile << "minimizeToTray=" << (_ui.minimizeToTray ? "true" : "false") << "\n";
		configFile << "showNotifications=" << (_ui.showNotifications ? "true" : "false") << "\n";
		
		_configFilePath = filePath;
		std::cout << "DiztinguishConfig: Saved configuration to " << filePath << std::endl;
		return true;
		
	} catch (const std::exception& e) {
		std::cout << "DiztinguishConfig: Error saving config file: " << e.what() << std::endl;
		return false;
	}
}

void DiztinguishConfig::ApplyPerformanceProfile(const std::string& profile)
{
	if (profile == "fast") {
		// Optimize for speed over quality
		_performance.socketBufferSize = 128 * 1024;  // 128KB
		_performance.maxTraceBufferSize = 2048;      // Larger buffer
		_performance.flushIntervalMs = 8;            // ~120 FPS
		_performance.enableBatching = true;
		_performance.maxBatchSize = 200;
		_performance.adaptiveBandwidth = true;
		
	} else if (profile == "balanced") {
		// Default balanced settings
		_performance.socketBufferSize = 64 * 1024;   // 64KB
		_performance.maxTraceBufferSize = 1024;      // Standard buffer
		_performance.flushIntervalMs = 16;           // ~60 FPS
		_performance.enableBatching = true;
		_performance.maxBatchSize = 100;
		_performance.adaptiveBandwidth = true;
		
	} else if (profile == "quality") {
		// Optimize for accuracy over speed
		_performance.socketBufferSize = 32 * 1024;   // 32KB
		_performance.maxTraceBufferSize = 512;       // Smaller buffer
		_performance.flushIntervalMs = 32;           // ~30 FPS
		_performance.enableBatching = false;         // Send immediately
		_performance.maxBatchSize = 50;
		_performance.adaptiveBandwidth = false;      // Consistent timing
	}
}

void DiztinguishConfig::EnableDebugMode(bool enable)
{
	_debug.enableStatistics = enable;
	_debug.enableBandwidthMonitoring = enable;
	_debug.enableConnectionHealth = enable;
	_debug.logMessages = enable;
	_debug.verboseLogging = enable;
	
	if (enable && _debug.logFilePath.empty()) {
		_debug.logFilePath = "diztinguish_debug.log";
	}
}

void DiztinguishConfig::OptimizeForBandwidth(uint32_t maxBandwidthKBps)
{
	// Adjust settings based on available bandwidth
	if (maxBandwidthKBps < 100) {  // Very limited bandwidth
		_performance.socketBufferSize = 16 * 1024;  // 16KB
		_performance.flushIntervalMs = 50;          // 20 FPS
		_performance.enableCompression = true;      // Enable compression
		_performance.maxBatchSize = 500;           // Large batches
		
	} else if (maxBandwidthKBps < 1000) {  // Limited bandwidth
		_performance.socketBufferSize = 32 * 1024;  // 32KB
		_performance.flushIntervalMs = 33;          // 30 FPS
		_performance.enableCompression = false;     // Optional compression
		_performance.maxBatchSize = 200;
		
	} else {  // High bandwidth
		_performance.socketBufferSize = 128 * 1024; // 128KB
		_performance.flushIntervalMs = 16;          // 60 FPS
		_performance.enableCompression = false;     // No compression needed
		_performance.maxBatchSize = 100;
	}
}

bool DiztinguishConfig::ValidateConfig() const
{
	// Validate port ranges
	if (_connection.defaultPort < 1024 || _connection.defaultPort > 65535) {
		return false;
	}
	
	// Validate buffer sizes
	if (_performance.socketBufferSize < 1024 || _performance.socketBufferSize > 1024 * 1024) {
		return false;
	}
	
	// Validate intervals
	if (_performance.flushIntervalMs < 1 || _performance.flushIntervalMs > 10000) {
		return false;
	}
	
	// Validate address ranges
	if (_filter.minAddress > _filter.maxAddress) {
		return false;
	}
	
	return true;
}

std::vector<std::string> DiztinguishConfig::GetConfigErrors() const
{
	std::vector<std::string> errors;
	
	if (_connection.defaultPort < 1024 || _connection.defaultPort > 65535) {
		errors.push_back("Invalid default port: must be between 1024 and 65535");
	}
	
	if (_performance.socketBufferSize < 1024) {
		errors.push_back("Socket buffer size too small: minimum 1KB");
	}
	
	if (_performance.socketBufferSize > 1024 * 1024) {
		errors.push_back("Socket buffer size too large: maximum 1MB");
	}
	
	if (_performance.flushIntervalMs < 1) {
		errors.push_back("Flush interval too small: minimum 1ms");
	}
	
	if (_filter.minAddress > _filter.maxAddress) {
		errors.push_back("Invalid address range: minimum address greater than maximum");
	}
	
	return errors;
}