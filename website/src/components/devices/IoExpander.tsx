import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";
import { useDevice } from "../../stores/Devices";
import IoExpanderConfig from "./IoExpanderConfig";

export function IoExpander(props: { id: string }) {
  const [device] = useDevice(props.id);

  const deviceType = device()?.type;
  const config = () => device()?.config;

  const expanderType = () => config()?.expanderType ?? "Unknown";
  const i2cAddress = () => {
    const addr = config()?.i2cAddress;
    return addr !== undefined ? `0x${addr.toString(16).toUpperCase().padStart(2, "0")}` : "Unknown";
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
    >
      <div style={{ padding: "0.5rem", "font-size": "0.9rem" }}>
        <div>
          <strong>Type:</strong> {expanderType()}
        </div>
        <div>
          <strong>Address:</strong> {i2cAddress()}
        </div>
        <div>
          <strong>Available Pins:</strong> 0-{pinCount() - 1} ({pinCount()} total)
        </div>
      </div>
    </Device>
  );
}
