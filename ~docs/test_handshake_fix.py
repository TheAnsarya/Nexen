#!/usr/bin/env python3
"""
Quick test for the fixed DiztinguishBridge handshake protocol
"""

import socket
import struct
import time

def test_handshake():
    """Test the handshake protocol with Mesen2"""
    print("🧪 Testing DiztinguishBridge handshake fix...")
    
    sock = None
    try:
        # Connect to Mesen2 DiztinGUIsh server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        
        print("📡 Connecting to localhost:9998...")
        sock.connect(('localhost', 9998))
        print("✅ TCP connection established")
        
        # Wait for handshake message
        print("⏳ Waiting for handshake message...")
        
        # Read message header (8 bytes: type + length)
        header_data = sock.recv(8)
        if len(header_data) < 8:
            print(f"❌ Incomplete header received: {len(header_data)} bytes")
            return False
            
        msg_type, msg_length = struct.unpack('<II', header_data)
        print(f"📨 Received message - Type: {msg_type}, Length: {msg_length}")
        
        if msg_type == 1:  # Handshake message
            # Read handshake payload
            handshake_data = sock.recv(msg_length)
            if len(handshake_data) < msg_length:
                print(f"❌ Incomplete handshake payload: {len(handshake_data)}/{msg_length}")
                return False
                
            # Parse handshake (version info + ROM info)
            if len(handshake_data) >= 8:
                version_major, version_minor = struct.unpack('<II', handshake_data[:8])
                print(f"🔗 Protocol version: {version_major}.{version_minor}")
                
                if len(handshake_data) >= 16:
                    rom_crc, rom_size = struct.unpack('<II', handshake_data[8:16])
                    print(f"🎮 ROM CRC32: 0x{rom_crc:08X}, Size: {rom_size} bytes")
                    
                    if len(handshake_data) >= 48:  # 16 + 32 bytes for ROM name
                        rom_name = handshake_data[16:48].rstrip(b'\x00').decode('utf-8', errors='ignore')
                        print(f"📄 ROM Name: '{rom_name}'")
                
            print("✅ Handshake received successfully!")
            print("🎉 DiztinguishBridge fix verified - server sends handshake without ROM!")
            return True
        else:
            print(f"❌ Unexpected message type: {msg_type}")
            return False
            
    except socket.timeout:
        print("❌ Connection timeout")
        return False
    except ConnectionRefusedError:
        print("❌ Connection refused - server not running")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass

if __name__ == "__main__":
    print("=" * 70)
    print("🔧 DiztinguishBridge Handshake Fix Verification")
    print("=" * 70)
    
    success = test_handshake()
    
    print("=" * 70)
    if success:
        print("🏁 Fix verified - handshake protocol working!")
    else:
        print("🏁 Fix needs more work - still failing")
    print("=" * 70)