import serial
import time
import threading

def serial_reader(ser):
    """Read from serial port continuously"""
    while True:
        try:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                print(data, end='')
            time.sleep(0.1)
        except:
            break

def main():
    port = 'COM12'
    baudrate = 115200
    
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected to {port} at {baudrate} baud")
        
        # Start reader thread
        reader_thread = threading.Thread(target=serial_reader, args=(ser,), daemon=True)
        reader_thread.start()
        
        # Wait a moment for ESP32 to be ready
        time.sleep(2)
        
        print("\n" + "="*60)
        print("Testing devices command...")
        print("="*60)
        
        # Send devices command
        ser.write(b'devices\n')
        ser.flush()
        
        # Wait for response
        time.sleep(3)
        
        print("\n" + "="*60)
        print("Testing Enter key (network info)...")
        print("="*60)
        
        # Send enter key
        ser.write(b'\n')
        ser.flush()
        
        # Wait for response
        time.sleep(2)
        
        print("\n" + "="*60)
        print("Test complete!")
        print("="*60)
        
        ser.close()
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()