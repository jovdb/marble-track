import { Device } from "./Device";
import { createMemo } from "solid-js";
import styles from "./Device.module.css";

import { LiftConfig } from "./LiftConfig";
import { useLift } from "../../stores/Lift";

export function Lift(props: { id: string }) {
  const liftStore = useLift(props.id);
  const device = () => liftStore[0];
  const actions = liftStore[1];

  const state = () => device()?.state;
  const config = () => device()?.config;
  // TODO: Handle error state - might need to be added to device state
  const error = () => undefined; // Placeholder until error handling is implemented

  const isMoving = createMemo(() => state()?.state === "MOVING");
  const currentPosition = () => state()?.currentPosition ?? 0;
  const minSteps = () => config()?.minSteps ?? 0;
  const maxSteps = () => config()?.maxSteps ?? 1000;

  function getStateString(state: string | undefined) {
    switch (state) {
      case "IDLE":
        return "Idle";
      case "MOVING":
        return "Moving...";
      case undefined:
        return "";
      default:
        return "Unknown";
    }
  }

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => (
        <LiftConfig device={device()} actions={actions} onClose={onClose} />
      )}
    >
      <div style={{ "max-width": "300px", margin: "0 auto", "text-align": "center" }}>
        {/* Lift visualization - simple bar */}
        <div
          style={{
            width: "50px",
            height: "200px",
            border: "2px solid currentColor",
            margin: "0 auto 1em",
            position: "relative",
            background: "currentColor",
            opacity: 0.1,
          }}
        >
          {/* Lift position indicator */}
          <div
            style={{
              position: "absolute",
              bottom: `${(currentPosition() / (maxSteps() - minSteps())) * 100}%`,
              left: "0",
              right: "0",
              height: "20px",
              background: "currentColor",
              border: "1px solid currentColor",
              transition: "bottom 0.3s ease",
            }}
          >
            <div
              style={{
                position: "absolute",
                top: "50%",
                left: "50%",
                transform: "translate(-50%, -50%)",
                color: "white",
                "font-size": "10px",
                "font-weight": "bold",
                "text-shadow": "0 0 2px black",
              }}
            >
              {currentPosition()}
            </div>
          </div>
        </div>

        {/* Position display */}
        <div style={{ "margin-bottom": "1em" }}>
          <div style={{ "font-size": "1.2em", "font-weight": "bold" }}>
            Position: {currentPosition()} steps
          </div>
          <div style={{ "font-size": "0.9em", opacity: 0.7 }}>
            Range: {minSteps()} - {maxSteps()}
          </div>
        </div>
      </div>

      {error() && <div class={styles.device__error}>{error()}</div>}
      {!error() && (
        <>
          <div class={styles.device__status}>
            <div>
              <span class={styles["device__status-text"]}>
                Status: {getStateString(state()?.state)}
              </span>
            </div>
          </div>
          <div class={styles.device__controls}>
            <button
              class={styles.device__button}
              onClick={() => actions.up()}
              disabled={isMoving()}
            >
              Up
            </button>
            <button
              class={styles.device__button}
              onClick={() => actions.down()}
              disabled={isMoving()}
            >
              Down
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
