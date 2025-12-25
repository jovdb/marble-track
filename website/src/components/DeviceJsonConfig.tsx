import { createMemo, createSignal, Component } from "solid-js";
import { useWebSocket2 } from "../hooks/useWebSocket";

type Props = {
  deviceId: string;
  config?: unknown;
  onClose: () => void;
};

const DeviceJsonConfig: Component<Props> = (props) => {
  const [, { sendMessage }] = useWebSocket2();
  const [jsonText, setJsonText] = createSignal("");
  const [error, setError] = createSignal("");

  const formatted = () => {
    try {
      return JSON.stringify(props.config ?? {}, null, 2);
    } catch {
      return String(props.config);
    }
  };

  // Initialize jsonText when config changes
  createMemo(() => {
    setJsonText(formatted());
    setError("");
  });

  const handleSave = () => {
    try {
      const parsed = JSON.parse(jsonText());
      sendMessage({
        type: "device-save-config",
        deviceId: props.deviceId,
        config: parsed,
      });
      setError("");
      props.onClose();
    } catch (e) {
      setError(`Invalid JSON: ${(e as Error).message}`);
    }
  };

  const handleCancel = () => {
    setJsonText(formatted());
    setError("");
    props.onClose();
  };

  return (
    <div
      style={{
        display: "flex",
        "flex-direction": "column",
        gap: "0.75rem",
      }}
    >
      <label htmlFor={`config-textarea-${props.deviceId}`} style={{ "font-weight": "500" }}>
        Device Configuration (JSON)
      </label>
      <textarea
        id={`config-textarea-${props.deviceId}`}
        value={jsonText()}
        onInput={(e) => setJsonText(e.currentTarget.value)}
        style={{
          "font-family":
            "ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace",
          "font-size": "0.85rem",
          padding: "0.5rem",
          "border-radius": "6px",
          border: "1px solid #e1e4e8",
          "min-height": "200px",
          resize: "vertical",
          background: "#f6f8fa",
        }}
      />
      {error() && (
        <div
          style={{
            color: "#d1242f",
            background: "#ffeef0",
            padding: "0.5rem",
            "border-radius": "6px",
            border: "1px solid #d1242f",
            "font-size": "0.85rem",
          }}
          role="alert"
        >
          {error()}
        </div>
      )}
      <div style={{ display: "flex", gap: "0.5rem", "justify-content": "flex-end" }}>
        <button
          onClick={handleCancel}
          style={{
            padding: "0.5rem 1rem",
            "border-radius": "6px",
            border: "1px solid #e1e4e8",
            background: "#fff",
            cursor: "pointer",
            "font-size": "0.9rem",
          }}
        >
          Cancel
        </button>
        <button
          onClick={handleSave}
          disabled={!!error()}
          style={{
            padding: "0.5rem 1rem",
            "border-radius": "6px",
            border: "1px solid #1f883d",
            background: "#1f883d",
            color: "#fff",
            cursor: error() ? "not-allowed" : "pointer",
            "font-size": "0.9rem",
            opacity: error() ? "0.6" : "1",
          }}
        >
          Save
        </button>
      </div>
    </div>
  );
};

export default DeviceJsonConfig;
