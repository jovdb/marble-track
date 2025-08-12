import { type Component, onMount, onCleanup, For } from "solid-js";

import Header from "./components/Header";
import DevicesList, { refreshDevices } from "./components/DevicesList";
import WebSocketMessages from "./components/WebSocketMessages";
import CollapsibleSection from "./components/CollapsibleSection";
import { Led } from "./components/devices/Led";
import { Servo } from "./components/devices/Servo";
import { Buzzer } from "./components/devices/Buzzer";
import { Button } from "./components/devices/Button";
import { Gate } from "./components/devices/Gate";
import { ClipboardIcon, RadioIcon } from "./components/icons/Icons";
import { devicesLoading, isConnected, availableDevices } from "./hooks/useWebSocket";
import AnimatedFavicon from "./utils/animatedFavicon";
import logo from "./assets/logo-64.png";
import styles from "./App.module.css";
import { logger } from "./stores/logger";
import { Wheel } from "./components/devices/Wheel";
import { Stepper } from "./components/devices/Stepper";

// Function to render device component based on type
export const renderDeviceComponent = (device: { id: string; type: string }) => {
  switch (device.type.toLowerCase()) {
    case "led":
      return <Led id={device.id} />;
    case "servo":
      return <Servo id={device.id} />;
    case "button":
      return <Button id={device.id} />;
    case "buzzer":
      return <Buzzer id={device.id} />;
    case "stepper":
      return <Stepper id={device.id} />;
    case "gate":
      return <Gate id={device.id} />;
    case "marble_wheel":
      return <Wheel id={device.id} />;

    default:
      // Return a generic component or null for unknown device types
      logger.error(`Unknown device type: ${device.type}`);
      return null;
  }
};

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
      } ${!isConnected() ? styles["app__refresh-icon--disabled"] : ""}`}
      width="20"
      height="20"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      stroke-width="2"
    >
      <polyline points="23 4 23 10 17 10" />
      <polyline points="1 20 1 14 7 14" />
      <path d="m3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15" />
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
              <svg
                width="20"
                height="20"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
              >
                <rect width="7" height="9" x="3" y="3" rx="1" />
                <rect width="7" height="5" x="14" y="3" rx="1" />
                <rect width="7" height="9" x="14" y="12" rx="1" />
                <rect width="7" height="5" x="3" y="16" rx="1" />
              </svg>
            }
          >
            <div class={styles["app__devices-grid"]}>
              {availableDevices().length === 0 ? (
                <div class={styles["app__no-devices"]}>
                  {isConnected()
                    ? "No devices available for control"
                    : "Connect to see available devices"}
                </div>
              ) : (
                <For each={availableDevices()}>{(device) => renderDeviceComponent(device)}</For>
              )}
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
