import { type Component, onMount, onCleanup } from "solid-js";

import Header from "./components/Header";
import DevicesList from "./components/DevicesList";
import WebSocketMessages from "./components/WebSocketMessages";
import { Led } from "./components/devices/Led";
import { Servo } from "./components/devices/Servo";
import { Buzzer } from "./components/devices/Buzzer";
import { Button } from "./components/devices/Button";
import AnimatedFavicon from "./utils/animatedFavicon";
import logo from "./assets/logo-64.png";
import styles from "./App.module.css";

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
    <div class={styles.app}>
      <Header />
      <main class={styles.app__main}>
        <section class={styles.app__section}>
          <DevicesList />
        </section>
        
        <section class={styles.app__section}>
          <div class={styles["app__devices-grid"]}>
            <Led id="test-led" />
            <Servo id="test-servo" />
            <Button id="test-button" />
            <Buzzer id="test-buzzer" />
          </div>
        </section>

        <section class={styles.app__section}>
          <WebSocketMessages />
        </section>
      </main>
    </div>
  );
};

export default App;
