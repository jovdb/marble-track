import { createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useDevice } from "../../stores/Devices";
import PinSelect from "../PinSelect";
import { useWebSocket2 } from "../../hooks/useWebSocket";
import { PinConfig, deserializePinConfig } from "../../interfaces/WebSockets";

interface I2cConfigProps {
  id: string;
  onClose: () => void;
}

export default function I2cConfig(props: I2cConfigProps) {
  const [device] = useDevice(props.id);
  const [, { sendMessage }] = useWebSocket2();

  const [name, setName] = createSignal<string>((device?.config?.name as string) ?? "I2C");
  const [sdaPin, setSdaPin] = createSignal<PinConfig>(
    deserializePinConfig((device?.config?.sdaPin as number | Record<string, any>) ?? 21)
  );
  const [sclPin, setSclPin] = createSignal<PinConfig>(
    deserializePinConfig((device?.config?.sclPin as number | Record<string, any>) ?? 22)
  );

  createEffect(() => {
    const config = device?.config;
    if (!config) {
      return;
    }

    if (typeof config.name === "string") {
      setName(config.name);
    }
    if (config.sdaPin !== undefined) {
      setSdaPin(deserializePinConfig(config.sdaPin as number | Record<string, any>));
    }
    if (config.sclPin !== undefined) {
      setSclPin(deserializePinConfig(config.sclPin as number | Record<string, any>));
    }
  });

  const handleSave = () => {
    sendMessage({
      type: "device-save-config",
      deviceId: props.id,
      config: {
        name: name(),
        sdaPin: sdaPin().pin,
        sclPin: sclPin().pin,
      },
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
    </DeviceConfig>
  );
}
