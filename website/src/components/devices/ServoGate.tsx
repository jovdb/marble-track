import { createMemo, For } from "solid-js";
import { Device } from "./Device";
import deviceStyles from "./Device.module.css";
import { useServoGate } from "../../stores/ServoGate";
import ServoGateConfig from "./ServoGateConfig";

type GateState = "Idle" | "WaitOpen" | "Opening" | "WaitClose" | "Closing" | "Between";

const STATE_LABELS: Record<GateState, string> = {
  Idle: "Idle",
  WaitOpen: "Waiting to open…",
  Opening: "Opening",
  WaitClose: "Open – holding…",
  Closing: "Closing",
  Between: "Between cycles…",
};

const STATE_COLOR: Record<GateState, string> = {
  Idle: "#aaa",
  WaitOpen: "#f0a030",
  Opening: "#4caf50",
  WaitClose: "#4caf50",
  Closing: "#f44336",
  Between: "#f0a030",
};

export function ServoGate(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const servoGateStore = useServoGate(props.id);
  const device = () => servoGateStore[0];
  const actions = servoGateStore[1];

  const state = () => device()?.state;
  const config = () => device()?.config;

  const gateState = createMemo<GateState>(() => (state()?.gateState as GateState) ?? "Idle");
  const queueCount = createMemo(() => state()?.queueCount ?? 0);
  const pulseCount = createMemo(() => state()?.pulseCount ?? 0);
  const fullQueueCount = createMemo(() => config()?.fullQueueCount ?? 5);

  const isBusy = createMemo(() => gateState() !== "Idle");

  // Servo open fraction (0-1) for visual
  const openFraction = createMemo(() => {
    switch (gateState()) {
      case "Opening":
      case "WaitClose":
        return 1;
      case "Closing":
      case "WaitOpen":
      case "Between":
        return 0.5;
      default:
        return 0;
    }
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <ServoGateConfig id={props.id} onClose={onClose} />}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
      stateComponent={() => null}
    >
      {/* Gate SVG visualisation */}
      <svg viewBox="0 0 100 60" width="120" height="72" style={{ margin: "0 auto", display: "block" }}>
        {/* Track */}
        <line x1="10" y1="40" x2="90" y2="40" stroke="#888" stroke-width="3" stroke-linecap="round" />
        {/* Gate arm – rotates around pivot at (50,40) */}
        <g
          style={{
            "transform-origin": "50px 40px",
            transform: `rotate(${-openFraction() * 90}deg)`,
            transition: "transform 0.4s ease",
          }}
        >
          <line
            x1="50"
            y1="40"
            x2="50"
            y2="10"
            stroke={STATE_COLOR[gateState()]}
            stroke-width="4"
            stroke-linecap="round"
          />
          <circle cx="50" cy="10" r="4" fill={STATE_COLOR[gateState()]} />
        </g>
        {/* Pivot */}
        <circle cx="50" cy="40" r="5" fill="#555" />
      </svg>

      {/* Status row */}
      <div class={deviceStyles.device__status}>
        <div
          style={{
            width: "10px",
            height: "10px",
            "border-radius": "50%",
            background: STATE_COLOR[gateState()],
            display: "inline-block",
            "margin-right": "0.5em",
          }}
        />
        <span class={deviceStyles["device__status-text"]}>
          {STATE_LABELS[gateState()]}
        </span>
      </div>

      {/* Queue bar */}
      <div style={{ padding: "0.25em 0.5em" }}>
        <div style={{ display: "flex", gap: "4px", "align-items": "center" }}>
          <span style={{ "font-size": "0.8em", color: "#666", "min-width": "4em" }}>
            Queue: {queueCount()}/{fullQueueCount()}
          </span>
          <div style={{ flex: 1, display: "flex", gap: "3px" }}>
            <For each={Array.from({ length: fullQueueCount() }, (_, i) => i)}>
              {(i) => (
                <div
                  style={{
                    flex: 1,
                    height: "8px",
                    "border-radius": "3px",
                    background: i < queueCount() ? "#4caf50" : "#ddd",
                  }}
                />
              )}
            </For>
          </div>
        </div>
        <div style={{ "font-size": "0.8em", color: "#666", "margin-top": "2px" }}>
          Total pulses: {pulseCount()}
        </div>
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
        <button
          class={deviceStyles.device__button}
          onClick={() => actions.reset()}
          disabled={!isBusy() && queueCount() === 0}
          title="Reset queue and return servo to closed position"
        >
          Reset
        </button>
      </div>
    </Device>
  );
}
