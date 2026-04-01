#pragma once
#include "pch.h"
#include "../../Utilities/Socket.h"
#include <mutex>

// DiztinGUIsh Binary Bridge - BSNES-compatible streaming
// Outputs simple binary packets compatible with DiztinGUIsh's BsnesTraceLogImporter
// Replaces the complex TCP protocol with direct binary format matching BSNES exactly

class SnesConsole;
class SnesDebugger;
class Debugger;

class DiztinguishBinaryBridge
{
private:
    SnesConsole* _console;
    SnesDebugger* _snesDebugger;
    Debugger* _debugger;

    // Binary stream output
    std::unique_ptr<Socket> _serverSocket;
    std::unique_ptr<Socket> _clientSocket;
    std::unique_ptr<std::thread> _serverThread;
    
    uint16_t _port;
    bool _serverRunning;
    bool _clientConnected;
    
    // Statistics
    uint64_t _packetsStreamed;
    uint64_t _bytesStreamed;
    uint64_t _connectionStartTime;
    
    // Thread synchronization
    mutable std::mutex _socketMutex;
    
    // BSNES binary packet format (22 bytes)
    struct BsnesBinaryPacket {
        uint8_t pc_low;          // PC bits 0-7
        uint8_t pc_mid;          // PC bits 8-15  
        uint8_t pc_high;         // PC bits 16-23
        uint8_t opcode_len;      // Length of opcode (1-4 bytes)
        uint8_t dp_low;          // Direct Page low byte
        uint8_t dp_high;         // Direct Page high byte
        uint8_t db;              // Data Bank register
        uint8_t flags;           // CPU flags (NVMXDIZC format)
        uint8_t opcode;          // Opcode byte
        uint8_t operand1;        // First operand (or 0)
        uint8_t operand2;        // Second operand (or 0) 
        uint8_t operand3;        // Third operand (or 0)
        uint8_t a_low;           // A register low byte
        uint8_t a_high;          // A register high byte
        uint8_t x_low;           // X register low byte
        uint8_t x_high;          // X register high byte
        uint8_t y_low;           // Y register low byte
        uint8_t y_high;          // Y register high byte
        uint8_t s_low;           // Stack pointer low byte
        uint8_t s_high;          // Stack pointer high byte
        uint8_t e_flag;          // Emulation mode flag
        uint8_t reserved;        // Reserved for alignment
    };
    static_assert(sizeof(BsnesBinaryPacket) == 22, "BSNES packet must be exactly 22 bytes");
    
    // Internal methods
    void ServerThreadMain();
    bool SendBinaryPacket(const BsnesBinaryPacket& packet);

public:
    DiztinguishBinaryBridge(SnesConsole* console, SnesDebugger* snesDebugger, Debugger* debugger);
    ~DiztinguishBinaryBridge();

    // Server control
    bool StartServer(uint16_t port);
    void StopServer();
    
    // Status
    bool IsServerRunning() const { return _serverRunning; }
    bool IsClientConnected() const { return _clientConnected; }
    uint16_t GetPort() const { return _port; }
    
    // Statistics 
    uint64_t GetPacketsStreamed() const { return _packetsStreamed; }
    uint64_t GetBytesStreamed() const { return _bytesStreamed; }
    uint64_t GetConnectionDuration() const;
    
    // Called by emulator core - simple binary streaming
    void OnCpuExec(uint32_t pc, uint8_t opcode, uint8_t opcodeLen, 
                   const uint8_t* operands, bool mFlag, bool xFlag, 
                   uint8_t db, uint16_t dp, uint16_t a, uint16_t x, 
                   uint16_t y, uint16_t s, bool emulationMode);
    
    // Lua API
    uint16_t GetServerPort() const { return _port; }  // Alias for Lua clarity
};