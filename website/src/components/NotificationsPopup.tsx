import { type Component, For, Show } from "solid-js";
import { Portal } from "solid-js/web";
import { useNotifications, type INotification } from "../stores/Notifications";
import { TrashIcon, WarningIcon } from "./icons/Icons";
import styles from "./NotificationsPopup.module.css";

interface NotificationsPopupProps {
  isOpen: boolean;
  onClose: () => void;
}

function formatTimestamp(ms: number): string {
  const totalSeconds = Math.floor(ms / 1000);
  const h = Math.floor(totalSeconds / 3600);
  const m = Math.floor((totalSeconds % 3600) / 60);
  const s = totalSeconds % 60;
  return `${String(h).padStart(2, "0")}:${String(m).padStart(2, "0")}:${String(s).padStart(2, "0")}`;
}

const NotificationItem: Component<{
  notification: INotification;
  onDismiss: (id: string) => void;
}> = (props) => {
  return (
    <li
      class={`${styles.notif__item} ${props.notification.notificationType === "warning" ? styles["notif__item--warning"] : styles["notif__item--info"]}`}
    >
      <span class={styles.notif__icon}>
        <WarningIcon width={16} height={16} />
      </span>
      <div class={styles.notif__body}>
        <div class={styles.notif__meta}>
          <span class={styles.notif__code}>{props.notification.code}</span>
          <span class={styles.notif__device}>{props.notification.deviceId}</span>
          <span class={styles.notif__time}>{formatTimestamp(props.notification.timestamp)}</span>
        </div>
        <p class={styles.notif__message}>{props.notification.message}</p>
      </div>
      <button
        class={styles.notif__dismiss}
        onClick={() => props.onDismiss(props.notification.id)}
        title="Dismiss"
        aria-label="Dismiss notification"
      >
        <TrashIcon width={14} height={14} />
      </button>
    </li>
  );
};

const NotificationsPopup: Component<NotificationsPopupProps> = (props) => {
  const [store, { dismiss, clearAll }] = useNotifications();

  return (
    <Portal mount={document.body}>
      <Show when={props.isOpen}>
        {/* backdrop */}
        <div class={styles.notif__backdrop} onClick={props.onClose} />

        <div class={styles.notif__panel}>
          <div class={styles.notif__header}>
            <h2 class={styles.notif__title}>Notifications</h2>
            <div class={styles.notif__headerActions}>
              <Show when={store.notifications.length > 0}>
                <button class={styles.notif__clearBtn} onClick={clearAll} title="Clear all">
                  Clear all
                </button>
              </Show>
              <button
                class={styles.notif__closeBtn}
                onClick={props.onClose}
                aria-label="Close notifications"
              >
                ×
              </button>
            </div>
          </div>

          <div class={styles.notif__content}>
            <Show
              when={store.notifications.length > 0}
              fallback={<p class={styles.notif__empty}>No notifications</p>}
            >
              <ul class={styles.notif__list}>
                <For each={store.notifications}>
                  {(n) => <NotificationItem notification={n} onDismiss={dismiss} />}
                </For>
              </ul>
            </Show>
          </div>
        </div>
      </Show>
    </Portal>
  );
};

export { NotificationsPopup };
