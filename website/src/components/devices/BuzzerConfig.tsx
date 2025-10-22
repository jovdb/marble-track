import { createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useBuzzer } from "../../stores/Buzzer";

interface BuzzerConfigProps {
  id: string;
  onClose: () => void;
}

export default function BuzzerConfig(props: BuzzerConfigProps) {
  const [device, { setDeviceConfig }] = useBuzzer(props.id);
  const [name, setName] = createSignal(device?.config?.name ?? "Buzzer");
  const [pin, setPin] = createSignal(device?.config?.pin ?? -1);

  createEffect(() => {
    const config = device?.config;
    if (!config) {
      return;
    }

    if (typeof config.name === "string") {
      setName(config.name);
    }

    if (typeof config.pin === "number") {
      setPin(config.pin);
    }
  });

  return (
    <DeviceConfig
      device={device}
      onSave={() =>
        setDeviceConfig({
          name: name(),
          pin: pin(),
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
            <input
              type="number"
              value={pin() ?? ""}
              min={-1}
              max={50}
              onInput={(event) => setPin(Number(event.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
