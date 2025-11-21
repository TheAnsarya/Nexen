#!/usr/bin/env python3
"""
Simple connection test to see what happens when we connect to Mesen
"""
import socket
import time

def simple_connection_test():
    """Just connect and see what happens"""
    print("=== SIMPLE CONNECTION TEST ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)  # Longer timeout
        print("Connecting to localhost:9998...")
        sock.connect(("localhost", 9998))
        print("✅ Connected!")
        
        # Just wait and see what happens
        print("Waiting 5 seconds for any data...")
        sock.settimeout(5)
        
        try:
            data = sock.recv(1024)
            print(f"Received {len(data)} bytes: {data.hex()}")
            if data:
                print(f"As text: {repr(data)}")
        except socket.timeout:
            print("No data received (timeout)")
        
        # Try to send something simple and see response
        print("Sending simple data...")
        sock.send(b"HELLO")
        
        try:
            response = sock.recv(1024)
            print(f"Response: {len(response)} bytes: {response.hex()}")
        except socket.timeout:
            print("No response to simple data")
        
        sock.close()
        return True
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

if __name__ == "__main__":
    simple_connection_test()