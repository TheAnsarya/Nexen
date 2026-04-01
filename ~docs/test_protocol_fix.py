#!/usr/bin/env python3
"""
Test updated DiztinGUIsh protocol with correct message types and structures.
Tests the fixed protocol implementation.
"""
import socket
import struct
import time

# Updated protocol constants (matching C++ DiztinguishProtocol.h)
class MessageType:
    Handshake = 0x01
    HandshakeAck = 0x02
    ConfigStream = 0x03
    Heartbeat = 0x04
    Disconnect = 0x05
    ExecTrace = 0x10
    ExecTraceBatch = 0x11
    MemoryAccess = 0x12
    CdlUpdate = 0x13
    CdlSnapshot = 0x14
    CpuState = 0x20
    CpuStateRequest = 0x21
    Error = 0xFF

def test_corrected_protocol():
    """Test the corrected DiztinGUIsh protocol"""
    print("=== CORRECTED PROTOCOL TEST ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(("localhost", 9998))
        print("✅ Connected successfully!")
        
        # Wait for handshake from server
        print("Waiting for handshake message...")
        data = sock.recv(1024)
        
        if len(data) >= 5:
            # Parse message header
            msg_type = data[0]
            msg_length = struct.unpack('<I', data[1:5])[0]
            print(f"Received message: Type=0x{msg_type:02X}, Length={msg_length}")
            
            if msg_type == MessageType.Handshake:
                if len(data) >= 5 + msg_length and msg_length >= 268:
                    # Parse handshake payload
                    payload = data[5:5+msg_length]
                    major = struct.unpack('<H', payload[0:2])[0]
                    minor = struct.unpack('<H', payload[2:4])[0]
                    checksum = struct.unpack('<I', payload[4:8])[0]
                    rom_size = struct.unpack('<I', payload[8:12])[0]
                    rom_name_bytes = payload[12:268]
                    rom_name = rom_name_bytes.rstrip(b'\x00').decode('ascii', errors='ignore')
                    
                    print(f"✅ CORRECT HANDSHAKE RECEIVED!")
                    print(f"   Protocol Version: {major}.{minor}")
                    print(f"   ROM Checksum: 0x{checksum:08X}")
                    print(f"   ROM Size: {rom_size} bytes")
                    print(f"   ROM Name: '{rom_name}'")
                    
                    # Send handshake ACK
                    ack_payload = struct.pack('<HHB', 1, 0, 1)  # major=1, minor=0, accepted=1
                    ack_payload += b"DiztinGUIsh Test Client" + b"\x00" * (64 - 23)  # 64-byte client name
                    ack_header = struct.pack('<BI', MessageType.HandshakeAck, len(ack_payload))
                    full_ack = ack_header + ack_payload
                    
                    sock.send(full_ack)
                    print("✅ Sent HandshakeAck")
                    
                    # Wait for any follow-up messages
                    print("Listening for trace data...")
                    sock.settimeout(3)
                    
                    try:
                        more_data = sock.recv(1024)
                        if more_data:
                            print(f"✅ Received {len(more_data)} bytes after handshake")
                            # Parse additional messages
                            offset = 0
                            while offset + 5 <= len(more_data):
                                msg_type = more_data[offset]
                                msg_len = struct.unpack('<I', more_data[offset+1:offset+5])[0]
                                print(f"   Message: Type=0x{msg_type:02X}, Length={msg_len}")
                                
                                if msg_type == MessageType.ExecTraceBatch and msg_len >= 6:
                                    # Parse ExecTraceBatch header
                                    if offset + 5 + msg_len <= len(more_data):
                                        batch_payload = more_data[offset+5:offset+5+msg_len]
                                        frame_num = struct.unpack('<I', batch_payload[0:4])[0]
                                        entry_count = struct.unpack('<H', batch_payload[4:6])[0]
                                        print(f"     ExecTraceBatch: Frame {frame_num}, {entry_count} entries")
                                
                                offset += 5 + msg_len
                                if offset + 5 > len(more_data):
                                    break
                        else:
                            print("No additional data received")
                    except socket.timeout:
                        print("No trace data received (normal if game not running)")
                    
                    print("✅ Protocol test SUCCESSFUL!")
                    return True
                else:
                    print(f"❌ Handshake payload too small: {len(data)} bytes (expected >= 273)")
            else:
                print(f"❌ Expected Handshake (0x01), got 0x{msg_type:02X}")
        
        sock.close()
        return False
        
    except ConnectionRefusedError:
        print("❌ Connection refused - DiztinGUIsh server not running")
        print("Start Mesen2 and run: emu.startDiztinguishServer(9998)")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

if __name__ == "__main__":
    print("DiztinGUIsh Protocol Fix Test")
    print("=============================")
    
    success = test_corrected_protocol()
    
    if success:
        print("\n🎉 PROTOCOL FIX SUCCESSFUL!")
        print("The handshake protocol is now working correctly.")
        print("DiztinGUIsh should now be able to connect properly.")
    else:
        print("\n❌ Protocol test failed.")
        print("Check that:")
        print("1. Mesen2 is running")  
        print("2. DiztinGUIsh server is started: emu.startDiztinguishServer(9998)")
        print("3. The updated C++ code is compiled and running")