import { createEffect, For, createSignal, onMount, onCleanup } from "solid-js";
import { getDeviceIcon, TrashIcon, ChevronRightIcon, ChevronDownIcon } from "./icons/Icons";
import styles from "./DevicesList.module.css";
import { useDevices, type IDevice } from "../stores/Devices";
import { useWebSocket2 } from "../hooks/useWebSocket";
import {
  IWsSendAddDeviceMessage,
  IWsSendRemoveDeviceMessage,
  DeviceType,
} from "../interfaces/WebSockets";

// Available composition device types (from esp32_ws/include/devices/composition/)
const COMPOSITION_DEVICE_TYPES = [
  "Button",
  "Buzzer",
  "IoExpander",
  "Led",
  "Lift",
  "MarbleController",
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

  // Track collapsed state for devices with children
  const [collapsedDevices, setCollapsedDevices] = createSignal<Set<string>>(new Set());

  // Initialize collapsed devices - start all devices with children collapsed
  createEffect(() => {
    const devices = Object.values(devicesState.devices);
    const devicesWithChildren = devices
      .filter((device) => device.children && device.children.length > 0)
      .map((device) => device.id);

    setCollapsedDevices(new Set(devicesWithChildren));
  });

  const toggleCollapse = (deviceId: string) => {
    setCollapsedDevices((prev) => {
      const next = new Set(prev);
      if (next.has(deviceId)) {
        next.delete(deviceId);
      } else {
        next.add(deviceId);
      }
      return next;
    });
  };

  // Download devices config handler
  const handleDownloadConfig = () => {
    socketActions.sendMessage({ type: "devices-config" });
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
      deviceType: deviceType.toLowerCase() as DeviceType,
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

  // Compute top-level devices (exclude devices that are children of other devices)
  const topLevelDevices = () =>
    Object.values(devicesState.devices).filter((device) => {
      return !Object.values(devicesState.devices).some((other) =>
        other.children?.some((child) => child.id === device.id)
      );
    });

  // Helper function to collect all pins from device and its children recursively
  const collectAllPins = (device: IDevice): number[] => {
    const pins: number[] = [...(device.pins || [])];

    device.children?.forEach((child) => {
      const childDevice = devicesState.devices[child.id];
      if (childDevice) {
        pins.push(...collectAllPins(childDevice));
      }
    });

    return pins;
  };

  // Recursive component to render a device and its children
  const DeviceRow = (props: { device: IDevice; depth?: number }) => {
    const depth = props.depth ?? 0;
    const hasChildren = props.device.children && props.device.children.length > 0;
    const isCollapsed = () => collapsedDevices().has(props.device.id);

    const indentStyle = {
      "padding-left": `${depth * 24}px`,
    };

    // Determine which pins to display
    const displayPins = () => {
      if (hasChildren && isCollapsed()) {
        // When collapsed, show all pins including children's pins
        return collectAllPins(props.device);
      } else {
        // When expanded or no children, show only this device's pins
        return props.device.pins || [];
      }
    };

    return (
      <>
        <tr
          class={styles["devices-list__table-row"]}
          style={{ cursor: "pointer" }}
          onClick={(e) => {
            // Don't scroll if clicking on buttons
            if ((e.target as HTMLElement).closest("button")) return;

            const deviceElement = document.getElementById(`device-${props.device.id}`);
            if (deviceElement) {
              deviceElement.scrollIntoView({ behavior: "smooth", block: "center" });
              // Add a brief highlight effect
              deviceElement.style.transition = "box-shadow 0.3s";
              deviceElement.style.boxShadow = "0 0 0 3px var(--color-primary-500)";
              setTimeout(() => {
                deviceElement.style.boxShadow = "";
              }, 1000);
            }
          }}
        >
          <td class={styles["devices-list__table-td"]}>
            <div class={styles["devices-list__device-cell"]} style={indentStyle}>
              {hasChildren && (
                <button
                  class={styles["devices-list__collapse-button"]}
                  onClick={(e) => {
                    e.stopPropagation();
                    toggleCollapse(props.device.id);
                  }}
                  aria-label={isCollapsed() ? "Expand" : "Collapse"}
                  style={{ width: "24px" }}
                >
                  {isCollapsed() ? <ChevronRightIcon /> : <ChevronDownIcon />}
                </button>
              )}
              {getDeviceIcon(props.device.type, {
                class: styles["devices-list__device-icon"],
              })}
            </div>
          </td>
          <td class={styles["devices-list__table-td"]}>
            <span class={styles["devices-list__type-badge"]}>{props.device.type}</span>
          </td>
          <td class={styles["devices-list__table-td"]}>
            <code class={styles["devices-list__device-id"]}>{props.device.id}</code>
          </td>
          <td class={styles["devices-list__table-td"]}>
            {displayPins().length > 0 ? (
              <code class={styles["devices-list__device-id"]}>{displayPins().join(", ")}</code>
            ) : (
              ""
            )}
          </td>
          <td class={styles["devices-list__table-td"]} style={{ "text-align": "right" }}>
            {depth === 0 && (
              <button
                class={styles["devices-list__remove-button"]}
                onClick={() => handleRemoveDevice(props.device.id)}
                title="Remove device"
                aria-label={`Remove device ${props.device.id}`}
              >
                <TrashIcon />
              </button>
            )}
          </td>
        </tr>
        {!isCollapsed() && (
          <For each={props.device.children}>
            {(child) => {
              const childDevice = devicesState.devices[child.id];
              return childDevice ? <DeviceRow device={childDevice} depth={depth + 1} /> : null;
            }}
          </For>
        )}
      </>
    );
  };

  // Subscribe to WebSocket messages for config download
  const unsubscribe = socketActions.subscribe((message) => {
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
                    <th class={styles["devices-list__table-th"]}>Pins</th>
                    <th class={styles["devices-list__table-th"]}></th>
                  </tr>
                </thead>
                <tbody class={styles["devices-list__table-body"]}>
                  <For each={topLevelDevices()}>{(device) => <DeviceRow device={device} />}</For>
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>

      {/* Add Device Button */}
      <div class={styles["device-list__actions"]}>
        <button
          class={styles["app__config-button"]}
          onClick={() => setShowAddModal(true)}
          disabled={!socketState.isConnected}
        >
          Add Device
        </button>
      </div>

      {/* Add Device Modal */}
      {showAddModal() && (
        <div class={styles["modal-overlay"]} onClick={() => setShowAddModal(false)}>
          <form
            class={styles["modal-content"]}
            onClick={(e) => e.stopPropagation()}
            onSubmit={(e) => {
              e.preventDefault();
              handleAddDevice();
            }}
          >
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
                <For each={COMPOSITION_DEVICE_TYPES}>
                  {(type) => <option value={type}>{type}</option>}
                </For>
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
              <button
                type="reset"
                class={styles["modal-button"]}
                onClick={() => setShowAddModal(false)}
              >
                Cancel
              </button>
              <button
                type="submit"
                class={styles["modal-button modal-button--primary"]}
                disabled={!socketState.isConnected}
              >
                Add Device
              </button>
            </div>
          </form>
        </div>
      )}

      <div class={styles["device-list__buttons"]}>
        <button
          class={styles["app__config-button"]}
          onClick={handleDownloadConfig}
          disabled={!socketState.isConnected}
        >
          Download Config
        </button>
        <button
          class={styles["app__config-button"]}
          onClick={handleUploadConfig}
          disabled={!socketState.isConnected}
        >
          Upload Config
        </button>
      </div>
    </>
  );
}
