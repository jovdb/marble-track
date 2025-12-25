import type { Component } from "solid-js";

type Props = {
  state?: unknown;
};

const DeviceJsonState: Component<Props> = (props) => {
  const formatted = () => {
    try {
      return JSON.stringify(props.state ?? {}, null, 2);
    } catch {
      return String(props.state);
    }
  };

  return (
    <pre
      aria-label="device-json-state"
      style={{
        "white-space": "pre-wrap",
        "font-family":
          "ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace",
        "font-size": "0.85rem",
        background: "#f6f8fa",
        padding: "0.5rem",
        "border-radius": "6px",
        border: "1px solid #e1e4e8",
        "margin-top": "0.5rem",
      }}
    >
      {formatted()}
    </pre>
  );
};

export default DeviceJsonState;
