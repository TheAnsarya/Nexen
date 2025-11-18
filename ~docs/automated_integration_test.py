#!/usr/bin/env python3
"""
Automated Integration Test Suite for DiztinGUIsh-Mesen2 Integration
====================================================================

This script automates the complete integration testing workflow:
1. Attempts to start Mesen2 with DiztinGUIsh server
2. Waits for server to become available
3. Runs comprehensive protocol tests
4. Generates detailed test report

Usage:
    python automated_integration_test.py [--mesen-path <path>] [--verbose]
"""

import argparse
import subprocess
import time
import socket
import os
import sys
from test_diztinguish_client import DiztinguishTestClient

class MesenIntegrationTestRunner:
    def __init__(self, mesen_path=None, port=9998, verbose=False):
        self.mesen_path = mesen_path or self._find_mesen_executable()
        self.port = port
        self.verbose = verbose
        self.mesen_process = None
        
    def _find_mesen_executable(self):
        """Find Mesen.exe in the build directories"""
        possible_paths = [
            r"C:\Users\me\source\repos\Mesen2\bin\win-x64\Debug\Mesen.exe",
            r"C:\Users\me\source\repos\Mesen2\bin\win-x64\Release\Mesen.exe",
            r"..\bin\win-x64\Debug\Mesen.exe",
            r"..\bin\win-x64\Release\Mesen.exe"
        ]
        
        for path in possible_paths:
            if os.path.exists(path):
                return path
                
        return None
    
    def _is_server_running(self, host='localhost', timeout=2):
        """Check if DiztinGUIsh server is running on the specified port"""
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.settimeout(timeout)
                result = sock.connect_ex((host, self.port))
                return result == 0
        except Exception:
            return False
    
    def start_mesen_server(self):
        """Start Mesen2 and try to enable DiztinGUIsh server"""
        if not self.mesen_path:
            print("❌ Could not find Mesen.exe executable")
            print("   Please specify path with --mesen-path or ensure Mesen2 is built")
            return False
            
        if not os.path.exists(self.mesen_path):
            print(f"❌ Mesen executable not found at: {self.mesen_path}")
            return False
            
        print(f"🚀 Starting Mesen2 from: {self.mesen_path}")
        
        try:
            # Start Mesen2 (this will open the GUI)
            self.mesen_process = subprocess.Popen([self.mesen_path], 
                                                stdout=subprocess.PIPE if not self.verbose else None,
                                                stderr=subprocess.PIPE if not self.verbose else None)
            
            print("⏳ Mesen2 started. Please manually:")
            print("   1. Load a SNES ROM")
            print("   2. Go to Tools → DiztinGUIsh Server")
            print("   3. Click 'Start Server' on port 9998")
            print("   4. Press Enter here when server is started...")
            
            # Wait for user confirmation
            input()
            
            # Check if server is running
            if self._is_server_running():
                print("✅ DiztinGUIsh server detected on port 9998")
                return True
            else:
                print("❌ DiztinGUIsh server not detected. Please ensure:")
                print("   - Server is started in Mesen2 Tools menu")
                print("   - Port 9998 is available")
                print("   - No firewall blocking localhost connections")
                return False
                
        except Exception as e:
            print(f"❌ Failed to start Mesen2: {e}")
            return False
    
    def wait_for_server(self, max_wait=60):
        """Wait for server to become available"""
        print(f"⏳ Waiting for server on port {self.port}...")
        
        for i in range(max_wait):
            if self._is_server_running():
                print("✅ Server is ready!")
                return True
                
            time.sleep(1)
            if i % 10 == 9:  # Print progress every 10 seconds
                print(f"   Still waiting... ({i+1}s)")
        
        print(f"❌ Server did not become available within {max_wait} seconds")
        return False
    
    def run_integration_tests(self):
        """Run the comprehensive integration test suite"""
        print("\n🧪 Running Integration Test Suite")
        print("=" * 50)
        
        client = DiztinguishTestClient('localhost', self.port)
        
        try:
            success = client.run_integration_tests()
            
            print("\n📊 Integration Test Report")
            print("=" * 50)
            
            if success:
                print("🎉 ALL INTEGRATION TESTS PASSED!")
                print("\n✅ Verified Functionality:")
                print("   • TCP connection and handshake")
                print("   • CPU state streaming")
                print("   • Breakpoint control (Execute/Read/Write)")
                print("   • Memory dump operations")
                print("   • Protocol message handling")
                print("   • Error recovery and stability")
            else:
                print("❌ SOME TESTS FAILED")
                print("\n   Check test output above for specific failures")
                
            return success
            
        except KeyboardInterrupt:
            print("\n🛑 Tests interrupted by user")
            return False
        except Exception as e:
            print(f"\n❌ Test execution error: {e}")
            return False
    
    def cleanup(self):
        """Clean up resources"""
        if self.mesen_process:
            print("\n🧹 Cleaning up...")
            try:
                # Give Mesen2 a chance to close gracefully
                self.mesen_process.terminate()
                self.mesen_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                print("   Force terminating Mesen2...")
                self.mesen_process.kill()
            except Exception as e:
                print(f"   Warning: Could not clean up Mesen2 process: {e}")
    
    def run_full_test_suite(self):
        """Run the complete automated test suite"""
        print("🚀 DiztinGUIsh-Mesen2 Integration Test Suite")
        print("=" * 60)
        print("This will test the complete streaming integration between")
        print("Mesen2 (SNES Emulator) and DiztinGUIsh (Disassembler)")
        print()
        
        try:
            # Check for existing server first
            if self._is_server_running():
                print("✅ DiztinGUIsh server already running, proceeding with tests")
            else:
                # Try to start Mesen2 and server
                if not self.start_mesen_server():
                    print("\n❌ Could not start Mesen2 server")
                    print("   Manual setup required:")
                    print("   1. Start Mesen2")
                    print("   2. Load SNES ROM")
                    print("   3. Tools → DiztinGUIsh Server → Start Server")
                    return False
            
            # Run the test suite
            success = self.run_integration_tests()
            
            if success:
                print("\n🎉 INTEGRATION COMPLETE - All systems operational!")
                print("   The DiztinGUIsh-Mesen2 streaming integration is working perfectly.")
                print("   Ready for production use!")
            
            return success
            
        finally:
            self.cleanup()

def main():
    parser = argparse.ArgumentParser(
        description="Automated Integration Test Suite for DiztinGUIsh-Mesen2",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python automated_integration_test.py
    python automated_integration_test.py --mesen-path "C:/path/to/Mesen.exe"
    python automated_integration_test.py --verbose
        """
    )
    
    parser.add_argument("--mesen-path", help="Path to Mesen.exe (auto-detected if not specified)")
    parser.add_argument("--port", type=int, default=9998, help="Server port (default: 9998)")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    
    args = parser.parse_args()
    
    runner = MesenIntegrationTestRunner(
        mesen_path=args.mesen_path,
        port=args.port,
        verbose=args.verbose
    )
    
    success = runner.run_full_test_suite()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()