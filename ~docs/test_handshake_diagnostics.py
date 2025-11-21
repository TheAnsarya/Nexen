#!/usr/bin/env python3
"""
DiztinGUIsh Protocol Handshake Diagnostics
Comprehensive testing of Mesen2 -> DiztinGUIsh protocol
"""
import socket
import struct
import time

# Protocol constants
PROTOCOL_VERSION_MAJOR = 1
PROTOCOL_VERSION_MINOR = 0

class MessageType:
    Handshake = 0
    HandshakeAck = 1
    ConfigStream = 2
    ExecTraceBatch = 3
    Error = 255

class ErrorCode:
    InvalidMessage = 1
    ProtocolMismatch = 2

def test_connection_only():
    """Test basic TCP connection without protocol"""
    print("=== BASIC CONNECTION TEST ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        result = sock.connect_ex(("localhost", 9998))
        if result == 0:
            print("✅ Connection successful!")
            sock.close()
            return True
        else:
            print(f"❌ Connection failed: {result}")
            print("   Make sure Mesen2 is running and DiztinGUIsh server is started")
            print("   In Mesen2 console, run: emu.startDiztinguishServer(9998)")
            return False
    except Exception as e:
        print(f"❌ Connection error: {e}")
        return False

def test_server_info():
    """Test what the server immediately sends on connection"""
    print("\n=== SERVER INITIAL MESSAGE TEST ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(3)
        sock.connect(("localhost", 9998))
        print("✅ Connected, waiting for server handshake...")
        
        # Wait for initial message from server
        data = sock.recv(1024)
        print(f"Server sent {len(data)} bytes immediately:")
        
        if len(data) >= 5:
            # Try to parse as message header
            msg_type = data[0]
            msg_length = struct.unpack('<I', data[1:5])[0]
            print(f"  Message Type: {msg_type}")
            print(f"  Message Length: {msg_length}")
            
            if len(data) >= 5 + msg_length:
                payload = data[5:5+msg_length]
                print(f"  Payload ({len(payload)} bytes): {payload.hex()}")
                
                # If this looks like a handshake
                if msg_type == MessageType.Handshake and len(payload) >= 270:
                    # Parse handshake: uint32 major, uint32 minor, uint32 checksum, uint32 size, char[256] name
                    major, minor, checksum, size = struct.unpack('<IIII', payload[:16])
                    name_bytes = payload[16:272]
                    name = name_bytes.rstrip(b'\x00').decode('ascii', errors='ignore')
                    
                    print(f"  ✅ HANDSHAKE MESSAGE:")
                    print(f"     Protocol Version: {major}.{minor}")
                    print(f"     ROM Checksum: 0x{checksum:08X}")
                    print(f"     ROM Size: {size}")
                    print(f"     ROM Name: '{name}'")
                    
                    # Send handshake acknowledgment
                    ack_msg = struct.pack('<BI', MessageType.HandshakeAck, 0)
                    sock.send(ack_msg)
                    print("  ✅ Sent HandshakeAck")
                    
                    return True
            
        print(f"Raw data: {data.hex()}")
        sock.close()
        return True
        
    except Exception as e:
        print(f"❌ Server info test error: {e}")
        return False

def test_client_initiated_handshake():
    """Test sending handshake first (client-initiated)"""
    print("\n=== CLIENT INITIATED HANDSHAKE TEST ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(("localhost", 9998))
        print("✅ Connected, sending client handshake...")
        
        # Create handshake message
        # HandshakeMessage: uint32 major, uint32 minor, uint32 checksum, uint32 size, char[256] name
        handshake_payload = struct.pack('<IIII', PROTOCOL_VERSION_MAJOR, PROTOCOL_VERSION_MINOR, 0, 0)
        handshake_payload += b"Test Client ROM\x00" + b"\x00" * (256 - 16)  # 256 byte name field
        
        # Message header: uint8 type, uint32 length
        header = struct.pack('<BI', MessageType.Handshake, len(handshake_payload))
        
        # Send complete message
        full_message = header + handshake_payload
        sock.send(full_message)
        print(f"✅ Sent handshake ({len(full_message)} bytes)")
        
        # Wait for response
        response = sock.recv(1024)
        print(f"✅ Received response ({len(response)} bytes)")
        
        if len(response) >= 5:
            resp_type = response[0] 
            resp_length = struct.unpack('<I', response[1:5])[0]
            print(f"  Response Type: {resp_type}")
            print(f"  Response Length: {resp_length}")
            
            if resp_type == MessageType.HandshakeAck:
                print("  ✅ Received HandshakeAck!")
            elif resp_type == MessageType.Error:
                if len(response) >= 6:
                    error_code = response[5]
                    print(f"  ❌ Received Error: {error_code}")
            else:
                print(f"  ? Unexpected response type: {resp_type}")
        
        sock.close()
        return True
        
    except Exception as e:
        print(f"❌ Client handshake test error: {e}")
        return False

def run_diagnostics():
    """Run all diagnostic tests"""
    print("DiztinGUIsh Protocol Handshake Diagnostics")
    print("==========================================")
    
    # Test 1: Basic connection
    if not test_connection_only():
        print("\n❌ Cannot connect to server. Stopping tests.")
        print("Instructions:")
        print("1. Start Mesen2")
        print("2. Open console (F9 or Debug > Console)")
        print("3. Type: emu.startDiztinguishServer(9998)")
        print("4. Re-run this test")
        return
    
    # Test 2: What server sends on connect
    test_server_info()
    
    # Small delay between tests
    time.sleep(1)
    
    # Test 3: Client-initiated handshake 
    test_client_initiated_handshake()
    
    print("\n=== SUMMARY ===")
    print("If tests show server immediately sends handshake:")
    print("  → Server-initiated protocol (Mesen sends first)")
    print("If tests show server waits for client:")
    print("  → Client-initiated protocol (DiztinGUIsh sends first)")
    print("Either way, we can now see the exact protocol flow!")

if __name__ == "__main__":
    run_diagnostics()