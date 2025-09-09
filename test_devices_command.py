import serial
import time

# Configure serial connection
port = 'COM12'
baudrate = 115200

try:
    # Open serial connection
    ser = serial.Serial(port, baudrate, timeout=1)
    print(f"Connected to {port} at {baudrate} baud")
    
    # Wait for ESP32 to be ready
    time.sleep(2)
    
    # Send "devices" command
    print("\nSending 'devices' command...")
    ser.write(b'devices\n')
    ser.flush()
    
    # Read response for 5 seconds
    print("Response:")
    print("=" * 50)
    
    start_time = time.time()
    while time.time() - start_time < 5:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(data, end='')
    
    print("\n" + "=" * 50)
    
    # Close connection
    ser.close()
    print("\nSerial connection closed.")
    
except Exception as e:
    print(f"Error: {e}")