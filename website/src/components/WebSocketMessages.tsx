import { type Component, For, createSignal, createMemo, createEffect, onMount } from "solid-js";
import { useWebSocket2 } from "../hooks/useWebSocket2";
import { useDevices } from "../stores/Devices";
import { IncomingMessageIcon, OutgoingMessageIcon, MessageIcon } from "./icons/Icons";
import styles from "./WebSocketMessages.module.css";

// JSON Tree Component
const JsonTree: Component<{ data: any; level?: number }> = (props) => {
  const level = () => props.level || 0;

  if (
    typeof props.data === "string" ||
    typeof props.data === "number" ||
    typeof props.data === "boolean"
  ) {
    return (
      <span class={styles["json-value"]} data-type={typeof props.data}>
        {typeof props.data === "string" ? `"${props.data}"` : String(props.data)}
      </span>
    );
  }

  if (props.data === null || props.data === undefined) {
    return (
      <span class={styles["json-value"]} data-type="null">
        null
      </span>
    );
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
          {isExpanded() ? "▼" : "▶"} [{props.data.length}]
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

  if (typeof props.data === "object") {
    const [isExpanded, setIsExpanded] = createSignal(level() === 0); // Auto-expand first level
    const keys = Object.keys(props.data);

    return (
      <div class={styles["json-object"]}>
        <button
          class={styles["json-toggle"]}
          onClick={() => setIsExpanded(!isExpanded())}
          style={{ "margin-left": `${level() * 20}px` }}
        >
          {isExpanded() ? "▼" : "▶"} {`{${keys.length}}`}
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
const ExpandableMessage: Component<{ message: string; direction: "incoming" | "outgoing"; timestamp: number }> = (
  props
) => {
  const [isExpanded, setIsExpanded] = createSignal(false);

  const parsedData = createMemo(() => {
    try {
      return JSON.parse(props.message);
    } catch {
      return null;
    }
  });

  const isJsonMessage = () => parsedData() !== null;

  const getTooltipText = () => {
    const directionText = props.direction === "incoming" ? "Incoming" : "Outgoing";
    const jsonText = isJsonMessage()
      ? isExpanded()
        ? " - Click to collapse JSON"
        : " - Click to expand JSON"
      : "";

    return `${directionText} message${jsonText}`;
  };

  const formatTimestamp = (timestamp: number) => {
    const date = new Date(timestamp);
    return date.toLocaleTimeString('en-US', {
      hour12: false,
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit'
    });
  };

  return (
    <div class={styles["websocket-messages__message"]}>
      <div
        class={`${styles["websocket-messages__message-header"]} ${isJsonMessage() ? styles["websocket-messages__message-header--clickable"] : ""}`}
        onClick={() => isJsonMessage() && setIsExpanded(!isExpanded())}
        title={getTooltipText()}
      >
        <div class={styles["websocket-messages__message-meta"]}>
          {props.direction === "incoming" ? (
            <IncomingMessageIcon class={styles["websocket-messages__message-icon"]} />
          ) : (
            <OutgoingMessageIcon class={styles["websocket-messages__message-icon"]} />
          )}
          <span class={styles["websocket-messages__timestamp"]}>{formatTimestamp(props.timestamp)}</span>
        </div>
        <span class={styles["websocket-messages__message-text"]}>{props.message}</span>
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
  const [devicesState] = useDevices();
  const [wsStore, wsActions] = useWebSocket2();

  const [messageTypeFilter, setMessageTypeFilter] = createSignal("");
  const [deviceIdFilter, setDeviceIdFilter] = createSignal("");

  // Auto-scroll state
  let scrollContainerRef: HTMLDivElement | undefined;
  const [shouldAutoScroll, setShouldAutoScroll] = createSignal(true);

  // Handle scroll events to detect if user is at bottom
  const handleScroll = () => {
    if (!scrollContainerRef) return;

    const { scrollTop, scrollHeight, clientHeight } = scrollContainerRef;
    const isAtBottom = scrollTop + clientHeight >= scrollHeight - 10; // 10px tolerance

    if (isAtBottom) {
      setShouldAutoScroll(true);
    } else {
      setShouldAutoScroll(false);
    }
  };

  // Auto-scroll to bottom when new messages arrive and user is at bottom
  createEffect(() => {
    // Watch for changes in filtered messages
    const messages = filteredMessages();

    if (shouldAutoScroll() && scrollContainerRef && messages.length > 0) {
      // Use setTimeout to ensure DOM has updated
      setTimeout(() => {
        if (scrollContainerRef) {
          scrollContainerRef.scrollTop = scrollContainerRef.scrollHeight;
        }
      }, 0);
    }
  });

  // Set up scroll event listener
  onMount(() => {
    if (scrollContainerRef) {
      scrollContainerRef.addEventListener("scroll", handleScroll);
    }
  });

  // Parse messages to extract deviceId and type
  const parseMessage = (message: string) => {
    try {
      const parsed = JSON.parse(message);
      // Check if this is our new message format with direction
      if (parsed.data && parsed.direction && parsed.timestamp) {
        const innerParsed = JSON.parse(parsed.data);
        return {
          raw: parsed.data,
          type: innerParsed.type || "",
          deviceId: innerParsed.deviceId || "",
          direction: parsed.direction,
          timestamp: parsed.timestamp,
          parsed: innerParsed,
        };
      } else {
        // Legacy format - assume incoming
        return {
          raw: message,
          type: parsed.type || "",
          deviceId: parsed.deviceId || "",
          direction: "incoming" as const,
          timestamp: Date.now(),
          parsed,
        };
      }
    } catch {
      return {
        raw: message,
        type: "",
        deviceId: "",
        direction: "incoming" as const,
        timestamp: Date.now(),
        parsed: null,
      };
    }
  };

  // Filter messages based on the current filters
  const filteredMessages = createMemo(() => {
    const messages = wsStore.lastMessages.map(parseMessage);
    const typeFilter = messageTypeFilter().toLowerCase().trim();
    const deviceFilter = deviceIdFilter().trim();

    return messages.filter((msg) => {
      const typeMatch = !typeFilter || msg.type.toLowerCase().includes(typeFilter);
      const deviceMatch = !deviceFilter || msg.deviceId === deviceFilter;
      return typeMatch && deviceMatch;
    });
  });

  // Get unique device IDs from available devices
  const deviceOptions = createMemo(() => {
    return Object.keys(devicesState.devices).sort();
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

        <button onClick={() => wsActions.clearMessages()}>Clear Messages</button>
      </div>

      <div ref={scrollContainerRef} class={styles["websocket-messages__scrollable-content"]}>
        {filteredMessages().length === 0 ? (
          <div class={styles["websocket-messages__empty"]}>
            <MessageIcon class={styles["websocket-messages__empty-icon"]} />
            {wsStore.lastMessages.length === 0 ? "No messages yet" : "No messages match filters"}
          </div>
        ) : (
          <div class={styles["websocket-messages__list"]}>
            <For each={filteredMessages()}>
              {(message) => (
                <ExpandableMessage 
                  message={message.raw} 
                  direction={message.direction} 
                  timestamp={message.timestamp}
                />
              )}
            </For>
          </div>
        )}
      </div>
    </div>
  );
};

export { WebSocketMessages };
