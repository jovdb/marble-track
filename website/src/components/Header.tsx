import { type Component } from "solid-js";
import styles from "./Header.module.css";
import { connectionStateName } from "../hooks/useWebSocket";

const Header: Component = () => {
  const getStatusClass = () => {
    switch (connectionStateName()) {
      case "Connected":
        return "";
      case "Disconnected":
        return styles["status--disconnected"];
      case "Connecting":
        return styles["status--connecting"];
      case "Disconnecting":
        return styles["status--disconnected"];
      default:
        return "";
    }
  };

  return (
    <div class={styles.header}>
      <div class={styles.header__left}>
        <h1>Marble Manager</h1>
      </div>
      <div class={styles.header__right}>
        <p class={getStatusClass()}>Status: {connectionStateName()}</p>
      </div>
    </div>
  );
};

export default Header;
