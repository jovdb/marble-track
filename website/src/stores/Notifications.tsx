import { createContext, onCleanup, onMount, useContext } from "solid-js";
import { createStore, produce } from "solid-js/store";
import { IWebSocketActions, useWebSocket2 } from "../hooks/useWebSocket";
import { IWsReceiveNotificationMessage, NotificationType } from "../interfaces/WebSockets";

export interface INotification {
  id: string;
  code: string;
  message: string;
  deviceId: string;
  notificationType: NotificationType;
  timestamp: number;
  /** True once the user has opened the notification panel */
  read: boolean;
}

export interface INotificationsStore {
  notifications: INotification[];
  unreadCount: number;
}

const MAX_NOTIFICATIONS = 100;

export function createNotificationsStore({
  subscribe,
}: Pick<IWebSocketActions, "subscribe">) {
  const [store, setStore] = createStore<INotificationsStore>({
    notifications: [],
    unreadCount: 0,
  });

  const handleMessage: Parameters<IWebSocketActions["subscribe"]>[0] = (message) => {
    if (message.type !== "device-notification") return;

    const n = message as IWsReceiveNotificationMessage;

    setStore(
      produce((draft) => {
        draft.notifications.unshift({
          id: n.id,
          code: n.code,
          message: n.message,
          deviceId: n.deviceId,
          notificationType: n.notificationType,
          timestamp: n.timestamp,
          read: false,
        });

        // Cap list size
        if (draft.notifications.length > MAX_NOTIFICATIONS) {
          draft.notifications.length = MAX_NOTIFICATIONS;
        }

        draft.unreadCount = draft.notifications.filter((x) => !x.read).length;
      })
    );
  };

  onMount(() => {
    const cleanup = subscribe(handleMessage);
    onCleanup(cleanup);
  });

  const markAllRead = () => {
    setStore(
      produce((draft) => {
        draft.notifications.forEach((n) => (n.read = true));
        draft.unreadCount = 0;
      })
    );
  };

  const dismiss = (id: string) => {
    setStore(
      produce((draft) => {
        draft.notifications = draft.notifications.filter((n) => n.id !== id);
        draft.unreadCount = draft.notifications.filter((x) => !x.read).length;
      })
    );
  };

  const clearAll = () => {
    setStore({ notifications: [], unreadCount: 0 });
  };

  return [store, { markAllRead, dismiss, clearAll }] as const;
}

type NotificationsActions = {
  markAllRead: () => void;
  dismiss: (id: string) => void;
  clearAll: () => void;
};

const NotificationsContext = createContext<
  readonly [INotificationsStore, NotificationsActions]
>([
  { notifications: [], unreadCount: 0 },
  {
    markAllRead: () => {},
    dismiss: () => {},
    clearAll: () => {},
  },
]);

export function NotificationsProvider(props: { children: any }) {
  const [, actions] = useWebSocket2();
  const [store, notifActions] = createNotificationsStore(actions);

  return (
    <NotificationsContext.Provider value={[store, notifActions] as const}>
      {props.children}
    </NotificationsContext.Provider>
  );
}

export function useNotifications() {
  return useContext(NotificationsContext);
}
