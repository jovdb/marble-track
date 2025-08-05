import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { debounce } from "@solid-primitives/scheduled";
interface IServoState {
  angle: number;
}

export function Servo(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] =
    createDeviceState<IServoState>(props.id);

  const setServoAngle = debounce((angle: number) => {
    // Send the message format expected by ESP32 WebSocketMessageHandler
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "setAngle",
        angle,
      })
    );
  }, 50);

  return (
    <fieldset>
      <legend>Servo: {props.id}</legend>
      {disabled() && <span>{error() || connectedState() + ""}</span>}
      {!disabled() && (
        <>
          <div>
            <label for="angle">Angle:</label>
            <input
              id="angle"
              type="range"
              min="0"
              max="179"
              value={deviceState()?.angle}
              onInput={(e) => setServoAngle(Number(e.currentTarget.value))}
            />
          </div>
        </>
      )}
    </fieldset>
  );
}
