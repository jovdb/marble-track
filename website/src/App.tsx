import { createSignal, createEffect, type Component } from "solid-js";

import styles from "./App.module.css";
import Header from "./components/Header";
import WebSocketMessages from "./components/WebSocketMessages";
import { webSocketStore } from "./stores/websocketStore";

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

        <WebSocketMessages />
      </div>
    </div>
  );
};

export default App;
