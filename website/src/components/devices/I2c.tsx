import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";
import { useI2c } from "../../stores/I2c";
import I2cConfig from "./I2cConfig";
import I2cScanner from "./I2cScanner";
import I2cTester from "./I2cTester";
import { createMemo, createSignal } from "solid-js";

export function I2c(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, { scanBus, writeI2c, readI2c }] = useI2c(props.id);
  const [selectedAddress, setSelectedAddress] = createSignal(0);

  const sdaPin = createMemo(() => (device()?.config?.sdaPin as number) ?? 21);
  const sclPin = createMemo(() => (device()?.config?.sclPin as number) ?? 22);
  const foundAddresses = createMemo(() => device()?.state?.foundAddresses ?? []);
  const lastOp = createMemo(() => device()?.state?.lastOp);

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
        <div style={{ "margin-bottom": "0.5rem" }}>
          <strong>SDA Pin:</strong> {sdaPin()}
        </div>
        <div style={{ "margin-bottom": "0.5rem" }}>
          <strong>SCL Pin:</strong> {sclPin()}
        </div>

        <I2cScanner
          foundAddresses={foundAddresses()}
          onScan={scanBus}
          onSelectAddress={setSelectedAddress}
        />

        <I2cTester
          address={selectedAddress()}
          onAddressChange={setSelectedAddress}
          onSend={writeI2c}
          onRead={readI2c}
          lastOp={lastOp()}
        />
      </div>
    </Device>
  );
}
