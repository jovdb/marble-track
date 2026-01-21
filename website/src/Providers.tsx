import { JSX } from "solid-js";
import { WebSocketProvider } from "./hooks/WebSocketProvider";
import { DevicesProvider } from "./stores/Devices";
import { SelectedDevicesProvider } from "./stores/SelectedDevices";

export function Providers(props: { children: JSX.Element }) {
  return (
    <WebSocketProvider>
      <DevicesProvider>
        <SelectedDevicesProvider>{props.children}</SelectedDevicesProvider>
      </DevicesProvider>
    </WebSocketProvider>
  );
}
