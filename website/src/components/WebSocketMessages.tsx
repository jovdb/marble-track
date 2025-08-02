import { type Component, For } from "solid-js";
import { webSocketStore } from "../stores/websocketStore";

const WebSocketMessages: Component = () => {
  return (
    <>
      {webSocketStore.state.messages.length > 0 && (
        <fieldset>
          <legend>Recent Messages</legend>
          <div style={{ "max-height": "200px", overflow: "auto" }}>
            <For each={webSocketStore.state.messages.slice(-5)}>
              {(msg) => (
                <div
                  style={{
                    "margin-bottom": "5px",
                    "font-family": "monospace",
                    "font-size": "12px",
                  }}
                >
                  <strong>{msg.type}:</strong> {JSON.stringify(msg.data)}
                </div>
              )}
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
