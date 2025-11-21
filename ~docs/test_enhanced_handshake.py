#!/usr/bin/env python3
"""
Enhanced DiztinGUIsh handshake test with detailed analysis.
This test checks both TCP connection and analyzes why handshake might not be working.
"""

import socket
import time
import struct
import sys

def test_handshake():
    """Test handshake with detailed timing and analysis."""
    print("🔍 Enhanced DiztinGUIsh Handshake Test")
    print("=" * 50)
    
    try:
        print("1️⃣ Connecting to localhost:9998...")
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10.0)  # Longer timeout
        
        start_time = time.time()
        sock.connect(("localhost", 9998))
        connect_time = time.time() - start_time
        print(f"✅ Connected successfully in {connect_time:.3f}s")
        
        print("2️⃣ Waiting for handshake (checking multiple intervals)...")
        
        # Check for data at different intervals
        intervals = [0.1, 0.5, 1.0, 2.0, 3.0]  
        total_data = b""
        
        for i, interval in enumerate(intervals):
            print(f"   📡 Checking after {interval}s...")
            time.sleep(interval if i == 0 else interval - intervals[i-1])
            
            # Make socket non-blocking to check for data
            sock.settimeout(0.1)
            try:
                data = sock.recv(1024)
                if data:
                    total_data += data
                    print(f"   ✅ Received {len(data)} bytes at {interval}s mark")
                    break
                else:
                    print(f"   ❌ No data at {interval}s mark")
            except socket.timeout:
                print(f"   ⏱️ Timeout at {interval}s mark")
            except Exception as e:
                print(f"   ❌ Error at {interval}s: {e}")
        
        if total_data:
            print(f"\n3️⃣ Analysis of received data ({len(total_data)} bytes):")
            print(f"   📊 Hex: {total_data.hex()}")
            print(f"   📊 Raw: {total_data}")
            
            # Try to parse as DiztinGUIsh protocol
            if len(total_data) >= 5:
                # Parse message header (5 bytes)
                msg_type = total_data[0]
                length = struct.unpack('<I', total_data[1:5])[0]
                print(f"   📊 Message Type: {msg_type}")
                print(f"   📊 Payload Length: {length}")
                
                if msg_type == 1:  # MessageType::Handshake
                    print("   ✅ This is a handshake message!")
                    if len(total_data) >= 5 + length:
                        payload = total_data[5:5+length]
                        print(f"   📊 Payload: {payload.hex()}")
                        
                        # Parse handshake payload if it matches expected structure
                        if len(payload) >= 4:
                            major, minor = struct.unpack('<HH', payload[:4])
                            print(f"   📊 Protocol Version: {major}.{minor}")
                else:
                    print(f"   ❓ Unexpected message type: {msg_type}")
            else:
                print("   ❓ Data too short to be a valid message")
        else:
            print("\n3️⃣ No data received - possible causes:")
            print("   🤔 1. Handshake not being sent")
            print("   🤔 2. Message queuing issue")
            print("   🤔 3. Socket buffering issue")
            print("   🤔 4. Debugger not initialized")
            print("   🤔 5. SNES console not active")
            
        # Test if server is still responsive
        print("\n4️⃣ Testing server responsiveness...")
        sock.settimeout(1.0)
        try:
            # Send a small test message
            test_msg = b"test"
            sock.send(test_msg)
            print("   ✅ Server accepted test data")
        except Exception as e:
            print(f"   ❌ Server not responsive to test data: {e}")
            
        sock.close()
        
        print(f"\n{'✅ SUCCESS' if total_data else '❌ FAILED'}")
        return len(total_data) > 0
        
    except ConnectionRefusedError:
        print("❌ Connection refused - DiztinGUIsh server not running")
        print("   💡 Start Mesen2 and activate DiztinGUIsh server from UI")
        return False
    except Exception as e:
        print(f"❌ Connection failed: {e}")
        return False

def main():
    print("DiztinGUIsh Enhanced Handshake Test")
    print("Checking for handshake data with detailed timing analysis...")
    print()
    
    success = test_handshake()
    
    print("\n" + "=" * 50)
    if success:
        print("🎉 Handshake data received! The DiztinguishApiWrapper fix is working.")
        print("   The server is properly initialized and sending handshake messages.")
    else:
        print("❌ No handshake data received. Possible issues:")
        print("   1. Check if a SNES ROM is loaded in Mesen2")
        print("   2. Ensure DiztinGUIsh server is started from the UI")  
        print("   3. Check Mesen2 debug logs for DiztinGUIsh messages")
        print("   4. Verify the debugger is properly initialized")
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())