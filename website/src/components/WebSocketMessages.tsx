import { type Component, For } from "solid-js";
import { clearMessages, lastMessages } from "../hooks/useWebSocket";

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

  return (
    <>
      <fieldset>
        <legend>Recent Messages</legend>
        <div style={{ "max-height": "200px", overflow: "auto" }}>
          <For each={lastMessages()}>
            {(message) => {
              return (
                <div
                  style={{
                    "margin-bottom": "5px",
                    "font-family": "monospace",
                    "font-size": "12px",
                  }}
                >
                  {message}
                </div>
              );
            }}
          </For>
        </div>
        <button onClick={() => clearMessages()}>Clear Messages</button>
      </fieldset>
    </>
  );
};

export default WebSocketMessages;
