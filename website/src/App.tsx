import { type Component, onMount, onCleanup } from "solid-js";

import Header from "./components/Header";
import DevicesList from "./components/DevicesList";
import WebSocketMessages from "./components/WebSocketMessages";
import CollapsibleSection from "./components/CollapsibleSection";
import { Led } from "./components/devices/Led";
import { Servo } from "./components/devices/Servo";
import { Buzzer } from "./components/devices/Buzzer";
import { Button } from "./components/devices/Button";
import { ClipboardIcon, RadioIcon } from "./components/icons/DeviceIcons";
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
          <CollapsibleSection 
            title="Available Devices" 
            icon={<ClipboardIcon />}
          >
            <DevicesList />
          </CollapsibleSection>
        </section>
        
        <section class={styles.app__section}>
          <CollapsibleSection 
            title="Device Controls" 
            icon={
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect width="7" height="9" x="3" y="3" rx="1"/>
                <rect width="7" height="5" x="14" y="3" rx="1"/>
                <rect width="7" height="9" x="14" y="12" rx="1"/>
                <rect width="7" height="5" x="3" y="16" rx="1"/>
              </svg>
            }
          >
            <div class={styles["app__devices-grid"]}>
              <Led id="test-led" />
              <Servo id="test-servo" />
              <Button id="test-button" />
              <Buzzer id="test-buzzer" />
            </div>
          </CollapsibleSection>
        </section>

        <section class={styles.app__section}>
          <CollapsibleSection 
            title="WebSocket Messages" 
            icon={<RadioIcon />}
          >
            <WebSocketMessages />
          </CollapsibleSection>
        </section>
      </main>
    </div>
  );
};

export default App;
