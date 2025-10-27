import { Device } from "./Device";
import {
  createEffect,
  createSignal,
  onCleanup,
  For,
  createMemo,
  Show,
  untrack,
} from "solid-js";
import styles from "./Device.module.css";
import { WheelConfig } from "./WheelConfig";
import { IWheelState, useWheel } from "../../stores/Wheel";

export function Wheel(props: { id: string }) {
  const wheelStore = useWheel(props.id);
  const device = () => wheelStore[0];
  const actions = wheelStore[1];

  const state = () => device()?.state;
  // TODO: Handle error state - might need to be added to device state
  const error = () => undefined; // Placeholder until error handling is implemented

  // TODO: Get child stepper position - need to access child devices
  const [uiAngle, setUiAngle] = createSignal<undefined | number>(undefined);

  const isCalibrated = createMemo(() => !!state()?.lastZeroPosition);
  const isSearchingZero = createMemo(() => !state()?.lastZeroPosition && state()?.state !== "IDLE");

  const onNextClicked = () => {
    actions.nextBreakpoint();
  };

  let animationFrame: number | null = null;
  let animationStartTime: number | null = null;

  type AnimationPlan = {
    startAngle: number;
    targetAngle: number;
    direction: 1 | -1;
    distance: number;
    acceleration: number;
    cruiseSpeed: number;
    accelDuration: number;
    cruiseDuration: number;
    decelDuration: number;
    accelDistance: number;
    cruiseDistance: number;
    totalDuration: number;
  };

  let currentAnimation: AnimationPlan | null = null;
  let activeMovementKey: string | null = null;
  let completedMovementKey: string | null = null;

  createEffect(() => {
    const angle = state()?.angle;
    if (angle !== undefined && angle !== null) setUiAngle(angle);
  });

  createEffect(() => {
    const deviceState = state();
    if (!deviceState) {
      currentAnimation = null;
      activeMovementKey = null;
      completedMovementKey = null;
      animationStartTime = null;
      return;
    }

    const status = deviceState.state;
    const targetAngle = deviceState.targetAngle;
    const speedRpm = deviceState.speedRpm;
    const accelerationRpm = deviceState.acceleration ?? 0;

    if (
      status !== "MOVING" ||
      targetAngle === undefined ||
      targetAngle === null ||
      speedRpm === undefined ||
      speedRpm === null ||
      speedRpm <= 0
    ) {
      currentAnimation = null;
      animationStartTime = null;
      activeMovementKey = null;
      if (status !== "MOVING") {
        completedMovementKey = null;
      }
      return;
    }

    const movementKey = `${targetAngle}:${speedRpm}:${accelerationRpm}`;

    if (movementKey === completedMovementKey) {
      return;
    }

    if (movementKey === activeMovementKey && currentAnimation) {
      return;
    }

    const startingAngle =
      deviceState.angle ?? untrack(() => uiAngle()) ?? targetAngle;

    const plan = createAnimationPlan(
      startingAngle,
      targetAngle,
      speedRpm,
      accelerationRpm,
    );

    if (!plan) {
      completedMovementKey = movementKey;
      currentAnimation = null;
      animationStartTime = null;
      setUiAngle(targetAngle);
      return;
    }

    currentAnimation = plan;
    activeMovementKey = movementKey;
    animationStartTime = null;
  });

  function createAnimationPlan(
    startAngle: number,
    targetAngle: number,
    speedRpm: number,
    accelerationRpm: number,
  ): AnimationPlan | null {
    if (!Number.isFinite(startAngle) || !Number.isFinite(targetAngle)) {
      return null;
    }

    const distanceRaw = targetAngle - startAngle;
    if (!Number.isFinite(distanceRaw) || Math.abs(distanceRaw) < 0.001) {
      return null;
    }

    const direction: 1 | -1 = distanceRaw >= 0 ? 1 : -1;
    const distance = Math.abs(distanceRaw);
    const cruiseSpeed = Math.abs(speedRpm) * 6; // rpm -> deg/s

    if (cruiseSpeed <= 0) {
      return null;
    }

    const acceleration = Math.abs(accelerationRpm) * 6; // rpm/s -> deg/s^2

    if (acceleration <= 0.0001) {
      const cruiseDuration = distance / cruiseSpeed;
      if (!Number.isFinite(cruiseDuration) || cruiseDuration <= 0) {
        return null;
      }
      return {
        startAngle,
        targetAngle,
        direction,
        distance,
        acceleration: 0,
        cruiseSpeed,
        accelDuration: 0,
        cruiseDuration,
        decelDuration: 0,
        accelDistance: 0,
        cruiseDistance: distance,
        totalDuration: cruiseDuration,
      };
    }

    let effectiveCruiseSpeed = cruiseSpeed;
    let accelDuration = effectiveCruiseSpeed / acceleration;
    let accelDistance = 0.5 * acceleration * accelDuration * accelDuration;
    let cruiseDistance = distance - 2 * accelDistance;
    let cruiseDuration = 0;

    if (cruiseDistance < 0) {
      effectiveCruiseSpeed = Math.sqrt(distance * acceleration);
      accelDuration = effectiveCruiseSpeed / acceleration;
      accelDistance = 0.5 * acceleration * accelDuration * accelDuration;
      cruiseDistance = 0;
      cruiseDuration = 0;
    } else {
      cruiseDuration = cruiseDistance / effectiveCruiseSpeed;
    }

    const decelDuration = accelDuration;
    const totalDuration = accelDuration + cruiseDuration + decelDuration;

    if (!Number.isFinite(totalDuration) || totalDuration <= 0) {
      return null;
    }

    return {
      startAngle,
      targetAngle,
      direction,
      distance,
      acceleration,
      cruiseSpeed: effectiveCruiseSpeed,
      accelDuration,
      cruiseDuration,
      decelDuration,
      accelDistance,
      cruiseDistance,
      totalDuration,
    };
  }

  function animateWheel(time: number) {
    const plan = currentAnimation;
    if (plan) {
      if (animationStartTime === null) {
        animationStartTime = time;
      }

      const elapsedSeconds = (time - animationStartTime) / 1000;
      const clampedElapsed = Math.min(elapsedSeconds, plan.totalDuration);

      let travelled = 0;

      if (plan.acceleration <= 0) {
        travelled = plan.cruiseSpeed * clampedElapsed;
      } else if (clampedElapsed <= plan.accelDuration) {
        travelled = 0.5 * plan.acceleration * clampedElapsed * clampedElapsed;
      } else if (clampedElapsed <= plan.accelDuration + plan.cruiseDuration) {
        const cruiseTime = clampedElapsed - plan.accelDuration;
        travelled = plan.accelDistance + plan.cruiseSpeed * cruiseTime;
      } else {
        const decelTime = clampedElapsed - plan.accelDuration - plan.cruiseDuration;
        const decelDistance =
          plan.cruiseSpeed * decelTime - 0.5 * plan.acceleration * decelTime * decelTime;
        travelled = plan.accelDistance + plan.cruiseDistance + decelDistance;
      }

      travelled = Math.min(plan.distance, travelled);
      const nextAngle = plan.startAngle + plan.direction * travelled;
      setUiAngle(nextAngle);

      if (elapsedSeconds >= plan.totalDuration - 0.0001) {
        setUiAngle(plan.targetAngle);
        completedMovementKey = activeMovementKey;
        currentAnimation = null;
        activeMovementKey = null;
        animationStartTime = null;
      }
    } else {
      animationStartTime = null;
    }

    animationFrame = requestAnimationFrame(animateWheel);
  }

  animationFrame = requestAnimationFrame(animateWheel);
  onCleanup(() => {
    if (animationFrame) cancelAnimationFrame(animationFrame);
  });

  const size = 100;
  const radius = size / 2 - 15; // Radius of the wheel in pixels - made even smaller

  // Get breakpoints from device config
  const breakpoints = () => device()?.config?.breakPoints || [];
  const zeroPointDegree = () => device()?.config?.zeroPointDegree || 0;

  function getStateString(state: IWheelState["state"] | undefined) {
    switch (state) {
      case "IDLE":
        return "Idle";
      case "CALIBRATING":
        return "Calibrating...";
      case "RESET":
        return "Resetting...";
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
        <WheelConfig device={device()} actions={actions} onClose={onClose} />
      )}
    >
      <div style={{ "max-width": "300px", margin: "0 auto" }}>
        <svg class={styles.svg} viewBox="0 0 100 100" style={{ "max-width": "300px" }}>
          <g
            transform={`rotate(${-(uiAngle() ?? 0)})`}
            transform-origin={`${size / 2}px ${size / 2}px`}
          >
            <circle
              cx={size / 2}
              cy={size / 2}
              r={radius}
              style={{ fill: "none", stroke: "currentColor", "stroke-width": "1" }}
            />

            {/* Question mark at center */}
            <Show when={!isCalibrated()}>
              <text
                x={size / 2}
                y={size / 2}
                text-anchor="middle"
                dominant-baseline="middle"
                style={{
                  "animation-name": isSearchingZero() ? "wheel-spin" : undefined,
                  fill: "currentColor",
                  opacity: 0.5,
                  "font-size": "16px",
                  "font-weight": "bold",
                  "font-family": "monospace",
                }}
              >
                ?
              </text>
            </Show>

            {/* Zero degree reference line (fixed, at zero point) */}
            <Show when={isCalibrated()}>
              {(() => {
                const angleRad = (-zeroPointDegree() / 180) * Math.PI;
                const innerRadius = radius;
                const outerRadius = radius * 0.8;
                return (
                  <line
                    x1={size / 2 + Math.sin(angleRad) * outerRadius}
                    y1={size / 2 - Math.cos(angleRad) * outerRadius}
                    x2={size / 2 + Math.sin(angleRad) * innerRadius}
                    y2={size / 2 - Math.cos(angleRad) * innerRadius}
                    style={{ stroke: "currentColor", "stroke-width": "1" }}
                  />
                );
              })()}
            </Show>
          </g>

          {/* Breakpoint indicators */}
          {/* Zero degree outer marker */}
          {(() => {
            const angleRad = (-zeroPointDegree() / 180) * Math.PI;
            const innerRadius = radius;
            const outerRadius = radius * 1.15;
            const textRadius = radius * 1.25;
            return (
              <g>
                <line
                  x1={size / 2 + Math.sin(angleRad) * innerRadius}
                  y1={size / 2 - Math.cos(angleRad) * innerRadius}
                  x2={size / 2 + Math.sin(angleRad) * outerRadius}
                  y2={size / 2 - Math.cos(angleRad) * outerRadius}
                  style={{ stroke: "currentColor", opacity: 0.5, "stroke-width": "1" }}
                />
                <text
                  x={size / 2 + Math.sin(angleRad) * textRadius}
                  y={size / 2 - Math.cos(angleRad) * textRadius}
                  text-anchor="middle"
                  dominant-baseline="middle"
                  style={{
                    fill: "currentColor",
                    opacity: 0.5,
                    "font-size": "6px",
                    "font-weight": "bold",
                    "font-family": "monospace",
                  }}
                >
                  0
                </text>
              </g>
            );
          })()}
          <For each={breakpoints()}>
            {(breakpoint: number, index) => {
              const angleRad = -((breakpoint + zeroPointDegree()) / 180) * Math.PI;
              const innerRadius = radius * 1.0; // Start at circle edge
              const outerRadius = radius * 1.15; // Extend outside the circle
              const textRadius = radius * 1.25; // Position text further out
              return (
                <g>
                  {/* Line marker */}
                  <line
                    x1={size / 2 + Math.sin(angleRad) * innerRadius}
                    y1={size / 2 - Math.cos(angleRad) * innerRadius}
                    x2={size / 2 + Math.sin(angleRad) * outerRadius}
                    y2={size / 2 - Math.cos(angleRad) * outerRadius}
                    style={{ stroke: "currentColor", "stroke-width": "1" }}
                  />
                  {/* Index number */}
                  <text
                    x={size / 2 + Math.sin(angleRad) * textRadius}
                    y={size / 2 - Math.cos(angleRad) * textRadius}
                    text-anchor="middle"
                    dominant-baseline="middle"
                    style={{
                      fill: "currentColor",
                      "font-size": "6px",
                      "font-weight": "bold",
                      "font-family": "monospace",
                    }}
                  >
                    {index() + 1}
                  </text>
                </g>
              );
            }}
          </For>
        </svg>
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
            {state()?.angle !== null && state()?.angle !== undefined && (
              <div>
                <span class={styles["device__status-text"]}>{state()?.angle?.toFixed(1)}°</span>
              </div>
            )}
            {state()?.targetAngle !== undefined &&
              state()?.targetAngle !== null &&
              state()?.targetAngle !== state()?.angle && (
                <div>
                  <span class={styles["device__status-text"]}>
                    -&gt; {state()?.targetAngle?.toFixed(1)}°
                  </span>
                </div>
              )}
          </div>
          <div class={styles.device__controls}>
            <button class={styles.device__button} onClick={() => actions.reset()}>
              Reset
            </button>
            <button
              class={styles.device__button}
              onClick={onNextClicked}
              disabled={state()?.lastZeroPosition === 0}
            >
              Next
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
