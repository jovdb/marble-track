import { createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { usePwm } from "../../stores/Pwm";

interface PwmConfigProps {
  id: string;
  onClose: () => void;
}

export default function PwmConfig(props: PwmConfigProps) {
  const [device, { setDeviceConfig }] = usePwm(props.id);

  const [name, setName] = createSignal(device?.config?.name ?? "");
  const [pin, setPin] = createSignal(device?.config?.pin ?? -1);
  const [frequency, setFrequency] = createSignal(device?.config?.frequency ?? 5000);
  const [resolution, setResolution] = createSignal(device?.config?.resolution ?? 8);
  const [channel, setChannel] = createSignal(device?.config?.channel ?? 0);

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
    if (typeof config.frequency === "number") {
      setFrequency(config.frequency);
    }
    if (typeof config.resolution === "number") {
      setResolution(config.resolution);
    }
    if (typeof config.channel === "number") {
      setChannel(config.channel);
    }
  });

  return (
    <DeviceConfig
      device={device}
      onSave={() => {
        setDeviceConfig({
          name: name(),
          pin: pin(),
          frequency: frequency(),
          resolution: resolution(),
          channel: channel(),
        });
      }}
      onClose={props.onClose}
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
              min={-1}
              max={50}
              onInput={(e) => setPin(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Frequency (Hz):">
            <input
              type="number"
              value={frequency() || "5000"}
              min={1}
              max={40000}
              onInput={(e) => setFrequency(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Resolution (bits):">
            <select
              value={resolution()}
              onChange={(e) => setResolution(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            >
              <option value={8}>8-bit (0-255)</option>
              <option value={10}>10-bit (0-1023)</option>
              <option value={12}>12-bit (0-4095)</option>
              <option value={16}>16-bit (0-65535)</option>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Channel:">
            <input
              type="number"
              value={channel() || "0"}
              min={0}
              max={15}
              onInput={(e) => setChannel(Number(e.currentTarget.value))}
              style={{ "margin-left": "0.5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
