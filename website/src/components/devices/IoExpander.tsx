import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";
import { useDevice, useDevices } from "../../stores/Devices";
import IoExpanderConfig from "./IoExpanderConfig";

export function IoExpander(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device] = useDevice(props.id);
  const [devicesStore] = useDevices();

  const deviceType = device?.type;
  const config = () => device?.config;

  const expanderType = () => (config()?.expanderType as string) ?? "Unknown";
  const i2cAddress = () => {
    const addr = config()?.i2cAddress as number | undefined;
    return addr !== undefined ? `0x${addr.toString(16).toUpperCase().padStart(2, "0")}` : "Unknown";
  };
  const i2cDeviceName = () => {
    const i2cDeviceId = config()?.i2cDeviceId as string;
    if (!i2cDeviceId) return "No I2C bus selected";
    const i2cDevice = devicesStore.devices[i2cDeviceId];
    return (i2cDevice?.config?.name as string) || i2cDevice?.id || "Unknown I2C bus";
  };
  const pinCount = () => {
    const type = expanderType();
    if (type === "PCF8574") return 8;
    if (type === "PCF8575" || type === "MCP23017") return 16;
    return 0;
  };

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <IoExpanderConfig id={props.id} onClose={onClose} />}
      icon={deviceType ? getDeviceIcon(deviceType) : null}
      stateComponent={() => null}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div style={{ padding: "0.5rem", "font-size": "0.9rem" }}>
        <div>
          <strong>Type:</strong> {expanderType()}
        </div>
        <div>
          <strong>I2C Bus:</strong> {i2cDeviceName()}
        </div>
        <div>
          <div>
            <strong>Address:</strong> {i2cAddress()}
          </div>
          <strong>Available Pins:</strong> 0-{pinCount() - 1} ({pinCount()} total)
        </div>
      </div>
    </Device>
  );
}
