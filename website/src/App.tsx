import { createSignal, createEffect, type Component, For } from "solid-js";

import styles from "./App.module.css";
import Header from "./Header";
import { webSocketStore } from "./websocketStore";

const App: Component = () => {
  let myCanvas: HTMLCanvasElement;

  const [position, setPosition] = createSignal(0);

  const handleSendMessage = () => {
    webSocketStore.send({
      type: "angle_update",
      data: { angle: position() }
    });
  };

  return (
    <div>
      <Header />
      <div style={{ padding: "20px" }}>
        <fieldset>
          <legend>Gate 1</legend>
          <div style={{ "margin-bottom": "10px" }}>
            Angle:{" "}
            <input
              type="range"
              min="0"
              max="179"
              value={position()}
              onInput={(e) => {
                setPosition(
                  parseFloat((e.target as unknown as HTMLInputElement).value)
                );
              }}
            />
            {position()}°
          </div>
          <button 
            onClick={handleSendMessage} 
            disabled={!webSocketStore.state.isConnected}
          >
            Send Angle Update
            </button>
        </fieldset>

        {webSocketStore.state.messages.length > 0 && (
          <fieldset>
            <legend>Recent Messages</legend>
            <div style={{ "max-height": "200px", overflow: "auto" }}>
              <For each={webSocketStore.state.messages.slice(-5)}>
                {(msg) => (
                  <div style={{ "margin-bottom": "5px", "font-family": "monospace", "font-size": "12px" }}>
                    <strong>{msg.type}:</strong> {JSON.stringify(msg.data)}
                  </div>
                )}
              </For>
            </div>
            <button onClick={() => webSocketStore.clearMessages()}>Clear Messages</button>
          </fieldset>
        )}
      </div>
    </div>
  );
};

export default App;
