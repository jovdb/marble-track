import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { debounce } from "@solid-primitives/scheduled";

interface IServoState {
  angle: number;
}

export function Servo(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IServoState>(props.id);

  const setAngle = debounce((angle: number) => {
    sendMessage(JSON.stringify({
      type: "device-fn",
      deviceId: props.id,
      fn: "setAngle",
      angle,
    }));
  }, 100);

  return (
    <fieldset>
      <legend>Servo: {props.id}</legend>
      {disabled() && <span>{error() || connectedState()}</span>}
      {!disabled() && (
        <div>
          <label for="angle">Angle: {deviceState()?.angle}°</label>
          <input
            id="angle"
            type="range"
            min="0"
            max="180"
            value={deviceState()?.angle || 90}
            onInput={(e) => setAngle(Number(e.currentTarget.value))}
          />
        </div>
      )}
    </fieldset>
  );
}
