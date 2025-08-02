/**
 * @file ExampleUsage.cpp
 * @brief Example demonstrating how to use the IControllable struct interface
 * 
 * This file shows how the struct-based interface can be used for
 * polymorphic device control in your marble track system.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "IControllable.h"
#include "Led.h"

// Example function that works with any IControllable device
bool controlDevice(IControllable* device, const String& action, JsonObject& payload) {
    if (device == nullptr) {
        Serial.println("Error: Device is null");
        return false;
    }
    
    Serial.println("Controlling device [" + device->getId() + "] of type [" + device->getType() + "]");
    return device->control(action, payload);
}

// Example function to demonstrate polymorphic usage
void demonstrateInterface() {
    // Create LED device
    Led statusLed(2, "status_led", "Main Status LED");
    statusLed.setup();
    
    // Treat LED as IControllable interface
    IControllable* controllableDevice = &statusLed;
    
    // Create JSON payload
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    payload["state"] = true;
    
    // Control device through interface
    if (controlDevice(controllableDevice, "set", payload)) {
        Serial.println("Device controlled successfully!");
        Serial.println("Device ID: " + controllableDevice->getId());
    } else {
        Serial.println("Failed to control device");
    }
}

/*
Example usage in main.cpp:

#include "Led.h"
#include "IControllable.h"

void setup() {
    Serial.begin(115200);
    
    // Create devices
    Led led1(2, "led1", "Status LED");
    Led led2(3, "led2", "Error LED");
    
    // Setup devices
    led1.setup();
    led2.setup();
    
    // Use through interface
    IControllable* devices[] = {&led1, &led2};
    
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    payload["state"] = true;
    
    // Control all devices polymorphically
    for (IControllable* device : devices) {
        Serial.println("Controlling: " + device->getType() + " [" + device->getId() + "]");
        device->control("set", payload);
    }
}
*/
