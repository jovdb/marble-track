import serial
import time
import sys

def test_styled_json():
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
        
        print("\n" + "=" * 80)
        print("🎨 TESTING DEVICES COMMAND WITH STYLED JSON")
        print("=" * 80)
        
        # Send "devices" command
        print("📤 Sending 'devices' command...")
        ser.write(b'devices\n')
        ser.flush()
        
        # Read response for 10 seconds (longer for formatted JSON)
        print("\n📥 Response:")
        print("-" * 80)
        
        start_time = time.time()
        full_response = ""
        while time.time() - start_time < 10:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                print(data, end='')
                full_response += data
        
        print("\n" + "-" * 80)
        
        # Analyze the response for JSON formatting
        if "{" in full_response and "}" in full_response:
            brace_count = full_response.count("{")
            print(f"✅ Found {brace_count} JSON objects in response")
        
        if "  " in full_response or "\t" in full_response:
            print("✅ Indentation detected - JSON appears to be formatted")
        else:
            print("⚠️  No clear indentation found")
        
        newline_count = full_response.count('\n')
        print(f"📝 Response contains {newline_count} lines")
        
        print("=" * 80)
        
    except Exception as e:
        print(f"❌ Error during test: {e}")
    
    finally:
        # Close connection
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("\n🔌 Serial connection closed.")

if __name__ == "__main__":
    test_styled_json()