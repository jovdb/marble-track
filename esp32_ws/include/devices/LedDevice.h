/**
 * @file LedDevice.h
 * @brief LED device implementation using TaskDevice
 */

#ifndef LED_DEVICE_H
#define LED_DEVICE_H

#include "devices/TaskDevice.h"

class LedDevice : public TaskDevice {
public:
    LedDevice(const String& id);
    
    /**
     * @brief Setup the LED and start the task
     * @param pin GPIO pin number
     * @param name Name of the device (used for task name)
     * @return true if successful
     */
    bool setup(uint8_t pin, const String& name);

    /**
     * @brief Set LED to static ON or OFF
     * @param state true for ON, false for OFF
     */
    void set(bool state);

    /**
     * @brief Blink the LED
     * @param onTime Time in ms to stay ON
     * @param offTime Time in ms to stay OFF
     */
    void blink(unsigned long onTime, unsigned long offTime);

protected:
    void task() override;

private:
    uint8_t _pin;
    String _name;
    
    enum class Mode { OFF, ON, BLINKING };
    
    // Thread-safe communication variables
    volatile Mode _targetMode = Mode::OFF;
    volatile bool _targetState = false;
    volatile unsigned long _targetBlinkOnTime = 500;
    volatile unsigned long _targetBlinkOffTime = 500;

    // Internal state
    bool _isOn = false;
};

#endif // LED_DEVICE_H
