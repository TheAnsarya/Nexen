#include "pch.h"
#include "DiztinguishBinaryBridge.h"
#include "../SNES/SnesConsole.h"
#include "../SNES/Debugger/SnesDebugger.h"
#include "Debugger.h"
#include "../../Utilities/Socket.h"

DiztinguishBinaryBridge::DiztinguishBinaryBridge(SnesConsole* console, SnesDebugger* snesDebugger, Debugger* debugger)
    : _console(console)
    , _snesDebugger(snesDebugger)
    , _debugger(debugger)
    , _port(0)
    , _serverRunning(false)
    , _clientConnected(false)
    , _packetsStreamed(0)
    , _bytesStreamed(0)
    , _connectionStartTime(0)
{
    _debugger->Log("[DiztinGUIsh Binary] Bridge initialized");
}

DiztinguishBinaryBridge::~DiztinguishBinaryBridge()
{
    StopServer();
}

bool DiztinguishBinaryBridge::StartServer(uint16_t port)
{
    if (_serverRunning) {
        return false;  // Already running
    }

    _port = port;
    _serverSocket = std::make_unique<Socket>();

    try {
        _serverSocket->Bind(_port);
        _serverSocket->Listen(1);  // Only accept 1 client
        _serverRunning = true;

        // Start server thread to accept connections
        _serverThread = std::make_unique<std::thread>(&DiztinguishBinaryBridge::ServerThreadMain, this);

        _debugger->Log("[DiztinGUIsh Binary] Server started on port " + std::to_string(_port));
        return true;
    }
    catch (const std::exception& e) {
        _debugger->Log("[DiztinGUIsh Binary] Failed to start server: " + std::string(e.what()));
        _serverSocket.reset();
        _serverRunning = false;
        return false;
    }
}

void DiztinguishBinaryBridge::StopServer()
{
    if (!_serverRunning) {
        return;
    }

    _serverRunning = false;
    _clientConnected = false;

    // Close sockets
    {
        std::lock_guard<std::mutex> lock(_socketMutex);
        if (_clientSocket) {
            _clientSocket->Close();
            _clientSocket.reset();
        }
        if (_serverSocket) {
            _serverSocket->Close();
            _serverSocket.reset();
        }
    }

    // Join thread
    if (_serverThread && _serverThread->joinable()) {
        _serverThread->join();
    }

    _debugger->Log("[DiztinGUIsh Binary] Server stopped");
}

void DiztinguishBinaryBridge::ServerThreadMain()
{
    _debugger->Log("[DiztinGUIsh Binary] Waiting for client connection...");

    while (_serverRunning) {
        try {
            // Accept incoming connection (blocking)
            auto clientSocket = _serverSocket->Accept();

            if (clientSocket && !clientSocket->ConnectionError()) {
                {
                    std::lock_guard<std::mutex> lock(_socketMutex);
                    _clientSocket = std::move(clientSocket);
                    _clientConnected = true;
                    _connectionStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count();
                }

                _debugger->Log("[DiztinGUIsh Binary] Client connected! Ready for binary streaming.");

                // Simple loop - just wait for disconnection
                while (_clientConnected && _serverRunning) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    
                    // Check if socket is still connected
                    {
                        std::lock_guard<std::mutex> lock(_socketMutex);
                        if (_clientSocket && _clientSocket->ConnectionError()) {
                            _clientConnected = false;
                            _debugger->Log("[DiztinGUIsh Binary] Client disconnected (socket error)");
                        }
                    }
                }

                // Cleanup
                {
                    std::lock_guard<std::mutex> lock(_socketMutex);
                    if (_clientSocket) {
                        _clientSocket->Close();
                        _clientSocket.reset();
                    }
                    _clientConnected = false;
                }

                _debugger->Log("[DiztinGUIsh Binary] Client session ended");
            }
        }
        catch (const std::exception& e) {
            if (_serverRunning) {
                _debugger->Log("[DiztinGUIsh Binary] Error in server thread: " + std::string(e.what()));
            }
        }
    }
}

bool DiztinguishBinaryBridge::SendBinaryPacket(const BsnesBinaryPacket& packet)
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    
    if (!_clientConnected || !_clientSocket) {
        return false;
    }

    try {
        // Send raw 22-byte packet directly (no headers, no protocols)
        _clientSocket->BufferedSend((char*)&packet, sizeof(packet));
        _clientSocket->SendBuffer();  // Flush immediately for low latency
        
        _packetsStreamed++;
        _bytesStreamed += sizeof(packet);
        return true;
    }
    catch (const std::exception& e) {
        _debugger->Log("[DiztinGUIsh Binary] Error sending packet: " + std::string(e.what()));
        _clientConnected = false;
        return false;
    }
}

uint64_t DiztinguishBinaryBridge::GetConnectionDuration() const
{
    if (!_clientConnected || _connectionStartTime == 0) {
        return 0;
    }
    
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return now - _connectionStartTime;
}

void DiztinguishBinaryBridge::OnCpuExec(uint32_t pc, uint8_t opcode, uint8_t opcodeLen,
                                        const uint8_t* operands, bool mFlag, bool xFlag,
                                        uint8_t db, uint16_t dp, uint16_t a, uint16_t x,
                                        uint16_t y, uint16_t s, bool emulationMode)
{
    if (!_clientConnected) {
        return;
    }

    // Create BSNES-compatible 22-byte binary packet
    BsnesBinaryPacket packet = {};

    // PC (24-bit address, 3 bytes)
    packet.pc_low = pc & 0xFF;
    packet.pc_mid = (pc >> 8) & 0xFF;
    packet.pc_high = (pc >> 16) & 0xFF;

    // Opcode length (1 byte)
    packet.opcode_len = opcodeLen;

    // Direct Page register (2 bytes, little-endian)
    packet.dp_low = dp & 0xFF;
    packet.dp_high = (dp >> 8) & 0xFF;

    // Data Bank register (1 byte)
    packet.db = db;

    // CPU flags (1 byte) - in BSNES format
    // We only need M and X flags for DiztinGUIsh
    packet.flags = 0;
    if (mFlag) packet.flags |= 0x20;  // M flag (bit 5)
    if (xFlag) packet.flags |= 0x10;  // X flag (bit 4)

    // Opcode and operands (4 bytes total)
    packet.opcode = opcode;
    packet.operand1 = (opcodeLen > 1 && operands) ? operands[0] : 0;
    packet.operand2 = (opcodeLen > 2 && operands) ? operands[1] : 0;
    packet.operand3 = (opcodeLen > 3 && operands) ? operands[2] : 0;

    // CPU registers (all 16-bit, little-endian)
    packet.a_low = a & 0xFF;
    packet.a_high = (a >> 8) & 0xFF;
    packet.x_low = x & 0xFF;
    packet.x_high = (x >> 8) & 0xFF;
    packet.y_low = y & 0xFF;
    packet.y_high = (y >> 8) & 0xFF;
    packet.s_low = s & 0xFF;
    packet.s_high = (s >> 8) & 0xFF;

    // Emulation mode flag
    packet.e_flag = emulationMode ? 1 : 0;
    packet.reserved = 0;

    // Send packet directly to client
    SendBinaryPacket(packet);
}