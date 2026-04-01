#!/usr/bin/env python3
"""
Simple test client for Mesen2 DiztinGUIsh streaming server.
Tests protocol handshake, execution traces, and CDL updates.

Usage: python test_mesen_stream.py [--port PORT] [--host HOST]
"""

import socket
import struct
import argparse
import sys
import time
from enum import IntEnum

class MessageType(IntEnum):
    """Message types from DiztinguishProtocol.h"""
    HANDSHAKE = 0x01
    CPU_STATE_REQUEST = 0x02
    CPU_STATE_RESPONSE = 0x03
    EXEC_TRACE = 0x04
    CDL_UPDATE = 0x05
    MEMORY_DUMP_REQUEST = 0x06
    MEMORY_DUMP_RESPONSE = 0x07
    LABEL_ADD = 0x08
    LABEL_UPDATE = 0x09
    LABEL_DELETE = 0x0A
    BREAKPOINT_ADD = 0x0B
    BREAKPOINT_REMOVE = 0x0C
    BREAKPOINT_HIT = 0x0D
    FRAME_START = 0x0E
    FRAME_END = 0x0F
    ERROR = 0xFF

class MesenStreamClient:
    """Client for testing Mesen2 DiztinGUIsh streaming protocol"""
    
    def __init__(self, host='127.0.0.1', port=9998):
        self.host = host
        self.port = port
        self.sock = None
        self.running = False
        
        # Statistics
        self.stats = {
            'handshakes': 0,
            'exec_traces': 0,
            'cdl_updates': 0,
            'frame_starts': 0,
            'frame_ends': 0,
            'errors': 0,
            'unknown': 0,
            'bytes_received': 0
        }
    
    def connect(self):
        """Connect to Mesen2 server"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(5.0)
            print(f"Connecting to {self.host}:{self.port}...")
            self.sock.connect((self.host, self.port))
            print("✅ Connected successfully")
            return True
        except Exception as e:
            print(f"❌ Connection failed: {e}")
            return False
    
    def read_message_header(self):
        """Read message type and size (5 bytes: type + uint32 size)"""
        try:
            header = self.sock.recv(5)
            if len(header) < 5:
                return None, None
            
            msg_type = header[0]
            msg_size = struct.unpack('<I', header[1:5])[0]
            self.stats['bytes_received'] += 5
            return msg_type, msg_size
        except socket.timeout:
            return None, None
        except Exception as e:
            print(f"❌ Error reading header: {e}")
            return None, None
    
    def read_message_body(self, size):
        """Read message body of specified size"""
        try:
            data = b''
            while len(data) < size:
                chunk = self.sock.recv(size - len(data))
                if not chunk:
                    break
                data += chunk
            self.stats['bytes_received'] += len(data)
            return data
        except Exception as e:
            print(f"❌ Error reading body: {e}")
            return None
    
    def parse_handshake(self, data):
        """Parse HANDSHAKE message: uint32 protocol_version, char[64] emulator_name"""
        if len(data) < 68:
            print(f"⚠️ Handshake too short: {len(data)} bytes")
            return
        
        protocol_ver = struct.unpack('<I', data[0:4])[0]
        emulator_name = data[4:68].decode('utf-8', errors='ignore').rstrip('\x00')
        
        print(f"📡 HANDSHAKE: Protocol v{protocol_ver}, Emulator: {emulator_name}")
        self.stats['handshakes'] += 1
    
    def parse_exec_trace(self, data):
        """Parse EXEC_TRACE: uint32 PC, uint8 opcode, bool M, bool X, uint8 DB, uint16 D, uint32 addr"""
        if len(data) < 13:
            print(f"⚠️ EXEC_TRACE too short: {len(data)} bytes")
            return
        
        pc, opcode, m_flag, x_flag, db, d, addr = struct.unpack('<IB??BHI', data[:13])
        
        # Only print first few and then sample
        if self.stats['exec_traces'] < 5 or self.stats['exec_traces'] % 1000 == 0:
            m_str = "M8" if m_flag else "M16"
            x_str = "X8" if x_flag else "X16"
            print(f"🔍 EXEC_TRACE #{self.stats['exec_traces']}: ${pc:06X} = ${opcode:02X} ({m_str} {x_str}) DB=${db:02X} D=${d:04X}")
        
        self.stats['exec_traces'] += 1
    
    def parse_cdl_update(self, data):
        """Parse CDL_UPDATE: uint32 rom_offset, uint8 flags"""
        if len(data) < 5:
            print(f"⚠️ CDL_UPDATE too short: {len(data)} bytes")
            return
        
        offset, flags = struct.unpack('<IB', data[:5])
        
        # Only print first few
        if self.stats['cdl_updates'] < 10:
            flag_str = []
            if flags & 0x01: flag_str.append("Code")
            if flags & 0x02: flag_str.append("Data")
            if flags & 0x04: flag_str.append("JumpTarget")
            if flags & 0x08: flag_str.append("SubEntryPoint")
            if flags & 0x10: flag_str.append("M8")
            if flags & 0x20: flag_str.append("X8")
            
            print(f"📝 CDL_UPDATE: ROM ${offset:06X} = {' | '.join(flag_str)} (0x{flags:02X})")
        
        self.stats['cdl_updates'] += 1
    
    def parse_frame_start(self, data):
        """Parse FRAME_START: uint32 frame_number"""
        if len(data) < 4:
            print(f"⚠️ FRAME_START too short: {len(data)} bytes")
            return
        
        frame_num = struct.unpack('<I', data[:4])[0]
        
        # Print every frame
        print(f"🎬 FRAME_START: Frame #{frame_num}")
        self.stats['frame_starts'] += 1
    
    def parse_frame_end(self, data):
        """Parse FRAME_END: (no payload)"""
        print(f"🎬 FRAME_END")
        self.stats['frame_ends'] += 1
    
    def process_message(self, msg_type, data):
        """Process a received message"""
        try:
            if msg_type == MessageType.HANDSHAKE:
                self.parse_handshake(data)
            elif msg_type == MessageType.EXEC_TRACE:
                self.parse_exec_trace(data)
            elif msg_type == MessageType.CDL_UPDATE:
                self.parse_cdl_update(data)
            elif msg_type == MessageType.FRAME_START:
                self.parse_frame_start(data)
            elif msg_type == MessageType.FRAME_END:
                self.parse_frame_end(data)
            elif msg_type == MessageType.ERROR:
                error_msg = data.decode('utf-8', errors='ignore').rstrip('\x00')
                print(f"❌ ERROR from server: {error_msg}")
                self.stats['errors'] += 1
            else:
                print(f"⚠️ Unknown message type: 0x{msg_type:02X} ({len(data)} bytes)")
                self.stats['unknown'] += 1
        except Exception as e:
            print(f"❌ Error processing message type 0x{msg_type:02X}: {e}")
    
    def run(self, duration=None):
        """Run client loop for specified duration (None = infinite)"""
        if not self.connect():
            return False
        
        self.running = True
        start_time = time.time()
        
        try:
            print(f"\n{'='*70}")
            print("📡 Listening for messages... (Ctrl+C to stop)")
            print(f"{'='*70}\n")
            
            while self.running:
                # Check duration
                if duration and (time.time() - start_time) > duration:
                    print(f"\n⏱️ Duration limit reached ({duration}s)")
                    break
                
                # Read message
                msg_type, msg_size = self.read_message_header()
                
                if msg_type is None:
                    continue
                
                # Read body
                data = self.read_message_body(msg_size)
                if data is None:
                    print("❌ Failed to read message body")
                    break
                
                # Process
                self.process_message(msg_type, data)
        
        except KeyboardInterrupt:
            print("\n\n⛔ Interrupted by user")
        except Exception as e:
            print(f"\n\n❌ Unexpected error: {e}")
        finally:
            self.cleanup()
    
    def cleanup(self):
        """Clean up connection and print statistics"""
        if self.sock:
            self.sock.close()
        
        elapsed = time.time() - start_time if 'start_time' in dir() else 0
        
        print(f"\n{'='*70}")
        print("📊 SESSION STATISTICS")
        print(f"{'='*70}")
        print(f"Duration:        {elapsed:.1f}s")
        print(f"Bytes Received:  {self.stats['bytes_received']:,} bytes ({self.stats['bytes_received']/1024:.1f} KB)")
        if elapsed > 0:
            print(f"Bandwidth:       {self.stats['bytes_received']/elapsed/1024:.2f} KB/s")
        print(f"\nMessages:")
        print(f"  Handshakes:    {self.stats['handshakes']}")
        print(f"  Exec Traces:   {self.stats['exec_traces']:,}")
        print(f"  CDL Updates:   {self.stats['cdl_updates']:,}")
        print(f"  Frame Starts:  {self.stats['frame_starts']}")
        print(f"  Frame Ends:    {self.stats['frame_ends']}")
        print(f"  Errors:        {self.stats['errors']}")
        print(f"  Unknown:       {self.stats['unknown']}")
        print(f"{'='*70}\n")

def main():
    parser = argparse.ArgumentParser(description='Test Mesen2 DiztinGUIsh streaming')
    parser.add_argument('--host', default='127.0.0.1', help='Server hostname (default: 127.0.0.1)')
    parser.add_argument('--port', type=int, default=9998, help='Server port (default: 9998)')
    parser.add_argument('--duration', type=int, help='Run for specified seconds (default: infinite)')
    args = parser.parse_args()
    
    print("=" * 70)
    print("Mesen2 DiztinGUIsh Streaming Test Client")
    print("=" * 70)
    print()
    
    client = MesenStreamClient(host=args.host, port=args.port)
    client.run(duration=args.duration)

if __name__ == '__main__':
    main()
