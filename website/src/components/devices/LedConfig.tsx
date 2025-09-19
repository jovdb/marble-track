import { createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useLed } from "../../stores/Led";

interface LedConfigProps {
  id: string;
}

export default function LedConfig(props: LedConfigProps) {
  const [device, { setDeviceConfig }] = useLed(props.id);
  const [name, setName] = createSignal(device?.config?.name || "Led");
  const [pin, setPin] = createSignal(device?.config?.pin || 1);

  return (
    <DeviceConfig
      id={props.id}
      onSave={() => {
        setDeviceConfig({
          name: name(),
          pin: pin(),
        });
      }}
    >
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
          <DeviceConfigItem name="Pin:">
            <input
              type="number"
              value={pin() || "1"}
              min={1}
              max={50}
              onInput={(e) => setPin(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
