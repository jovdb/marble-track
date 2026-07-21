import { createMemo } from "solid-js";
import { Device } from "./Device";
import { AccelerometerIcon } from "../icons/Icons";
import { useAdxl345 } from "../../stores/Adxl345";
import Adxl345Config from "./Adxl345Config";
import styles from "./Device.module.css";

export function Adxl345(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, { refresh }] = useAdxl345(props.id);

  const state = createMemo(() => device()?.state);
  const x = createMemo(() => state()?.x ?? 0);
  const y = createMemo(() => state()?.y ?? 0);
  const z = createMemo(() => state()?.z ?? 0);
  const roll = createMemo(() => state()?.roll ?? 0);
  const pitch = createMemo(() => state()?.pitch ?? 0);
  const status = createMemo(() => state()?.status);

  const statusText = createMemo(() => {
    const s = status();
    if (s === "Ready") return <span style={{ color: "green" }}>Ready</span>;
    if (s === "Error") return <span style={{ color: "red" }}>Error</span>;
    return <span>—</span>;
  });

  // Calculate bubble position (max 15 degrees for visualization)
  const bubblePos = createMemo(() => {
    const maxDeg = 15;
    const bx = Math.max(-1, Math.min(1, roll() / maxDeg)) * 50;
    const by = Math.max(-1, Math.min(1, -pitch() / maxDeg)) * 50;
    return { x: 50 + bx, y: 50 + by };
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <Adxl345Config id={props.id} onClose={onClose} />}
      icon={<AccelerometerIcon width={24} height={24} />}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div style={{ padding: "0 0.5rem 0.5rem 0.5rem", "font-size": "0.9rem" }}>
        <div
          style={{ display: "flex", "justify-content": "space-between", "align-items": "center" }}
        >
          <div>
            <strong>Status:</strong> {statusText()}
          </div>
          <div style={{ "font-size": "0.8rem", color: "#666" }}>Unit: m/s²</div>
        </div>

        {/* Spirit Level Visualization */}
        <div
          style={{
            margin: "0.5rem auto",
            width: "100px",
            height: "100px",
            border: "2px solid #ccc",
            "border-radius": "50%",
            position: "relative",
            background: "#f0f0f0",
            overflow: "hidden",
          }}
        >
          {/* Target Center */}
          <div
            style={{
              position: "absolute",
              top: "50%",
              left: "50%",
              width: "20px",
              height: "20px",
              border: "1px dashed #999",
              "border-radius": "50%",
              transform: "translate(-50%, -50%)",
            }}
          />
          {/* The Bubble */}
          <div
            style={{
              position: "absolute",
              top: `${bubblePos().y}%`,
              left: `${bubblePos().x}%`,
              width: "16px",
              height: "16px",
              background: "rgba(0, 255, 0, 0.6)",
              border: "1px solid green",
              "border-radius": "50%",
              transform: "translate(-50%, -50%)",
              transition: "all 0.1s ease-out",
            }}
          />
        </div>

        <div
          style={{
            "margin-top": "0.5rem",
            display: "grid",
            "grid-template-columns": "1fr 1fr",
            gap: "0.5rem",
          }}
        >
          <div
            style={{
              background: "rgba(0,0,0,0.05)",
              padding: "0.3rem",
              "border-radius": "4px",
              "text-align": "center",
            }}
          >
            <div style={{ "font-size": "0.7rem", color: "#666" }}>Roll</div>
            <div style={{ "font-weight": "bold", color: Math.abs(roll()) > 2 ? "red" : "inherit" }}>
              {roll().toFixed(1)}°
            </div>
          </div>
          <div
            style={{
              background: "rgba(0,0,0,0.05)",
              padding: "0.3rem",
              "border-radius": "4px",
              "text-align": "center",
            }}
          >
            <div style={{ "font-size": "0.7rem", color: "#666" }}>Pitch</div>
            <div
              style={{ "font-weight": "bold", color: Math.abs(pitch()) > 2 ? "red" : "inherit" }}
            >
              {pitch().toFixed(1)}°
            </div>
          </div>
        </div>

        <div
          style={{
            "margin-top": "0.5rem",
            display: "grid",
            "grid-template-columns": "1fr 1fr 1fr",
            gap: "0.3rem",
          }}
        >
          <div style={{ "font-size": "0.75rem", "text-align": "center" }}>
            X: <strong>{x().toFixed(2)}</strong>
          </div>
          <div style={{ "font-size": "0.75rem", "text-align": "center" }}>
            Y: <strong>{y().toFixed(2)}</strong>
          </div>
          <div style={{ "font-size": "0.75rem", "text-align": "center" }}>
            Z: <strong>{z().toFixed(2)}</strong>
          </div>
        </div>
      </div>
      <div class={styles.device__controls}>
        <button
          class={styles.device__button}
          onClick={() => refresh()}
          title="Request updated values"
        >
          Refresh
        </button>
      </div>
    </Device>
  );
}
