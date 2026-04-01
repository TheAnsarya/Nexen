#!/usr/bin/env python3
"""
Clean DiztinGUIsh Protocol Test

Test the Mesen2 DiztinGUIsh streaming protocol to diagnose handshake issues.
"""

import socket
import struct
import time
from enum import IntEnum

class MessageType(IntEnum):
    HANDSHAKE = 0x01
    HANDSHAKE_ACK = 0x02
    CONFIG_STREAM = 0x03
    HEARTBEAT = 0x04
    DISCONNECT = 0x05
    EXEC_TRACE = 0x10
    EXEC_TRACE_BATCH = 0x11
    MEMORY_ACCESS = 0x12
    CDL_UPDATE = 0x13
    CDL_SNAPSHOT = 0x14
    CPU_STATE = 0x20
    CPU_STATE_REQUEST = 0x21
    ERROR = 0xFF

def test_mesen_protocol(host="localhost", port=9998):
    """Test Mesen DiztinGUIsh protocol"""
    print("🔍 Testing Mesen DiztinGUIsh Protocol")
    print("=" * 50)
    
    sock = None
    try:
        # Connect
        print(f"📡 Connecting to {host}:{port}...")
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        sock.connect((host, port))
        print("✅ Connected successfully")
        
        # Send handshake (we are the client)
        print("🤝 Sending handshake...")
        
        # HandshakeMessage structure from DiztinguishProtocol.h:
        # uint16_t protocolVersionMajor;
        # uint16_t protocolVersionMinor;
        # uint32_t romChecksum;
        # uint32_t romSize;
        # char romName[256];
        handshake_payload = struct.pack('<HHII256s',
                                       1,      # protocolVersionMajor
                                       0,      # protocolVersionMinor
                                       0,      # romChecksum (we don't know it)
                                       0,      # romSize
                                       b"Test Client\x00")  # romName
        
        # Message header: MessageType (1 byte) + length (4 bytes)
        header = struct.pack('<BI', MessageType.HANDSHAKE, len(handshake_payload))
        message = header + handshake_payload
        
        sock.send(message)
        print(f"   📤 Sent handshake ({len(message)} bytes)")
        
        # Wait for server's handshake response
        print("   📥 Waiting for server handshake...")
        sock.settimeout(3.0)
        
        # Read header
        header_data = sock.recv(5)
        if len(header_data) != 5:
            print(f"❌ Failed to read header (got {len(header_data)} bytes)")
            return False
        
        msg_type, msg_length = struct.unpack('<BI', header_data)
        print(f"   Received header: type=0x{msg_type:02X}, length={msg_length}")
        
        if msg_type != MessageType.HANDSHAKE:
            print(f"❌ Expected HANDSHAKE (0x{MessageType.HANDSHAKE:02X}), got 0x{msg_type:02X}")
            return False
        
        if msg_length > 1024:
            print(f"❌ Invalid message length: {msg_length}")
            return False
        
        # Read payload
        payload = b""
        while len(payload) < msg_length:
            chunk = sock.recv(msg_length - len(payload))
            if not chunk:
                break
            payload += chunk
        
        if len(payload) != msg_length:
            print(f"❌ Incomplete payload (got {len(payload)}/{msg_length} bytes)")
            return False
        
        # Parse server handshake
        if len(payload) >= 268:  # sizeof(HandshakeMessage)
            proto_major, proto_minor, rom_crc, rom_size = struct.unpack('<HHII', payload[:12])
            rom_name = payload[12:268].rstrip(b'\x00').decode('utf-8', errors='ignore')
            
            print(f"✅ Server handshake received:")
            print(f"   Protocol: v{proto_major}.{proto_minor}")
            print(f"   ROM: '{rom_name}'")
            print(f"   CRC: 0x{rom_crc:08X}")
            print(f"   Size: {rom_size} bytes")
        else:
            print(f"❌ Invalid handshake payload size: {len(payload)} bytes")
            return False
        
        # Send handshake acknowledgment
        print("📤 Sending handshake ACK...")
        
        # HandshakeAckMessage structure:
        # uint16_t protocolVersionMajor;
        # uint16_t protocolVersionMinor;  
        # uint8_t accepted;
        # char clientName[64];
        ack_payload = struct.pack('<HHB64s',
                                 1,    # protocolVersionMajor
                                 0,    # protocolVersionMinor
                                 1,    # accepted
                                 b"DiagnosticClient\x00")  # clientName
        
        ack_header = struct.pack('<BI', MessageType.HANDSHAKE_ACK, len(ack_payload))
        sock.send(ack_header + ack_payload)
        print(f"   📤 Sent ACK ({len(ack_header + ack_payload)} bytes)")
        
        # Send configuration
        print("⚙️ Sending stream configuration...")
        
        # ConfigStreamMessage structure:
        # uint8_t enableExecTrace;
        # uint8_t enableMemoryAccess;
        # uint8_t enableCdlUpdates;
        # uint8_t traceFrameInterval;
        # uint16_t maxTracesPerFrame;
        config_payload = struct.pack('<BBBBH',
                                   1,     # enableExecTrace
                                   0,     # enableMemoryAccess (high bandwidth)
                                   1,     # enableCdlUpdates
                                   1,     # traceFrameInterval (every frame)
                                   1000)  # maxTracesPerFrame
        
        config_header = struct.pack('<BI', MessageType.CONFIG_STREAM, len(config_payload))
        sock.send(config_header + config_payload)
        print(f"   📤 Sent config: ExecTrace=ON, MemAccess=OFF, CDL=ON, Interval=1, Max=1000")
        
        # Wait for streaming data
        print("⏳ Waiting for streaming data (10 seconds)...")
        sock.settimeout(10.0)
        
        messages_received = 0
        start_time = time.time()
        
        while time.time() - start_time < 10.0:
            try:
                # Read message header
                header_data = sock.recv(5)
                if len(header_data) != 5:
                    continue
                
                msg_type, msg_length = struct.unpack('<BI', header_data)
                
                # Sanity check
                if msg_length > 100000:
                    print(f"⚠️ Suspicious message length: {msg_length}")
                    continue
                
                # Read payload
                payload = b""
                while len(payload) < msg_length:
                    chunk = sock.recv(msg_length - len(payload))
                    if not chunk:
                        break
                    payload += chunk
                
                if len(payload) != msg_length:
                    continue
                
                messages_received += 1
                
                # Parse different message types
                if msg_type == MessageType.EXEC_TRACE_BATCH:
                    if len(payload) >= 6:
                        frame_num, entry_count = struct.unpack('<IH', payload[:6])
                        print(f"📊 EXEC_TRACE_BATCH: Frame {frame_num}, {entry_count} entries")
                    else:
                        print(f"📊 EXEC_TRACE_BATCH: {msg_length} bytes")
                
                elif msg_type == MessageType.CDL_UPDATE:
                    print(f"🏷️  CDL_UPDATE: {msg_length} bytes")
                
                elif msg_type == MessageType.CPU_STATE:
                    print(f"💻 CPU_STATE: {msg_length} bytes")
                
                elif msg_type == MessageType.HEARTBEAT:
                    print(f"💓 HEARTBEAT")
                
                elif msg_type == MessageType.ERROR:
                    if len(payload) >= 1:
                        error_code = payload[0]
                        error_msg = payload[1:].rstrip(b'\x00').decode('utf-8', errors='ignore')
                        print(f"❌ SERVER ERROR: Code {error_code}, Message: '{error_msg}'")
                    else:
                        print(f"❌ SERVER ERROR: {msg_length} bytes")
                
                else:
                    print(f"📦 Unknown message: type=0x{msg_type:02X}, length={msg_length}")
                
            except socket.timeout:
                break
            except Exception as e:
                print(f"⚠️ Error receiving message: {e}")
                break
        
        print(f"\n📈 Results:")
        print(f"   Messages received: {messages_received}")
        print(f"   Connection duration: {time.time() - start_time:.1f}s")
        
        if messages_received > 0:
            print("🎉 SUCCESS: Protocol is working!")
            return True
        else:
            print("❌ FAILURE: No streaming messages received")
            print("💡 Possible causes:")
            print("   - Emulation not running (press F5 to start)")
            print("   - No ROM loaded")
            print("   - DiztinGUIsh bridge not initialized")
            return False
    
    except ConnectionRefusedError:
        print("❌ CONNECTION REFUSED: Server not running")
        print("💡 Make sure Mesen2 DiztinGUIsh server is started")
        return False
    except socket.timeout:
        print("❌ TIMEOUT: Server not responding")
        return False
    except Exception as e:
        print(f"❌ ERROR: {e}")
        return False
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass

if __name__ == "__main__":
    import sys
    
    host = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 9998
    
    success = test_mesen_protocol(host, port)
    print(f"\n🏁 Test {'PASSED' if success else 'FAILED'}")