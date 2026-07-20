import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";
import { useI2c } from "../../stores/I2c";
import I2cConfig from "./I2cConfig";
import { createMemo, For } from "solid-js";

export function I2c(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, { scan }] = useI2c(props.id);

  const sdaPin = createMemo(() => (device()?.config?.sdaPin as number) ?? 21);
  const sclPin = createMemo(() => (device()?.config?.sclPin as number) ?? 22);
  const foundAddresses = createMemo(() => device()?.state?.foundAddresses ?? []);

  const icon = createMemo(() => {
    const type = device()?.type;
    return type ? getDeviceIcon(type, props.id) : null;
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <I2cConfig id={props.id} onClose={onClose} />}
      icon={icon()}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div style={{ padding: "0.5rem", "font-size": "0.9rem" }}>
        <div style={{ "margin-bottom": "0.5rem" }}>
          <strong>SDA Pin:</strong> {sdaPin()}
        </div>
        <div style={{ "margin-bottom": "0.5rem" }}>
          <strong>SCL Pin:</strong> {sclPin()}
        </div>

        <div style={{ "margin-top": "0.5rem" }}>
          <div style={{ display: "flex", "justify-content": "space-between", "align-items": "center", "margin-bottom": "0.25rem" }}>
            <strong>Found Addresses:</strong>
          <button
            onClick={() => scan()}
            style={{
              padding: "2px 8px",
              "font-size": "0.8rem",
              cursor: "pointer",
              background: "var(--color-surface)",
              color: "var(--color-text-primary)",
              border: "1px solid var(--color-border)",
              "border-radius": "var(--radius-sm, 4px)",
              width: "auto",
              margin: 0,
            }}
          >
            Scan
          </button>
        </div>
        <div
          style={{
            background: "rgba(0,0,0,0.05)",
            padding: "0.25rem",
            "border-radius": "var(--radius-sm, 4px)",
            "min-height": "1.5rem",
            display: "flex",
            "flex-wrap": "wrap",
            gap: "4px",
          }}
        >
          <For each={foundAddresses()} fallback={<span style={{ opacity: 0.5 }}>None found. Click Scan.</span>}>
            {(addr) => (
              <span
                style={{
                  background: "var(--color-primary-600)",
                  color: "white",
                  padding: "1px 6px",
                  "border-radius": "3px",
                  "font-family": "monospace",
                  "font-size": "0.8rem",
                }}
              >
                0x{addr.toString(16).toUpperCase().padStart(2, "0")}
              </span>
            )}
          </For>
          </div>
        </div>
      </div>
    </Device>
  );
}
