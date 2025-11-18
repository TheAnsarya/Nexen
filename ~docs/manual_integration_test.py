#!/usr/bin/env python3
"""
Manual Integration Test for DiztinGUIsh-Mesen2 Integration
==========================================================

Use this script when you have Mesen2 already running with DiztinGUIsh server enabled.

Prerequisites:
1. Mesen2 is running with a SNES ROM loaded
2. Tools → DiztinGUIsh Server is started on port 9998

Usage:
    python manual_integration_test.py
"""

import time
import socket
from test_diztinguish_client import DiztinguishTestClient

def check_server_availability(host='localhost', port=9998):
    """Check if the DiztinGUIsh server is available"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(2)
            result = sock.connect_ex((host, port))
            return result == 0
    except Exception:
        return False

def run_manual_integration_test():
    """Run integration tests against an already running server"""
    
    print("🧪 Manual DiztinGUIsh-Mesen2 Integration Test")
    print("=" * 50)
    print("Prerequisites:")
    print("  ✓ Mesen2 is running with SNES ROM loaded")
    print("  ✓ Tools → DiztinGUIsh Server started on port 9998")
    print()
    
    # Check server availability
    print("🔍 Checking server availability...")
    if not check_server_availability():
        print("❌ DiztinGUIsh server not found on localhost:9998")
        print()
        print("Please ensure:")
        print("  1. Mesen2 is running")
        print("  2. A SNES ROM is loaded")
        print("  3. Tools → DiztinGUIsh Server is open")
        print("  4. Server is started on port 9998")
        return False
    
    print("✅ DiztinGUIsh server detected on port 9998")
    print()
    
    # Run comprehensive tests
    print("🧪 Running Comprehensive Integration Tests...")
    print("-" * 50)
    
    client = DiztinguishTestClient('localhost', 9998)
    
    try:
        success = client.run_integration_tests()
        
        print("\n📊 Manual Integration Test Report")
        print("=" * 50)
        
        if success:
            print("🎉 ALL TESTS PASSED!")
            print()
            print("✅ Verified Integration Features:")
            print("   • TCP connection and protocol handshake")
            print("   • Real-time CPU state streaming")
            print("   • Breakpoint control (Execute/Read/Write types)")
            print("   • Memory dump operations (WRAM/ROM)")
            print("   • Binary protocol message handling")
            print("   • Connection stability and error recovery")
            print()
            print("🚀 The DiztinGUIsh-Mesen2 integration is FULLY OPERATIONAL!")
            print("   Ready for real-time disassembly during SNES gameplay.")
        else:
            print("❌ SOME TESTS FAILED")
            print()
            print("   Please check the test output above for specific issues.")
            print("   Common problems:")
            print("   • Server disconnected during testing")
            print("   • ROM not loaded in Mesen2")
            print("   • Server not properly initialized")
            
        return success
        
    except KeyboardInterrupt:
        print("\n🛑 Test interrupted by user")
        return False
    except Exception as e:
        print(f"\n❌ Test execution error: {e}")
        return False

def main():
    """Main test execution"""
    success = run_manual_integration_test()
    
    if success:
        print("\n" + "=" * 60)
        print("🎯 INTEGRATION TESTING PHASE 2 COMPLETE")
        print("=" * 60)
        print("Status: ✅ ALL CORE FUNCTIONALITY VERIFIED")
        print()
        print("Next Steps:")
        print("  • Issue #12: Document test results and mark complete")
        print("  • Issue #13: Continue DiztinGUIsh UI integration")
        print("  • Issue #14: Create user documentation")
        print("  • Issue #15: Performance optimization")
    else:
        print("\n" + "=" * 60)
        print("❌ INTEGRATION TESTING NEEDS ATTENTION")
        print("=" * 60)
        print("Please resolve test failures before proceeding.")
    
    return success

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)