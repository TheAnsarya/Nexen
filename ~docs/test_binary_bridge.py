#!/usr/bin/env python3
"""
DiztinGUIsh Binary Bridge Test
Tests the BSNES-compatible 22-byte binary packets
"""
import socket
import struct
import time

# BSNES Binary packet format (22 bytes total)
# uint32 pc, uint8 op, uint8 a, uint8 x, uint8 y, uint8 sp, uint8 flags, 
# uint8 bank_db, uint8 bank_pb, uint8 e_flag, uint8 m_flag, uint8 x_flag,
# uint8 irq_flag, uint8 nmi_flag, uint32 padding

def test_binary_bridge_connection():
    """Test connection to binary bridge port"""
    print("=== BINARY BRIDGE CONNECTION TEST ===")
    
    # Try different ports that might be used for binary bridge
    ports_to_try = [9999, 10000, 9997, 9998]  # 9998 is protocol, others might be binary
    
    for port in ports_to_try:
        print(f"Trying port {port}...")
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(3)
            result = sock.connect_ex(("localhost", port))
            if result == 0:
                print(f"✅ Connected to port {port}!")
                
                # Wait for data (binary stream)
                data = sock.recv(1024)
                if data:
                    print(f"Received {len(data)} bytes immediately")
                    print(f"Data: {data.hex()}")
                    
                    # Check if it looks like 22-byte packets
                    if len(data) % 22 == 0:
                        print("✅ Data appears to be 22-byte packets!")
                        packets = len(data) // 22
                        print(f"Received {packets} complete packets")
                        
                        # Parse first packet
                        if packets > 0:
                            packet = data[:22]
                            pc, op, a, x, y, sp, flags = struct.unpack('<IBBBBBB', packet[:11])
                            bank_db, bank_pb, e_flag, m_flag, x_flag, irq_flag, nmi_flag = struct.unpack('BBBBBBB', packet[11:18])
                            padding = struct.unpack('<I', packet[18:22])[0]
                            
                            print(f"First packet:")
                            print(f"  PC: ${pc:06X}")
                            print(f"  Opcode: ${op:02X}")
                            print(f"  A: ${a:02X}, X: ${x:02X}, Y: ${y:02X}")
                            print(f"  SP: ${sp:02X}, Flags: ${flags:02X}")
                            print(f"  Banks: DB=${bank_db:02X}, PB=${bank_pb:02X}")
                            print(f"  E/M/X flags: {e_flag}/{m_flag}/{x_flag}")
                            print(f"  IRQ/NMI flags: {irq_flag}/{nmi_flag}")
                            
                    else:
                        print(f"? Data length ({len(data)}) not multiple of 22")
                        print("  Might not be binary bridge format")
                else:
                    print("No immediate data - might be protocol-based")
                
                sock.close()
                return port
            else:
                print(f"  Connection failed: {result}")
        except Exception as e:
            print(f"  Error: {e}")
    
    print("❌ No binary bridge found on any port")
    return None

def test_binary_streaming(port):
    """Test continuous streaming from binary bridge"""
    print(f"\n=== BINARY STREAMING TEST (Port {port}) ===")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)  
        sock.connect(("localhost", port))
        print("✅ Connected, monitoring stream...")
        
        packets_received = 0
        total_bytes = 0
        start_time = time.time()
        
        while packets_received < 10:  # Collect 10 packets or timeout
            data = sock.recv(1024)
            if not data:
                break
                
            total_bytes += len(data)
            new_packets = len(data) // 22
            packets_received += new_packets
            
            print(f"Received {new_packets} packets ({len(data)} bytes)")
            
            # Parse a sample packet
            if new_packets > 0 and packets_received <= 3:  # Show first 3 packets
                packet_start = 0
                packet = data[packet_start:packet_start+22]
                pc, op = struct.unpack('<IB', packet[:5])
                print(f"  Sample packet {packets_received}: PC=${pc:06X} OP=${op:02X}")
        
        elapsed = time.time() - start_time
        rate = packets_received / elapsed if elapsed > 0 else 0
        
        print(f"\n✅ Stream Summary:")
        print(f"  Packets: {packets_received}")
        print(f"  Bytes: {total_bytes}")
        print(f"  Time: {elapsed:.2f}s")
        print(f"  Rate: {rate:.1f} packets/sec")
        
        sock.close()
        return True
        
    except Exception as e:
        print(f"❌ Streaming test error: {e}")
        return False

def run_binary_tests():
    """Run all binary bridge tests"""
    print("DiztinGUIsh Binary Bridge Test")
    print("==============================")
    
    # Find binary bridge
    port = test_binary_bridge_connection()
    
    if port:
        # Test streaming
        test_binary_streaming(port)
    else:
        print("\n❌ Binary bridge not found")
        print("The binary bridge may not be implemented or enabled")
        print("Check if there's an option to enable BSNES compatibility")

if __name__ == "__main__":
    run_binary_tests()