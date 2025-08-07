import { For, onMount, createEffect } from "solid-js";
import { availableDevices, devicesLoaded, devicesLoading, requestDevices, isConnected } from "../hooks/useWebSocket";
import styles from "./DevicesList.module.css";

export default function DevicesList() {
  // Request devices when component mounts and WebSocket is connected
  onMount(() => {
    if (isConnected()) {
      requestDevices();
    }
  });

  // Request devices when WebSocket becomes connected
  createEffect(() => {
    if (isConnected() && !devicesLoaded() && !devicesLoading()) {
      requestDevices();
    }
  });

  const refreshDevices = () => {
    requestDevices();
  };

  return (
    <div class={styles["devices-list"]}>
      <div class={styles["devices-list__header"]}>
        <h3 class={styles["devices-list__title"]}>Available Devices</h3>
        <button
          onClick={refreshDevices}
          disabled={devicesLoading() || !isConnected()}
          class={`${styles["devices-list__refresh-button"]} ${
            devicesLoading() ? styles["devices-list__refresh-button--loading"] : ""
          } ${
            (!devicesLoading() && !isConnected()) ? styles["devices-list__refresh-button--disabled"] : ""
          }`}
        >
          {devicesLoading() ? "Refreshing..." : "Refresh"}
        </button>
      </div>

      {!devicesLoading() && !isConnected() && (
        <div class={`${styles["devices-list__status"]} ${styles["devices-list__status--disconnected"]}`}>
          WebSocket not connected
        </div>
      )}

      {devicesLoaded() && availableDevices().length === 0 && (
        <div class={styles["devices-list__status"]}>
          No devices found
        </div>
      )}

      {devicesLoaded() && availableDevices().length > 0 && (
        <div>
          <div class={styles["devices-list__count"]}>
            Found {availableDevices().length} device(s):
          </div>
          
          <div class={styles["devices-list__grid"]}>
            <For each={availableDevices()}>
              {(device) => (
                <div class={styles["device-card"]}>
                  <div class={styles["device-card__name"]}>
                    {device.name || device.id}
                  </div>
                  <div class={styles["device-card__detail"]}>
                    ID: {device.id}
                  </div>
                  <div class={`${styles["device-card__detail"]} ${styles["device-card__detail--type"]}`}>
                    Type: {device.type}
                  </div>
                  {device.pins && device.pins.length > 0 && (
                    <div class={styles["device-card__pins"]}>
                      Pin{device.pins.length > 1 ? 's' : ''}: {device.pins.join(', ')}
                    </div>
                  )}
                </div>
              )}
            </For>
          </div>
        </div>
      )}
    </div>
  );
}
