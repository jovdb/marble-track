import { type Component, createSignal } from "solid-js";
import styles from "./Header.module.css";
import logo from "../assets/logo-64.png";
import { useWebSocket2 } from "../hooks/useWebSocket";
import { ConnectedIcon, DisconnectedIcon, RestartIcon, WifiConnectedIcon } from "./icons/Icons";
import { TransparentButton } from "./TransparentButton";
import { NetworkConfig } from "./NetworkConfig";

const Header: Component = () => {
  const [webSocket, { sendMessage }] = useWebSocket2();
  const [isNetworkPopupOpen, setIsNetworkPopupOpen] = createSignal(false);

  // Reset button handler
  const handleReset = () => {
    sendMessage({ type: "restart" });
  };

  // Network button handler
  const handleNetworkClick = () => {
    setIsNetworkPopupOpen(true);
  };

  return (
    <>
      <header class={styles.header}>
        <div class={styles.header__left}>
          <img
            src={logo}
            alt="Logo"
            class={`${styles.header__logo} ${webSocket.isConnected ? styles["header__logo--connected"] : ""}`}
          />
          <h1 class={styles.header__title} title={`Build: ${__BUILD_DATE__}`}>
            Marble Manager
          </h1>
        </div>
        <div class={styles.header__right}>
          <span title={`Connected to ${webSocket.url}`} class={styles.header__button}>
            {webSocket.isConnected ? <ConnectedIcon /> : <DisconnectedIcon />}
          </span>
          <TransparentButton
            disabled={!webSocket.isConnected}
            title="Wifi connection"
            onClick={handleNetworkClick}
            class={styles.header__button}
          >
            <WifiConnectedIcon />
          </TransparentButton>

          <TransparentButton
            disabled={!webSocket.isConnected}
            title="Restart device"
            onClick={handleReset}
            class={styles.header__button}
          >
            <RestartIcon />
          </TransparentButton>
        </div>
      </header>

      <NetworkConfig isOpen={isNetworkPopupOpen()} onClose={() => setIsNetworkPopupOpen(false)} />
    </>
  );
};

export { Header };
