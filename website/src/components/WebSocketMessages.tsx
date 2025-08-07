import { type Component, For, createSignal, createMemo } from "solid-js";
import { clearMessages, lastMessages, availableDevices } from "../hooks/useWebSocket";
import { MessageIcon } from "./icons/DeviceIcons";
import styles from "./WebSocketMessages.module.css";

// JSON Tree Component
const JsonTree: Component<{ data: any; level?: number }> = (props) => {
  const level = () => props.level || 0;
  
  if (typeof props.data === 'string' || typeof props.data === 'number' || typeof props.data === 'boolean') {
    return (
      <span class={styles["json-value"]} data-type={typeof props.data}>
        {typeof props.data === 'string' ? `"${props.data}"` : String(props.data)}
      </span>
    );
  }
  
  if (props.data === null || props.data === undefined) {
    return <span class={styles["json-value"]} data-type="null">null</span>;
  }
  
  if (Array.isArray(props.data)) {
    const [isExpanded, setIsExpanded] = createSignal(level() === 0); // Auto-expand first level
    
    return (
      <div class={styles["json-array"]}>
        <button 
          class={styles["json-toggle"]}
          onClick={() => setIsExpanded(!isExpanded())}
          style={{ "margin-left": `${level() * 20}px` }}
        >
          {isExpanded() ? '▼' : '▶'} [{props.data.length}]
        </button>
        {isExpanded() && (
          <div class={styles["json-children"]}>
            <For each={props.data}>
              {(item, index) => (
                <div class={styles["json-item"]}>
                  <span class={styles["json-index"]}>{index()}:</span>
                  <JsonTree data={item} level={level() + 1} />
                </div>
              )}
            </For>
          </div>
        )}
      </div>
    );
  }
  
  if (typeof props.data === 'object') {
    const [isExpanded, setIsExpanded] = createSignal(level() === 0); // Auto-expand first level
    const keys = Object.keys(props.data);
    
    return (
      <div class={styles["json-object"]}>
        <button 
          class={styles["json-toggle"]}
          onClick={() => setIsExpanded(!isExpanded())}
          style={{ "margin-left": `${level() * 20}px` }}
        >
          {isExpanded() ? '▼' : '▶'} {`{${keys.length}}`}
        </button>
        {isExpanded() && (
          <div class={styles["json-children"]}>
            <For each={keys}>
              {(key) => (
                <div class={styles["json-item"]}>
                  <span class={styles["json-key"]}>"{key}":</span>
                  <JsonTree data={props.data[key]} level={level() + 1} />
                </div>
              )}
            </For>
          </div>
        )}
      </div>
    );
  }
  
  return null;
};

// Expandable Message Component
const ExpandableMessage: Component<{ message: string }> = (props) => {
  const [isExpanded, setIsExpanded] = createSignal(false);
  
  const parsedData = createMemo(() => {
    try {
      return JSON.parse(props.message);
    } catch {
      return null;
    }
  });
  
  const isJsonMessage = () => parsedData() !== null;
  
  return (
    <div class={styles["websocket-messages__message"]}>
      <div 
        class={`${styles["websocket-messages__message-header"]} ${isJsonMessage() ? styles["websocket-messages__message-header--clickable"] : ""}`}
        onClick={() => isJsonMessage() && setIsExpanded(!isExpanded())}
        title={isJsonMessage() ? (isExpanded() ? "Click to collapse JSON" : "Click to expand JSON") : undefined}
      >
        <span class={styles["websocket-messages__message-text"]}>
          {props.message}
        </span>
      </div>
      
      {isExpanded() && isJsonMessage() && (
        <div class={styles["websocket-messages__json-view"]}>
          <JsonTree data={parsedData()} />
        </div>
      )}
    </div>
  );
};

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
                <ExpandableMessage message={message.raw} />
              )}
            </For>
          </div>
        )}
      </div>
    </div>
  );
};

export default WebSocketMessages;
