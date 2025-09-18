import { createSignal, onMount } from "solid-js";
import DeviceConfig from "./DeviceConfig";
import { useDevice } from "../../stores/Devices";
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
      <>
        <div>
          <label>
            Name:
            <input
              type="text"
              value={name() || ""}
              onInput={(e) => setName(e.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            />
          </label>
        </div>
        <div>
          <label>
            Pin number:
            <input
              type="number"
              value={pin() || "1"}
              min={1}
              max={40}
              onInput={(e) => setPin(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            />
          </label>
        </div>
      </>
    </DeviceConfig>
  );
}
