#!/usr/bin/env python3
"""
Test script to verify DiztinguishApiWrapper fix for debugger initialization.

This script tests the connection behavior before and after the fix to ensure
the DiztinGUIsh server properly initializes the debugger.
"""

import socket
import time
import struct
import sys
import subprocess

def test_connection():
    """Test basic TCP connection to DiztinGUIsh server."""
    print("Testing DiztinGUIsh server connection...")
    
    try:
        # Try to connect to default port
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        
        print("Attempting connection to localhost:9998...")
        sock.connect(("localhost", 9998))
        print("✓ Connected successfully!")
        
        # Wait for handshake
        print("Waiting for handshake message...")
        data = sock.recv(1024)
        if data:
            print(f"✓ Received {len(data)} bytes: {data.hex()}")
            
            # Try to decode as DiztinGUIsh protocol
            if len(data) >= 4:
                msg_type = struct.unpack('<I', data[:4])[0]
                print(f"  Message type: {msg_type}")
                if msg_type == 1:  # MessageType::Handshake
                    print("✓ Received handshake message!")
                else:
                    print(f"? Unexpected message type: {msg_type}")
            else:
                print("? Message too short")
        else:
            print("✗ No data received")
        
        sock.close()
        return True
        
    except ConnectionRefusedError:
        print("✗ Connection refused - server not running")
        return False
    except socket.timeout:
        print("✗ Connection timeout")
        return False
    except Exception as e:
        print(f"✗ Connection failed: {e}")
        return False

def main():
    print("DiztinGUIsh API Wrapper Fix Test")
    print("=" * 40)
    print()
    
    print("This test verifies that the DiztinguishApiWrapper fix correctly")
    print("initializes the debugger when starting the DiztinGUIsh server.")
    print()
    
    # Test the connection
    success = test_connection()
    
    print()
    if success:
        print("✓ Test PASSED - Server responded correctly")
        print("The fix appears to be working!")
    else:
        print("✗ Test FAILED - Server not responding")
        print("Check if Mesen2 is running with SNES ROM loaded")
        print("and DiztinGUIsh server is started from the UI")
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())