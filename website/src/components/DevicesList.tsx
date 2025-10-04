import { createEffect, For, createSignal, onMount, onCleanup } from "solid-js";
import { getDeviceIcon, TrashIcon } from "./icons/Icons";
import styles from "./DevicesList.module.css";
import { useDevices } from "../stores/Devices";
import { useWebSocket2 } from "../hooks/useWebSocket2";
import {
  IWsSendAddDeviceMessage,
  IWsSendRemoveDeviceMessage,
  IWsReceiveMessage,
} from "../interfaces/WebSockets";

// Available device types
const DEVICE_TYPES = [
  "Button",
  "Buzzer",
  "DividerWheel",
  "GateWithSensor",
  "Led",
  "PwmMotor",
  "Pwm",
  "Servo",
  "Stepper",
  "Wheel",
] as const;

export function DevicesList() {
  const [devicesState, { loadDevices }] = useDevices();
  const [socketState, socketActions] = useWebSocket2();

  // Signals for add device modal
  const [showAddModal, setShowAddModal] = createSignal(false);
  const [newDeviceType, setNewDeviceType] = createSignal("");
  const [newDeviceId, setNewDeviceId] = createSignal("");

  // Download devices config handler
  const handleDownloadConfig = () => {
    socketActions.sendMessage({ type: "get-devices-config" });
  };

  // Add device handler
  const handleAddDevice = () => {
    const deviceType = newDeviceType();
    const deviceId = newDeviceId().trim();

    if (!deviceType || !deviceId) {
      alert("Please select a device type and enter a device ID");
      return;
    }

    const message: IWsSendAddDeviceMessage = {
      type: "add-device",
      deviceType,
      deviceId,
      config: {},
    };

    if (socketActions.sendMessage(message)) {
      setShowAddModal(false);
      setNewDeviceType("");
      setNewDeviceId("");
      // Device list will be refreshed automatically via WebSocket response
    } else {
      alert("Failed to send add device message");
    }
  };

  // Remove device handler
  const handleRemoveDevice = (deviceId: string) => {
    if (!confirm(`Are you sure you want to remove device "${deviceId}"?`)) {
      return;
    }

    const message: IWsSendRemoveDeviceMessage = {
      type: "remove-device",
      deviceId,
    };

    if (!socketActions.sendMessage(message)) {
      alert("Failed to send remove device message");
    }
  };
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
        if (!socketActions.sendMessage({ type: "set-devices-config", config: json })) {
          alert("Failed to upload config. Please check your connection.");
        }
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

  // Subscribe to WebSocket messages for config download
  const unsubscribe = socketActions.subscribe((message: IWsReceiveMessage) => {
    if (message.type === "devices-config" && "config" in message) {
      const blob = new Blob([JSON.stringify(message.config, null, 2)], {
        type: "application/json",
      });
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = "config.json";
      document.body.appendChild(a);
      a.click();

      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    }
  });

  onMount(() => {
    onCleanup(() => {
      unsubscribe();
    });
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
                        <td
                          class={styles["devices-list__table-td"]}
                          style={{ "text-align": "right" }}
                        >
                          <button
                            class={styles["devices-list__remove-button"]}
                            onClick={() => handleRemoveDevice(device.id)}
                            title="Remove device"
                            aria-label={`Remove device ${device.id}`}
                          >
                            <TrashIcon />
                          </button>
                        </td>
                        <td class={styles["devices-list__table-td"]}>
                          {/* {device.pins && device.pins.length > 0 ? (
                          <code class={styles["devices-list__pins-list"]}>
                            {device.pins.join(", ")}
                          </code>
                        ) : (
                          <span class={styles["devices-list__no-pins"]}>-</span>
                        )} */}
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

      {/* Add Device Button */}
      <div class={styles["device-list__actions"]}>
        <button class={styles["app__config-button"]} onClick={() => setShowAddModal(true)}>
          Add Device
        </button>
      </div>

      {/* Add Device Modal */}
      {showAddModal() && (
        <div class={styles["modal-overlay"]} onClick={() => setShowAddModal(false)}>
          <div class={styles["modal-content"]} onClick={(e) => e.stopPropagation()}>
            <h3>Add New Device</h3>

            <div class={styles["modal-field"]}>
              <label for="device-type">Device Type:</label>
              <select
                id="device-type"
                value={newDeviceType()}
                onChange={(e) => setNewDeviceType(e.target.value)}
                class={styles["modal-select"]}
              >
                <option value="">Select a device type...</option>
                <For each={DEVICE_TYPES}>{(type) => <option value={type}>{type}</option>}</For>
              </select>
            </div>

            <div class={styles["modal-field"]}>
              <label for="device-id">Device ID:</label>
              <input
                id="device-id"
                type="text"
                value={newDeviceId()}
                onInput={(e) => setNewDeviceId(e.target.value)}
                placeholder="Enter device ID"
                class={styles["modal-input"]}
              />
            </div>

            <div class={styles["modal-buttons"]}>
              <button class={styles["modal-button"]} onClick={() => setShowAddModal(false)}>
                Cancel
              </button>
              <button
                class={styles["modal-button modal-button--primary"]}
                onClick={handleAddDevice}
              >
                Add Device
              </button>
            </div>
          </div>
        </div>
      )}

      <div class={styles["device-list__buttons"]}>
        <button class={styles["app__config-button"]} onClick={handleDownloadConfig}>
          Download Config
        </button>
        <button class={styles["app__config-button"]} onClick={handleUploadConfig}>
          Upload Config
        </button>
      </div>
    </>
  );
}
