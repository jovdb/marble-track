import { Device } from "./Device";
import { createEffect, createSignal, onCleanup, For, createMemo, Show } from "solid-js";
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

  let lastAnimationTime: number | null = null;

  const onNextClicked = () => {
    actions.nextBreakpoint();
  };

  let animationFrame: number | null = null;

  createEffect(() => {
    const angle = state()?.angle;
    if (angle !== undefined && angle !== null) setUiAngle(angle);
  });

  function animateWheel(time: number) {
    if (lastAnimationTime === null) lastAnimationTime = time;
    const dt = (time - lastAnimationTime) / 1000; // seconds
    lastAnimationTime = time;

    // TODO
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
          <g transform={`rotate(${- (uiAngle() ?? 0)})`} transform-origin={`${size / 2}px ${size / 2}px`}>
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

            {/* Zero degree reference line (fixed, doesn't rotate) */}
            <Show when={isCalibrated()}>
              <line
                x1={size / 2}
                y1={size / 2 - radius}
                x2={size / 2}
                y2={size / 2 - radius * 0.8}
                style={{ stroke: "currentColor", "stroke-width": "1" }}
              />
            </Show>
          </g>

          {/* Breakpoint indicators */}
          {/* Zero degree outer marker */}
          <line
            x1={size / 2}
            y1={size / 2 - radius}
            x2={size / 2}
            y2={size / 2 - radius * 1.15}
            style={{ stroke: "currentColor", opacity: 0.5, "stroke-width": "1" }}
          />
          {/* Zero degree label */}
          <text
            x={size / 2}
            y={size / 2 - radius * 1.25}
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
          <For each={breakpoints()}>
            {(breakpoint: number, index) => {
              const angleRad = - (breakpoint / 180) * Math.PI;
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
