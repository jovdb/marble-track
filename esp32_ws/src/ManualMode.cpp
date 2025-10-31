#include "ManualMode.h"
#include "Logging.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Wheel.h"

ManualMode::ManualMode(DeviceManager &deviceManager) : deviceManager(deviceManager)
{
}

void ManualMode::setup()
{
    MLOG_INFO("ManualMode setup complete");
    // Add any initialization logic here that should run after all devices are set up
}

void ManualMode::loop()
{
    // Manual mode: devices are controlled via WebSocket commands
    // Check for button press to toggle LED
    Button *testButton = deviceManager.getDeviceByIdAs<Button>("test-button");
    Buzzer *testBuzzer = deviceManager.getDeviceByIdAs<Buzzer>("test-buzzer");
    Wheel *wheel = deviceManager.getDeviceByIdAs<Wheel>("wheel");
    Button *testButton2 = deviceManager.getDeviceByIdAs<Button>("test-button2");

    if (testButton && testBuzzer && wheel && testButton->isPressed())
    {
        MLOG_INFO("Button test-button pressed - triggering buzzer and wheel");
        testBuzzer->tone(200, 100);
        wheel->move(8000); // Move stepper 100 steps on button press
    }

    // Check for second button press to trigger buzzer
    if (testButton2 && testBuzzer && testButton2->wasPressed())
    {
        MLOG_INFO("Button test-button2 pressed - playing tone (1000Hz, 200ms)");
        testBuzzer->tone(1000, 200); // Play 1000Hz tone for 200ms
    }

    /*
    static unsigned long ballActionStart = 0;
    static int ballActionStep = 0;
    if (ballSensor.wasPressed())
    {
      Serial.println("Ball detected!");
      testLed.set(true);
      ballActionStart = millis();
      ballActionStep = 1;
    }
    // Non-blocking sequence for ball sensor actions
    if (ballActionStep == 1 && millis() - ballActionStart >= 350)
    {
      // testServo.setSpeed(80);
      testServo.setSpeed(240);
      testServo.setAngle(170);
      testBuzzer.tone(400, 200);
      ballActionStart = millis();
      ballActionStep = 2;
    }
    if (ballActionStep == 2 && millis() - ballActionStart >= 750)
    {
      // testServo.setSpeed(60);
      testServo.setSpeed(200);
      testServo.setAngle(28);
      ballActionStep = 0;
    }
    else if (ballSensor.wasReleased())
    {
      // Ball sensor released, perform action
      Serial.println("Ball released!");
      testLed.set(false); // Turn off LED
    }
    */

    // This is the default mode where users control devices through the web interface
}