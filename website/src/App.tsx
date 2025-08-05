import { type Component } from "solid-js";

import Header from "./components/Header";
import WebSocketMessages from "./components/WebSocketMessages";
import { Led } from "./components/devices/Led";
import { Servo } from "./components/devices/Servo";

const App: Component = () => {
  return (
    <div>
      <Header />
      <div style={{ padding: "20px" }}>
        <Led id="test-led" />
        <Servo id="test-servo" />
        <WebSocketMessages />
      </div>
    </div>
  );
};

export default App;
