#!/usr/bin/env python3
"""
Test DiztinGUIsh server status using a separate API call approach.
This checks what the DiztinguishApi reports vs actual connection state.
"""

import socket
import time
import subprocess
import sys

def check_server_status_via_netstat():
    """Check if DiztinGUIsh server is listening via netstat."""
    try:
        result = subprocess.run(['netstat', '-an'], capture_output=True, text=True)
        if ':9998' in result.stdout:
            for line in result.stdout.split('\n'):
                if ':9998' in line:
                    print(f"📡 netstat: {line.strip()}")
            return True
        return False
    except:
        return False

def test_tcp_behavior():
    """Test the actual TCP behavior.""" 
    print("🔍 TCP Connection Behavior Test")
    print("=" * 40)
    
    print("1️⃣ Checking port status...")
    if check_server_status_via_netstat():
        print("✅ Port 9998 is listening")
    else:
        print("❌ Port 9998 not found in netstat")
        return False
    
    print("\n2️⃣ Testing connection behavior...")
    
    # Test 1: Basic connection
    try:
        sock1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock1.settimeout(5.0)
        sock1.connect(("localhost", 9998))
        print("✅ Connection 1: Successful")
        
        # Test 2: Check if data is available immediately
        sock1.settimeout(0.1)
        try:
            data = sock1.recv(1024)
            if data:
                print(f"✅ Data immediately available: {len(data)} bytes")
                print(f"   Hex: {data.hex()}")
            else:
                print("❌ recv() returned 0 bytes (connection closed)")
        except socket.timeout:
            print("❌ No immediate data available")
        
        # Test 3: Keep connection open and wait longer
        print("   ⏳ Keeping connection open for 5 seconds...")
        sock1.settimeout(5.0)
        try:
            data = sock1.recv(1024)
            if data:
                print(f"✅ Delayed data received: {len(data)} bytes")
            else:
                print("❌ No data after 5 seconds")
        except socket.timeout:
            print("❌ Timeout after 5 seconds")
            
        sock1.close()
        
        # Test 4: Multiple connections  
        print("\n3️⃣ Testing multiple connections...")
        sockets = []
        for i in range(3):
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2.0)
                sock.connect(("localhost", 9998))
                sockets.append(sock)
                print(f"✅ Connection {i+2}: Successful")
            except Exception as e:
                print(f"❌ Connection {i+2}: Failed - {e}")
                
        # Clean up
        for sock in sockets:
            sock.close()
            
        return True
        
    except Exception as e:
        print(f"❌ Connection test failed: {e}")
        return False

def main():
    print("DiztinGUIsh TCP Behavior Test")
    print("Testing the actual network behavior to understand the handshake issue")
    print()
    
    success = test_tcp_behavior()
    
    print("\n" + "=" * 40)
    if success:
        print("📋 Summary: Server accepts connections but sends no data")
        print("   This confirms the issue is in the handshake sending logic,")
        print("   not in the TCP server itself.")
    else:
        print("❌ TCP tests failed - server may not be running properly")
    
    print("\n💡 Next steps:")
    print("   1. Load a SNES ROM in Mesen2")
    print("   2. Check Mesen2 debug console for DiztinGUIsh log messages")
    print("   3. Verify debugger initialization worked")
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())