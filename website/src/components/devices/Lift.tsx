import { Device } from "./Device";
import { createMemo, createSignal, onMount, createEffect } from "solid-js";
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
      state()?.state === "Init" || state()?.state === "MovingUp" || state()?.state === "MovingDown"
  );

  const isInError = createMemo(() => state()?.state === "Error");

  const canLoad = createMemo(() => state()?.state === "LiftDownUnloaded");

  const canUnload = createMemo(
    () => state()?.state === "LiftUp"
  );

  const canUp = createMemo(() => {
    const currentState = state()?.state;
    return (
      currentState === "LiftDownUnloaded" ||
      currentState === "LiftDownLoaded" ||
      currentState === "MovingDown"
    );
  });

  const canDown = createMemo(() => {
    const currentState = state()?.state;
    return (
      currentState === "LiftUp" ||
      currentState === "MovingUp"
    );
  });

  const canLoadOrUnload = createMemo(() => canLoad() || canUnload());

  const getLoadUnloadText = createMemo(() => {
    if (canUnload()) return "Unload";
    if (canLoad()) return "Load";
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
      state()?.state === "LiftUp" ||
      state()?.state === "MovingUp"
  );
  /*
  const minSteps = () => config()?.minSteps ?? 0;
  const maxSteps = () => config()?.maxSteps ?? 1000;
*/

  const positionPercent = createMemo(() => {
    switch (state()?.state) {
      case "LiftDownUnloaded":
      case "LiftDownLoaded":
      case "LiftDownLoading":
        return 0;
      case "LiftUp":
      case "LiftUpUnloading":
        return 1;
      case "MovingUp":
      case "MovingDown":
        // TODO: animate
        return 0.5;
      case "Init":
      case "Error":
      case "Unknown":
      default:
        return 0.5;
    }
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => (
        <LiftConfig device={device()} actions={actions} onClose={onClose} />
      )}
    >
      <svg viewBox="0 0 100 200" width="100" height="200" style="margin: 5 auto; display: block;">
        <line x1={50} y1={0} x2={50} y2={200} stroke="black" stroke-width={2}></line>
        <circle
          cx={75}
          cy={10}
          r={8}
          fill={
            state()?.state === "LiftUpUnloading" && state()?.isLoaded ? "#4444ff" : "transparent"
          }
        />
        <circle cx={75} cy={188} r={8} fill={state()?.isBallWaiting ? "#4444ff" : "transparent"} />

        <g transform={`translate(50, ${188 - positionPercent() * 180})`}>
          <circle
            cx={0}
            cy={0}
            r={8}
            fill={
              state()?.isLoaded && state()?.state !== "LiftUpUnloading" ? "#4444ff" : "transparent"
            }
          />
          <path d="M -10 0 A 10 10 0 0 0 10 0" fill="transparent" stroke="black" stroke-width={2} />
        </g>
      </svg>

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
