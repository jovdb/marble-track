import { type Component } from "solid-js";

import Header from "./components/Header";
import WebSocketMessages from "./components/WebSocketMessages";
import { Led } from "./components/devices/Led";
import { Servo } from "./components/devices/Servo";
import { Buzzer } from "./components/devices/Buzzer";

import { Button } from "./components/devices/Button";
const App: Component = () => {
  return (
    <div>
      <Header />
      <div style={{ padding: "20px" }}>
        <Led id="test-led" />
        <Servo id="test-servo" />
        <Button id="test-button" />
        <Buzzer id="test-buzzer" />
        <WebSocketMessages />
      </div>
    </div>
  );
};

export default App;
