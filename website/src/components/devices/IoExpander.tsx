import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";
import { IDevice, useDevices } from "../../stores/Devices";
import { useIoExpander } from "../../stores/IoExpander";
import IoExpanderConfig from "./IoExpanderConfig";
import styles from "./Device.module.css";
import { II2cConfig, II2cState } from "../../stores/I2c";

export function IoExpander(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, actions] = useIoExpander(props.id);
  const [devicesStore] = useDevices();

  const deviceType = device?.type;
  const config = () => device?.config;
  const state = () => device?.state;

  const expanderState = () => state()?.state;
  const expanderType = () => config()?.expanderType ?? "Unknown";
  const i2cAddress = () => {
    const addr = config()?.i2cAddress;
    return addr !== undefined ? `0x${addr.toString(16).toUpperCase().padStart(2, "0")}` : "Unknown";
  };
  const i2cDeviceName = () => {
    const i2cDeviceId = config()?.i2cDeviceId;
    if (!i2cDeviceId) return "No I²C bus selected";
    const i2cDevice = devicesStore.devices[i2cDeviceId] as
      | IDevice<II2cState, II2cConfig>
      | undefined;
    return i2cDevice?.config?.name || i2cDevice?.id || "Unknown I²C bus";
  };
  const pinCount = () => {
    const type = expanderType();
    if (type === "PCF8574") return 8;
    if (type === "PCF8575" || type === "MCP23017") return 16;
    return 0;
  };

  const statusText = () => {
    const s = expanderState();
    if (s === "Ready") return <span style={{ color: "green" }}>Connected</span>;
    if (s === "Init") return <span style={{ color: "cornflowerblue" }}>Initializing&hellip;</span>;
    if (s === "Error") return <span style={{ color: "red" }}>Error</span>;
    return <span>Unknown</span>;
  };

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <IoExpanderConfig id={props.id} onClose={onClose} />}
      icon={deviceType ? getDeviceIcon(deviceType, props.id) : null}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div style={{ padding: "0.5rem", "font-size": "0.9rem" }}>
        <div>
          <strong>Status:</strong> {statusText()}
        </div>
        <div>
          <strong>Type:</strong> {expanderType()}
        </div>
        <div>
          <strong>I²C Bus:</strong> {i2cDeviceName()}
        </div>
        <div>
          <strong>Address:</strong> {i2cAddress()}
        </div>
        <div>
          <strong>Available Pins:</strong> {pinCount()}
        </div>
      </div>
      <div class={styles.device__controls}>
        <button
          class={styles.device__button}
          onClick={() => actions.init()}
          disabled={expanderState() === "Ready" || expanderState() === "Init"}
          title="Re-probe the I²C bus and re-initialise the expander"
        >
          Init
        </button>
      </div>
    </Device>
  );
}
