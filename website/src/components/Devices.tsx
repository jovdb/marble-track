import { For, onMount, createMemo } from "solid-js";
import { useWebSocket2 } from "../hooks/useWebSocket";
import { useDevices } from "../stores/Devices";
import { Led } from "./devices/Led";
import { logger } from "../stores/logger";
import { Button } from "./devices/Button";
import { Buzzer } from "./devices/Buzzer";
import { Gate } from "./devices/Gate";
import { Servo } from "./devices/Servo";
import { Stepper } from "./devices/Stepper";
import { Wheel } from "./devices/Wheel";
import { Lift } from "./devices/Lift";
import { Device } from "./devices/Device";

import styles from "./Devices.module.css";

export function Devices() {
  const [webSocket] = useWebSocket2();
  const [devicesState, { loadDevices }] = useDevices();

  // Compute top-level devices (exclude devices that are children of other devices)
  const topLevelDevices = createMemo(() =>
    Object.values(devicesState.devices).filter((device) => {
      return !Object.values(devicesState.devices).some((other) =>
        other.children?.some((child) => child.id === device.id)
      );
    })
  );

  onMount(() => {
    // Request devices on mount
    if (webSocket.isConnected) {
      loadDevices();
    }
  });

  return (
    <div class={styles["app__devices-grid"]}>
      {topLevelDevices().length === 0 ? (
        <div class={styles["app__no-devices"]}>
          {webSocket.isConnected
            ? "No devices available for control"
            : "Connect to see available devices"}
        </div>
      ) : (
        <For each={topLevelDevices()}>{(device) => <div>{renderDeviceComponent(device)}</div>}</For>
      )}
    </div>
  );
}

export function renderDeviceComponent(device: {
  id: string;
  type: string;
  children?: Array<{ id: string; type: string }>;
}) {
  switch (device.type.toLowerCase()) {
    case "led":
      return <Led id={device.id} />;
    case "button":
      return <Button id={device.id} />;
    case "buzzer":
      return <Buzzer id={device.id} />;
    case "stepper":
      return <Stepper id={device.id} />;
    case "gate":
      return <Gate id={device.id} />;
    case "wheel":
      return <Wheel id={device.id} />;
    case "servo":
      return <Servo id={device.id} />;
    case "lift":
      return <Lift id={device.id} />;
    default:
      // For unknown device types with children, render them in a parent device block
      if (device.children && device.children.length > 0) {
        return <Device id={device.id} />;
      }
      logger.error(`Unknown device type: ${device.type}`);
      return null;
  }
}
