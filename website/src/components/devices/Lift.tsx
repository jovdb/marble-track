import { Device } from "./Device";
import { createMemo, createSignal, onMount, createEffect, onCleanup } from "solid-js";
import styles from "./Device.module.css";

import { LiftConfig } from "./LiftConfig";
import { useLift } from "../../stores/Lift";
import { useStepper } from "../../stores/Stepper";

export function Lift(props: { id: string }) {
  const liftStore = useLift(props.id);
  const device = () => liftStore[0];
  const actions = liftStore[1];
  const stepperDevice = () => useStepper(props.id + "-stepper")[0];

  const state = () => device()?.state;
  const error = () =>
    state()?.errorMessage || (state()?.state === "Error" ? "Device is in error state" : undefined);

  // Track movement start time for animation
  const [movementStartTime, setMovementStartTime] = createSignal<number | null>(null);

  // Track previous state to detect changes
  const [prevState, setPrevState] = createSignal<string | undefined>(undefined);

  // Animated position for smooth updates
  const [animatedPosition, setAnimatedPosition] = createSignal(0.5);

  // RAF animation loop
  let animationFrame: number | undefined;

  const animate = () => {
    const currentState = state()?.state;
    const startTime = movementStartTime();

    if ((currentState === "MovingUp" || currentState === "MovingDown") && startTime) {
      const elapsed = (Date.now() - startTime) / 1000; // seconds
      const steps = Math.abs(
        (device()?.config?.maxSteps ?? 1000) - (device()?.config?.minSteps ?? 0)
      );
      const speed = stepperDevice()?.config?.defaultSpeed ?? 500;
      const acceleration = stepperDevice()?.config?.defaultAcceleration ?? 500;

      // Calculate acceleration phase duration: t = v / a
      const accelerationDuration = speed / acceleration; // time to reach max speed

      // Distance covered during acceleration: s = 0.5 * a * t^2
      const accelerationDistance = 0.5 * acceleration * accelerationDuration * accelerationDuration;

      // Remaining distance at constant speed
      const remainingDistance = steps - accelerationDistance * 2;

      // Time at constant speed: t = s / v
      const constantSpeedDuration = remainingDistance / speed;
      // Total duration
      const duration = accelerationDuration * 2 + constantSpeedDuration; // seconds

      const progress = Math.min(elapsed / duration, 1);

      console.log(duration, progress);

      if (currentState === "MovingUp") {
        setAnimatedPosition(progress); // 0 to 1
      } else if (currentState === "MovingDown") {
        setAnimatedPosition(1 - progress); // 1 to 0
      }
    } else {
      // Not moving, snap to target position
      switch (currentState) {
        case "LiftDownUnloaded":
        case "LiftDownLoaded":
        case "LiftDownLoading":
          setAnimatedPosition(0);
          break;
        case "LiftUpUnloaded":
        case "LiftUpLoaded":
        case "LiftUpUnloading":
          setAnimatedPosition(1);
          break;
        default:
          setAnimatedPosition(0.5);
      }
    }

    animationFrame = requestAnimationFrame(animate);
  };

  onMount(() => {
    animate(); // Start the animation loop
  });

  onCleanup(() => {
    if (animationFrame) {
      cancelAnimationFrame(animationFrame);
    }
  });

  createEffect(() => {
    const currentState = state()?.state;
    const previousState = prevState();

    // Set movement start time when state changes to MovingUp or MovingDown
    if (
      (currentState === "MovingUp" || currentState === "MovingDown") &&
      previousState !== "MovingUp" &&
      previousState !== "MovingDown"
    ) {
      setMovementStartTime(Date.now());
    }

    setPrevState(currentState);
  });

  const isMoving = createMemo(
    () =>
      state()?.state === "Init" || state()?.state === "MovingUp" || state()?.state === "MovingDown"
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
    return (
      currentState === "LiftDownUnloaded" ||
      currentState === "LiftDownLoaded" ||
      currentState === "MovingDown"
    );
  });

  const canDown = createMemo(() => {
    const currentState = state()?.state;
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
  /*
  const minSteps = () => config()?.minSteps ?? 0;
  const maxSteps = () => config()?.maxSteps ?? 1000;
*/

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => (
        <LiftConfig device={device()} actions={actions} onClose={onClose} />
      )}
    >
      <svg viewBox="0 0 50 200" width="50" height="200" style="margin: 5 auto; display: block;">
        <line x1={25} y1={0} x2={25} y2={200} stroke="black" stroke-width={2}></line>
        <g transform={`translate(25, ${188 - animatedPosition() * 180})`}>
          <circle cx={0} cy={0} r={8} fill={state()?.isLoaded ? "#4444ff" : "transparent"} />
          <path d="M -10 0 A 10 10 0 0 0 10 0" fill="transparent" stroke="black" stroke-width={2} />
        </g>
        <circle cx={18} cy={190} r={8} fill={state()?.isBallWaiting ? "#4444ff" : "transparent"} />
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
