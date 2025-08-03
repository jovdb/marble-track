import { sendMessage } from "../../hooks/useWebSocket";

export function Led() {
  const name = "Jo";
  const state = "on";
  const disabled = false;

  const setLed = (state: boolean) => {
    // Send the message format expected by ESP32 WebSocketMessageHandler
    sendMessage(
      JSON.stringify({
        action: "device-fn",
        deviceId: "test-led",
        fn: state ? "on" : "off",
      })
    );
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
