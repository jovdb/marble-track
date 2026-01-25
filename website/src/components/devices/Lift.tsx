import { Device } from "./Device";
import { createMemo, createSignal, createEffect } from "solid-js";
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

  const [lastPosition, setLastPosition] = createSignal(0);
  const [animationStart, setAnimationStart] = createSignal<{
    startPos: number;
    targetPos: number;
    startTime: number;
  } | null>(null);
  const [animationProgress, setAnimationProgress] = createSignal(0);

  createEffect(() => {
    const anim = animationStart();
    if (anim) {
      const speedScaleFactor = 0.6; // Try to more correct to reality
      const distance = Math.abs(anim.targetPos - anim.startPos);
      const stepper = stepperDevice;
      const speed =
        (stepper ? ((stepper.config as { defaultSpeed?: number })?.defaultSpeed ?? 150) : 150) *
        speedScaleFactor;
      const acceleration =
        (stepper
          ? ((stepper.config as { defaultAcceleration?: number })?.defaultAcceleration ?? 50)
          : 50) * speedScaleFactor;

      // Calculate motion profile
      const accel = acceleration;
      const maxSpeed = speed;
      const d = distance;

      const t_accel = maxSpeed / accel;
      const d_accel = 0.5 * accel * t_accel * t_accel;
      const d_decel = d_accel;

      let t_constant = 0;
      let d_constant = d - d_accel - d_decel;
      let t_total = 0;

      if (d_constant > 0) {
        // Trapezoidal profile
        t_constant = d_constant / maxSpeed;
        t_total = t_accel + t_constant + t_accel;
      } else {
        // Triangular profile
        const v_max = Math.sqrt(accel * d);
        const t_accel_tri = v_max / accel;
        t_total = 2 * t_accel_tri;
        d_constant = 0;
      }

      const getPositionAtTime = (t: number): number => {
        if (t < t_accel) {
          // Accelerating
          return 0.5 * accel * t * t;
        } else if (d_constant > 0 && t < t_accel + t_constant) {
          // Constant speed
          return d_accel + maxSpeed * (t - t_accel);
        } else if (t < t_total) {
          // Decelerating
          const t_dec = t - (t_accel + t_constant);
          return d_accel + d_constant + maxSpeed * t_dec - 0.5 * accel * t_dec * t_dec;
        } else {
          return d;
        }
      };

      const interval = setInterval(() => {
        const elapsed = (Date.now() - anim.startTime) / 1000;
        const pos = getPositionAtTime(elapsed);
        const progress = Math.min(pos / d, 1);
        setAnimationProgress(progress);
        if (progress >= 1) {
          clearInterval(interval);
        }
      }, 16); // ~60fps

      // Cleanup
      return () => clearInterval(interval);
    } else {
      setAnimationProgress(0);
    }
  });

  createEffect(() => {
    const currentState = state()?.state;
    const currentPos = state()?.currentPosition;
    if (currentPos !== undefined) {
      setLastPosition(currentPos);
    }
    if (currentState === "MovingUp" || currentState === "MovingDown") {
      if (!animationStart()) {
        const startPos = lastPosition();
        const targetPos =
          currentState === "MovingUp" ? (config()?.maxSteps ?? 2255) : (config()?.minSteps ?? 0);
        setAnimationStart({ startPos, targetPos, startTime: Date.now() });
      }
    } else {
      setAnimationStart(null);
    }
  });

  const state = () => device()?.state;
  const config = () => device()?.config;
  const error = () =>
    state()?.errorMessage || (state()?.state === "Error" ? "Device is in error state" : undefined);

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
  /*
  const minSteps = () => config()?.minSteps ?? 0;
  const maxSteps = () => config()?.maxSteps ?? 1000;
*/

  const positionPercent = createMemo(() => {
    const anim = animationStart();
    if (anim) {
      const progress = animationProgress();
      const currentPos = anim.startPos + (anim.targetPos - anim.startPos) * progress;
      const maxSteps = config()?.maxSteps ?? 2255;
      return currentPos / maxSteps;
    }

    switch (state()?.state) {
      case "LiftDownLoading":
      case "LiftDown":
        return 0;
      case "LiftUp":
      case "LiftUpUnloading":
        return 1;
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
