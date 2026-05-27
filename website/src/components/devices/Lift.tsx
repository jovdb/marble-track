import { Device } from "./Device";
import { createMemo } from "solid-js";
import styles from "./Device.module.css";

import { LiftConfig } from "./LiftConfig";
import { useLift } from "../../stores/Lift";
import { useStepper } from "../../stores/Stepper";

export function Lift(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const liftStore = useLift(props.id);
  const device = () => liftStore[0];
  const actions = liftStore[1];

  const stepperId = `${props.id}-stepper`;
  const [stepperDevice] = useStepper(stepperId);

  const state = () => device()?.state;
  const config = () => device()?.config;

  // Compute total move duration (ms) from a trapezoidal motion profile.
  // Uses stepsPerSecond from state (set by firmware during movement) for the actual speed,
  // falling back to the stepper config defaultSpeed when not available.
  const transitionDurationMs = createMemo(() => {
    const maxSpeed =
      state()?.stepsPerSecond ??
      (stepperDevice?.config as { defaultSpeed?: number })?.defaultSpeed ??
      150; // steps/s
    const accel =
      (stepperDevice?.config as { defaultAcceleration?: number })?.defaultAcceleration ?? 50; // steps/s²
    const distance = (config()?.maxSteps ?? 2255) - (config()?.minSteps ?? 0);

    const t_accel = maxSpeed / accel;
    const d_accel = 0.5 * accel * t_accel * t_accel;

    let t_total: number;
    if (distance >= 2 * d_accel) {
      // Trapezoidal: accelerate, cruise, decelerate
      const t_constant = (distance - 2 * d_accel) / maxSpeed;
      t_total = 2 * t_accel + t_constant;
    } else {
      // Triangular: accelerate then immediately decelerate (never reaches maxSpeed)
      t_total = 2 * Math.sqrt(distance / accel);
    }

    return Math.round(t_total * 1000);
  });

  const isMoving = createMemo(
    () =>
      state()?.state === "Init" || state()?.state === "MovingUp" || state()?.state === "MovingDown"
  );

  const isInError = createMemo(() => state()?.state === "Error");

  const canLoad = createMemo(() => state()?.state === "LiftDown" && !state()?.isLoaded);

  const canUnload = createMemo(() => state()?.state === "LiftUp");

  const canUp = createMemo(() => {
    const currentState = state()?.state;
    return currentState === "LiftDown" || currentState === "MovingDown";
  });

  const canDown = createMemo(() => {
    const currentState = state()?.state;
    return currentState === "LiftUp" || currentState === "MovingUp";
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

  const isLiftUp = createMemo(() => state()?.state === "LiftUp" || state()?.state === "MovingUp");

  // 0 = bottom, 1 = top. CSS transition animates between these when moving.
  const positionPercent = createMemo(() => {
    switch (state()?.state) {
      case "LiftUp":
      case "LiftUpUnloading":
      case "MovingUp":
        return 1;
      default:
        return 0;
    }
  });

  const isAnimating = createMemo(
    () => state()?.state === "MovingUp" || state()?.state === "MovingDown"
  );

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => (
        <LiftConfig device={device()} actions={actions} onClose={onClose} />
      )}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
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

        <g
          style={{
            transform: `translate(50px, ${188 - positionPercent() * 180}px)`,
            transition: isAnimating()
              ? `transform ${transitionDurationMs() / 1000}s ease-in-out`
              : "transform 0s",
          }}
        >
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
