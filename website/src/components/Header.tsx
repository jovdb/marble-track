import { type Component, createSignal } from "solid-js";
import styles from "./Header.module.css";
import logo from "../assets/logo-64.png";
import { useWebSocket2 } from "../hooks/useWebSocket2";
import { ConnectedIcon, DisconnectedIcon, RestartIcon } from "./icons/Icons";
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

  // Network icon component
  const NetworkIcon = (props: { width?: number; height?: number; class?: string }) => (
    <svg
      width={props.width || 24}
      height={props.height || 24}
      viewBox="0 0 24 24"
      fill="none"
      class={props.class}
      xmlns="http://www.w3.org/2000/svg"
    >
      <path
        d="M12 18C15.866 18 19 14.866 19 11C19 7.13401 15.866 4 12 4C8.13401 4 5 7.13401 5 11C5 14.866 8.13401 18 12 18Z"
        stroke="currentColor"
        stroke-width="1.5"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
      <path
        d="M12 13C13.1046 13 14 12.1046 14 11C14 9.89543 13.1046 9 12 9C10.8954 9 10 9.89543 10 11C10 12.1046 10.8954 13 12 13Z"
        stroke="currentColor"
        stroke-width="1.5"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
      <path
        d="M2 8C2.55228 8 3 7.55228 3 7C3 6.44772 2.55228 6 2 6C1.44772 6 1 6.44772 1 7C1 7.55228 1.44772 8 2 8Z"
        fill="currentColor"
      />
      <path
        d="M6 4C6.55228 4 7 3.55228 7 3C7 2.44772 6.55228 2 6 2C5.44772 2 5 2.44772 5 3C5 3.55228 5.44772 4 6 4Z"
        fill="currentColor"
      />
      <path
        d="M18 4C18.5523 4 19 3.55228 19 3C19 2.44772 18.5523 2 18 2C17.4477 2 17 2.44772 17 3C17 3.55228 17.4477 4 18 4Z"
        fill="currentColor"
      />
      <path
        d="M22 8C22.5523 8 23 7.55228 23 7C23 6.44772 22.5523 6 22 6C21.4477 6 21 6.44772 21 7C21 7.55228 21.4477 8 22 8Z"
        fill="currentColor"
      />
    </svg>
  );

  return (
    <>
      <header class={styles.header}>
        <div class={styles.header__left}>
          <img
            src={logo}
            alt="Logo"
            class={`${styles.header__logo} ${webSocket.isConnected ? styles["header__logo--connected"] : ""}`}
          />
          <h1 class={styles.header__title}>Marble Manager</h1>
        </div>
        <div class={styles.header__right}>
          {webSocket.isConnected ? <ConnectedIcon /> : <DisconnectedIcon />}
          <TransparentButton
            disabled={!webSocket.isConnected}
            title="Show network info"
            onClick={handleNetworkClick}
          >
            <NetworkIcon />
          </TransparentButton>
          <TransparentButton
            disabled={!webSocket.isConnected}
            title="Restart device"
            onClick={handleReset}
          >
            <RestartIcon />
          </TransparentButton>
        </div>
      </header>

      <NetworkConfig
        isOpen={isNetworkPopupOpen()}
        onClose={() => setIsNetworkPopupOpen(false)}
      />
    </>
  );
};

export { Header };
