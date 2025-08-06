import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { debounce } from "@solid-primitives/scheduled";
import { createSignal } from "solid-js";

interface IServoState {
  angle: number;
  targetAngle: number;
  speed: number;
  isMoving: boolean;
  pin: number;
  pwmChannel: number;
  name: string;
  type: string;
}

export function Servo(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IServoState>(props.id);
  const [currentSpeed, setCurrentSpeed] = createSignal(60); // Default speed in degrees/second

  const setAngle = debounce((angle: number) => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "setAngle",
        angle,
        speed: currentSpeed(),
      })
    );
  }, 100);

  const setSpeed = debounce((speed: number) => {
    setCurrentSpeed(speed);
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "setSpeed",
        speed,
      })
    );
  }, 300);

  const stopMovement = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "stop",
      })
    );
  };

  return (
    <fieldset>
      <legend>Servo: {deviceState()?.name || props.id}</legend>
      {disabled() && <span>{error() || connectedState()}</span>}
      {!disabled() && (
        <div>
          <div style={{ "margin-bottom": "15px" }}>
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
            <div style={{ display: "flex", "justify-content": "space-between", "align-items": "center" }}>
              <span>Current: {deviceState()?.angle || 0}°</span>
              {deviceState()?.isMoving && (
                <span style={{ color: "orange" }}>→ Target: {deviceState()?.targetAngle}°</span>
              )}
              {deviceState()?.isMoving && (
                <button onClick={stopMovement} style={{ "background-color": "#ff6b6b", color: "white" }}>
                  Stop
                </button>
              )}
            </div>
          </div>

          <div style={{ "margin-bottom": "15px" }}>
            <label for="speed">Speed (°/s):</label>
            <input
              id="speed"
              type="range"
              min="40"
              max="180"
              value={currentSpeed()}
              onInput={(e) => setSpeed(Number(e.currentTarget.value))}
              style={{ width: "100%" }}
            />
            <span>{currentSpeed()}°/s</span>
          </div>

          <div style={{ display: "flex", gap: "8px", "flex-wrap": "wrap" }}>
            <button onClick={() => setAngle(0)} disabled={disabled()}>
              0°
            </button>
            <button onClick={() => setAngle(45)} disabled={disabled()}>
              45°
            </button>
            <button onClick={() => setAngle(90)} disabled={disabled()}>
              90°
            </button>
            <button onClick={() => setAngle(135)} disabled={disabled()}>
              135°
            </button>
            <button onClick={() => setAngle(180)} disabled={disabled()}>
              180°
            </button>
          </div>

          <div style={{ "margin-top": "10px", display: "flex", gap: "8px", "flex-wrap": "wrap" }}>
            <button onClick={() => setSpeed(40)} disabled={disabled()}>
              Slow (40°/s)
            </button>
            <button onClick={() => setSpeed(60)} disabled={disabled()}>
              Medium (60°/s)
            </button>
            <button onClick={() => setSpeed(180)} disabled={disabled()}>
              Fast (180°/s)
            </button>
          </div>

          {deviceState() && (
            <div style={{ "margin-top": "10px", "font-size": "12px", color: "#666" }}>
              Pin: {deviceState()?.pin} | PWM Ch: {deviceState()?.pwmChannel} | 
              Speed: {deviceState()?.speed}°/s
            </div>
          )}
        </div>
      )}
    </fieldset>
  );
}
