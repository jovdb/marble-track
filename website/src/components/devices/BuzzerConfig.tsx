import { createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useBuzzer } from "../../stores/Buzzer";
import PinSelect from "../PinSelect";
import { PinConfig, deserializePinConfig } from "../../interfaces/WebSockets";

interface BuzzerConfigProps {
  id: string;
  onClose: () => void;
}

export default function BuzzerConfig(props: BuzzerConfigProps) {
  const [device, { setDeviceConfig }] = useBuzzer(props.id);
  const [name, setName] = createSignal(device?.config?.name ?? "Buzzer");
  const [pin, setPin] = createSignal<PinConfig>(
    deserializePinConfig(device?.config?.pin ?? -1)
  );

  createEffect(() => {
    const config = device?.config;
    if (!config) {
      return;
    }

    if (typeof config.name === "string") {
      setName(config.name);
    }

    if (typeof config.pin === "number" || typeof config.pin === "object") {
      setPin(deserializePinConfig(config.pin));
    }
  });

  return (
    <DeviceConfig
      device={device}
      onSave={() =>
        setDeviceConfig({
          name: name(),
          pin: pin().pin,
        })
      }
      onClose={props.onClose}
    >
      <DeviceConfigTable>
        <DeviceConfigRow>
          <DeviceConfigItem name="Name:">
            <input
              type="text"
              value={name() || ""}
              onInput={(event) => setName(event.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Pin:">
            <PinSelect
              value={pin()}
              onChange={setPin}
              style={{ "margin-left": "0.5rem" }}
              excludeDeviceId={props.id}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
