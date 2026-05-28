import { createMemo } from "solid-js";
import { Device } from "./Device";
import { useLauncher, ILauncherState } from "../../stores/Launcher";
import LauncherConfig from "./LauncherConfig";
import { getDeviceIcon } from "../icons/Icons";
import styles from "./Launcher.module.css";

type LauncherStateLabel = ILauncherState["state"];

export function Launcher(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const launcherStore = useLauncher(props.id);
  const device = () => launcherStore[0];
  const actions = launcherStore[1];

  const deviceType = () => device()?.type;
  const state = () => device()?.state;

  const launcherState = createMemo<LauncherStateLabel>(
    () => (state()?.state as LauncherStateLabel) ?? "Init"
  );
  const isBallLoaded = createMemo(() => Boolean(state()?.isBallLoaded));
  const isBallWaiting = createMemo(() => Boolean(state()?.isBallWaiting));

  // Arm angle in degrees: 0° = horizontal right, negative = tilted up
  // DOWN: arm nearly horizontal (10°), UP: arm raised (~-75°)
  const armAngle = createMemo(() => {
    switch (launcherState()) {
      case "Up":
      case "MovingUp":
        return 95;
      case "Down":
      case "MovingDown":
      default:
        return -5;
    }
  });

  // Arm angle in degrees: 0° = horizontal right, negative = tilted up
  // DOWN: arm nearly horizontal (10°), UP: arm raised (~-75°)
  const armDuration = createMemo(() => {
    switch (launcherState()) {
      case "Up":
      case "MovingUp":
        return 200;
      case "Down":
      case "MovingDown":
      default:
        return 2000; // TODO: use config
    }
  });

  const isMoving = createMemo(
    () => launcherState() === "MovingUp" || launcherState() === "MovingDown"
  );

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <LauncherConfig id={props.id} onClose={onClose} />}
      isCollapsible={!props.isPopup}
      icon={deviceType() ? getDeviceIcon(deviceType()!, props.id) : null}
      onClose={props.onClose}
    >
      <div class={styles.launcher}>
        {/* Arm visualisation */}
        <div class={styles.launcher__visual}>
          <svg
            width={100}
            height={100}
            viewBox="0 0 48 48"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
            stroke-linejoin="round"
            xmlns="http://www.w3.org/2000/svg"
          >
            <g style={{ transform: "translateY(10px)" }}>
              <g
                style={{
                  transform: `rotate(${armAngle()}deg)`,
                  "transform-origin": "33.5px 25px",
                  "transition-duration": `${armDuration()}ms`,
                  "transition-timing-function": "linear",
                }}
              >
                <path
                  d="M 2 22 A 2 2 0 1 0 11 22 L 14 22 L 35 22 A 4 4 0 0 0 35 13 M 33.5
            22 L 33.5 25 L 31 30 A 5 5 0 0 0 38.5 20"
                />
                <circle
                  cx="6.5"
                  cy="22"
                  r="3.5"
                  fill={
                    isBallLoaded() && (state()?.state === "Down" || state()?.state === "MovingUp")
                      ? "blue"
                      : "none"
                  }
                  stroke="none"
                />
                <circle
                  cx="35"
                  cy="17.5"
                  r="3.5"
                  fill={isBallLoaded() && state()?.state === "MovingDown" ? "blue" : "none"}
                  stroke="none"
                />
              </g>
              <g
                style={{
                  transform: `rotate(95deg)`,
                  "transform-origin": "33.5px 25px",
                }}
              >
                <circle
                  cx="35"
                  cy="17.5"
                  r="3.5"
                  fill={isBallWaiting() ? "blue" : "none"}
                  stroke="none"
                />
              </g>
            </g>
          </svg>
        </div>

        {/* Status row */}
        <div class={styles.launcher__status}>
          <span class={styles["launcher__state-badge"]}>{launcherState()}</span>

          <span class={styles.launcher__indicator}>
            <span
              class={`${styles.launcher__dot} ${isBallWaiting() ? styles["launcher__dot--active-waiting"] : ""}`}
            />
            Waiting
          </span>

          <span class={styles.launcher__indicator}>
            <span
              class={`${styles.launcher__dot} ${isBallLoaded() ? styles["launcher__dot--active-loaded"] : ""}`}
            />
            Loaded
          </span>
        </div>

        {/* Controls */}
        <div class={styles.launcher__controls}>
          <button
            class={styles.launcher__button}
            onClick={() => actions.init()}
            title="Move arm to DOWN and assume ball is loaded"
          >
            Init
          </button>
          <button
            class={styles.launcher__button}
            onClick={() => actions.load()}
            title="Move arm UP then DOWN to load next ball"
          >
            Load
          </button>
          <button
            class={`${styles.launcher__button} ${styles["launcher__button--launch"]}`}
            onClick={() => actions.launch()}
            title="Swing arm UP fast to launch the ball"
          >
            Launch!
          </button>
        </div>
      </div>
    </Device>
  );
}
