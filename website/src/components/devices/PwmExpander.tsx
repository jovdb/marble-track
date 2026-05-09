import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";
import { useDevice, useDevices } from "../../stores/Devices";
import PwmExpanderConfig from "./PwmExpanderConfig";

export function PwmExpander(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device] = useDevice(props.id);
  const [devicesStore] = useDevices();

  const deviceType = device?.type;
  const config = () => device?.config;

  const i2cAddress = () => {
    const addr = config()?.i2cAddress as number | undefined;
    return addr !== undefined ? `0x${addr.toString(16).toUpperCase().padStart(2, "0")}` : "Unknown";
  };

  const i2cDeviceName = () => {
    const i2cDeviceId = config()?.i2cDeviceId as string;
    if (!i2cDeviceId) return "No I²C bus selected";
    const i2cDevice = devicesStore.devices[i2cDeviceId];
    return (i2cDevice?.config?.name as string) || i2cDevice?.id || "Unknown I²C bus";
  };

  const frequency = () => {
    const freq = config()?.frequency as number | undefined;
    return freq !== undefined ? `${freq} Hz` : "Unknown";
  };

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <PwmExpanderConfig id={props.id} onClose={onClose} />}
      icon={deviceType ? getDeviceIcon(deviceType, props.id) : null}
      stateComponent={() => null}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div style={{ padding: "0.5rem", "font-size": "0.9rem" }}>
        <div>
          <strong>I²C Bus:</strong> {i2cDeviceName()}
        </div>
        <div>
          <strong>Address:</strong> {i2cAddress()}
        </div>
        <div>
          <strong>Frequency:</strong> {frequency()}
        </div>
        <div>
          <strong>Available Pins:</strong> 0-15 (16 total)
        </div>
      </div>
    </Device>
  );
}
