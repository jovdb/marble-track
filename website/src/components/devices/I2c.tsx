import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";
import { useI2c } from "../../stores/I2c";
import I2cConfig from "./I2cConfig";

export function I2c(props: { id: string }) {
  const [device] = useI2c(props.id);

  const deviceType = device?.type;
  const config = () => device?.config;

  const sdaPin = () => (config()?.sdaPin as number) ?? 21;
  const sclPin = () => (config()?.sclPin as number) ?? 22;

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <I2cConfig id={props.id} onClose={onClose} />}
      icon={deviceType ? getDeviceIcon(deviceType) : null}
      stateComponent={() => null}
    >
      <div style={{ padding: "0.5rem", "font-size": "0.9rem" }}>
        <div>
          <strong>SDA Pin:</strong> {sdaPin()}
        </div>
        <div>
          <strong>SCL Pin:</strong> {sclPin()}
        </div>
      </div>
    </Device>
  );
}
