import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { debounce } from "@solid-primitives/scheduled";
import { createSignal } from "solid-js";
import { ServoIcon } from "../icons/DeviceIcons";
import styles from "./Device.module.css";

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
  const [currentSpeed, setCurrentSpeed] = createSignal(60);

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
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>
          <ServoIcon />
          {deviceState()?.name || props.id}
        </h3>
        <span class={styles["device__type-badge"]}>SERVO</span>
      </div>
      
      <div class={styles.device__content}>
        {disabled() && (
          <div class={styles.device__error}>
            {error() || `Connection ${connectedState()}`}
          </div>
        )}
        
        {!disabled() && (
          <>
            <div class={styles.device__status}>
              <div class={`${styles["device__status-indicator"]} ${
                deviceState()?.isMoving 
                  ? styles["device__status-indicator--moving"] 
                  : styles["device__status-indicator--off"]
              }`}></div>
              <span class={styles["device__status-text"]}>
                {deviceState()?.isMoving 
                  ? `Moving to ${deviceState()?.targetAngle}°` 
                  : `At ${deviceState()?.angle || 0}°`
                }
              </span>
              {deviceState()?.isMoving && (
                <button 
                  class={`${styles.device__button} ${styles["device__button--danger"]}`}
                  onClick={stopMovement}
                  style={{ "margin-left": "auto" }}
                >
                  Stop
                </button>
              )}
            </div>

            <div class={styles["device__input-group"]}>
              <label class={styles.device__label} for={`angle-${props.id}`}>
                Angle: {deviceState()?.angle || 0}°
              </label>
              <input
                id={`angle-${props.id}`}
                class={styles.device__input}
                type="range"
                min="0"
                max="180"
                value={deviceState()?.angle || 90}
                onInput={(e) => setAngle(Number(e.currentTarget.value))}
              />
            </div>

            <div class={styles["device__input-group"]}>
              <label class={styles.device__label} for={`speed-${props.id}`}>
                Speed: {currentSpeed()}°/s
              </label>
              <input
                id={`speed-${props.id}`}
                class={styles.device__input}
                type="range"
                min="40"
                max="180"
                value={currentSpeed()}
                onInput={(e) => setSpeed(Number(e.currentTarget.value))}
              />
            </div>

            <div class={styles.device__controls}>
              <button 
                class={styles.device__button}
                onClick={() => setAngle(0)} 
                disabled={disabled()}
              >
                0°
              </button>
              <button 
                class={styles.device__button}
                onClick={() => setAngle(45)} 
                disabled={disabled()}
              >
                45°
              </button>
              <button 
                class={styles.device__button}
                onClick={() => setAngle(90)} 
                disabled={disabled()}
              >
                90°
              </button>
              <button 
                class={styles.device__button}
                onClick={() => setAngle(135)} 
                disabled={disabled()}
              >
                135°
              </button>
              <button 
                class={styles.device__button}
                onClick={() => setAngle(180)} 
                disabled={disabled()}
              >
                180°
              </button>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
