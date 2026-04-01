#!/usr/bin/env python3
"""
DiztinGUIsh Streaming Diagnostics - Correct Protocol Flow

Tests following the proper protocol flow:
1. Client connects
2. Server sends HANDSHAKE
3. Client sends HANDSHAKE_ACK  
4. Client sends CONFIG_STREAM
5. Server streams data
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

def test_correct_protocol_flow(host="localhost", port=9998):
    """Test following the correct protocol flow"""
    
    print("🔍 DiztinGUIsh Protocol Test (Correct Flow)")
    print("=" * 55)
    
    sock = None
    try:
        # Step 1: Connect to server
        print(f"📡 Connecting to {host}:{port}...")
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        
        start_time = time.time()
        result = sock.connect_ex((host, port))
        connect_time = time.time() - start_time
        
        if result != 0:
            print(f"❌ CONNECTION FAILED: {result}")
            return False
            
        print(f"✅ Connected successfully ({connect_time:.3f}s)")
        
        # Step 2: Wait for server to send HANDSHAKE
        print("🤝 Waiting for server handshake...")
        sock.settimeout(3.0)
        
        try:
            # Read message header (try 5-byte first, then 8-byte if that fails)
            header_data = sock.recv(5)
            print(f"   📥 Received header: {header_data.hex()} ({len(header_data)} bytes)")
            
            if len(header_data) == 0:
                print("❌ Server closed connection immediately")
                return False
                
            elif len(header_data) == 5:
                # Try parsing as 5-byte header
                msg_type, msg_length = struct.unpack('<BI', header_data)
                print(f"   Parsed: type=0x{msg_type:02X}, length={msg_length}")
                
            elif len(header_data) < 5:
                # Try to read more bytes
                remaining = sock.recv(8 - len(header_data))
                header_data += remaining
                print(f"   Full header: {header_data.hex()} ({len(header_data)} bytes)")
                
                if len(header_data) == 8:
                    # Parse as 8-byte header
                    msg_type, msg_length = struct.unpack('<II', header_data)
                    print(f"   Parsed (8-byte): type=0x{msg_type:08X}, length={msg_length}")
                else:
                    print(f"❌ Invalid header length: {len(header_data)}")
                    return False
            else:
                print(f"❌ Unexpected header length: {len(header_data)}")
                return False
            
            # Validate message type and length
            if msg_type != MessageType.HANDSHAKE:
                print(f"❌ Expected HANDSHAKE (0x{MessageType.HANDSHAKE:02X}), got 0x{msg_type:02X}")
                return False
                
            if msg_length > 1024:
                print(f"❌ Message too large: {msg_length}")
                return False
                
            # Read handshake payload
            print(f"   📥 Reading handshake payload ({msg_length} bytes)...")
            payload = b""
            while len(payload) < msg_length:
                chunk = sock.recv(msg_length - len(payload))
                if not chunk:
                    break
                payload += chunk
                
            if len(payload) != msg_length:
                print(f"❌ Incomplete payload: {len(payload)}/{msg_length}")
                return False
                
            print(f"✅ Received handshake payload ({len(payload)} bytes)")
            
            # Parse handshake (protocolVersionMajor(2) + protocolVersionMinor(2) + romChecksum(4) + romSize(4) + romName(256))
            if len(payload) >= 12:
                proto_major, proto_minor, rom_checksum, rom_size = struct.unpack('<HHII', payload[:12])
                rom_name = ""
                if len(payload) >= 268:
                    rom_name = payload[12:268].rstrip(b'\x00').decode('utf-8', errors='ignore')
                    
                print(f"   Protocol: {proto_major}.{proto_minor}")
                print(f"   ROM: '{rom_name}'")
                print(f"   Checksum: 0x{rom_checksum:08X}")
                print(f"   Size: {rom_size}")
            
            # Step 3: Send HANDSHAKE_ACK
            print("📤 Sending handshake acknowledgment...")
            ack_payload = struct.pack('<HHB64s', 
                                    1,  # protocolVersionMajor
                                    0,  # protocolVersionMinor
                                    1,  # accepted
                                    b"DiagnosticClient v1.0\x00")
                                    
            # Use the same header format as server
            if len(header_data) == 5:
                ack_header = struct.pack('<BI', MessageType.HANDSHAKE_ACK, len(ack_payload))
            else:
                ack_header = struct.pack('<II', MessageType.HANDSHAKE_ACK, len(ack_payload))
                
            sock.send(ack_header + ack_payload)
            print(f"   ✅ Sent ACK ({len(ack_header) + len(ack_payload)} bytes)")
            
            # Step 4: Send CONFIG_STREAM  
            print("⚙️ Sending streaming configuration...")
            config_payload = struct.pack('<BBBBH', 
                                       1,  # enableExecTrace
                                       0,  # enableMemoryAccess
                                       1,  # enableCdlUpdates  
                                       1,  # traceFrameInterval (every frame)
                                       2000)  # maxTracesPerFrame
                                       
            if len(header_data) == 5:
                config_header = struct.pack('<BI', MessageType.CONFIG_STREAM, len(config_payload))
            else:
                config_header = struct.pack('<II', MessageType.CONFIG_STREAM, len(config_payload))
                
            sock.send(config_header + config_payload)
            print(f"   ✅ Sent config ({len(config_header) + len(config_payload)} bytes)")
            
            # Step 5: Wait for streaming data
            print("⏳ Waiting for streaming data...")
            sock.settimeout(10.0)
            
            messages_received = 0
            start_wait = time.time()
            header_size = 5 if len(header_data) == 5 else 8
            
            while time.time() - start_wait < 10.0:
                try:
                    # Read message header
                    header_bytes = sock.recv(header_size)
                    if len(header_bytes) != header_size:
                        continue
                        
                    if header_size == 5:
                        msg_type, msg_length = struct.unpack('<BI', header_bytes)
                    else:
                        msg_type, msg_length = struct.unpack('<II', header_bytes)
                        
                    if msg_length > 65536:
                        print(f"⚠️ Suspicious message size: {msg_length}")
                        continue
                    
                    # Read message payload
                    payload = b""
                    while len(payload) < msg_length:
                        chunk = sock.recv(msg_length - len(payload))
                        if not chunk:
                            break
                        payload += chunk
                    
                    if len(payload) == msg_length:
                        messages_received += 1
                        
                        if msg_type == MessageType.EXEC_TRACE_BATCH:
                            if len(payload) >= 6:
                                frame_num, entry_count = struct.unpack('<IH', payload[:6])
                                print(f"✅ EXEC_TRACE: Frame {frame_num}, {entry_count} entries")
                            else:
                                print(f"✅ EXEC_TRACE: {msg_length} bytes")
                                
                        elif msg_type == MessageType.CDL_UPDATE_BATCH:
                            print(f"✅ CDL_UPDATE: {msg_length} bytes")
                            
                        else:
                            print(f"✅ Message 0x{msg_type:02X}: {msg_length} bytes")
                
                except socket.timeout:
                    break
                except Exception as e:
                    print(f"⚠️ Error receiving: {e}")
                    break
            
            if messages_received > 0:
                print(f"🎉 SUCCESS: Received {messages_received} messages!")
                print("   DiztinGUIsh streaming is working!")
                return True
            else:
                print(f"❌ NO DATA: Handshake worked but no streaming data")
                print("   Possible causes:")
                print("   1. Emulation PAUSED - Press F5 in Mesen2")
                print("   2. No ROM loaded")  
                print("   3. DiztinGUIsh hooks not called during emulation")
                return False
                
        except socket.timeout:
            print("❌ TIMEOUT: Server didn't send handshake")
            print("   Server accepts connections but doesn't initiate handshake")
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

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 9998
    
    success = test_correct_protocol_flow(host, port)
    
    print("\n" + "=" * 55)
    if success:
        print("🎯 RESULT: DiztinGUIsh streaming is working!")
    else:
        print("🔧 RESULT: DiztinGUIsh needs troubleshooting.")
        print("\n💡 Next steps:")
        print("   1. Check Mesen2 is running emulation (not paused)")
        print("   2. Verify SNES ROM is loaded")
        print("   3. Restart DiztinGUIsh server")
        print("   4. Check for build/compilation issues")

if __name__ == "__main__":
    main()