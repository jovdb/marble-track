import { createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useWheelLoader } from "../../stores/WheelLoader";

interface WheelLoaderConfigProps {
  id: string;
  onClose: () => void;
}

export default function WheelLoaderConfig(props: WheelLoaderConfigProps) {
  const [device, actions] = useWheelLoader(props.id);

  const [name, setName] = createSignal(device()?.config?.name ?? "Wheel Loader");
  const [innerCenter, setInnerCenter] = createSignal<string>(
    String(device()?.config?.innerCenter ?? 0.5)
  );
  const [outerCenter, setOuterCenter] = createSignal<string>(
    String(device()?.config?.outerCenter ?? 0.5)
  );

  const toNumber = (value: string, fallback = 0.5) => {
    const num = Number(value);
    return Number.isFinite(num) ? num : fallback;
  };

  createEffect(() => {
    const config = device()?.config;
    if (!config) {
      return;
    }

    if (typeof config.name === "string") {
      setName(config.name);
    }
    if (typeof config.innerCenter === "number") {
      setInnerCenter(String(config.innerCenter));
    }
    if (typeof config.outerCenter === "number") {
      setOuterCenter(String(config.outerCenter));
    }
  });

  const handleSave = () => {
    actions.setDeviceConfig(props.id, {
      name: name(),
      innerCenter: toNumber(innerCenter()),
      outerCenter: toNumber(outerCenter()),
    });
  };

  return (
    <DeviceConfig device={device} onSave={handleSave} onClose={props.onClose} title="Wheel Loader">
      <DeviceConfigTable>
        <DeviceConfigRow>
          <DeviceConfigItem name="Name">
            <input
              type="text"
              value={name()}
              onInput={(e) => setName(e.currentTarget.value)}
              placeholder="Wheel Loader"
            />
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="Inner Center (0-1)">
            <div style={{ display: "flex", "align-items": "center", gap: "10px" }}>
              <input
                type="range"
                min="0"
                max="1"
                step="0.01"
                value={innerCenter()}
                onInput={(e) => setInnerCenter(e.currentTarget.value)}
              />
              <span>{toNumber(innerCenter()).toFixed(2)}</span>
            </div>
          </DeviceConfigItem>
        </DeviceConfigRow>

        <DeviceConfigRow>
          <DeviceConfigItem name="Outer Center (0-1)">
            <div style={{ display: "flex", "align-items": "center", gap: "10px" }}>
              <input
                type="range"
                min="0"
                max="1"
                step="0.01"
                value={outerCenter()}
                onInput={(e) => setOuterCenter(e.currentTarget.value)}
              />
              <span>{toNumber(outerCenter()).toFixed(2)}</span>
            </div>
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
