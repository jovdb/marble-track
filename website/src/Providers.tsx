import { JSX } from "solid-js";
import { WebSocketProvider } from "./hooks/WebSocketProvider";
import { DevicesProvider } from "./stores/Devices";

export function Providers(props: { children: JSX.Element }) {
  return (
    <WebSocketProvider>
      <DevicesProvider>{props.children} </DevicesProvider>
    </WebSocketProvider>
  );
}
