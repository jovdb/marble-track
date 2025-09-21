import { type Component, onMount, onCleanup } from "solid-js";

import { Header } from "./components/Header";
import { DevicesList } from "./components/DevicesList";
import { WebSocketMessages } from "./components/WebSocketMessages";
import { CollapsibleSection } from "./components/CollapsibleSection";
import { ClipboardIcon, RadioIcon } from "./components/icons/Icons";
import AnimatedFavicon from "./utils/animatedFavicon";
import logo from "./assets/logo-64.png";
import styles from "./App.module.css";
import { Providers } from "./Providers";
import { Devices } from "./components/Devices";

const App: Component = () => {
  let animatedFavicon: AnimatedFavicon;

  onMount(async () => {
    animatedFavicon = new AnimatedFavicon();
    await animatedFavicon.start(logo);
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
              icon={<RadioIcon height={24} width={24} />}
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
