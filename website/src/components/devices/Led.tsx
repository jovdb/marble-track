import { webSocketStore } from "../../stores/websocketStore";

export function Led() {
  const name = "Jo";
  const state = "on";
  const disabled = false;

  const setLed = (state: boolean) => {
    // Send the message format expected by ESP32 WebSocketMessageHandler
    // The WebSocket wrapper will add timestamp and send as JSON
    webSocketStore.send({
      type: "device-command",
      data: {
        action: "device-fn",
        deviceId: "test-led",
        fn: state ? "on" : "off",
      },
    });
  };

  return (
    <fieldset>
      <legend>Led: {name}</legend>
      <div style={{ "margin-bottom": "10px" }}>
        {state === "on" ? "On" : "Off"}
      </div>
      <button onClick={() => setLed(true)} disabled={disabled}>
        On
      </button>
      <button onClick={() => setLed(false)} disabled={disabled}>
        Off
      </button>
    </fieldset>
  );
}
