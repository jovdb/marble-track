import { For, onMount, createEffect } from "solid-js";
import { availableDevices, devicesLoaded, devicesLoading, requestDevices, isConnected } from "../hooks/useWebSocket";
import { ClipboardIcon, getDeviceIcon } from "./icons/DeviceIcons";
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
        <h2 class={styles["devices-list__title"]}>
          <ClipboardIcon class={styles["devices-list__title-icon"]} />
          Available Devices
        </h2>
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

      <div class={styles["devices-list__content"]}>
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
          <>
            <div class={styles["devices-list__count"]}>
              <ClipboardIcon class={styles["devices-list__count-icon"]} />
              Found {availableDevices().length} device{availableDevices().length !== 1 ? 's' : ''}
            </div>
            
            <div class={styles["devices-list__grid"]}>
              <For each={availableDevices()}>
                {(device) => (
                  <div class={styles["device-card"]}>
                    <div class={styles["device-card__header"]}>
                      <h3 class={styles["device-card__name"]}>
                        {getDeviceIcon(device.type, { class: styles["device-card__icon"] })}
                        {device.name || device.id}
                      </h3>
                      <span class={styles["device-card__type-badge"]}>
                        {device.type}
                      </span>
                    </div>
                    
                    <div class={styles["device-card__details"]}>
                      <div class={styles["device-card__detail"]}>
                        <span class={styles["device-card__detail-label"]}>ID:</span>
                        <span class={styles["device-card__detail-value"]}>{device.id}</span>
                      </div>
                      
                      {device.pins && device.pins.length > 0 && (
                        <div class={styles["device-card__pins"]}>
                          <span class={styles["device-card__pins-label"]}>
                            Pin{device.pins.length > 1 ? 's' : ''}:
                          </span>
                          <span class={styles["device-card__pins-list"]}>
                            {device.pins.join(', ')}
                          </span>
                        </div>
                      )}
                    </div>
                  </div>
                )}
              </For>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
