import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";
import { IDevice, useDevices } from "../../stores/Devices";
import { usePwmExpander } from "../../stores/PwmExpander";
import PwmExpanderConfig from "./PwmExpanderConfig";
import styles from "./Device.module.css";
import { II2cConfig, II2cState } from "../../stores/I2c";

export function PwmExpander(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, actions] = usePwmExpander(props.id);
  const [devicesStore] = useDevices();
  const devicesState = () => devicesStore; // Wrap in a function to avoid stale closure issues
  const deviceType = device?.type;
  const config = () => device?.config;
  const state = () => device?.state;

  const expanderState = () => state()?.state;

  const i2cAddress = () => {
    const addr = config()?.i2cAddress;
    return addr !== undefined ? `0x${addr.toString(16).toUpperCase().padStart(2, "0")}` : "Unknown";
  };

  const i2cDeviceName = () => {
    const i2cDeviceId = config()?.i2cDeviceId;
    if (!i2cDeviceId) return "No I²C bus selected";
    const i2cDevice = devicesState().devices[i2cDeviceId] as
      | IDevice<II2cState, II2cConfig>
      | undefined;
    return i2cDevice?.config?.name || i2cDevice?.id || "Unknown I²C bus";
  };

  const frequency = () => {
    const freq = config()?.frequency;
    return freq !== undefined ? `${freq} Hz` : "Unknown";
  };

  const statusText = () => {
    const s = expanderState();
    if (s === "Ready") return <span style={{ color: "green" }}>Connected</span>;
    if (s === "Init") return <span style={{ color: "cornflowerblue" }}>Initializing&hellip;</span>;
    return <span>Unknown</span>;
  };

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <PwmExpanderConfig id={props.id} onClose={onClose} />}
      icon={deviceType ? getDeviceIcon(deviceType, props.id) : null}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div style={{ padding: "0.5rem", "font-size": "0.9rem" }}>
        <div>
          <strong>Status:</strong> {statusText()}
        </div>
        <div>
          <strong>Type:</strong> PCA9685
        </div>
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
          <strong>Available Pins:</strong> 16
        </div>
      </div>
      <div class={styles.device__controls}>
        <button
          class={styles.device__button}
          onClick={() => actions.init()}
          disabled={expanderState() === "Ready" || expanderState() === "Init"}
          title="Re-probe the I²C bus and re-initialize the PCA9685"
        >
          Init
        </button>
      </div>
    </Device>
  );
}
