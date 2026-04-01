#!/usr/bin/env python3
"""
Manual DiztinGUIsh Server Control via Mesen2 API
This will try to automate the server startup through Mesen2's API if possible
"""

import subprocess
import time
import socket

def check_port_listening(port=9998):
    """Check if a port is listening"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(1.0)
        result = sock.connect_ex(('localhost', port))
        sock.close()
        return result == 0
    except:
        return False

def wait_for_server_start(port=9998, timeout=30):
    """Wait for the server to start listening on the port"""
    print(f"⏳ Waiting for DiztinGUIsh server on port {port}...")
    
    start_time = time.time()
    while time.time() - start_time < timeout:
        if check_port_listening(port):
            print(f"✅ Server is now listening on port {port}")
            return True
        time.sleep(1)
    
    print(f"❌ Timeout waiting for server after {timeout} seconds")
    return False

def main():
    print("=" * 70)
    print("🎮 Manual DiztinGUIsh Server Controller")
    print("=" * 70)
    
    # Check if server is already running
    if check_port_listening():
        print("✅ DiztinGUIsh server is already running")
    else:
        print("❌ DiztinGUIsh server is not running")
        print()
        print("Please manually start the server:")
        print("1. In Mesen2, go to Tools → DiztinGUIsh Server")
        print("2. Click 'Start Server' button")
        print("3. Verify port is 9998")
        print()
        
        input("Press Enter when you have started the server...")
        
        if wait_for_server_start():
            print("🎉 Server started successfully!")
        else:
            print("💥 Server failed to start - check Mesen2")
            return False
    
    print()
    print("🧪 Now testing the handshake fix...")
    
    # Import and run our handshake test
    try:
        import sys
        import os
        sys.path.append(os.path.dirname(__file__))
        from test_handshake_fix import test_handshake
        
        success = test_handshake()
        
        if success:
            print("🎉 HANDSHAKE FIX VERIFIED!")
            print("✅ DiztinguishBridge now sends handshake without ROM!")
        else:
            print("❌ Handshake fix still not working")
            
    except ImportError as e:
        print(f"❌ Could not import test: {e}")
    
    print("=" * 70)

if __name__ == "__main__":
    main()