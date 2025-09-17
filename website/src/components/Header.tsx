import { type Component } from "solid-js";
import styles from "./Header.module.css";
import logo from "../assets/logo-64.png";
import { useWebSocket2 } from "../hooks/useWebSocket2";
import { ConnectedIcon, DisconnectedIcon, RestartIcon } from "./icons/Icons";
import { TransparentButton } from "./TransparentButton";

const Header: Component = () => {
  const [webSocket, { sendMessage }] = useWebSocket2();

  // Reset button handler
  const handleReset = () => {
    sendMessage({ type: "restart" });
  };

  return (
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
          title="Restart device"
          onClick={handleReset}
        >
          <RestartIcon />
        </TransparentButton>
      </div>
    </header>
  );
};

export default Header;
