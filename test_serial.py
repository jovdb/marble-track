#!/usr/bin/env python3
import serial
import time
import sys

def test_serial_connection():
    try:
        # Open serial connection
        ser = serial.Serial('COM12', 115200, timeout=1)
        print(f"Connected to {ser.name} at {ser.baudrate} baud")
        print("Listening for serial data... (Press Ctrl+C to exit)")
        print("=" * 50)
        
        # Try to reset the ESP32 by toggling DTR
        print("Attempting to reset ESP32...")
        ser.setDTR(False)
        time.sleep(0.1)
        ser.setDTR(True)
        time.sleep(0.1)
        ser.setDTR(False)
        
        start_time = time.time()
        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                print(data, end='')
                sys.stdout.flush()
            
            # Show activity indicator every 5 seconds
            if time.time() - start_time > 5:
                print(f"\n[{time.strftime('%H:%M:%S')}] Still listening...")
                start_time = time.time()
                
            time.sleep(0.1)
            
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Serial connection closed.")

if __name__ == "__main__":
    test_serial_connection()