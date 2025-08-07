import { type Component, For, createSignal, createMemo } from "solid-js";
import { clearMessages, lastMessages, availableDevices } from "../hooks/useWebSocket";
import { MessageIcon } from "./icons/DeviceIcons";
import styles from "./WebSocketMessages.module.css";

const WebSocketMessages: Component = () => {
  const [messageTypeFilter, setMessageTypeFilter] = createSignal("");
  const [deviceIdFilter, setDeviceIdFilter] = createSignal("");

  // Parse messages to extract deviceId and type
  const parseMessage = (message: string) => {
    try {
      const parsed = JSON.parse(message);
      return {
        raw: message,
        type: parsed.type || "",
        deviceId: parsed.deviceId || "",
        parsed
      };
    } catch {
      return {
        raw: message,
        type: "",
        deviceId: "",
        parsed: null
      };
    }
  };

  // Filter messages based on the current filters
  const filteredMessages = createMemo(() => {
    const messages = lastMessages().map(parseMessage);
    const typeFilter = messageTypeFilter().toLowerCase().trim();
    const deviceFilter = deviceIdFilter().trim();

    return messages.filter(msg => {
      const typeMatch = !typeFilter || msg.type.toLowerCase().includes(typeFilter);
      const deviceMatch = !deviceFilter || msg.deviceId === deviceFilter;
      return typeMatch && deviceMatch;
    });
  });

  // Get unique device IDs from available devices
  const deviceOptions = createMemo(() => {
    return availableDevices().map(device => device.id).sort();
  });
  return (
    <div class={styles["websocket-messages"]}>
      <div class={styles["websocket-messages__header"]}>
        <div class={styles["websocket-messages__filters"]}>
          <div class={styles["websocket-messages__filter-group"]}>
            <label class={styles["websocket-messages__filter-label"]} for="message-type-filter">
              Message Type:
            </label>
            <input
              id="message-type-filter"
              class={styles["websocket-messages__filter-input"]}
              type="text"
              placeholder="Filter by type..."
              value={messageTypeFilter()}
              onInput={(e) => setMessageTypeFilter(e.currentTarget.value)}
            />
          </div>
          
          <div class={styles["websocket-messages__filter-group"]}>
            <label class={styles["websocket-messages__filter-label"]} for="device-id-filter">
              Device ID:
            </label>
            <select
              id="device-id-filter"
              class={styles["websocket-messages__filter-select"]}
              value={deviceIdFilter()}
              onChange={(e) => setDeviceIdFilter(e.currentTarget.value)}
            >
              <option value="">All Devices</option>
              <For each={deviceOptions()}>
                {(deviceId) => <option value={deviceId}>{deviceId}</option>}
              </For>
            </select>
          </div>
        </div>
        
        <button 
          class={styles["websocket-messages__clear-button"]}
          onClick={() => clearMessages()}
        >
          Clear Messages
        </button>
      </div>
      
      <div class={styles["websocket-messages__scrollable-content"]}>
        {filteredMessages().length === 0 ? (
          <div class={styles["websocket-messages__empty"]}>
            <MessageIcon class={styles["websocket-messages__empty-icon"]} />
            {lastMessages().length === 0 ? "No messages yet" : "No messages match filters"}
          </div>
        ) : (
          <div class={styles["websocket-messages__list"]}>
            <For each={filteredMessages().slice().reverse()}>
              {(message) => (
                <div class={styles["websocket-messages__message"]}>
                  {message.raw}
                </div>
              )}
            </For>
          </div>
        )}
      </div>
    </div>
  );
};

export default WebSocketMessages;
