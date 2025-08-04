import { createSignal, createEffect, type Component } from "solid-js";

import styles from "./App.module.css";
import Header from "./components/Header";
import WebSocketMessages from "./components/WebSocketMessages";
import WebSocketSender from "./components/WebSocketSender";
import { Led } from "./components/devices/Led";
import { isConnected, sendMessage } from "./hooks/useWebSocket";

const App: Component = () => {
  let myCanvas: HTMLCanvasElement;

  const [position, setPosition] = createSignal(0);

  const handleSendMessage = () => {
    sendMessage(
      JSON.stringify({
        type: "angle_update",
        data: { angle: position() },
      })
    );
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
          <button onClick={handleSendMessage} disabled={!isConnected()}>
            Send Angle Update
          </button>
        </fieldset>

        <Led id="test-led" />
        <WebSocketSender />
        <WebSocketMessages />
      </div>
    </div>
  );
};

export default App;
