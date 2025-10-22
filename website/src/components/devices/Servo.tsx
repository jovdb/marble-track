import { Device } from "./Device";
import { debounce } from "@solid-primitives/scheduled";
import { createSignal } from "solid-js";
import deviceStyles from "./Device.module.css";
import servoStyles from "./Servo.module.css";
import { useServo } from "../../stores/Servo";
import { ServoIcon } from "../icons/Icons";

export function Servo(props: { id: string }) {
  const servoStore = useServo(props.id);
  const device = () => servoStore[0];
  const actions = servoStore[1];

  const state = () => device()?.state;
  // TODO: Handle error state - might need to be added to device state
  const error = () => undefined; // Placeholder until error handling is implemented
  const [currentSpeed, setCurrentSpeed] = createSignal(60);

  return (
    <Device id={props.id} icon={<ServoIcon />}>
      {error() && <div class={deviceStyles.device__error}>{error()}</div>}
      {!error() && (
        <>
          <div class={deviceStyles.device__status}>
            <div
              classList={{
                [servoStyles["servo__status-indicator"]]: true,
                [servoStyles["servo__status-indicator--moving"]]: Boolean(state()?.isMoving),
                [servoStyles["servo__status-indicator--off"]]: !state()?.isMoving,
              }}
            ></div>
            <span class={deviceStyles["device__status-text"]}>
              {state()?.isMoving
                ? `Moving to ${state()?.targetAngle}°`
                : `At ${state()?.angle || 0}°`}
            </span>
            {state()?.isMoving && (
              <button
                class={`${deviceStyles.device__button} ${deviceStyles["device__button--danger"]}`}
                onClick={() => {
                  actions.stop();
                }}
                style={{ "margin-left": "auto" }}
              >
                Stop
              </button>
            )}
          </div>
          <div class={deviceStyles["device__input-group"]}>
            <label class={deviceStyles.device__label} for={`angle-${props.id}`}>
              Angle: {state()?.angle || 0}°
            </label>
            <input
              id={`angle-${props.id}`}
              class={deviceStyles.device__input}
              type="range"
              min="0"
              max="180"
              value={state()?.angle || 90}
              onInput={(e) =>
                debounce(
                  () =>
                    actions.setAngle({
                      angle: Number(e.currentTarget.value),
                      speed: currentSpeed(),
                    }),
                  100
                )
              }
            />
            <div class={deviceStyles.device__controls}>
              <button
                class={deviceStyles.device__button}
                onClick={() => actions.setAngle({ angle: 0, speed: currentSpeed() })}
              >
                0°
              </button>
              <button
                class={deviceStyles.device__button}
                onClick={() => actions.setAngle({ angle: 45, speed: currentSpeed() })}
              >
                45°
              </button>
              <button
                class={deviceStyles.device__button}
                onClick={() => actions.setAngle({ angle: 90, speed: currentSpeed() })}
              >
                90°
              </button>
              <button
                class={deviceStyles.device__button}
                onClick={() => actions.setAngle({ angle: 135, speed: currentSpeed() })}
              >
                135°
              </button>
              <button
                class={deviceStyles.device__button}
                onClick={() => actions.setAngle({ angle: 180, speed: currentSpeed() })}
              >
                180°
              </button>
            </div>
          </div>
          <div class={deviceStyles["device__input-group"]}>
            <label class={deviceStyles.device__label} for={`speed-${props.id}`}>
              Speed: {currentSpeed()}°/s
            </label>
            <input
              id={`speed-${props.id}`}
              class={deviceStyles.device__input}
              type="range"
              min="40"
              max="180"
              value={currentSpeed()}
              onInput={(e) => setCurrentSpeed(Number(e.currentTarget.value))}
            />
          </div>
        </>
      )}
    </Device>
  );
}
