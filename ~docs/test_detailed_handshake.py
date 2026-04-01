#!/usr/bin/env python3
"""
Detailed DiztinguishBridge protocol test with more debugging
"""

import socket
import struct
import time

def test_detailed_handshake():
    """Test with detailed protocol debugging"""
    print("🔍 Detailed DiztinguishBridge Protocol Test")
    print("-" * 50)
    
    sock = None
    try:
        # Connect to Mesen2 DiztinGUIsh server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10.0)  # Longer timeout
        
        print("📡 Connecting to localhost:9998...")
        sock.connect(('localhost', 9998))
        print("✅ TCP connection established")
        
        # Set shorter timeout for reading
        sock.settimeout(2.0)
        
        print("⏳ Waiting for any data...")
        
        # Try to read any data that comes
        received_data = b""
        start_time = time.time()
        
        while time.time() - start_time < 5.0:  # Wait up to 5 seconds
            try:
                chunk = sock.recv(1024)  # Read any available data
                if chunk:
                    received_data += chunk
                    print(f"📨 Received {len(chunk)} bytes: {chunk.hex()}")
                    if len(chunk) < 1024:  # Probably got full message
                        break
                else:
                    print("❌ Connection closed by server")
                    break
            except socket.timeout:
                print("⏰ Timeout waiting for data")
                break
            except Exception as e:
                print(f"❌ Error reading: {e}")
                break
        
        if received_data:
            print(f"\n📊 Total received: {len(received_data)} bytes")
            print(f"🔢 Raw data: {received_data.hex()}")
            
            # Try to parse as DiztinGUIsh protocol
            if len(received_data) >= 5:  # MessageHeader is 5 bytes
                msg_type = received_data[0]
                msg_length = struct.unpack('<I', received_data[1:5])[0]  # Little-endian uint32
                
                print(f"📋 Message type: {msg_type} (0x{msg_type:02X})")
                print(f"📏 Message length: {msg_length}")
                
                if msg_type == 1:  # Handshake
                    print("🤝 Message type is Handshake!")
                    
                    if len(received_data) >= 5 + msg_length:
                        payload = received_data[5:5+msg_length]
                        print(f"📦 Payload ({len(payload)} bytes): {payload.hex()}")
                        
                        # Parse handshake payload
                        if len(payload) >= 8:
                            version_major, version_minor = struct.unpack('<HH', payload[:4])
                            print(f"🔗 Protocol version: {version_major}.{version_minor}")
                            
                            if len(payload) >= 16:
                                rom_crc, rom_size = struct.unpack('<II', payload[8:16])
                                print(f"🎮 ROM CRC32: 0x{rom_crc:08X}")
                                print(f"📀 ROM Size: {rom_size} bytes")
                                
                                if len(payload) >= 272:  # 16 + 256 bytes for ROM name
                                    rom_name_bytes = payload[16:272]
                                    rom_name = rom_name_bytes.rstrip(b'\x00').decode('utf-8', errors='ignore')
                                    print(f"📄 ROM Name: '{rom_name}'")
                                    
                                print("\n✅ HANDSHAKE RECEIVED SUCCESSFULLY!")
                                print("🎉 DiztinguishBridge fix is working!")
                                return True
                            else:
                                print(f"❌ Incomplete handshake payload: {len(payload)} bytes")
                        else:
                            print(f"❌ Handshake payload too short: {len(payload)} bytes")
                    else:
                        print(f"❌ Incomplete message: expected {5+msg_length}, got {len(received_data)}")
                else:
                    print(f"❌ Unexpected message type: {msg_type}")
            else:
                print(f"❌ Data too short for protocol header: {len(received_data)} bytes")
        else:
            print("❌ No data received from server")
            print("💡 This suggests the server accepts connections but doesn't send handshake")
            
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

def main():
    print("=" * 70)
    print("🔧 Detailed DiztinguishBridge Handshake Analysis")
    print("=" * 70)
    
    success = test_detailed_handshake()
    
    print("\n" + "=" * 70)
    if success:
        print("🏁 SUCCESS: Handshake protocol working!")
    else:
        print("🏁 FAILURE: Handshake protocol still not working")
        print("\n💡 Troubleshooting suggestions:")
        print("   1. Check if DiztinGUIsh server is actually started in Mesen2")
        print("   2. Look at Mesen2 logs for any error messages")
        print("   3. Verify the server is listening properly")
        print("   4. Check if our fix was compiled correctly")
    print("=" * 70)

if __name__ == "__main__":
    main()