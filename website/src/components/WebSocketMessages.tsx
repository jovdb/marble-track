import { type Component, For } from "solid-js";
import { webSocketStore } from "../stores/websocketStore";
import type { TimestampedMessage } from "../stores/websocketStore";

const WebSocketMessages: Component = () => {
  const parseMessage = (msg: string) => {
    try {
      const parsed = JSON.parse(msg);
      if (typeof parsed === "object" && parsed !== null && "type" in parsed) {
        return parsed;
      }
      return { type: "raw", data: msg };
    } catch {
      return { type: "raw", data: msg };
    }
  };

  const formatTimestamp = (timestamp: number) => {
    const date = new Date(timestamp);
    return date.toLocaleTimeString("en-US", {
      hour12: false,
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
      fractionalSecondDigits: 3,
    });
  };

  return (
    <>
      <fieldset>
        <legend>Recent Messages</legend>
        <div style={{ "max-height": "200px", overflow: "auto" }}>
          <For each={webSocketStore.state.messages.slice(-5)}>
            {(timestampedMsg: TimestampedMessage) => {
              const parsedMsg = parseMessage(timestampedMsg.message);
              const timestamp = formatTimestamp(timestampedMsg.timestamp);
              return (
                <div
                  style={{
                    "margin-bottom": "5px",
                    "font-family": "monospace",
                    "font-size": "12px",
                  }}
                >
                  <span style={{ color: "#666", "margin-right": "8px" }}>
                    [{timestamp}]
                  </span>
                  {timestampedMsg.message}
                </div>
              );
            }}
          </For>
        </div>
        <button onClick={() => webSocketStore.clearMessages()}>
          Clear Messages
        </button>
      </fieldset>
    </>
  );
};

export default WebSocketMessages;
