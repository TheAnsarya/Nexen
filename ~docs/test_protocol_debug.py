#!/usr/bin/env python3
"""
DiztinGUIsh Protocol Debugging Tool

Tests both 5-byte and 8-byte header formats to diagnose protocol issues.
"""

import socket
import struct
import time

def test_header_format(host="localhost", port=9998):
    """Test what header format the server is actually using"""
    
    print("🔍 Testing DiztinGUIsh Protocol Header Format")
    print("=" * 50)
    
    sock = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        
        result = sock.connect_ex((host, port))
        if result != 0:
            print(f"❌ Cannot connect to {host}:{port}")
            return False
            
        print(f"✅ Connected to {host}:{port}")
        
        # Send handshake with 5-byte header format
        handshake_payload = struct.pack('<HHII256s', 
                                       1, 0, 0, 0, b"Test ROM\x00")
        
        # Try 5-byte header: MessageType(1) + Length(4)  
        header_5byte = struct.pack('<BI', 0x01, len(handshake_payload))
        
        print(f"📤 Sending handshake with 5-byte header...")
        print(f"   Header: {header_5byte.hex()}")
        print(f"   Payload: {len(handshake_payload)} bytes")
        
        sock.send(header_5byte + handshake_payload)
        
        # Try to read response with different header formats
        sock.settimeout(2.0)
        
        # Try reading as 5-byte header first
        try:
            print("📥 Attempting to read 5-byte header...")
            header_data = sock.recv(5)
            print(f"   Received: {header_data.hex()} ({len(header_data)} bytes)")
            
            if len(header_data) == 5:
                msg_type, msg_length = struct.unpack('<BI', header_data)
                print(f"   Parsed as: type=0x{msg_type:02X}, length={msg_length}")
                
                if msg_length < 1000:  # Reasonable size
                    payload = sock.recv(msg_length)
                    print(f"   Payload: {len(payload)} bytes")
                    print(f"✅ 5-byte header format works!")
                    return True
                    
        except socket.timeout:
            print("⏱️ No response to 5-byte header format")
            
        except Exception as e:
            print(f"❌ Error with 5-byte header: {e}")
            
        # Reconnect and try 8-byte header
        sock.close()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        sock.connect((host, port))
        
        # Try 8-byte header: MessageType(4) + Length(4)
        print(f"📤 Sending handshake with 8-byte header...")
        header_8byte = struct.pack('<II', 0x01, len(handshake_payload))
        print(f"   Header: {header_8byte.hex()}")
        
        sock.send(header_8byte + handshake_payload)
        
        try:
            print("📥 Attempting to read 8-byte header...")
            header_data = sock.recv(8)
            print(f"   Received: {header_data.hex()} ({len(header_data)} bytes)")
            
            if len(header_data) == 8:
                msg_type, msg_length = struct.unpack('<II', header_data)
                print(f"   Parsed as: type=0x{msg_type:08X}, length={msg_length}")
                
                if msg_length < 1000:  # Reasonable size
                    payload = sock.recv(msg_length)
                    print(f"   Payload: {len(payload)} bytes")
                    print(f"✅ 8-byte header format works!")
                    return True
                    
        except socket.timeout:
            print("⏱️ No response to 8-byte header format")
            
        except Exception as e:
            print(f"❌ Error with 8-byte header: {e}")
        
        print("❌ Neither header format worked")
        return False
        
    except Exception as e:
        print(f"❌ Test failed: {e}")
        return False
        
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass

def test_raw_connection(host="localhost", port=9998):
    """Just connect and see what the server sends"""
    
    print("\n🔍 Raw Connection Test")
    print("=" * 30)
    
    sock = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        
        sock.connect((host, port))
        print(f"✅ Connected, waiting for server to send data...")
        
        sock.settimeout(3.0)
        data = sock.recv(1024)
        
        if data:
            print(f"📥 Server sent: {data.hex()} ({len(data)} bytes)")
            print(f"   As text: {data}")
        else:
            print("📥 Server sent no data immediately")
            
        # Try sending a simple byte and see what happens
        print("📤 Sending test byte...")
        sock.send(b'\x01')
        
        data = sock.recv(1024)
        if data:
            print(f"📥 Response: {data.hex()} ({len(data)} bytes)")
        else:
            print("📥 No response to test byte")
            
    except socket.timeout:
        print("⏱️ No immediate response from server")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass

if __name__ == "__main__":
    test_header_format()
    test_raw_connection()