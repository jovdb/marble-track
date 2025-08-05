import { type Component } from "solid-js";
import styles from "./Header.module.css";
import { connectionStateName } from "../hooks/useWebSocket";
import logo from "../assets/logo-64.png";

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
        <img src={logo} alt="Logo" width="28" height="28" style={{ "margin-right": "12px" }} />
        <h1>Marble Manager</h1>
      </div>
      <div class={styles.header__right}>
        <p class={getStatusClass()}>Status: {connectionStateName()}</p>
      </div>
    </div>
  );
};

export default Header;
