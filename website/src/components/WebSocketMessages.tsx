import { type Component, For } from "solid-js";
import { webSocketStore } from "../stores/websocketStore";
import type { WebSocketMessageEntry } from "../stores/websocketStore";
import type { WebSocketSendMessage } from "../websocket";

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

  const formatMessage = (entry: WebSocketMessageEntry) => {
    if (entry.direction === 'sent') {
      // For sent messages, we have the structured object
      const sendMsg = entry.message as WebSocketSendMessage;
      return { type: sendMsg.type, data: sendMsg.data };
    } else {
      // For received messages, parse the string
      return parseMessage(entry.message as string);
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

  const getDirectionStyle = (direction: 'sent' | 'received') => {
    return {
      color: direction === 'sent' ? '#2563eb' : '#16a34a', // Blue for sent, green for received
      'font-weight': 'bold' as const,
      'margin-right': '8px'
    };
  };

  const getDirectionIcon = (direction: 'sent' | 'received') => {
    return direction === 'sent' ? '↗️' : '↙️';
  };

  return (
    <>
      {webSocketStore.state.messages.length > 0 && (
        <fieldset>
          <legend>Recent Messages</legend>
          <div style={{ "max-height": "200px", overflow: "auto" }}>
            <For each={webSocketStore.state.messages.slice(-5)}>
              {(messageEntry: WebSocketMessageEntry) => {
                const parsedMsg = formatMessage(messageEntry);
                const timestamp = formatTimestamp(messageEntry.timestamp);
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
                    <span style={getDirectionStyle(messageEntry.direction)}>
                      {getDirectionIcon(messageEntry.direction)} {messageEntry.direction.toUpperCase()}
                    </span>
                    <strong>{parsedMsg.type}:</strong>{" "}
                    {JSON.stringify(parsedMsg.data)}
                  </div>
                );
              }}
            </For>
          </div>
          <button onClick={() => webSocketStore.clearMessages()}>
            Clear Messages
          </button>
        </fieldset>
      )}
    </>
  );
};

export default WebSocketMessages;
