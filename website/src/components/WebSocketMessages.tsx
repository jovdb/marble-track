import { type Component, For } from "solid-js";
import { clearMessages, lastMessages } from "../hooks/useWebSocket";
import { RadioIcon, MessageIcon } from "./icons/DeviceIcons";
import styles from "./WebSocketMessages.module.css";

const WebSocketMessages: Component = () => {
  return (
    <div class={styles["websocket-messages"]}>
      <div class={styles["websocket-messages__header"]}>
        <h3 class={styles["websocket-messages__title"]}>
          <RadioIcon class={styles["websocket-messages__title-icon"]} />
          Recent Messages
        </h3>
        <button 
          class={styles["websocket-messages__clear-button"]}
          onClick={() => clearMessages()}
        >
          Clear Messages
        </button>
      </div>
      
      <div class={styles["websocket-messages__content"]}>
        {lastMessages().length === 0 ? (
          <div class={styles["websocket-messages__empty"]}>
            <MessageIcon class={styles["websocket-messages__empty-icon"]} />
            No messages yet
          </div>
        ) : (
          <div class={styles["websocket-messages__list"]}>
            <For each={lastMessages()}>
              {(message) => (
                <div class={styles["websocket-messages__message"]}>
                  {message}
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
