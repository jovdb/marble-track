import { For, onMount, createEffect } from "solid-js";
import {
  availableDevices,
  devicesLoaded,
  devicesLoading,
  requestDevices,
  isConnected,
} from "../hooks/useWebSocket";
import { getDeviceIcon } from "./icons/DeviceIcons";
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
        <button
          onClick={refreshDevices}
          disabled={devicesLoading() || !isConnected()}
          class={`${styles["devices-list__refresh-button"]} ${
            devicesLoading() ? styles["devices-list__refresh-button--loading"] : ""
          } ${
            !devicesLoading() && !isConnected()
              ? styles["devices-list__refresh-button--disabled"]
              : ""
          }`}
        >
          {devicesLoading() ? "Refreshing..." : "Refresh"}
        </button>
      </div>

      <div>
        {!devicesLoading() && !isConnected() && (
          <div
            class={`${styles["devices-list__status"]} ${styles["devices-list__status--disconnected"]}`}
          >
            WebSocket not connected
          </div>
        )}

        {devicesLoaded() && availableDevices().length === 0 && (
          <div class={styles["devices-list__status"]}>No devices found</div>
        )}

        {devicesLoaded() && availableDevices().length > 0 && (
          <div class={styles["devices-list__table-container"]}>
            <table class={styles["devices-list__table"]}>
              <thead class={styles["devices-list__table-header"]}>
                <tr>
                  <th class={styles["devices-list__table-th"]}>Device</th>
                  <th class={styles["devices-list__table-th"]}>Type</th>
                  <th class={styles["devices-list__table-th"]}>ID</th>
                  <th class={styles["devices-list__table-th"]}>Pins</th>
                </tr>
              </thead>
              <tbody class={styles["devices-list__table-body"]}>
                <For each={availableDevices()}>
                  {(device) => (
                    <tr class={styles["devices-list__table-row"]}>
                      <td class={styles["devices-list__table-td"]}>
                        <div class={styles["devices-list__device-cell"]}>
                          {getDeviceIcon(device.type, {
                            class: styles["devices-list__device-icon"],
                          })}
                          <span class={styles["devices-list__device-name"]}>
                            {device.name || device.id}
                          </span>
                        </div>
                      </td>
                      <td class={styles["devices-list__table-td"]}>
                        <span class={styles["devices-list__type-badge"]}>{device.type}</span>
                      </td>
                      <td class={styles["devices-list__table-td"]}>
                        <code class={styles["devices-list__device-id"]}>{device.id}</code>
                      </td>
                      <td class={styles["devices-list__table-td"]}>
                        {device.pins && device.pins.length > 0 ? (
                          <code class={styles["devices-list__pins-list"]}>
                            {device.pins.join(", ")}
                          </code>
                        ) : (
                          <span class={styles["devices-list__no-pins"]}>-</span>
                        )}
                      </td>
                    </tr>
                  )}
                </For>
              </tbody>
            </table>
          </div>
        )}
      </div>
    </div>
  );
}
