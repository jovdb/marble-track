import { createEffect, For } from "solid-js";
import { requestDevices } from "../hooks/useWebSocket";
import { getDeviceIcon } from "./icons/Icons";
import styles from "./DevicesList.module.css";
import { useDevices } from "../stores/Devices";
import { useWebSocket2 } from "../hooks/useWebSocket2";

// Export refresh function for use in parent components
export const refreshDevices = () => {
  requestDevices();
};

export default function DevicesList() {
  const [devicesState, { loadDevices }] = useDevices();

  const [socketState] = useWebSocket2();

  // Request devices on mount or when WebSocket becomes connected
  createEffect(() => {
    if (socketState.isConnected) {
      loadDevices();
    }
  });

  return (
    <div class={styles["devices-list"]}>
      <div>
        {Object.values(devicesState.devices).length === 0 && (
          <div class={styles["devices-list__status"]}>No devices found</div>
        )}

        {Object.values(devicesState.devices).length > 0 && (
          <div class={styles["devices-list__table-container"]}>
            <table class={styles["devices-list__table"]}>
              <thead class={styles["devices-list__table-header"]}>
                <tr>
                  <th class={styles["devices-list__table-th"]}></th>
                  <th class={styles["devices-list__table-th"]}>Type</th>
                  <th class={styles["devices-list__table-th"]}>ID</th>
                </tr>
              </thead>
              <tbody class={styles["devices-list__table-body"]}>
                <For each={Object.values(devicesState.devices)}>
                  {(device) => (
                    <tr class={styles["devices-list__table-row"]}>
                      <td class={styles["devices-list__table-td"]}>
                        <div class={styles["devices-list__device-cell"]}>
                          {getDeviceIcon(device.type, {
                            class: styles["devices-list__device-icon"],
                          })}
                        </div>
                      </td>
                      <td class={styles["devices-list__table-td"]}>
                        <span class={styles["devices-list__type-badge"]}>{device.type}</span>
                      </td>
                      <td class={styles["devices-list__table-td"]}>
                        <code class={styles["devices-list__device-id"]}>{device.id}</code>
                      </td>
                      {/* <td class={styles["devices-list__table-td"]}>
                        {device.pins && device.pins.length > 0 ? (
                          <code class={styles["devices-list__pins-list"]}>
                            {device.pins.join(", ")}
                          </code>
                        ) : (
                          <span class={styles["devices-list__no-pins"]}>-</span>
                        )}
                      </td> */}
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
