export function getDevices() {
  return {
    type: "devices-list",
    devices: [
      {
        id: "led-1",
        name: "LED",
        type: "led",
      },
      {
        id: "button-1",
        name: "Button",
        type: "button",
      },
      {
        id: "buzzer-1",
        name: "Buzzer",
        type: "buzzer",
      },
      {
        id: "stepper-1",
        name: "Stepper",
        type: "stepper",
      },
      {
        id: "servo-1",
        name: "Servo",
        type: "servo",
      },
      {
        id: "wheel-1",
        name: "Wheel",
        type: "wheel",
        children: [
          {
            id: "wheel-1-stepper",
            name: "Wheel Stepper",
            type: "stepper",
          },
          {
            id: "wheel-1-button",
            name: "Wheel Reset",
            type: "button",
          },
        ],
      },
    ],
  };
}
