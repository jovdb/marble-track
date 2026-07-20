import { createSignal, createEffect, For } from "solid-js";

interface LogEntry {
  timestamp: string;
  type: "TX" | "RX";
  address: string;
  data: string;
  status: string;
}

export default function I2cTester(props: {
  address: number;
  onAddressChange: (addr: number) => void;
  onSend: (addr: number, data: string) => void;
  onRead: (addr: number, len: number) => void;
  lastOp?: any;
}) {
  const [writeData, setWriteData] = createSignal("");
  const [readLen, setReadLen] = createSignal(1);
  const [log, setLog] = createSignal<LogEntry[]>([]);
  let logRef: HTMLDivElement | undefined;

  createEffect(() => {
    if (props.lastOp && props.lastOp.timestamp > 0) {
      const entry: LogEntry = {
        timestamp: new Date().toLocaleTimeString(),
        type: props.lastOp.type === "write" ? "TX" : "RX",
        address: `0x${props.lastOp.address.toString(16).toUpperCase().padStart(2, "0")}`,
        data: props.lastOp.data || (props.lastOp.type === "read" ? "No Data" : ""),
        status: props.lastOp.status,
      };

      // Simple deduplication for state updates if necessary, or just append
      setLog((prev) => [...prev, entry].slice(-50)); // Keep last 50
    }
  });

  createEffect(() => {
    if (logRef) {
      logRef.scrollTop = logRef.scrollHeight;
    }
  });

  return (
    <div
      style={{
        "margin-top": "1rem",
        "border-top": "1px solid var(--color-border)",
        "padding-top": "0.75rem",
      }}
    >
      <div style={{ "margin-bottom": "0.5rem" }}>
        <strong>I2C Tester:</strong>
      </div>

      <div
        style={{
          display: "grid",
          "grid-template-columns": "80px 1fr",
          gap: "8px",
          "align-items": "center",
          "margin-bottom": "0.5rem",
        }}
      >
        <span style={{ "font-size": "0.8rem" }}>Address:</span>
        <input
          type="text"
          value={`0x${props.address.toString(16).toUpperCase().padStart(2, "0")}`}
          onInput={(e) => {
            const val = e.currentTarget.value.replace("0x", "");
            props.onAddressChange(parseInt(val, 16) || 0);
          }}
          style={{
            background: "var(--color-surface)",
            color: "var(--color-text-primary)",
            border: "1px solid var(--color-border)",
            padding: "2px 6px",
            "border-radius": "4px",
            width: "60px",
            "font-family": "monospace",
          }}
        />
      </div>

      {/* Write Row */}
      <div
        style={{
          display: "grid",
          "grid-template-columns": "80px 1fr auto",
          gap: "8px",
          "align-items": "center",
          "margin-bottom": "0.5rem",
        }}
      >
        <span style={{ "font-size": "0.8rem" }}>Data:</span>
        <input
          style={{
            flex: 1,
            background: "var(--color-surface)",
            color: "var(--color-text-primary)",
            border: "1px solid var(--color-border)",
            padding: "2px 6px",
            "border-radius": "4px",
          }}
          placeholder="Hex (e.g. 00 FF)"
          value={writeData()}
          onInput={(e) => setWriteData(e.currentTarget.value)}
        />
        <button onClick={() => props.onSend(props.address, writeData())}>Write</button>
      </div>

      {/* Read Row */}
      <div
        style={{
          display: "grid",
          "grid-template-columns": "80px 1fr auto",
          gap: "8px",
          "align-items": "center",
          "margin-bottom": "0.5rem",
        }}
      >
        <span style={{ "font-size": "0.8rem" }}>Length:</span>
        <input
          type="number"
          style={{
            flex: 1,
            background: "var(--color-surface)",
            color: "var(--color-text-primary)",
            border: "1px solid var(--color-border)",
            padding: "2px 6px",
            "border-radius": "4px",
          }}
          placeholder="Count"
          value={readLen()}
          onInput={(e) => setReadLen(parseInt(e.currentTarget.value) || 0)}
        />
        <button onClick={() => props.onRead(props.address, readLen())}>Read</button>
      </div>

      {/* Terminal Log */}
      <div
        ref={logRef}
        style={{
          background: "#1e1e1e",
          color: "#d4d4d4",
          "font-family": "monospace",
          "font-size": "0.75rem",
          padding: "8px",
          height: "120px",
          "overflow-y": "auto",
          "margin-top": "10px",
          "border-radius": "4px",
          border: "1px solid #333",
        }}
      >
        <For
          each={log()}
          fallback={
            <div style={{ opacity: 0.5, "font-style": "italic" }}>
              Log is empty. Run a command above.
            </div>
          }
        >
          {(entry) => (
            <div
              style={{
                "margin-bottom": "2px",
                color: entry.status === "OK" ? "#4EC9B0" : "#F44747",
              }}
            >
              <span style={{ color: "#808080" }}>[{entry.timestamp}]</span>{" "}
              <span style={{ color: entry.type === "TX" ? "#569CD6" : "#CE9178" }}>
                {entry.type}
              </span>{" "}
              {entry.address} : {entry.data} <span style={{ opacity: 0.8 }}>({entry.status})</span>
            </div>
          )}
        </For>
      </div>

      <button
        style={{
          "margin-top": "5px",
          width: "100%",
          padding: "2px",
          "font-size": "0.7rem",
          cursor: "pointer",
          background: "transparent",
          color: "var(--color-text-primary)",
          border: "1px solid var(--color-border)",
          "border-radius": "4px",
        }}
        onClick={() => setLog([])}
      >
        Clear Log
      </button>
    </div>
  );
}
