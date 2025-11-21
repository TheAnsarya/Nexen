#!/usr/bin/env python3
"""
Mesen2 DiztinGUIsh Server Simulator
Simulates the Mesen2 DiztinGUIsh protocol server for testing DiztinGUIsh integration.
"""
import socket
import struct
import threading
import time
import random
from typing import Optional

class MessageType:
    """Message types matching C++ DiztinguishProtocol.h"""
    Handshake = 0x01
    HandshakeAck = 0x02
    ConfigStream = 0x03
    Heartbeat = 0x04
    Disconnect = 0x05
    ExecTrace = 0x10
    ExecTraceBatch = 0x11
    MemoryAccess = 0x12
    CdlUpdate = 0x13
    CdlSnapshot = 0x14
    CpuState = 0x20
    CpuStateRequest = 0x21
    Error = 0xFF

class MesenSimulator:
    """Simulates Mesen2 DiztinGUIsh server behavior"""
    
    def __init__(self, port=9998):
        self.port = port
        self.server_socket: Optional[socket.socket] = None
        self.client_socket: Optional[socket.socket] = None
        self.running = False
        self.server_thread: Optional[threading.Thread] = None
        self.handshake_complete = False
        self.streaming = False
        
    def start_server(self):
        """Start the simulation server"""
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind(('localhost', self.port))
            self.server_socket.listen(1)
            self.running = True
            
            print(f"✅ Mesen2 Simulator started on port {self.port}")
            print("Waiting for DiztinGUIsh connection...")
            
            self.server_thread = threading.Thread(target=self._accept_connections)
            self.server_thread.start()
            return True
            
        except Exception as e:
            print(f"❌ Failed to start server: {e}")
            return False
    
    def stop_server(self):
        """Stop the simulation server"""
        self.running = False
        self.streaming = False
        
        if self.client_socket:
            try:
                self.client_socket.close()
            except:
                pass
            
        if self.server_socket:
            try:
                self.server_socket.close()
            except:
                pass
                
        if self.server_thread and self.server_thread.is_alive():
            self.server_thread.join(timeout=2)
            
        print("🛑 Server stopped")
    
    def _accept_connections(self):
        """Accept and handle client connections"""
        while self.running:
            try:
                self.client_socket, addr = self.server_socket.accept()
                print(f"✅ DiztinGUIsh connected from {addr}")
                
                # Send handshake immediately
                self._send_handshake()
                
                # Handle client messages
                self._handle_client()
                
            except Exception as e:
                if self.running:
                    print(f"❌ Connection error: {e}")
                break
    
    def _send_handshake(self):
        """Send handshake message to client"""
        try:
            # Create handshake payload (268 bytes total)
            # uint16_t protocolVersionMajor, uint16_t protocolVersionMinor
            # uint32_t romChecksum, uint32_t romSize, char[256] romName
            
            payload = struct.pack('<HHII', 1, 0, 0x12345678, 0x200000)  # 12 bytes so far
            rom_name = b"Simulated ROM for Testing\x00" + b"\x00" * (256 - 26)  # 256 bytes
            payload += rom_name  # Total: 12 + 256 = 268 bytes
            
            # Send message with header
            header = struct.pack('<BI', MessageType.Handshake, len(payload))
            full_message = header + payload
            
            self.client_socket.send(full_message)
            print("📤 Sent handshake to DiztinGUIsh")
            
        except Exception as e:
            print(f"❌ Failed to send handshake: {e}")
    
    def _handle_client(self):
        """Handle messages from client"""
        try:
            while self.running and self.client_socket:
                # Read message header (5 bytes)
                header_data = self._recv_exact(5)
                if not header_data:
                    break
                    
                msg_type = header_data[0]
                msg_length = struct.unpack('<I', header_data[1:5])[0]
                
                # Read payload
                payload = b''
                if msg_length > 0:
                    payload = self._recv_exact(msg_length)
                    if not payload:
                        break
                
                print(f"📥 Received message: Type=0x{msg_type:02X}, Length={msg_length}")
                
                # Process message
                if msg_type == MessageType.HandshakeAck:
                    self._handle_handshake_ack(payload)
                elif msg_type == MessageType.ConfigStream:
                    self._handle_config_stream(payload)
                elif msg_type == MessageType.CpuStateRequest:
                    self._send_cpu_state()
                elif msg_type == MessageType.Disconnect:
                    print("📤 Client requested disconnect")
                    break
                else:
                    print(f"❓ Unknown message type: 0x{msg_type:02X}")
                    
        except Exception as e:
            print(f"❌ Client handling error: {e}")
        finally:
            if self.client_socket:
                self.client_socket.close()
                self.client_socket = None
            self.handshake_complete = False
            self.streaming = False
            print("🔌 Client disconnected")
    
    def _recv_exact(self, count: int) -> Optional[bytes]:
        """Receive exactly count bytes"""
        data = b''
        while len(data) < count:
            chunk = self.client_socket.recv(count - len(data))
            if not chunk:
                return None
            data += chunk
        return data
    
    def _handle_handshake_ack(self, payload: bytes):
        """Handle handshake acknowledgment"""
        if len(payload) >= 5:
            major, minor, accepted = struct.unpack('<HHB', payload[:5])
            client_name = payload[5:69].rstrip(b'\x00').decode('ascii', errors='ignore')
            
            print(f"📥 HandshakeAck: Protocol {major}.{minor}, Accepted: {accepted}, Client: '{client_name}'")
            
            if accepted:
                self.handshake_complete = True
                print("✅ Handshake completed successfully!")
                
                # Start streaming simulation
                self._start_streaming()
            else:
                print("❌ Handshake rejected by client")
    
    def _handle_config_stream(self, payload: bytes):
        """Handle configuration stream message"""
        if len(payload) >= 6:
            enable_exec, enable_mem, enable_cdl, frame_interval, max_traces = struct.unpack('<BBBBB', payload[:6])
            print(f"📥 Config: ExecTrace={enable_exec}, Memory={enable_mem}, CDL={enable_cdl}, Interval={frame_interval}, MaxTraces={max_traces}")
            
            # If exec tracing enabled, start sending trace data
            if enable_exec:
                self.streaming = True
    
    def _start_streaming(self):
        """Start streaming simulated trace data"""
        def stream_worker():
            frame_number = 0
            while self.streaming and self.client_socket:
                try:
                    # Send ExecTraceBatch with simulated data
                    self._send_exec_trace_batch(frame_number)
                    frame_number += 1
                    time.sleep(0.1)  # 10 FPS simulation
                    
                except Exception as e:
                    print(f"❌ Streaming error: {e}")
                    break
        
        streaming_thread = threading.Thread(target=stream_worker)
        streaming_thread.daemon = True
        streaming_thread.start()
        print("🎬 Started streaming simulated trace data")
    
    def _send_exec_trace_batch(self, frame_number: int):
        """Send ExecTraceBatch message with simulated traces"""
        # Create batch header
        entry_count = random.randint(10, 50)  # Random number of entries
        batch_header = struct.pack('<IH', frame_number, entry_count)
        
        # Create trace entries (15 bytes each)
        entries = b''
        for i in range(entry_count):
            # Simulate SNES execution: random addresses, opcodes, flags
            pc = 0x8000 + random.randint(0, 0x7FFF)  # SNES ROM space
            opcode = random.choice([0xA9, 0x8D, 0xAD, 0x18, 0x38, 0xEA])  # Common opcodes
            m_flag = random.randint(0, 1)
            x_flag = random.randint(0, 1)
            db_register = 0x80
            dp_register = 0x0000
            effective_addr = 0x7E0000 + random.randint(0, 0x1FFF)  # SNES RAM
            
            entry = struct.pack('<IBBBBHI', pc, opcode, m_flag, x_flag, db_register, dp_register, effective_addr)
            entries += entry
        
        # Build complete message
        payload = batch_header + entries
        header = struct.pack('<BI', MessageType.ExecTraceBatch, len(payload))
        full_message = header + payload
        
        self.client_socket.send(full_message)
        print(f"📤 Sent ExecTraceBatch: Frame {frame_number}, {entry_count} entries")
    
    def _send_cpu_state(self):
        """Send CPU state message"""
        # Simulate SNES CPU state (17 bytes)
        pc = 0x8000 + random.randint(0, 0x7FFF)
        a_reg = random.randint(0, 0xFFFF)
        x_reg = random.randint(0, 0xFF)
        y_reg = random.randint(0, 0xFF)
        sp = 0x01FF
        flags = random.randint(0, 0xFF)
        db = 0x80
        dp = 0x0000
        emulation_mode = 0
        
        payload = struct.pack('<IHHHHBBBB', pc, a_reg, x_reg, y_reg, sp, flags, db, dp, emulation_mode)
        header = struct.pack('<BI', MessageType.CpuState, len(payload))
        full_message = header + payload
        
        self.client_socket.send(full_message)
        print("📤 Sent CPU state")

def main():
    """Main simulator entry point"""
    print("Mesen2 DiztinGUIsh Server Simulator")
    print("===================================")
    
    simulator = MesenSimulator(9998)
    
    try:
        success = simulator.start_server()
        if not success:
            return
        
        print("\nSimulator running! Commands:")
        print("  'status' - Show connection status")
        print("  'stop' - Stop server")
        print("  'quit' - Exit simulator")
        
        while True:
            try:
                cmd = input("\n> ").strip().lower()
                
                if cmd == 'status':
                    status = "Connected" if simulator.client_socket else "Waiting for connection"
                    streaming = "Streaming" if simulator.streaming else "Not streaming"
                    print(f"Status: {status}, {streaming}")
                    
                elif cmd in ['stop', 'quit', 'exit']:
                    break
                    
                elif cmd == 'help':
                    print("Available commands: status, stop, quit")
                    
                elif cmd:
                    print(f"Unknown command: {cmd}")
                    
            except KeyboardInterrupt:
                break
                
    except KeyboardInterrupt:
        pass
    finally:
        simulator.stop_server()
        print("\nSimulator shutdown complete")

if __name__ == "__main__":
    main()