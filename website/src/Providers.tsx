import { JSX } from "solid-js";
import { WebSocketProvider } from "./hooks/WebSocketProvider";
import { DevicesProvider } from "./stores/Devices";
import { SelectedDevicesProvider } from "./stores/SelectedDevices";
import { SystemInfoProvider } from "./stores/SystemInfo";
import { NotificationsProvider } from "./stores/Notifications";

export function Providers(props: { children: JSX.Element }) {
  return (
    <WebSocketProvider>
      <SystemInfoProvider>
        <NotificationsProvider>
          <DevicesProvider>
            <SelectedDevicesProvider>{props.children}</SelectedDevicesProvider>
          </DevicesProvider>
        </NotificationsProvider>
      </SystemInfoProvider>
    </WebSocketProvider>
  );
}
