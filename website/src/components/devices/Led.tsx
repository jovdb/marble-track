import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";

interface ILedState {
  mode: "ON" | "OFF";
  pin: number;
  name: string;
  type: string;
}

export function Led(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<ILedState>(props.id);

  const setLed = (state: boolean) => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id, // Use props.id instead of hardcoded "test-led"
        fn: state ? "on" : "off",
      })
    );
  };

  return (
    <fieldset>
      <legend>
        <legend>LED: {deviceState()?.name || props.id}</legend>
      </legend>
      {disabled() && <span>{error() || connectedState() + ""}</span>}
      {!disabled() && (
        <>
          <div
            style={{
              "margin-bottom": "10px",
              display: "flex",
              "align-items": "center",
              gap: "8px",
            }}
          >
            <svg width="20" height="20" viewBox="0 0 20 20">
              <circle cx="10" cy="10" r="8" fill="#d3d3d3" stroke="#999" stroke-width="1" />
              <circle
                cx="10"
                cy="10"
                r="6"
                fill={deviceState()?.mode === "ON" ? "#ff4444" : "#e0e0e0"}
              />
            </svg>
            <span>{deviceState()?.mode === "ON" ? "On" : "Off"}</span>
          </div>
          <button onClick={() => setLed(true)} disabled={disabled()}>
            On
          </button>
          <button onClick={() => setLed(false)} disabled={disabled()}>
            Off
          </button>
        </>
      )}
    </fieldset>
  );
}
