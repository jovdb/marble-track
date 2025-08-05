import { createSignal, createEffect, type Component } from "solid-js";

import styles from "./App.module.css";
import Header from "./components/Header";
import WebSocketMessages from "./components/WebSocketMessages";
import WebSocketSender from "./components/WebSocketSender";
import { Led } from "./components/devices/Led";
import { isConnected, sendMessage } from "./hooks/useWebSocket";
import { Servo } from "./components/devices/Servo";

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
        <Led id="test-led" />
        <Servo id="test-servo" />
        {/*<WebSocketSender />*/}
        <WebSocketMessages />
      </div>
    </div>
  );
};

export default App;
