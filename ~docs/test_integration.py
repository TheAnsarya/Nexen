#!/usr/bin/env python3
"""
Test DiztinGUIsh Mesen Integration
Tests the complete pipeline from simulator to DiztinGUIsh
"""
import subprocess
import time
import threading
import os
import sys

def run_simulator():
    """Run the Mesen simulator in background"""
    print("🚀 Starting Mesen simulator...")
    
    # Start simulator in background
    process = subprocess.Popen(
        [sys.executable, "mesen_simulator.py"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        cwd=os.path.dirname(__file__)
    )
    
    # Wait for startup
    time.sleep(2)
    
    return process

def test_diztinguish_connection():
    """Test connecting DiztinGUIsh to the simulator"""
    print("🔌 Testing DiztinGUIsh connection...")
    
    # Start simulator
    sim_process = run_simulator()
    
    try:
        print("✅ Simulator started")
        print("📋 Instructions:")
        print("   1. Open DiztinGUIsh (should already be running)")
        print("   2. Create a new project or open existing one")
        print("   3. Go to Import menu")
        print("   4. Select 'Mesen2 Live Stream'")
        print("   5. Use host: localhost, port: 9998")
        print("   6. Click Connect")
        print()
        print("Expected behavior:")
        print("   - Should connect successfully")
        print("   - Should receive handshake from simulator")
        print("   - Should start streaming trace data")
        print("   - DiztinGUIsh should mark ROM bytes as opcodes")
        
        # Wait for user to test
        input("\nPress Enter when done testing (or Ctrl+C to stop)...")
        
    except KeyboardInterrupt:
        print("\n🛑 Test interrupted")
    finally:
        # Stop simulator
        print("🛑 Stopping simulator...")
        try:
            sim_process.stdin.write("stop\n")
            sim_process.stdin.flush()
            time.sleep(1)
            sim_process.terminate()
        except:
            pass
        
        print("✅ Test complete")

if __name__ == "__main__":
    test_diztinguish_connection()