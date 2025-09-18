import { createSignal, onMount } from "solid-js";
import DeviceConfig from "./DeviceConfig";
import { useDevice } from "../../stores/Devices";
import { useLed } from "../../stores/Led";

interface LedConfigProps {
  id: string;
}

export default function LedConfig(props: LedConfigProps) {
  const [device, { getDeviceConfig, setDeviceConfig }] = useLed(props.id);
  //const [name, setName] = createSignal("Led");
  //const [pin, setPin] = createSignal(device.config?.pin || "");

  onMount(() => {
    getDeviceConfig();
  });

  return (
    <DeviceConfig
      id={props.id}
      onSave={() => {
        setDeviceConfig({
          name: "Jo",
          pin: 3,
        });
      }}
    >
      <>
        <div>
          <label>
            Name:
            <input
              type="text"
              value={device.config?.name || ""}
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
              value={device.config?.pin || ""}
              min={0}
              onInput={(e) => setPin(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            />
          </label>
        </div>
      </>
    </DeviceConfig>
  );
}
