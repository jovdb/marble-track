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
  const status = createMemo(() => state()?.status);

  const statusText = createMemo(() => {
    const s = status();
    if (s === "Ready") return <span style={{ color: "green" }}>Ready</span>;
    if (s === "Error") return <span style={{ color: "red" }}>Error</span>;
    return <span>—</span>;
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
        <div>
          <strong>Status:</strong> {statusText()}
        </div>

        <div
          style={{
            "margin-top": "0.5rem",
            display: "grid",
            "grid-template-columns": "1fr 1fr 1fr",
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
            <div style={{ "font-size": "0.7rem", color: "#666" }}>X</div>
            <div style={{ "font-weight": "bold" }}>{x().toFixed(2)}</div>
          </div>
          <div
            style={{
              background: "rgba(0,0,0,0.05)",
              padding: "0.3rem",
              "border-radius": "4px",
              "text-align": "center",
            }}
          >
            <div style={{ "font-size": "0.7rem", color: "#666" }}>Y</div>
            <div style={{ "font-weight": "bold" }}>{y().toFixed(2)}</div>
          </div>
          <div
            style={{
              background: "rgba(0,0,0,0.05)",
              padding: "0.3rem",
              "border-radius": "4px",
              "text-align": "center",
            }}
          >
            <div style={{ "font-size": "0.7rem", color: "#666" }}>Z</div>
            <div style={{ "font-weight": "bold" }}>{z().toFixed(2)}</div>
          </div>
        </div>

        <div style={{ "margin-top": "0.5rem", "font-size": "0.8rem", color: "#666" }}>
          Unit: m/s²
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
