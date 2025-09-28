import { For, onMount } from "solid-js";
import { useWebSocket2 } from "../hooks/useWebSocket2";
import { useDevices } from "../stores/Devices";
import { Led } from "./devices/Led";
import { logger } from "../stores/logger";
import { Button } from "./devices/Button";
import { Buzzer } from "./devices/Buzzer";
import { Gate } from "./devices/Gate";
import { PwmMotor } from "./devices/PwmMotor";
import { Servo } from "./devices/Servo";
import { Stepper } from "./devices/Stepper";
import { Wheel } from "./devices/Wheel";
import { Pwm } from "./devices/Pwm";
import { PwdDevice } from "./devices/PwdDevice";

import styles from "./Devices.module.css";

export function Devices() {
  const [webSocket] = useWebSocket2();
  const [devicesState, { loadDevices }] = useDevices();

  onMount(() => {
    // Request devices on mount
    if (webSocket.isConnected) {
      loadDevices();
    }
  });

  return (
    <div class={styles["app__devices-grid"]}>
      {Object.values(devicesState.devices).length === 0 ? (
        <div class={styles["app__no-devices"]}>
          {webSocket.isConnected
            ? "No devices available for control"
            : "Connect to see available devices"}
        </div>
      ) : (
        <For each={Object.values(devicesState.devices)}>
          {(device) => renderDeviceComponent(device)}
        </For>
      )}
    </div>
  );
}

export function renderDeviceComponent(device: { id: string; type: string }) {
  switch (device.type.toLowerCase()) {
    case "led":
      return <Led id={device.id} />;
    case "servo":
      return <Servo id={device.id} />;
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
    case "pwmmotor":
      return <PwmMotor id={device.id} />;
    case "pwm":
      return <Pwm id={device.id} />;
    case "pwddevice":
      return <PwdDevice id={device.id} />;

    default:
      logger.error(`Unknown device type: ${device.type}`);
      return null;
  }
}
