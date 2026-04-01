#!/usr/bin/env python3
"""
DiztinGUIsh Protocol Test Client
================================

A simple test client for validating the DiztinGUIsh-Mesen2 streaming protocol.
This client implements the core protocol messages for testing integration.

Usage:
    python test_diztinguish_client.py [--host HOST] [--port PORT]

Test Phases:
1. Connection and Handshake
2. CPU State Streaming  
3. Breakpoint Control
4. Memory Dump Requests
5. Label Synchronization

Author: GitHub Copilot
Date: November 18, 2025
"""

import socket
import struct
import time
import sys
import argparse
from typing import Optional, Tuple, Dict, Any
from enum import IntEnum


class MessageType(IntEnum):
    """DiztinGUIsh Protocol Message Types"""
    Handshake = 0
    HandshakeAck = 1
    ConfigStream = 2
    ExecTrace = 3
    CdlUpdate = 4
    MemoryAccess = 5
    CpuState = 6
    CpuStateRequest = 7
    LabelAdd = 8
    LabelUpdate = 9
    LabelDelete = 10
    LabelSyncRequest = 11
    LabelSyncResponse = 12
    BreakpointAdd = 13
    BreakpointRemove = 14
    MemoryDumpRequest = 15
    MemoryDumpResponse = 16
    Heartbeat = 17
    Disconnect = 18


