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

  // const [webSocket, { sendMessage }] = useWebSocket2();

  // Download devices config handler
  /*
  const handleDownloadConfig = () => {
    sendMessage({ type: "get-devices-config" });
  };
*/
  /*
  onMount(() => {
    // Listen for devices-config response and trigger download
    const wsHandler = (event: MessageEvent) => {
      try {
        const data =
          typeof event.data === "string"
            ? (JSON.parse(event.data) as IWsReceiveMessage)
            : undefined;
        
        if (data && data.type === "devices-config" && "config" in data) {
          const blob = new Blob([JSON.stringify(data.config, null, 2)], {
            type: "application/json",
          });
          const url = URL.createObjectURL(blob);
          const a = document.createElement("a");
          a.href = url;
          a.download = "devices.json";
          document.body.appendChild(a);
          a.click();
          document.body.removeChild(a);
          URL.revokeObjectURL(url);
        }
      } catch {
        // Ignore parse errors
      }
    };
    websocket.addEventListener("message", wsHandler);
    onCleanup(() => {
      websocket.removeEventListener("message", wsHandler);
    });
  });
  */

  // Upload devices config handler
  const handleUploadConfig = () => {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = ".json,application/json";
    input.style.display = "none";
    input.onchange = async () => {
      const file = (input.files && input.files[0]) || null;
      if (!file) return;
      try {
        const text = await file.text();
        const json = JSON.parse(text);
        //    sendMessage({ type: "set-devices-config", config: json });
        alert("Config uploaded. Please refresh devices after upload.");
      } catch {
        alert("Invalid JSON file.");
      }
    };
    document.body.appendChild(input);
    input.click();
    document.body.removeChild(input);
  };

  // Request devices on mount or when WebSocket becomes connected
  createEffect(() => {
    if (socketState.isConnected) {
      loadDevices();
    }
  });

  return (
    <>
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
      <div class={styles["device-list__buttons"]}>
        {/* <button class={styles["app__config-button"]} onClick={handleDownloadConfig}>
          Download
        </button> */}
        <button class={styles["app__config-button"]} onClick={handleUploadConfig}>
          Upload
        </button>
      </div>
    </>
  );
}
