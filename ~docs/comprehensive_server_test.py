#!/usr/bin/env python3
"""
Comprehensive Mesen2 DiztinGUIsh Server Implementation Test
=========================================================

This script thoroughly tests the Mesen2 DiztinGUIsh server to verify:
1. If TCP server is actually listening on port 9998
2. If it accepts connections properly
3. If it sends the expected handshake message
4. If it responds to protocol messages

Use this to diagnose server implementation issues.
"""

import socket
import struct
import time
import sys

def test_port_listening(host='localhost', port=9998):
    """Test if anything is listening on the port"""
    print(f"🔍 Testing if port {port} is listening...")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2.0)
        result = sock.connect_ex((host, port))
        sock.close()
        
        if result == 0:
            print(f"✅ Port {port} is open and accepting connections")
            return True
        else:
            print(f"❌ Port {port} is not accepting connections (error code: {result})")
            return False
    except Exception as e:
        print(f"❌ Error testing port: {e}")
        return False

def test_handshake_protocol(host='localhost', port=9998):
    """Test if server sends proper handshake message"""
    print(f"🤝 Testing handshake protocol...")
    
    sock = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        
        print(f"📡 Connecting to {host}:{port}...")
        sock.connect((host, port))
        print("✅ TCP connection established")
        
        # Wait for handshake message
        print("⏳ Waiting for handshake message...")
        
        # Read message header (should be 5 bytes: type + length)
        header_data = b''
        while len(header_data) < 5:
            chunk = sock.recv(5 - len(header_data))
            if not chunk:
                print("❌ Connection closed while reading header")
                return False
            header_data += chunk
        
        msg_type = header_data[0]
        msg_length = struct.unpack('<I', header_data[1:5])[0]
        
        print(f"📦 Header received - Type: {msg_type}, Length: {msg_length}")
        
        if msg_type != 0:  # Should be handshake message (type 0)
            print(f"⚠️  Expected handshake message (type 0), got type {msg_type}")
            return False
        
        # Read handshake payload
        if msg_length > 0:
            payload_data = b''
            while len(payload_data) < msg_length:
                chunk = sock.recv(msg_length - len(payload_data))
                if not chunk:
                    print("❌ Connection closed while reading payload")
                    return False
                payload_data += chunk
            
            print(f"📄 Payload received: {len(payload_data)} bytes")
            
            # Try to decode handshake (basic validation)
            if len(payload_data) >= 21:  # Minimum handshake size
                version = struct.unpack('<I', payload_data[0:4])[0]
                emu_name_len = payload_data[4]
                
                if len(payload_data) >= 5 + emu_name_len + 16:
                    emu_name = payload_data[5:5+emu_name_len].decode('utf-8', errors='ignore')
                    port_used = struct.unpack('<H', payload_data[5+emu_name_len:7+emu_name_len])[0]
                    rom_size = struct.unpack('<I', payload_data[7+emu_name_len:11+emu_name_len])[0]
                    
                    print(f"🎮 Handshake decoded:")
                    print(f"   Version: {version}")
                    print(f"   Emulator: {emu_name}")
                    print(f"   Port: {port_used}")
                    print(f"   ROM Size: {rom_size} bytes")
                    
                    print("✅ Handshake protocol working correctly!")
                    return True
                else:
                    print("⚠️  Handshake payload too short for full decode")
                    return False
            else:
                print(f"⚠️  Handshake payload too short: {len(payload_data)} bytes (expected >= 21)")
                return False
        else:
            print("⚠️  Handshake has no payload")
            return False
            
    except socket.timeout:
        print("❌ Timeout waiting for handshake - server not responding")
        return False
    except ConnectionRefusedError:
        print("❌ Connection refused - server not running")
        return False
    except Exception as e:
        print(f"❌ Handshake test error: {e}")
        return False
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass

def test_data_streaming(host='localhost', port=9998):
    """Test if server streams execution data"""
    print(f"📡 Testing data streaming...")
    
    sock = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2.0)
        
        sock.connect((host, port))
        
        # Skip handshake message
        header = sock.recv(5)
        if len(header) == 5:
            msg_length = struct.unpack('<I', header[1:5])[0]
            if msg_length > 0:
                sock.recv(msg_length)  # Discard handshake payload
        
        # Wait for additional messages
        print("⏳ Waiting for streaming data (3 seconds)...")
        start_time = time.time()
        message_count = 0
        
        sock.settimeout(0.5)  # Short timeout for data messages
        while time.time() - start_time < 3.0:
            try:
                header = sock.recv(5)
                if len(header) < 5:
                    break
                
                msg_type = header[0]
                msg_length = struct.unpack('<I', header[1:5])[0]
                
                # Read payload if present
                if msg_length > 0:
                    payload = sock.recv(msg_length)
                    if len(payload) < msg_length:
                        break
                
                message_count += 1
                print(f"📨 Data message #{message_count}: Type {msg_type}, Length {msg_length}")
                
            except socket.timeout:
                continue
            except Exception as e:
                print(f"⚠️  Error reading data: {e}")
                break
        
        if message_count > 0:
            print(f"✅ Data streaming working! Received {message_count} messages")
            return True
        else:
            print("⚠️  No streaming data received - emulation may be paused")
            return False
            
    except Exception as e:
        print(f"❌ Data streaming test error: {e}")
        return False
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass

def main():
    """Run comprehensive server tests"""
    print("=" * 70)
    print("🧪 Mesen2 DiztinGUIsh Server Implementation Test")
    print("=" * 70)
    print()
    
    # Test 1: Port listening
    if not test_port_listening():
        print("\n❌ CRITICAL: Server is not listening on port 9998")
        print("\n🔧 Troubleshooting:")
        print("   1. Make sure Mesen2 is running")
        print("   2. Start DiztinGUIsh server: Tools → DiztinGUIsh Server → Start")
        print("   3. Or use Lua: emu.startDiztinguishServer(9998)")
        print("   4. Check server status in Mesen2 UI")
        return False
    
    print()
    
    # Test 2: Handshake protocol
    if not test_handshake_protocol():
        print("\n❌ CRITICAL: Handshake protocol failure")
        print("\n🔧 This indicates a server implementation issue")
        print("   1. Server is listening but not following DiztinGUIsh protocol")
        print("   2. Check Mesen2 DiztinguishBridge implementation")
        print("   3. Verify handshake message format")
        return False
    
    print()
    
    # Test 3: Data streaming
    streaming_ok = test_data_streaming()
    
    print()
    print("=" * 70)
    
    if streaming_ok:
        print("🎉 ALL TESTS PASSED - Server implementation is working correctly!")
        print("💡 If DiztinGUIsh still has connection issues, the problem is client-side")
    else:
        print("⚠️  PARTIAL SUCCESS - Server works but no streaming data")
        print("💡 Possible causes:")
        print("   • Emulation is paused in Mesen2")
        print("   • No ROM is loaded")
        print("   • ROM execution is not active")
        print("   • Server is configured to not stream execution traces")
    
    print("\n📊 Summary:")
    print(f"   ✅ Port listening: Yes")
    print(f"   ✅ Handshake protocol: Yes")
    print(f"   {'✅' if streaming_ok else '⚠️ '} Data streaming: {'Yes' if streaming_ok else 'Limited'}")
    
    return True

if __name__ == "__main__":
    success = main()
    
    print("\n" + "=" * 70)
    if success:
        print("🏁 Test completed successfully")
    else:
        print("🏁 Test failed - check Mesen2 server setup")
    
    input("\nPress Enter to exit...")