class DiztinguishTestClient:
    """Test client for DiztinGUIsh-Mesen2 protocol validation"""
    
    def __init__(self, host: str = "127.0.0.1", port: int = 9998):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        self.connected = False
        self.handshake_complete = False
        
        # Test statistics
        self.messages_sent = 0
        self.messages_received = 0
        self.test_results = {}
        
    def connect(self) -> bool:
        """Connect to Mesen2 DiztinGUIsh server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(5.0)  # 5 second timeout
            
            print(f"Connecting to {self.host}:{self.port}...")
            self.socket.connect((self.host, self.port))
            self.connected = True
            print("✅ Connected successfully!")
            return True
            
        except Exception as e:
            print(f"❌ Connection failed: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from server"""
        if self.socket:
            try:
                # Send disconnect message
                self.send_message(MessageType.Disconnect, b"")
                self.socket.close()
            except:
                pass
            self.socket = None
            self.connected = False
            self.handshake_complete = False
            print("🔌 Disconnected")
    
    def send_message(self, msg_type: MessageType, payload: bytes) -> bool:
        """Send a message to the server"""
        if not self.connected:
            print("❌ Not connected to server")
            return False
            
        try:
            # Message format: [Type:1][Length:4][Payload:Length]
            header = struct.pack('<BI', msg_type.value, len(payload))
            message = header + payload
            
            self.socket.sendall(message)
            self.messages_sent += 1
            print(f"📤 Sent {msg_type.name} message ({len(payload)} bytes)")
            return True
            
        except Exception as e:
            print(f"❌ Send failed: {e}")
            return False
    
    def receive_message(self, timeout: float = 2.0) -> Optional[Tuple[MessageType, bytes]]:
        """Receive a message from the server"""
        if not self.connected:
            return None
            
        try:
            self.socket.settimeout(timeout)
            
            # Read header (5 bytes: type + length)
            header = b""
            while len(header) < 5:
                chunk = self.socket.recv(5 - len(header))
                if not chunk:
                    raise ConnectionError("Server closed connection")
                header += chunk
            
            msg_type_val, payload_length = struct.unpack('<BI', header)
            msg_type = MessageType(msg_type_val)
            
            # Read payload
            payload = b""
            while len(payload) < payload_length:
                chunk = self.socket.recv(payload_length - len(payload))
                if not chunk:
                    raise ConnectionError("Server closed connection")
                payload += chunk
            
            self.messages_received += 1
            print(f"📥 Received {msg_type.name} message ({len(payload)} bytes)")
            return msg_type, payload
            
        except socket.timeout:
            print("⏰ Receive timeout")
            return None
        except Exception as e:
            print(f"❌ Receive failed: {e}")
            return None
    
    def perform_handshake(self) -> bool:
        """Perform protocol handshake"""
        print("\n🤝 Starting handshake...")
        
        # Send handshake message
        handshake_payload = struct.pack('<HH', 1, 0)  # Version 1.0
        if not self.send_message(MessageType.Handshake, handshake_payload):
            return False
        
        # Wait for handshake ack
        response = self.receive_message(timeout=5.0)
        if response is None:
            print("❌ No handshake response")
            return False
        
        msg_type, payload = response
        if msg_type != MessageType.HandshakeAck:
            print(f"❌ Expected HandshakeAck, got {msg_type.name}")
            return False
        
        if len(payload) >= 4:
            server_major, server_minor = struct.unpack('<HH', payload[:4])
            print(f"✅ Handshake complete - Server version {server_major}.{server_minor}")
        else:
            print("✅ Handshake complete - Unknown server version")
        
        self.handshake_complete = True
        return True
    
    def test_cpu_state_streaming(self) -> bool:
        """Test CPU state request/response"""
        print("\n🧠 Testing CPU State Streaming...")
        
        if not self.handshake_complete:
            print("❌ Handshake required before CPU state testing")
            return False
        
        # Send CPU state request
        if not self.send_message(MessageType.CpuStateRequest, b""):
            return False
        
        # Wait for CPU state response
        response = self.receive_message(timeout=3.0)
        if response is None:
            print("❌ No CPU state response")
            return False
        
        msg_type, payload = response
        if msg_type != MessageType.CpuState:
            print(f"❌ Expected CpuState, got {msg_type.name}")
            return False
        
        # Parse CPU state (assuming 17 bytes based on CpuStateSnapshot)
        if len(payload) >= 17:
            a, x, y, s, d, db, pc, p, em = struct.unpack('<HHHHHBIB?', payload[:17])
            print(f"✅ CPU State received:")
            print(f"   A=${a:04X} X=${x:04X} Y=${y:04X} S=${s:04X}")
            print(f"   D=${d:04X} DB=${db:02X} PC=${pc:06X} P=${p:02X}")
            print(f"   EmulationMode={em}")
            
            # Performance test - send 10 rapid requests
            print("⚡ Testing CPU state performance...")
            start_time = time.time()
            for i in range(10):
                if not self.send_message(MessageType.CpuStateRequest, b""):
                    break
                resp = self.receive_message(timeout=1.0)
                if resp is None or resp[0] != MessageType.CpuState:
                    print(f"❌ Performance test failed at request {i+1}")
                    return False
            
            end_time = time.time()
            avg_time = (end_time - start_time) / 10 * 1000  # Convert to ms
            print(f"✅ Average response time: {avg_time:.1f}ms")
            
            self.test_results['cpu_state'] = {
                'success': True,
                'avg_response_time_ms': avg_time,
                'last_state': {
                    'A': a, 'X': x, 'Y': y, 'S': s,
                    'D': d, 'DB': db, 'PC': pc, 'P': p,
                    'EmulationMode': em
                }
            }
            return True
        else:
            print(f"❌ Invalid CPU state payload size: {len(payload)} bytes")
            return False
    
    def test_breakpoint_control(self) -> bool:
        """Test breakpoint add/remove operations"""
        print("\n🛑 Testing Breakpoint Control...")
        
        if not self.handshake_complete:
            print("❌ Handshake required before breakpoint testing")
            return False
        
        # Test execute breakpoint at $008000 (common ROM start)
        test_address = 0x008000
        
        # Add execute breakpoint
        print(f"Adding execute breakpoint at ${test_address:06X}...")
        bp_payload = struct.pack('<IB?', test_address, 0, True)  # address, type (Execute), enabled
        if not self.send_message(MessageType.BreakpointAdd, bp_payload):
            return False
        
        time.sleep(0.1)  # Brief pause for server processing
        
        # Add read breakpoint at WRAM location
        wram_address = 0x7E0000
        print(f"Adding read breakpoint at ${wram_address:06X}...")
        bp_payload = struct.pack('<IB?', wram_address, 1, True)  # address, type (Read), enabled
        if not self.send_message(MessageType.BreakpointAdd, bp_payload):
            return False
        
        time.sleep(0.1)
        
        # Add write breakpoint at different WRAM location
        wram_write_address = 0x7E0100
        print(f"Adding write breakpoint at ${wram_write_address:06X}...")
        bp_payload = struct.pack('<IB?', wram_write_address, 2, True)  # address, type (Write), enabled
        if not self.send_message(MessageType.BreakpointAdd, bp_payload):
            return False
        
        time.sleep(0.5)  # Allow time for breakpoints to be applied
        
        # Remove breakpoints
        print("Removing breakpoints...")
        for addr, bp_type in [(test_address, 0), (wram_address, 1), (wram_write_address, 2)]:
            bp_payload = struct.pack('<IB?', addr, bp_type, False)  # address, type, disabled
            if not self.send_message(MessageType.BreakpointRemove, bp_payload):
                return False
            time.sleep(0.1)
        
        print("✅ Breakpoint operations completed successfully")
        self.test_results['breakpoints'] = {
            'success': True,
            'tested_types': ['Execute', 'Read', 'Write'],
            'test_addresses': [test_address, wram_address, wram_write_address]
        }
        return True
    
    def test_memory_dump(self) -> bool:
        """Test memory dump request/response"""
        print("\n💾 Testing Memory Dump...")
        
        if not self.handshake_complete:
            print("❌ Handshake required before memory dump testing")
            return False
        
        # Request WRAM dump (first 1KB)
        print("Requesting WRAM dump (1KB)...")
        # MemoryDumpRequest format: memoryType(1), startAddress(4), length(4)
        dump_payload = struct.pack('<BII', 0, 0x7E0000, 1024)  # WRAM type, start addr, length
        if not self.send_message(MessageType.MemoryDumpRequest, dump_payload):
            return False
        
        # Wait for memory dump response
        response = self.receive_message(timeout=5.0)
        if response is None:
            print("❌ No memory dump response")
            return False
        
        msg_type, payload = response
        if msg_type != MessageType.MemoryDumpResponse:
            print(f"❌ Expected MemoryDumpResponse, got {msg_type.name}")
            return False
        
        if len(payload) >= 9:  # Header size
            mem_type, start_addr, data_length = struct.unpack('<BII', payload[:9])
            data = payload[9:]
            print(f"✅ Memory dump received:")
            print(f"   Type: {mem_type}, Start: ${start_addr:06X}, Length: {data_length}")
            print(f"   Data received: {len(data)} bytes")
            
            if len(data) > 0:
                # Show first 16 bytes as hex
                hex_data = ' '.join(f'{b:02X}' for b in data[:16])
                print(f"   First 16 bytes: {hex_data}")
                
                self.test_results['memory_dump'] = {
                    'success': True,
                    'requested_size': 1024,
                    'received_size': len(data),
                    'memory_type': mem_type,
                    'start_address': start_addr
                }
                return True
            else:
                print("❌ No data in memory dump response")
                return False
        else:
            print(f"❌ Invalid memory dump response size: {len(payload)} bytes")
            return False
    
    def test_heartbeat(self) -> bool:
        """Test heartbeat functionality"""
        print("\n💓 Testing Heartbeat...")
        
        if not self.handshake_complete:
            print("❌ Handshake required before heartbeat testing")
            return False
        
        # Send heartbeat
        if not self.send_message(MessageType.Heartbeat, b""):
            return False
        
        print("✅ Heartbeat sent successfully")
        self.test_results['heartbeat'] = {'success': True}
        return True
    
    def run_integration_tests(self) -> bool:
        """Run complete integration test suite"""
        print("🧪 Starting DiztinGUIsh Integration Tests")
        print("=" * 50)
        
        success = True
        
        # Phase 1: Connection and Handshake
        if not self.connect():
            return False
        
        if not self.perform_handshake():
            success = False
        
        # Phase 2: Protocol Tests
        if success and not self.test_cpu_state_streaming():
            success = False
        
        if success and not self.test_breakpoint_control():
            success = False
        
        if success and not self.test_memory_dump():
            success = False
        
        if success and not self.test_heartbeat():
            success = False
        
        # Summary
        print("\n📊 Test Results Summary")
        print("=" * 30)
        print(f"Messages sent: {self.messages_sent}")
        print(f"Messages received: {self.messages_received}")
        
        for test_name, result in self.test_results.items():
            status = "✅ PASS" if result.get('success', False) else "❌ FAIL"
            print(f"{test_name}: {status}")
        
        overall_result = "✅ ALL TESTS PASSED" if success else "❌ SOME TESTS FAILED"
        print(f"\nOverall Result: {overall_result}")
        
        self.disconnect()
        return success


def main():
    parser = argparse.ArgumentParser(description="DiztinGUIsh Protocol Test Client")
    parser.add_argument("--host", default="127.0.0.1", help="Server host (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=9998, help="Server port (default: 9998)")
    parser.add_argument("--test", choices=['all', 'connect', 'cpu', 'breakpoints', 'memory'], 
                       default='all', help="Test to run (default: all)")
    
    args = parser.parse_args()
    
    client = DiztinguishTestClient(args.host, args.port)
    
    try:
        if args.test == 'all':
            return client.run_integration_tests()
        elif args.test == 'connect':
            return client.connect() and client.perform_handshake()
        elif args.test == 'cpu':
            return (client.connect() and client.perform_handshake() and 
                   client.test_cpu_state_streaming())
        elif args.test == 'breakpoints':
            return (client.connect() and client.perform_handshake() and 
                   client.test_breakpoint_control())
        elif args.test == 'memory':
            return (client.connect() and client.perform_handshake() and 
                   client.test_memory_dump())
    except KeyboardInterrupt:
        print("\n🛑 Test interrupted by user")
        client.disconnect()
        return False
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        client.disconnect()
        return False


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)