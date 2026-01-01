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
  const error = () =>
    state()?.errorMessage || (state()?.state === "Error" ? "Device is in error state" : undefined);

  const isMoving = createMemo(
    () =>
      state()?.state === "Reset" || state()?.state === "MovingUp" || state()?.state === "MovingDown"
  );

  const isInError = createMemo(() => state()?.state === "Error");

  const canLoad = createMemo(
    () => state()?.state === "LiftDownUnloaded" || state()?.state === "LiftDownLoaded"
  );

  const canUnload = createMemo(
    () => state()?.state === "LiftUpLoaded" || state()?.state === "LiftUpUnloaded"
  );

  const canUp = createMemo(() => {
    const currentState = state()?.state;
    console.log("canUp", currentState);
    return (
      currentState === "LiftDownUnloaded" ||
      currentState === "LiftDownLoaded" ||
      currentState === "MovingDown"
    );
  });

  const canDown = createMemo(() => {
    const currentState = state()?.state;
    console.log("canDown", currentState);
    return (
      currentState === "LiftUpUnloaded" ||
      currentState === "LiftUpLoaded" ||
      currentState === "MovingUp"
    );
  });

  const canLoadOrUnload = createMemo(() => canLoad() || canUnload());

  const getLoadUnloadText = createMemo(() => {
    if (
      state()?.state === "LiftUpLoaded" ||
      state()?.state === "LiftUpUnloading" ||
      state()?.state === "LiftUpUnloaded" ||
      state()?.state === "MovingUp"
    )
      return "Unload";
    return "Load"; // fallback
  });

  const handleLoadUnload = () => {
    if (canUnload()) {
      actions.unloadBall();
    } else if (canLoad()) {
      actions.loadBall();
    }
  };

  const isLiftUp = createMemo(
    () =>
      state()?.state === "LiftUpLoaded" ||
      state()?.state === "LiftUpUnloaded" ||
      state()?.state === "LiftUpUnloading" ||
      state()?.state === "MovingUp"
  );

  const currentPosition = () => state()?.currentPosition ?? 0;
  const minSteps = () => config()?.minSteps ?? 0;
  const maxSteps = () => config()?.maxSteps ?? 1000;

  // Track lift direction toggle state
  function getStateString(state: string | undefined) {
    switch (state) {
      case "Unknown":
        return "Unknown";
      case "Reset":
        return "Resetting...";
      case "Error":
        return "Error";
      case "LiftDownLoading":
        return "Lift Down Loading";
      case "LiftDownLoaded":
        return "Lift Down Loaded";
      case "LiftUpUnloading":
        return "Lift Up Unloading";
      case "LiftUpUnloaded":
        return "Lift Up Unloaded";
      case "LiftUpLoaded":
        return "Lift Up Loaded";
      case "LiftDownUnloaded":
        return "Lift Down Unloaded";
      case "MovingUp":
        return "Moving Up";
      case "MovingDown":
        return "Moving Down";
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

      <div class={styles.device__controls}>
        <button class={styles.device__button} onClick={() => actions.init()} disabled={isMoving()}>
          Init
        </button>
        <button
          class={styles.device__button}
          onClick={handleLoadUnload}
          disabled={!canLoadOrUnload() || isInError()}
        >
          {getLoadUnloadText()}
        </button>
        <button
          class={styles.device__button}
          onClick={() => {
            if (isLiftUp()) {
              actions.down();
            } else {
              actions.up();
            }
          }}
          disabled={(isLiftUp() ? !canDown() : !canUp()) || isInError()}
        >
          {isLiftUp() ? "Down" : "Up"}
        </button>
      </div>
    </Device>
  );
}
