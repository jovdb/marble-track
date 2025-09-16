import { createMemo, type Component } from "solid-js";
import styles from "./Header.module.css";
import logo from "../assets/logo-64.png";
import { useWebSocket2 } from "../hooks/useWebSocket2";
import { ConnectedIcon, DisconnectedIcon } from "./icons/Icons";

const Header: Component = () => {
  const [state] = useWebSocket2();

  return (
    <header class={styles.header}>
      <div class={styles.header__left}>
        <img src={logo} alt="Logo" class={styles.header__logo} />
        <h1 class={styles.header__title}>Marble Manager</h1>
      </div>
      <div class={styles.header__right}>
        <div class={styles.header__status}>
          {state.isConnected ? <ConnectedIcon /> : <DisconnectedIcon />}
          <span class={styles["header__status-text"]}>{state.connectionStateName}</span>
        </div>
      </div>
    </header>
  );
};

export default Header;
