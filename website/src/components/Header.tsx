import { type Component } from "solid-js";
import styles from "./Header.module.css";
import { connectionStateName } from "../hooks/useWebSocket";
import logo from "../assets/logo-64.png";

const Header: Component = () => {
  const getStatusIndicatorClass = () => {
    switch (connectionStateName()) {
      case "Connected":
        return styles["header__status-indicator"];
      case "Disconnected":
        return `${styles["header__status-indicator"]} ${styles["header__status-indicator--disconnected"]}`;
      case "Connecting":
        return `${styles["header__status-indicator"]} ${styles["header__status-indicator--connecting"]}`;
      case "Disconnecting":
        return `${styles["header__status-indicator"]} ${styles["header__status-indicator--disconnected"]}`;
      default:
        return styles["header__status-indicator"];
    }
  };

  return (
    <header class={styles.header}>
      <div class={styles.header__left}>
        <img src={logo} alt="Logo" class={styles.header__logo} />
        <h1 class={styles.header__title}>Marble Manager</h1>
      </div>
      <div class={styles.header__right}>
        <div class={styles.header__status}>
          <div class={getStatusIndicatorClass()}></div>
          <span>Status: {connectionStateName()}</span>
        </div>
      </div>
    </header>
  );
};

export default Header;
