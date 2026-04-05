import { type Component, onMount, onCleanup, Show } from "solid-js";

import { Header } from "./components/Header";
import { DevicesList } from "./components/DevicesList";
import { WebSocketMessages } from "./components/WebSocketMessages";
import { CollapsibleSection } from "./components/CollapsibleSection";
import { BroadcastIcon, ClipboardIcon } from "./components/icons/Icons";
import AnimatedFavicon from "./utils/animatedFavicon";
import logo from "./assets/logo-64.png";
import styles from "./App.module.css";
import { Providers } from "./Providers";
import { Devices } from "./components/Devices";
import { SerialLog } from "./components/SerialLog";
import { isSerialOpen, openSerialPanel } from "./stores/serial";

const App: Component = () => {
  let animatedFavicon: AnimatedFavicon;

  const openSerialPanelIfUsbAvailable = async () => {
    const serialApi = (navigator as any).serial;
    if (!serialApi?.getPorts) {
      return;
    }

    try {
      const ports = await serialApi.getPorts();
      if (ports.length > 0) {
        openSerialPanel();
      }
    } catch (e) {
      console.warn("Failed to read available serial ports", e);
    }
  };

  onMount(async () => {
    animatedFavicon = new AnimatedFavicon();
    await animatedFavicon.start(logo);
    await openSerialPanelIfUsbAvailable();
  });

  onCleanup(() => {
    if (animatedFavicon) {
      animatedFavicon.stop();
    }
  });
  /*
  const devicesRefreshButton = (
    <svg
      onClick={refreshDevices}
      class={`${styles["app__refresh-icon"]} ${webSocket.isConnected ? styles["app__refresh-icon--disabled"] : ""}`}
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
*/
  return (
    <div class={styles.app}>
      <Providers>
        <Header />
        <main>
          <Show when={isSerialOpen()}>
            <section class={styles.app__section}>
              <SerialLog />
            </section>
          </Show>

          <section class={styles.app__section}>
            <CollapsibleSection
              title="Available Devices"
              icon={<ClipboardIcon height={24} width={24} />}
              //   headerAction={devicesRefreshButton}
            >
              <DevicesList />
            </CollapsibleSection>
          </section>

          <section class={styles.app__section}>
            <CollapsibleSection
              title="Device Controls"
              defaultCollapsed={false}
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
              <Devices />
            </CollapsibleSection>
          </section>

          <section class={styles.app__section}>
            <CollapsibleSection
              title="WebSocket Messages"
              icon={<BroadcastIcon height={24} width={24} />}
            >
              <WebSocketMessages />
            </CollapsibleSection>
          </section>
        </main>
      </Providers>
    </div>
  );
};

export default App;
