import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";

interface ILedState {
  mode: "ON" | "OFF";
}

export function Led(props: { id: string }) {
  const deviceState = createDeviceState<ILedState>(props.id);
  const disabled = false; // deviceState() === undefined;

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
      <legend>Led: {props.id}</legend>
      <div style={{ "margin-bottom": "10px", display: "flex", "align-items": "center", gap: "8px" }}>
        <svg width="20" height="20" viewBox="0 0 20 20">
          <circle 
            cx="10" 
            cy="10" 
            r="8" 
            fill="#d3d3d3" 
            stroke="#999" 
            stroke-width="1"
          />
          <circle 
            cx="10" 
            cy="10" 
            r="6" 
            fill={deviceState()?.mode === "ON" ? "#ff4444" : "#e0e0e0"}
          />
        </svg>
        <span>{deviceState()?.mode === "ON" ? "On" : "Off"}</span>
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
