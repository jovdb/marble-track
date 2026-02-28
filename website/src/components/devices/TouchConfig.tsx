import { createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import PinSelect from "../PinSelect";
import { PinConfig, deserializePinConfig } from "../../interfaces/WebSockets";
import { useTouch } from "../../stores/Touch";
import { CURRENT_ESP32_TYPE, getTouchPinsForEsp32Type } from "../../utils/esp32Pins";

interface TouchConfigProps {
  id: string;
  onClose: () => void;
}

function normalizeName(value: unknown): string {
  if (typeof value === "string" && value.trim().length > 0) {
    return value;
  }
  return "Touch";
}

function normalizePinConfig(value: unknown): PinConfig {
  if (typeof value === "number" || typeof value === "object") {
    return deserializePinConfig(value as number | Record<string, any>);
  }

  return deserializePinConfig(-1);
}

function normalizeThreshold(value: unknown): number {
  if (typeof value === "number" && Number.isFinite(value)) {
    return Math.max(0, Math.floor(value));
  }

  return 30000;
}

function normalizeDuration(value: unknown): number {
  if (typeof value === "number" && Number.isFinite(value)) {
    return Math.max(1, Math.floor(value));
  }

  return 50;
}

export default function TouchConfig(props: TouchConfigProps) {
  const [device, { setDeviceConfig }] = useTouch(props.id);
  const touchPins = getTouchPinsForEsp32Type(CURRENT_ESP32_TYPE);

  const [name, setName] = createSignal(normalizeName(device?.config?.name));
  const [pin, setPin] = createSignal<PinConfig>(normalizePinConfig(device?.config?.pin));
  const [threshold, setThreshold] = createSignal(
    String(normalizeThreshold(device?.config?.threshold))
  );
  const [durationMs, setDurationMs] = createSignal(
    String(normalizeDuration(device?.config?.durationMs))
  );

  const toNumber = (value: string, fallback: number) => {
    const number = Number(value);
    if (!Number.isFinite(number)) {
      return fallback;
    }

    return number;
  };

  createEffect(() => {
    const config = device?.config;
    if (!config) {
      return;
    }

    setName(normalizeName(config.name));
    setPin(normalizePinConfig(config.pin));
    setThreshold(String(normalizeThreshold(config.threshold)));
    setDurationMs(String(normalizeDuration(config.durationMs)));
  });

  const handleSave = () => {
    setDeviceConfig({
      name: name(),
      pin: pin().pin,
      threshold: Math.max(0, Math.floor(toNumber(threshold(), 30000))),
      durationMs: Math.max(1, Math.floor(toNumber(durationMs(), 50))),
    });
  };

  return (
    <DeviceConfig device={device} onSave={handleSave} onClose={props.onClose}>
      <DeviceConfigTable>
        <DeviceConfigRow>
          <DeviceConfigItem name="Name:">
            <input
              type="text"
              value={name()}
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
              showExpanderPins={false}
              availableGpioPins={touchPins}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Threshold:">
            <input
              type="number"
              value={threshold()}
              min={0}
              onInput={(event) => setThreshold(event.currentTarget.value)}
              style={{ "margin-left": "0.5rem", width: "6rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Duration (ms):">
            <input
              type="number"
              value={durationMs()}
              min={1}
              onInput={(event) => setDurationMs(event.currentTarget.value)}
              style={{ "margin-left": "0.5rem", width: "6rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
