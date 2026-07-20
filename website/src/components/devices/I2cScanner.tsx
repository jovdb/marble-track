import { For } from "solid-js";

interface I2cScannerProps {
  foundAddresses: number[];
  onScan: () => void;
}

export default function I2cScanner(props: I2cScannerProps) {
  return (
    <div style={{ "margin-top": "0.75rem", "border-top": "1px solid var(--color-border)", "padding-top": "0.75rem" }}>
      <div style={{ display: "flex", "justify-content": "space-between", "align-items": "center", "margin-bottom": "0.5rem" }}>
        <strong>I2C Bus Scan:</strong>
        <button
          type="button"
          onClick={() => props.onScan()}
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
        <For each={props.foundAddresses} fallback={<span style={{ opacity: 0.5, "font-size": "0.8rem" }}>None found. Click Scan.</span>}>
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
  );
}
