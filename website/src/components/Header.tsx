import { type Component, createMemo, createSignal } from "solid-js";
import styles from "./Header.module.css";
import logo from "../assets/logo-64.png";
import { useWebSocket2 } from "../hooks/useWebSocket";
import {
  BellIcon,
  BugIcon,
  ConnectedIcon,
  DisconnectedIcon,
  RestartIcon,
  WifiConnectedIcon,
} from "./icons/Icons";
import { TransparentButton } from "./TransparentButton";
import { NetworkConfig } from "./NetworkConfig";
import { isSerialOpen, toggleSerialPanel } from "../stores/serial";
import { useSystemInfo } from "../stores/SystemInfo";
import { SystemInfoPopup } from "./SystemInfoPopup";
import { useNotifications } from "../stores/Notifications";
import { NotificationsPopup } from "./NotificationsPopup";
import { BatteryStateIcon } from "./devices/BatteryStateIcon";

const Header: Component = () => {
  const [webSocket, { sendMessage }] = useWebSocket2();
  const [systemInfo] = useSystemInfo();
  const [notifications] = useNotifications();
  const [isNetworkPopupOpen, setIsNetworkPopupOpen] = createSignal(false);
  const [isSystemInfoOpen, setIsSystemInfoOpen] = createSignal(false);
  const [isNotificationsOpen, setIsNotificationsOpen] = createSignal(false);
  const [, { markAllRead }] = useNotifications();

  const titleText = createMemo(() => {
    const parts = [`Website build: ${__BUILD_DATE__}`];

    if (systemInfo.firmwareBuild) {
      parts.push(`ESP32 build: ${systemInfo.firmwareBuild}`);
    }
    if (systemInfo.hostname) {
      parts.push(`Host: ${systemInfo.hostname}.local`);
    }
    if (systemInfo.ipAddress) {
      parts.push(`IP: ${systemInfo.ipAddress}`);
    }

    return parts.join("\n");
  });

  const connectionTitle = createMemo(() => {
    return webSocket.isConnected
      ? `Connected to ${webSocket.url}`
      : `Disconnected from ${webSocket.url}`;
  });

  const handleReset = () => {
    sendMessage({ type: "restart" });
  };

  const handleNetworkClick = () => {
    setIsNetworkPopupOpen(true);
  };

  const handleSystemInfoClick = () => {
    if (webSocket.isConnected) {
      sendMessage({ type: "system-info" });
    }
    setIsSystemInfoOpen(true);
  };

  const handleNotificationsClick = () => {
    setIsNotificationsOpen(true);
    markAllRead();
  };

  return (
    <>
      <header class={styles.header}>
        <div class={styles.header__left}>
          <img
            src={logo}
            alt="Logo"
            class={`${styles.header__logo} ${webSocket.isConnected ? styles["header__logo--connected"] : ""}`}
          />
          <h1 class={styles.header__title} title={titleText()}>
            Marble Manager
          </h1>
        </div>
        <div class={styles.header__right}>
          <BatteryStateIcon deviceId="battery" />

          <span title={connectionTitle()} class={styles.header__statusIcon}>
            {webSocket.isConnected ? <ConnectedIcon /> : <DisconnectedIcon />}
          </span>
          <TransparentButton
            disabled={!webSocket.isConnected}
            title="Wifi connection"
            onClick={handleNetworkClick}
            class={styles.header__button}
          >
            <WifiConnectedIcon />
          </TransparentButton>

          <TransparentButton
            disabled={!webSocket.isConnected}
            title="Restart device"
            onClick={handleReset}
            class={styles.header__button}
          >
            <RestartIcon />
          </TransparentButton>

          <TransparentButton
            title="ESP32 information"
            onClick={handleSystemInfoClick}
            class={`${styles.header__button} ${styles["header__icon-button"]}`}
            aria-label="ESP32 information"
          >
            <svg
              width="20"
              height="20"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
            >
              <circle cx="12" cy="12" r="9" />
              <path d="M12 10v6" />
              <path d="M12 7h.01" />
            </svg>
          </TransparentButton>

          <TransparentButton
            title={isSerialOpen() ? "Hide USB monitoring" : "Show USB monitoring"}
            aria-pressed={isSerialOpen()}
            onClick={toggleSerialPanel}
            class={`${styles.header__button} ${isSerialOpen() ? styles["header__button--active"] : ""}`}
          >
            <BugIcon />
          </TransparentButton>

          <TransparentButton
            title="Notifications"
            onClick={handleNotificationsClick}
            class={`${styles.header__button} ${styles.header__notifButton} ${notifications.unreadCount > 0 ? styles["header__button--active"] : ""}`}
            aria-label={`Notifications${notifications.unreadCount > 0 ? ` (${notifications.unreadCount} unread)` : ""}`}
          >
            <BellIcon width={20} height={20} />
            {notifications.unreadCount > 0 && (
              <span class={styles.header__badge}>{notifications.unreadCount}</span>
            )}
          </TransparentButton>
        </div>
      </header>

      <NetworkConfig isOpen={isNetworkPopupOpen()} onClose={() => setIsNetworkPopupOpen(false)} />
      <SystemInfoPopup
        isOpen={isSystemInfoOpen()}
        onClose={() => setIsSystemInfoOpen(false)}
        systemInfo={systemInfo}
      />
      <NotificationsPopup
        isOpen={isNotificationsOpen()}
        onClose={() => setIsNotificationsOpen(false)}
      />
    </>
  );
};

export { Header };
