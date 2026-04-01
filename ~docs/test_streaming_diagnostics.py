#!/usr/bin/env python3
"""
DiztinGUIsh Streaming Diagnostics Test

Tests the current state of the DiztinGUIsh server and streaming functionality.
This script provides detailed diagnostics about why streaming might not be working.
"""

import socket
import struct
import time
import sys
from enum import IntEnum

class MessageType(IntEnum):
    HANDSHAKE = 0x01
    HANDSHAKE_ACK = 0x02
    CONFIG_STREAM = 0x03
    EXEC_TRACE_BATCH = 0x04
    CDL_UPDATE_BATCH = 0x05
    CPU_STATE = 0x06
    MEMORY_DUMP = 0x07
    LABEL_ADD = 0x08
    LABEL_UPDATE = 0x09
    LABEL_DELETE = 0x0A
    BREAKPOINT_ADD = 0x0B
    BREAKPOINT_DELETE = 0x0C
    ERROR = 0xFF

def test_connection_and_diagnostics(host="localhost", port=9998):
    """Test connection and provide detailed diagnostics"""
    
    print("🔍 DiztinGUIsh Streaming Diagnostics")
    print("=" * 50)
    
    sock = None
    try:
        # Test basic TCP connection
        print(f"📡 Testing connection to {host}:{port}...")
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        
        start_time = time.time()
        result = sock.connect_ex((host, port))
        connect_time = time.time() - start_time
        
        if result != 0:
            print(f"❌ CONNECTION FAILED: Cannot connect to {host}:{port}")
            print(f"   Error code: {result}")
            print(f"   Make sure:")
            print(f"   1. Mesen2 is running")
            print(f"   2. DiztinGUIsh server is started (Tools -> DiztinGUIsh Server)")
            print(f"   3. Server is listening on port {port}")
            return False
            
        print(f"✅ TCP connection successful ({connect_time:.3f}s)")
        
        # Test handshake
        print("🤝 Testing handshake...")
        
        # Send handshake
        # Protocol format: [MessageType(1)][Length(4)][HandshakeMessage struct]
        # HandshakeMessage: protocolVersionMajor(2) + protocolVersionMinor(2) + romChecksum(4) + romSize(4) + romName(256)
        handshake_payload = struct.pack('<HHII256s', 
                                       1,    # protocolVersionMajor
                                       0,    # protocolVersionMinor  
                                       0,    # romChecksum
                                       0,    # romSize
                                       b"DiagnosticClient Test ROM\x00")  # romName (256 bytes)
        
        # Send message with proper protocol format: MessageType + Length + Payload
        header = struct.pack('<BI', MessageType.HANDSHAKE, len(handshake_payload))
        sock.send(header + handshake_payload)
        message_length = len(header) + len(handshake_payload)
        print(f"   📤 Sent handshake ({message_length} bytes)")
        
        # Wait for handshake response
        sock.settimeout(3.0)
        try:
            # Read message header: MessageType(1) + Length(4)
            header_data = sock.recv(5)
            if len(header_data) != 5:
                print(f"❌ HANDSHAKE FAILED: Invalid header (got {len(header_data)} bytes)")
                return False
                
            response_type, message_size = struct.unpack('<BI', header_data)
            print(f"   📥 Expecting handshake response (type: 0x{response_type:02X}, size: {message_size} bytes)")
            
            if message_size > 1024:  # Sanity check
                print(f"❌ HANDSHAKE FAILED: Message size too large ({message_size})")
                return False
            
            # Receive handshake response
            response_data = b""
            while len(response_data) < message_size:
                chunk = sock.recv(message_size - len(response_data))
                if not chunk:
                    break
                response_data += chunk
            
            if len(response_data) != message_size:
                print(f"❌ HANDSHAKE FAILED: Incomplete response (got {len(response_data)}/{message_size} bytes)")
                return False
            
            # Parse handshake response - verify it's a HANDSHAKE message
            if response_type == MessageType.HANDSHAKE:
                # HandshakeMessage: protocolVersionMajor(2) + protocolVersionMinor(2) + romChecksum(4) + romSize(4) + romName(256)
                if len(response_data) >= 268:  # Minimum size for HandshakeMessage
                    proto_major, proto_minor, rom_checksum, rom_size = struct.unpack('<HHII', response_data[:12])
                    rom_name = response_data[12:268].rstrip(b'\x00').decode('utf-8', errors='ignore')
                    
                    print(f"✅ Received handshake from server")
                    print(f"   Protocol: {proto_major}.{proto_minor}")
                    print(f"   ROM: {rom_name}")
                    print(f"   Checksum: 0x{rom_checksum:08X}")
                    print(f"   Size: {rom_size} bytes")
                    
                    # Send handshake acknowledgment
                    # HandshakeAckMessage: protocolVersionMajor(2) + protocolVersionMinor(2) + accepted(1) + clientName(64)
                    ack_payload = struct.pack('<HHB64s', 
                                            1,  # protocolVersionMajor
                                            0,  # protocolVersionMinor
                                            1,  # accepted
                                            b"DiagnosticClient v1.0\x00")  # clientName
                    ack_header = struct.pack('<BI', MessageType.HANDSHAKE_ACK, len(ack_payload))
                    sock.send(ack_header + ack_payload)
                    print(f"   📤 Sent handshake ACK")
                
                # Send configuration
                print("⚙️ Sending streaming configuration...")
                # ConfigStreamMessage: enableExecTrace(1) + enableMemoryAccess(1) + enableCdlUpdates(1) + traceFrameInterval(1) + maxTracesPerFrame(2)
                config_payload = struct.pack('<BBBBH', 
                                           1,  # enableExecTrace
                                           0,  # enableMemoryAccess (disabled for testing)
                                           1,  # enableCdlUpdates  
                                           1,  # traceFrameInterval (every frame for testing)
                                           2000)  # maxTracesPerFrame
                
                config_header = struct.pack('<BI', MessageType.CONFIG_STREAM, len(config_payload))
                sock.send(config_header + config_payload)
                print(f"   📤 Sent config (exec trace: ON, CDL: ON, interval: 1 frame, max: 2000)")
                
                # Wait for streaming data
                print("⏳ Waiting for streaming data...")
                sock.settimeout(10.0)
                
                messages_received = 0
                start_wait = time.time()
                
                while time.time() - start_wait < 10.0:
                    try:
                        # Read message header: MessageType(1) + Length(4)
                        header_data = sock.recv(5)
                        if len(header_data) != 5:
                            continue
                            
                        msg_type, message_size = struct.unpack('<BI', header_data)
                        if message_size > 65536:  # Sanity check
                            print(f"⚠️ Suspicious message size: {message_size}")
                            continue
                        
                        message_data = b""
                        while len(message_data) < message_size:
                            chunk = sock.recv(message_size - len(message_data))
                            if not chunk:
                                break
                            message_data += chunk
                        
                        if len(message_data) == message_size:
                            messages_received += 1
                            
                            if msg_type == MessageType.EXEC_TRACE_BATCH:
                                # ExecTraceBatchMessage: frameNumber(4) + entryCount(2) + entries...
                                if len(message_data) >= 6:
                                    frame_num, entry_count = struct.unpack('<IH', message_data[:6])
                                    print(f"✅ Received EXEC_TRACE batch: Frame {frame_num}, {entry_count} entries")
                                else:
                                    print(f"✅ Received EXEC_TRACE batch: {message_size} bytes")
                                    
                            elif msg_type == MessageType.CDL_UPDATE_BATCH:
                                print(f"✅ Received CDL_UPDATE batch: {message_size} bytes")
                                
                            else:
                                print(f"✅ Received message type: 0x{msg_type:02X} ({message_size} bytes)")
                    
                    except socket.timeout:
                        break
                    except Exception as e:
                        print(f"⚠️ Error receiving data: {e}")
                        break
                
                if messages_received > 0:
                    print(f"🎉 SUCCESS: Received {messages_received} streaming messages!")
                    print(f"   DiztinGUIsh server is working correctly.")
                    return True
                else:
                    print(f"❌ NO STREAMING DATA: Server connected but no messages received")
                    print(f"   Possible causes:")
                    print(f"   1. Emulation is PAUSED - Press F5 in Mesen2 to start emulation")
                    print(f"   2. No SNES ROM loaded")
                    print(f"   3. DiztinGUIsh bridge not properly initialized")
                    print(f"   4. Frame interval too high (try running emulation longer)")
                    return False
            else:
                print(f"❌ HANDSHAKE FAILED: Invalid handshake response size ({len(response_data)} bytes, expected >= 268)")
                return False
        else:
            print(f"❌ HANDSHAKE FAILED: Expected HANDSHAKE (0x{MessageType.HANDSHAKE:02X}), got 0x{response_type:02X}")
            return False
                
    except socket.timeout:
        print(f"❌ HANDSHAKE TIMEOUT: Server not responding")
        print(f"   Server accepts connections but doesn't send handshake")
        print(f"   This indicates the DiztinGUIsh bridge is not properly initialized")
        return False
            
    except Exception as e:
        print(f"❌ TEST FAILED: {e}")
        return False
        
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass

def main():
    """Main diagnostic test"""
    
    if len(sys.argv) > 1:
        if sys.argv[1] in ['-h', '--help']:
            print("DiztinGUIsh Streaming Diagnostics")
            print("Usage: python test_streaming_diagnostics.py [host] [port]")
            print("       python test_streaming_diagnostics.py localhost 9998")
            return
    
    host = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 9998
    
    success = test_connection_and_diagnostics(host, port)
    
    print()
    if success:
        print("🎯 DIAGNOSIS: DiztinGUIsh streaming is working correctly!")
        print("   You can now use DiztinGUIsh client to connect.")
    else:
        print("🔧 DIAGNOSIS: DiztinGUIsh streaming needs attention.")
        print("   Check the issues listed above and try again.")
        print()
        print("💡 Quick fixes to try:")
        print("   1. Make sure Mesen2 emulation is RUNNING (not paused)")
        print("   2. Load a SNES ROM file in Mesen2") 
        print("   3. Restart the DiztinGUIsh server in Tools menu")
        print("   4. Check that port 9998 is not blocked by firewall")

if __name__ == "__main__":
    main()