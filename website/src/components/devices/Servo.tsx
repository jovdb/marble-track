import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { debounce } from "@solid-primitives/scheduled";

interface IServoState {
  angle: number;
  pin: number;
  name: string;
  type: string;
}

export function Servo(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IServoState>(props.id);

  const setAngle = debounce((angle: number) => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "setAngle",
        angle,
      })
    );
  }, 100);

  return (
    <fieldset>
      <legend>Servo: {deviceState()?.name || props.id}</legend>
      {disabled() && <span>{error() || connectedState()}</span>}
      {!disabled() && (
        <div>
          <label for="angle">Angle:</label>
          <input
            id="angle"
            type="range"
            min="0"
            max="180"
            value={deviceState()?.angle || 90}
            onInput={(e) => setAngle(Number(e.currentTarget.value))}
            style={{ width: "100%" }}
          />
          {deviceState()?.angle}°
          <div style={{ "margin-top": "10px", display: "flex", gap: "8px" }}>
            <button onClick={() => setAngle(0)} disabled={disabled()}>
              0°
            </button>
            <button onClick={() => setAngle(90)} disabled={disabled()}>
              90°
            </button>
            <button onClick={() => setAngle(180)} disabled={disabled()}>
              180°
            </button>
          </div>
        </div>
      )}
    </fieldset>
  );
}
