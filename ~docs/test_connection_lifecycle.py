#!/usr/bin/env python3
"""
Quick test to check Mesen2 DiztinGUIsh server connection lifecycle.
Tests if server is running, accepting connections, and streaming data.
"""

import socket
import time
import struct

def test_mesen_connection():
    """Test basic connection to Mesen2 server"""
    host = "localhost" 
    port = 9998
    sock = None
    
    print(f"🧪 Testing connection to Mesen2 at {host}:{port}")
    print("=" * 50)
    
    # Test 1: Basic TCP connection
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)  # 5 second timeout
        
        print("📡 Attempting TCP connection...")
        sock.connect((host, port))
        print(f"✅ Connected to {host}:{port}")
        
        # Test 2: Wait for handshake message
        print("🤝 Waiting for handshake message...")
        try:
            # Read message header (5 bytes: type + length) 
            header = sock.recv(5)
            if len(header) < 5:
                print(f"❌ Incomplete header received: {len(header)} bytes")
                return False
                
            msg_type = header[0]
            msg_length = struct.unpack('<I', header[1:])[0]  # Little-endian uint32
            
            print(f"📦 Message received - Type: {msg_type}, Length: {msg_length}")
            
            # Read message body if present
            if msg_length > 0:
                body = sock.recv(msg_length)
                print(f"📄 Message body: {len(body)} bytes received")
                
                # If handshake (type 0), decode some basic info
                if msg_type == 0 and len(body) >= 21:
                    version = struct.unpack('<I', body[0:4])[0]
                    emu_name_len = struct.unpack('<B', body[4:5])[0]
                    if len(body) >= 5 + emu_name_len + 16:
                        emu_name = body[5:5+emu_name_len].decode('utf-8', errors='ignore')
                        print(f"🎮 Handshake - Version: {version}, Emulator: {emu_name}")
            
            # Test 3: Wait for additional messages
            print("⏳ Waiting for streaming data (5 seconds)...")
            start_time = time.time()
            message_count = 1  # Already got handshake
            
            sock.settimeout(1.0)  # Short timeout for subsequent messages
            while time.time() - start_time < 5.0:
                try:
                    header = sock.recv(5)
                    if len(header) < 5:
                        break
                    
                    msg_type = header[0]
                    msg_length = struct.unpack('<I', header[1:])[0]
                    
                    # Read body if present
                    if msg_length > 0:
                        body = sock.recv(msg_length) 
                        if len(body) < msg_length:
                            break
                    
                    message_count += 1
                    print(f"📨 Message #{message_count}: Type {msg_type}, Length {msg_length}")
                    
                except socket.timeout:
                    time.sleep(0.1)
                    continue
                except Exception as e:
                    print(f"⚠️  Error reading message: {e}")
                    break
            
            print(f"📊 Total messages received: {message_count}")
            
            if message_count == 1:
                print("⚠️  Only handshake received - no streaming data")
                print("💡 Possible causes:")
                print("   • Mesen2 emulation is paused")
                print("   • No ROM is loaded")
                print("   • DiztinGUIsh server is not actively streaming")
                return False
            else:
                print(f"✅ Streaming is working! Received {message_count-1} data messages")
                return True
                
        except socket.timeout:
            print("❌ Timeout waiting for handshake - server not responding")
            return False
            
    except ConnectionRefusedError:
        print("❌ Connection refused - Mesen2 server not running or port blocked")
        return False
    except Exception as e:
        print(f"❌ Connection error: {e}")
        return False
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass

if __name__ == "__main__":
    success = test_mesen_connection()
    if success:
        print("\n🎉 Connection test PASSED - streaming is working")
        print("💡 If DiztinGUIsh still has issues, check the UI cancellation logic")
    else:
        print("\n❌ Connection test FAILED")
        print("🔧 Troubleshooting steps:")
        print("   1. Make sure Mesen2 is running")
        print("   2. Load a ROM in Mesen2") 
        print("   3. Enable DiztinGUIsh server: emu.startDiztinguishServer(9998)")
        print("   4. Start emulation (unpause)")