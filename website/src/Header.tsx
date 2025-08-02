import { createSignal, createEffect, onMount, type Component } from "solid-js";
import { webSocketStore } from "./websocketStore";
import styles from "./Header.module.css";

const App: Component = () => {
  const getStatusClass = () => {
    switch (webSocketStore.state.status) {
      case 'connected':
        return '';
      case 'disconnected':
        return styles['status--disconnected'];
      case 'connecting':
        return styles['status--connecting'];
      case 'error':
        return styles['status--disconnected'];
      default:
        return '';
    }
  };

  return (
    <div class={styles.header}>
      <div class={styles.header__left}>
        <h1>Marble Manager</h1>
      </div>
      <div class={styles.header__right}>
        <p class={getStatusClass()}>Status: {webSocketStore.state.status}</p>
      </div>
    </div>
  );
};

export default App;
