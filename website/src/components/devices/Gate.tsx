import { Device } from "./Device";
import { createSignal, onCleanup } from "solid-js";
import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import styles from "./Device.module.css";
import { IDeviceState } from "../../stores/Device";
import { IWsSendMessage } from "../../interfaces/WebSockets";

interface IGateState extends IDeviceState {
  gateState: "Closed" | "IsOpening" | "Opened" | "Closing";
}

export function Gate(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IGateState>(props.id);
  const closedAngle = 0;
  const openedAngle = -110;
  const [angle, setAngle] = createSignal(closedAngle);
  const [gateState] = createSignal<"Closed" | "IsOpening" | "Opened" | "Closing">("Closed");

  const openGate = () => {
    sendMessage({
      type: "device-fn",
      deviceType: "gate",
      deviceId: props.id,
      fn: "open",
    } as IWsSendMessage);
  };

  // Animation logic (can be improved)
  let animationFrame: number | null = null;
  let lastTime: number | null = null;
  function animateGate(time: number) {
    if (lastTime === null) lastTime = time;
    const dt = (time - lastTime) / 1000;
    lastTime = time;
    const speed = 180;
    const currentState = gateState();
    if (currentState === "IsOpening" || currentState === "Opened") {
      setAngle((prev) => Math.max(prev - speed * dt, openedAngle));
    } else if (currentState === "Closing" || currentState === "Closed") {
      setAngle((prev) => Math.min(prev + speed * dt, closedAngle));
    }
    animationFrame = requestAnimationFrame(animateGate);
  }
  animationFrame = requestAnimationFrame(animateGate);
  onCleanup(() => {
    if (animationFrame) cancelAnimationFrame(animationFrame);
  });

  return (
    <Device id={props.id} deviceState={deviceState()}>
      <div>
        <svg viewBox="18 18 80 70">
          <g style="transform-origin: 50px 50px;" transform={`rotate(-3)`}>
            <g style="transform-origin: 50px 50px;" transform={`rotate(${angle()})`}>
              <line style="stroke: rgb(0, 0, 0);" x1="50" y1="50" x2="80" y2="50"></line>
              <line
                style="stroke: rgb(0, 0, 0); transform-origin: 50px 50px;"
                x1="50"
                y1="50"
                x2="39.739"
                y2="78.191"
              ></line>
              <path
                d="M 80 50 A 30 30 0 0 1 50 80 A 30 30 0 0 1 20 50 A 30 30 0 0 1 50 20 A 30 30 0 0 1 80 50 Z"
                style="fill: none;"
              ></path>
              <path
                d="M 80.5 50 C 80.5 58.386 77.059 66.075 71.567 71.567 C 66.075 77.059 58.386 80.5 50 80.5 C 41.614 80.5 33.925 77.059 28.433 71.567 C 22.941 66.075 19.5 58.386 19.5 50 C 19.5 41.614 22.941 33.925 28.433 28.433 C 33.925 22.941 41.614 19.5 50 19.5 C 58.386 19.5 66.075 22.941 71.567 28.433 C 77.059 33.925 80.5 41.614 80.5 50 Z M 70.86 29.14 C 65.494 23.774 58.182 20.5 50 20.5 C 41.818 20.5 34.506 23.774 29.14 29.14 C 23.774 34.506 20.5 41.818 20.5 50 C 20.5 58.182 23.774 65.494 29.14 70.86 C 34.506 76.226 41.818 79.5 50 79.5 C 58.182 79.5 65.494 76.226 70.86 70.86 C 76.226 65.494 79.5 58.182 79.5 50 C 79.5 41.818 76.226 34.506 70.86 29.14 Z"
                style=""
              ></path>
              <rect
                x="18.104"
                y="17.591"
                width="63.609"
                height="31.893"
                style="fill: rgb(255, 255, 255);"
              ></rect>
              <rect
                x="13.498"
                y="46.118"
                width="29.59"
                height="32.425"
                style="fill: rgb(255, 255, 255); transform-box: fill-box; transform-origin: 50% 50%;"
                transform="matrix(0.939693, 0.34202, -0.34202, 0.939693, 0.885934, -0.177167)"
              ></rect>
              <path
                style="stroke: rgb(0, 0, 0); fill: none;"
                d="M 80 49.962 L 78.33333333333333 49.99100000000001 C 76.66666666666667 50.02 73.33333333333333 50.078 71.66666666666667 48.41766666666667 C 70 46.75733333333333 70 43.37866666666667 70 41.68933333333333 L 70 40"
              ></path>
            </g>
            <line
              style="fill: rgb(216, 216, 216); stroke: rgb(0, 0, 0);"
              x1="82"
              y1="50"
              x2="102"
              y2="50"
            ></line>
            <path
              style="fill: rgb(216, 216, 216); stroke: rgb(0, 0, 0); transform-box: fill-box; transform-origin: 50% 50%;"
              d="M 32 35 L 12 35"
              transform="matrix(1, -0.000001, 0.000001, 1, 0, 0)"
            ></path>
          </g>
        </svg>
      </div>
      {disabled() && (
        <div class={styles.device__error}>
          {error() || (connectedState() === "Disconnected" ? "Disconnected" : connectedState())}
        </div>
      )}
      {!disabled() && (
        <>
          <div class={styles.device__status}>
            <div
              class={`${styles["device__status-indicator"]} ${deviceState()?.gateState === "Opened" ? styles["device__status-indicator--on"] : styles["device__status-indicator--off"]}`}
            ></div>
            <span class={styles["device__status-text"]}>
              Status: {deviceState()?.gateState || "Unknown"}
            </span>
          </div>
          <div class={styles.device__controls}>
            <button
              class={styles.device__button}
              onClick={openGate}
              disabled={disabled() || deviceState()?.gateState !== "Closed"}
            >
              Open
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
