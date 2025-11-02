export function getDevices() {
  return {
    type: "devices-list",
    devices: [
      {
        id: "led-1",
        type: "led",
      },
      {
        id: "button-1",
        type: "button",
      },
      {
        id: "buzzer-1",
        type: "buzzer",
      },
      {
        id: "stepper-1",
        type: "stepper",
      },
      {
        id: "wheel-1",
        type: "wheel",
        children: [
          {
            id: "wheel-1-stepper",
            type: "stepper",
          },
          {
            id: "wheel-1-button",
            type: "button",
          },
        ],
      },
    ],
  };
}
