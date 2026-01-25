import { createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useHv20tAudio } from "../../stores/Hv20tAudio";
import PinSelect from "../PinSelect";
import { PinConfig, deserializePinConfig } from "../../interfaces/WebSockets";

interface Hv20tAudioConfigProps {
  id: string;
  onClose: () => void;
}

export default function Hv20tAudioConfig(props: Hv20tAudioConfigProps) {
  const [device, { setDeviceConfig }] = useHv20tAudio(props.id);
  const [name, setName] = createSignal(device?.config?.name ?? "HV20T");
  const [rxPin, setRxPin] = createSignal<PinConfig>(
    deserializePinConfig(device?.config?.rxPin ?? -1)
  );
  const [txPin, setTxPin] = createSignal<PinConfig>(
    deserializePinConfig(device?.config?.txPin ?? -1)
  );
  const [busyPin, setBusyPin] = createSignal<PinConfig>(
    deserializePinConfig(device?.config?.busyPin ?? -1)
  );
  const [defaultVolumePercent, setDefaultVolumePercent] = createSignal(
    device?.config?.defaultVolumePercent ?? 50
  );

  createEffect(() => {
    const config = device?.config;
    if (!config) {
      return;
    }

    if (typeof config.name === "string") {
      setName(config.name);
    }
    if (typeof config.rxPin === "number" || typeof config.rxPin === "object") {
      setRxPin(deserializePinConfig(config.rxPin));
    }
    if (typeof config.txPin === "number" || typeof config.txPin === "object") {
      setTxPin(deserializePinConfig(config.txPin));
    }
    if (typeof config.busyPin === "number" || typeof config.busyPin === "object") {
      setBusyPin(deserializePinConfig(config.busyPin));
    }
    if (typeof config.defaultVolumePercent === "number") {
      setDefaultVolumePercent(config.defaultVolumePercent);
    }
  });

  return (
    <DeviceConfig
      device={device}
      onSave={() =>
        setDeviceConfig({
          name: name(),
          rxPin: rxPin(),
          txPin: txPin(),
          busyPin: busyPin(),
          defaultVolumePercent: Number(defaultVolumePercent()),
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
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="RX Pin:">
            <PinSelect value={rxPin()} onChange={setRxPin} excludeDeviceId={props.id} />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="TX Pin:">
            <PinSelect value={txPin()} onChange={setTxPin} excludeDeviceId={props.id} />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Busy Pin:">
            <PinSelect value={busyPin()} onChange={setBusyPin} excludeDeviceId={props.id} />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Default volume (%):">
            <input
              type="number"
              min="0"
              max="100"
              value={defaultVolumePercent()}
              onInput={(event) => setDefaultVolumePercent(Number(event.currentTarget.value))}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
