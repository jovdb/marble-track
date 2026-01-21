import { createEffect, For, createSignal, onMount, onCleanup } from "solid-js";
import {
  getDeviceIcon,
  TrashIcon,
  ChevronRightIcon,
  ChevronDownIcon,
  GripIcon,
} from "./icons/Icons";
import styles from "./DevicesList.module.css";
import { useDevices, type IDevice } from "../stores/Devices";
import { useWebSocket2 } from "../hooks/useWebSocket";
import { useSelectedDevices } from "../stores/SelectedDevices";
import {
  IWsSendAddDeviceMessage,
  IWsSendRemoveDeviceMessage,
  IWsSendReorderDevicesMessage,
  DeviceType,
} from "../interfaces/WebSockets";

// Available composition device types (from esp32_ws/include/devices/composition/)
const COMPOSITION_DEVICE_TYPES = [
  "Button",
  "Buzzer",
  "I2c",
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

  // Track selected devices
  const [selectedDevicesState, selectedDevicesActions] = useSelectedDevices();
  const [draggedDeviceId, setDraggedDeviceId] = createSignal<string | null>(null);
  const [dragOverDeviceId, setDragOverDeviceId] = createSignal<string | null>(null);
  const [dragDirection, setDragDirection] = createSignal<"up" | "down" | null>(null);

  // Initialize collapsed devices - expand root devices one level
  createEffect(() => {
    const devices = Object.values(devicesState.devices);
    const rootDevices = devices.filter((device) => {
      return !devices.some((other) => other.children?.some((child) => child.id === device.id));
    });

    const nextCollapsed = new Set<string>();
    rootDevices.forEach((root) => {
      root.children?.forEach((child) => {
        const childDevice = devicesState.devices[child.id];
        if (childDevice && childDevice.children && childDevice.children.length > 0) {
          nextCollapsed.add(childDevice.id);
        }
      });
    });

    setCollapsedDevices(nextCollapsed);
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

  // Drag and drop handlers for reordering
  const handleDragStart = (e: DragEvent, deviceId: string) => {
    if (!e.dataTransfer) return;
    setDraggedDeviceId(deviceId);
    e.dataTransfer.effectAllowed = "move";
    e.dataTransfer.setData("text/plain", deviceId);
    // Add a slight delay to allow the drag image to be set
    setTimeout(() => {
      const target = e.target as HTMLElement;
      target.closest("tr")?.classList.add(styles["devices-list__table-row--dragging"]);
    }, 0);
  };

  const handleDragEnd = () => {
    setDraggedDeviceId(null);
    setDragOverDeviceId(null);
    setDragDirection(null);
    // Remove dragging class from all rows
    document.querySelectorAll(`.${styles["devices-list__table-row--dragging"]}`).forEach((el) => {
      el.classList.remove(styles["devices-list__table-row--dragging"]);
    });
  };

  const handleDragOver = (e: DragEvent, deviceId: string) => {
    e.preventDefault();
    if (!e.dataTransfer) return;
    e.dataTransfer.dropEffect = "move";

    const sourceDeviceId = draggedDeviceId();
    if (sourceDeviceId !== deviceId) {
      setDragOverDeviceId(deviceId);

      // Determine drag direction
      const currentDevices = topLevelDevices();
      const sourceIndex = currentDevices.findIndex((d) => d.id === sourceDeviceId);
      const targetIndex = currentDevices.findIndex((d) => d.id === deviceId);

      if (sourceIndex !== -1 && targetIndex !== -1) {
        setDragDirection(sourceIndex < targetIndex ? "down" : "up");
      }
    }
  };

  const handleDragLeave = () => {
    setDragOverDeviceId(null);
  };

  const handleDrop = (e: DragEvent, targetDeviceId: string) => {
    e.preventDefault();
    const sourceDeviceId = draggedDeviceId();

    if (!sourceDeviceId || sourceDeviceId === targetDeviceId) {
      setDraggedDeviceId(null);
      setDragOverDeviceId(null);
      return;
    }

    // Get current top-level devices and reorder
    const currentDevices = topLevelDevices();
    const sourceIndex = currentDevices.findIndex((d) => d.id === sourceDeviceId);
    const targetIndex = currentDevices.findIndex((d) => d.id === targetDeviceId);

    if (sourceIndex === -1 || targetIndex === -1) {
      setDraggedDeviceId(null);
      setDragOverDeviceId(null);
      return;
    }

    // Create new order
    const newOrder = [...currentDevices];
    const [removed] = newOrder.splice(sourceIndex, 1);
    newOrder.splice(targetIndex, 0, removed);

    // Send reorder message to backend
    const message: IWsSendReorderDevicesMessage = {
      type: "reorder-devices",
      deviceIds: newOrder.map((d) => d.id),
    };

    if (!socketActions.sendMessage(message)) {
      alert("Failed to send reorder message");
    }

    setDraggedDeviceId(null);
    setDragOverDeviceId(null);
    setDragDirection(null);
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
    const isTopLevel = depth === 0;
    const isDragging = () => draggedDeviceId() === props.device.id;
    const isDragOver = () => dragOverDeviceId() === props.device.id && !isDragging();
    const dragDirectionClass = () => {
      if (!isDragOver()) return "";
      const direction = dragDirection();
      return direction === "up"
        ? styles["devices-list__table-row--drag-over-up"]
        : styles["devices-list__table-row--drag-over-down"];
    };

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
          class={`${styles["devices-list__table-row"]} ${dragDirectionClass()} ${
            selectedDevicesState.selectedDevices().has(props.device.id)
              ? styles["devices-list__table-row--selected"]
              : ""
          }`}
          style={{ cursor: "pointer" }}
          draggable={isTopLevel}
          onDragStart={(e) => isTopLevel && handleDragStart(e, props.device.id)}
          onDragEnd={handleDragEnd}
          onDragOver={(e) => isTopLevel && handleDragOver(e, props.device.id)}
          onDragLeave={handleDragLeave}
          onDrop={(e) => isTopLevel && handleDrop(e, props.device.id)}
          onClick={(e) => {
            // Don't scroll if clicking on buttons or drag handle
            if ((e.target as HTMLElement).closest("button")) return;
            if ((e.target as HTMLElement).closest(`.${styles["devices-list__drag-handle"]}`))
              return;

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

            // Toggle selection for this device (single selection)
            selectedDevicesActions.setSelectedDevices((prev) => {
              if (prev.has(props.device.id)) {
                return new Set(); // Deselect if already selected
              } else {
                return new Set([props.device.id]); // Select only this device
              }
            });
          }}
        >
          <td class={styles["devices-list__table-td"]}>
            <input
              type="checkbox"
              class={styles["devices-list__checkbox"]}
              checked={selectedDevicesState.selectedDevices().has(props.device.id)}
              aria-label={`Select device ${props.device.id}`}
              onClick={(e) => e.stopPropagation()}
              onChange={(e) => {
                const checked = e.target.checked;
                selectedDevicesActions.setSelectedDevices((prev) => {
                  const next = new Set(prev);
                  if (checked) {
                    next.add(props.device.id);
                  } else {
                    next.delete(props.device.id);
                  }
                  return next;
                });
              }}
            />
          </td>
          <td class={styles["devices-list__table-td"]}>
            <div class={styles["devices-list__device-cell"]} style={indentStyle}>
              {isTopLevel && (
                <span
                  class={styles["devices-list__drag-handle"]}
                  title="Drag to reorder"
                  onMouseDown={(e) => e.stopPropagation()}
                >
                  <GripIcon />
                </span>
              )}
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
              {getDeviceIcon(props.device.type)}
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
