import { type Component, onMount, onCleanup } from "solid-js";

import Header from "./components/Header";
import DevicesList, { refreshDevices } from "./components/DevicesList";
import WebSocketMessages from "./components/WebSocketMessages";
import CollapsibleSection from "./components/CollapsibleSection";
import { Led } from "./components/devices/Led";
import { Servo } from "./components/devices/Servo";
import { Buzzer } from "./components/devices/Buzzer";
import { Button } from "./components/devices/Button";
import { ClipboardIcon, RadioIcon } from "./components/icons/DeviceIcons";
import { 
  devicesLoading, 
  isConnected 
} from "./hooks/useWebSocket";
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

  // Create refresh button for devices section
  const devicesRefreshButton = (
    <svg
      onClick={refreshDevices}
      class={`${styles["app__refresh-icon"]} ${
        devicesLoading() ? styles["app__refresh-icon--loading"] : ""
      } ${
        !isConnected() ? styles["app__refresh-icon--disabled"] : ""
      }`}
      width="20" 
      height="20" 
      viewBox="0 0 24 24" 
      fill="none" 
      stroke="currentColor" 
      stroke-width="2"
    >
      <polyline points="23 4 23 10 17 10"/>
      <polyline points="1 20 1 14 7 14"/>
      <path d="m3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/>
    </svg>
  );

  return (
    <div class={styles.app}>
      <Header />
      <main class={styles.app__main}>
        <section class={styles.app__section}>
          <CollapsibleSection 
            title="Available Devices" 
            icon={<ClipboardIcon height={24} width={24} />}
            headerAction={devicesRefreshButton}
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
            icon={<RadioIcon height={24} width={24} />}
          >
            <WebSocketMessages />
          </CollapsibleSection>
        </section>
      </main>
    </div>
  );
};

export default App;
