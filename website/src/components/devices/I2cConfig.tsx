import { createEffect, createSignal, createMemo } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import PinSelect from "../PinSelect";
import { PinConfig, deserializePinConfig } from "../../interfaces/WebSockets";
import { useI2c } from "../../stores/I2c";
import I2cScanner from "./I2cScanner";

interface I2cConfigProps {
  id: string;
  onClose: () => void;
}

export default function I2cConfig(props: I2cConfigProps) {
  const [device, { setDeviceConfig, scanBus }] = useI2c(props.id);

  const [name, setName] = createSignal<string>(device()?.config?.name ?? "I2C");
  const [sdaPin, setSdaPin] = createSignal<PinConfig>(
    deserializePinConfig(device()?.config?.sdaPin ?? 21)
  );
  const [sclPin, setSclPin] = createSignal<PinConfig>(
    deserializePinConfig(device()?.config?.sclPin ?? 22)
  );

  const foundAddresses = createMemo(() => device()?.state?.foundAddresses ?? []);

  createEffect(() => {
    const config = device()?.config;
    if (!config) {
      return;
    }

    if (typeof config.name === "string") {
      setName(config.name);
    }
    if (config.sdaPin !== undefined) {
      setSdaPin(deserializePinConfig(config.sdaPin));
    }
    if (config.sclPin !== undefined) {
      setSclPin(deserializePinConfig(config.sclPin));
    }
  });

  const handleSave = () => {
    setDeviceConfig({
      name: name(),
      sdaPin: sdaPin().pin,
      sclPin: sclPin().pin,
    });
  };

  return (
    <DeviceConfig device={device} onSave={handleSave} onClose={props.onClose}>
      <DeviceConfigTable>
        <DeviceConfigRow>
          <DeviceConfigItem name="Name:">
            <input
              type="text"
              value={name() || ""}
              onInput={(e) => setName(e.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="SDA Pin:">
            <PinSelect
              value={sdaPin()}
              onChange={setSdaPin}
              style={{ "margin-left": "0.5rem" }}
              excludeDeviceId={props.id}
              showExpanderPins={false}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="SCL Pin:">
            <PinSelect
              value={sclPin()}
              onChange={setSclPin}
              style={{ "margin-left": "0.5rem" }}
              excludeDeviceId={props.id}
              showExpanderPins={false}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>

      <I2cScanner foundAddresses={foundAddresses()} onScan={scanBus} />
    </DeviceConfig>
  );
}
