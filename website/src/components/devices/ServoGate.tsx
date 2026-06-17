import { createMemo } from "solid-js";
import { Device } from "./Device";
import deviceStyles from "./Device.module.css";
import { useServoGate } from "../../stores/ServoGate";
import ServoGateConfig from "./ServoGateConfig";
import { useServo } from "../../stores/Servo";
import { getDeviceIcon } from "../icons/Icons";

type GateState = "Idle" | "WaitOpen" | "Opening" | "WaitClose" | "Closing" | "Between";

export function ServoGate(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, actions] = useServoGate(props.id);

  const deviceType = device()?.type;
  const state = () => device()?.state;
  const config = () => device()?.config;

  const gateState = createMemo<GateState>(() => (state()?.gateState as GateState) ?? "Idle");
  const queueCount = createMemo(() => state()?.queueCount ?? 0);
  const fullQueueCount = createMemo(() => config()?.fullQueueCount ?? 5);

  const servoDeviceId = `${props.id}-servo`;
  // Get config of child servo
  const [servoState] = useServo(servoDeviceId);
  const servoDuration = createMemo(() => servoState()?.config?.defaultDurationInMs ?? 500);

  // Servo open fraction (0-1) for visual
  const openFraction = createMemo(() => {
    switch (gateState()) {
      case "WaitOpen":
      case "Opening":
      case "WaitClose":
        return 1;
      case "Closing":
      case "Between":
      default:
        return 0;
    }
  });

  const ballInWheel = createMemo(() => {
    switch (gateState()) {
      case "Between":
      case "WaitOpen":
      case "Opening":
        return !!state()?.queueCount;
      case "WaitClose":
      case "Closing":
      default:
        return false;
    }
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <ServoGateConfig id={props.id} onClose={onClose} />}
      isCollapsible={!props.isPopup}
      icon={deviceType ? getDeviceIcon(deviceType, props.id) : null}
      onClose={props.onClose}
    >
      {/* Gate SVG visualisation */}
      <div style={{ "text-align": "center", "margin-bottom": "10px" }}>
        <svg
          width="96"
          height="120"
          viewBox="0 0 48 60"
          fill="none"
          stroke="currentColor"
          stroke-width="1"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <g transform="translate(-0,-3)">
            <circle
              class="ball_2"
              cx="24"
              cy="41"
              r="2.5"
              fill="blue"
              stroke-width="0"
              style={{ opacity: queueCount() > 1 ? 2 : 0 }}
            />
            <circle
              class="ball_3"
              cx="24"
              cy="36"
              r="2.5"
              fill="blue"
              stroke-width="0"
              style={{ opacity: queueCount() > 2 ? 1 : 0 }}
            />
            <circle
              class="ball_4"
              cx="24"
              cy="31"
              r="2.5"
              fill="blue"
              stroke-width="0"
              style={{ opacity: queueCount() > 3 ? 1 : 0 }}
            />
            <circle
              class="ball_5"
              cx="24"
              cy="26"
              r="2.5"
              fill="blue"
              stroke-width="0"
              style={{ opacity: queueCount() > 4 ? 1 : 0 }}
            />
            <circle
              class="ball_6"
              cx="24"
              cy="21"
              r="2.5"
              fill="blue"
              stroke-width="0"
              style={{ opacity: queueCount() > 5 ? 1 : 0 }}
            />
            <circle
              class="ball_7"
              cx="24"
              cy="16"
              r="2.5"
              fill="blue"
              stroke-width="0"
              style={{ opacity: queueCount() > 6 ? 1 : 0 }}
            />
            <circle
              class="ball_8"
              cx="24"
              cy="11"
              r="2.5"
              fill="blue"
              stroke-width="0"
              style={{ opacity: queueCount() > 7 ? 1 : 0 }}
            />
            <path
              d="M 16 5 
           L 32 5
           L 27 11
           L 27 42 
           L 21 42 
           L 21 11 
           L 16 5"
            />
            <g
              style={{
                "transform-origin": "24px 50px",
                transition: `transform ${servoDuration()}ms linear`,
                transform: `rotate(${openFraction() * 110}deg)`,
              }}
            >
              <circle
                class="ball_1"
                cx="24"
                cy="46"
                r="2.5"
                fill="blue"
                stroke-width="0"
                style={{ opacity: ballInWheel() ? 1 : 0 }}
              />
              <path
                d="M 20 44 
           Q 20 48, 24 49 
           Q 28 48, 28 44 
           A 8 8 0 1 1 20 44 Z"
              />
              <path
                d="M 16 51
           L 32 51 
           M 24 49
           L 24 58.5"
              />
            </g>
          </g>
        </svg>
      </div>
      {/* Controls */}
      <div class={deviceStyles.device__controls}>
        <button
          class={deviceStyles.device__button}
          onClick={() => actions.trigger()}
          disabled={queueCount() >= fullQueueCount()}
          title="Add one cycle to the queue"
        >
          Trigger
        </button>
      </div>
    </Device>
  );
}
