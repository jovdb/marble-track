import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";
import { useI2c } from "../../stores/I2c";
import I2cConfig from "./I2cConfig";
import { createMemo } from "solid-js";

export function I2c(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device] = useI2c(props.id);

  const sdaPin = createMemo(() => (device()?.config?.sdaPin as number) ?? 21);
  const sclPin = createMemo(() => (device()?.config?.sclPin as number) ?? 22);
  const icon = createMemo(() => {
    const type = device()?.type;
    return type ? getDeviceIcon(type, props.id) : null;
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <I2cConfig id={props.id} onClose={onClose} />}
      icon={icon()}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
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
