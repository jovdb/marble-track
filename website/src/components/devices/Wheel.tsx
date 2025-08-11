import { createSignal, onCleanup } from "solid-js";
import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import styles from "./Device.module.css";

interface IWheelState {
  position: number;
  name: string;
  type: string;
}

export function Wheel(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IWheelState>(props.id);
  const [steps, setSteps] = createSignal<undefined | number>(undefined);
  const [direction, setDirection] = createSignal(0);

  const [angle, setAngle] = createSignal<undefined | number>(undefined);

  const handleNext = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "goto",
        target: "next-breakpoint",
      })
    );
  };

  setDirection(1);
  
  const interval = setInterval(() => {
    if (direction() === 1) {
      setAngle((prev) => ((prev ?? 0) + 0.1) % 360);
    } else if (direction() === -1) {
      setAngle((prev) => ((prev ?? 0) - 0.1 + 360) % 360);
    }
  }, 1000 / 60);

  onCleanup(() => {
    clearInterval(interval);
  });

  const size = 100;
  const radius = size / 2 - 1; // Radius of the wheel in pixels
  const angle1 = 0;
  const angle2 = 120;

  const arrowAngle = 45 * direction(); // Angle for the arrow in degrees
  const arrowRadius = radius * 0.8; // Radius for the arrow path

  return (
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>{deviceState()?.name || props.id}</h3>
        <span class={styles["device__type-badge"]}>WHEEL</span>
      </div>
      <div class={styles.device__content}>
        <div style={{ "max-width": "300px", margin: "0 auto" }}>
          <svg class={styles.svg} viewBox="0 0 100 100" style={{ "max-width": "300px" }}>
            <g transform={`rotate(${angle()})`} transform-origin={`${size / 2}px ${size / 2}px`}>
              <circle
                cx={size / 2}
                cy={size / 2}
                r={radius}
                style={{ fill: "none", stroke: "currentColor", "stroke-width": 1 }}
              />

              {steps() !== undefined && (
                <>
                  <circle
                    cx={size / 2 + Math.sin((angle1 / 180) * Math.PI) * radius * 0.85}
                    cy={size / 2 + Math.cos((angle1 / 180) * Math.PI) * radius * 0.85}
                    r={size / 40}
                    style={{ fill: "none", stroke: "currentColor", "stroke-width": 1 }}
                  />

                  <circle
                    cx={size / 2 + Math.sin((angle2 / 180) * Math.PI) * radius * 0.85}
                    cy={size / 2 + Math.cos((angle2 / 180) * Math.PI) * radius * 0.85}
                    r={size / 40}
                    style={{ fill: "none", stroke: "currentColor", "stroke-width": 1 }}
                  />
                </>
              )}

              {!steps() && direction() !== 0 && (
                <g
                  transform={`rotate(${120 + arrowAngle})`}
                  transform-origin={`${size / 2}px ${size / 2}px`}
                >
                  <path
                    //d={`M ${size / 2} ${size / 2} L ${size / 2 + Math.sin((angle1 / 180) * Math.PI) * radius * 0.85} ${size / 2 + Math.cos((angle1 / 180) * Math.PI) * radius * 0.85}`}
                    d={`M ${size / 2 + Math.sin((0 / 180) * Math.PI) * arrowRadius} ${size / 2 + Math.cos((0 / 180) * Math.PI) * arrowRadius} A ${radius * 0.8} ${radius * 0.8} 0 0 ${arrowAngle < 0 ? 1 : 0} ${size / 2 + Math.sin((arrowAngle / 180) * Math.PI) * arrowRadius} ${size / 2 + Math.cos((arrowAngle / 180) * Math.PI) * arrowRadius}`}
                    style={{ stroke: "currentColor", "stroke-width": 1, fill: "none" }}
                  />
                  <line
                    x1={size / 2 + Math.sin((0 / 180) * Math.PI) * arrowRadius}
                    y1={size / 2 + Math.cos((0 / 180) * Math.PI) * arrowRadius}
                    x2={
                      size / 2 +
                      Math.sin((0 / 180) * Math.PI) * arrowRadius +
                      (arrowAngle > 0 ? 5 : -5)
                    }
                    y2={
                      size / 2 +
                      Math.cos((0 / 180) * Math.PI) * arrowRadius +
                      (arrowAngle > 0 ? 5 : -5)
                    }
                    style={{ stroke: "currentColor", "stroke-width": 1 }}
                  />
                  <line
                    x1={size / 2 + Math.sin((0 / 180) * Math.PI) * arrowRadius}
                    y1={size / 2 + Math.cos((0 / 180) * Math.PI) * arrowRadius}
                    x2={
                      size / 2 +
                      Math.sin((0 / 180) * Math.PI) * arrowRadius +
                      (arrowAngle > 0 ? 5 : -5)
                    }
                    y2={
                      size / 2 +
                      Math.cos((0 / 180) * Math.PI) * arrowRadius -
                      (arrowAngle > 0 ? 5 : -5)
                    }
                    style={{ stroke: "currentColor", "stroke-width": 1 }}
                  />
                </g>
              )}
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
              <span class={styles["device__status-text"]}>Position: {deviceState()?.position}</span>
            </div>
            <div class={styles.device__controls}>
              <button class={styles.device__button} onClick={handleNext} disabled={disabled()}>
                Next
              </button>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
