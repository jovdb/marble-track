/**
 * ESP32 GPIO pin constants for the Marble Track system
 *
 * This file defines the available GPIO pins for device configuration.
 * Pins 35, 36, and 37 are excluded as they're internally used by the ESP32 firmware.
 */

// Available GPIO pins for device configuration (0-49, excluding 35, 36, 37)
export const ESP32_AVAILABLE_PINS = [
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 32, 33, 34, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49
];

// Reserved pins that should not be available for user configuration
export const ESP32_RESERVED_PINS = [35, 36, 37];

// All possible ESP32 GPIO pins (0-49)
export const ESP32_ALL_PINS = Array.from({ length: 50 }, (_, i) => i);

// Helper function to check if a pin is available
export function isPinAvailable(pin: number): boolean {
  return ESP32_AVAILABLE_PINS.includes(pin);
}

// Helper function to check if a pin is reserved
export function isPinReserved(pin: number): boolean {
  return ESP32_RESERVED_PINS.includes(pin);
}

// Helper function to get all used pins from devices, excluding a specific device
export function getUsedPins(devices: Record<string, any>, excludeDeviceId?: string): Map<number, string> {
  const usedPins = new Map<number, string>();

  Object.entries(devices).forEach(([deviceId, device]) => {
    if (excludeDeviceId && deviceId === excludeDeviceId) {
      return; // Skip the device being configured
    }

    if (device.pins && Array.isArray(device.pins)) {
      device.pins.forEach((pin: number) => {
        usedPins.set(pin, deviceId);
      });
    }
  });

  return usedPins;
}