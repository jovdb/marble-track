import { type Component, onMount, onCleanup } from "solid-js";

import Header from "./components/Header";
import WebSocketMessages from "./components/WebSocketMessages";
import { Led } from "./components/devices/Led";
import { Servo } from "./components/devices/Servo";
import { Buzzer } from "./components/devices/Buzzer";
import { Button } from "./components/devices/Button";
import AnimatedFavicon from "./utils/animatedFavicon";
import logo from "./assets/logo-64.png";
const App: Component = () => {
  let animatedFavicon: AnimatedFavicon;

  onMount(async () => {
    // Initialize and start animated favicon
    animatedFavicon = new AnimatedFavicon();
    await animatedFavicon.start(logo);
  });

  onCleanup(() => {
    // Clean up animation when component unmounts
    if (animatedFavicon) {
      animatedFavicon.stop();
    }
  });

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
