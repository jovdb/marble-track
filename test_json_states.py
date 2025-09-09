import serial
import time
import sys

def test_devices_json():
    # Configure serial connection
    port = 'COM12'
    baudrate = 115200
    
    max_attempts = 3
    attempt = 0
    
    while attempt < max_attempts:
        try:
            print(f"Attempt {attempt + 1}: Trying to connect to {port}...")
            # Open serial connection
            ser = serial.Serial(port, baudrate, timeout=1)
            print(f"✅ Connected to {port} at {baudrate} baud")
            break
        except Exception as e:
            print(f"❌ Connection failed: {e}")
            if attempt < max_attempts - 1:
                print("Waiting 3 seconds before retry...")
                time.sleep(3)
            attempt += 1
    
    if attempt >= max_attempts:
        print("❌ Could not connect after all attempts. Monitor might be running.")
        print("💡 Try closing any open PlatformIO monitors or serial connections.")
        return
    
    try:
        # Wait for ESP32 to be ready
        time.sleep(2)
        
        print("\n" + "=" * 70)
        print("🔍 TESTING DEVICES COMMAND WITH JSON STATES")
        print("=" * 70)
        
        # Send "devices" command
        print("📤 Sending 'devices' command...")
        ser.write(b'devices\n')
        ser.flush()
        
        # Read response for 8 seconds (longer to get all JSON)
        print("\n📥 Response:")
        print("-" * 70)
        
        start_time = time.time()
        full_response = ""
        while time.time() - start_time < 8:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                print(data, end='')
                full_response += data
        
        print("\n" + "-" * 70)
        
        # Analyze the response
        if "JSON State:" in full_response:
            json_count = full_response.count("JSON State:")
            print(f"✅ Found {json_count} JSON state entries")
        else:
            print("⚠️  No 'JSON State:' found in response")
        
        if '{"' in full_response and '"}' in full_response:
            print("✅ JSON formatting detected in response")
        else:
            print("⚠️  No clear JSON objects found")
        
        print("=" * 70)
        
    except Exception as e:
        print(f"❌ Error during test: {e}")
    
    finally:
        # Close connection
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("\n🔌 Serial connection closed.")

if __name__ == "__main__":
    test_devices_json()