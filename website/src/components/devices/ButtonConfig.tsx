import { For, createEffect, createSignal } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useButton } from "../../stores/Button";

interface ButtonConfigProps {
  id: string;
  onClose: () => void;
}

const BUTTON_PIN_MODE_OPTIONS = [
  { label: "Floating", value: "Floating" },
  { label: "Pull-up", value: "PullUp" },
  { label: "Pull-down", value: "PullDown" },
] as const;

type ButtonPinMode = (typeof BUTTON_PIN_MODE_OPTIONS)[number]["value"];

function normalizeName(value: unknown): string {
  if (typeof value === "string" && value.trim().length > 0) {
    return value;
  }
  return "";
}

function normalizePin(value: unknown): number {
  if (typeof value === "number" && Number.isFinite(value)) {
    return value;
  }
  return -1;
}

function normalizeDebounce(value: unknown): number {
  if (typeof value === "number" && Number.isFinite(value) && value >= 0) {
    return value;
  }
  return 50;
}

function normalizePinMode(pinModeValue: unknown): ButtonPinMode {
  if (pinModeValue === "Floating" || pinModeValue === "PullUp" || pinModeValue === "PullDown") {
    return pinModeValue;
  }

  return "Floating";
}

export default function ButtonConfig(props: ButtonConfigProps) {
  const [device, { setDeviceConfig }] = useButton(props.id);

  const [name, setName] = createSignal(normalizeName(device?.config?.name));
  const [pin, setPin] = createSignal(normalizePin(device?.config?.pin));
  const [pinMode, setPinMode] = createSignal<ButtonPinMode>(
    normalizePinMode(device?.config?.pinMode)
  );
  const [debounce, setDebounce] = createSignal(normalizeDebounce(device?.config?.debounceTimeInMs));

  createEffect(() => {
    const config = device?.config;
    if (!config) {
      return;
    }

    setName(normalizeName(config.name));
    setPin(normalizePin(config.pin));
    setPinMode(normalizePinMode(config.pinMode));
    setDebounce(normalizeDebounce(config.debounceTimeInMs));
  });

  const handleSave = () => {
    const selectedMode = pinMode();
    setDeviceConfig({
      name: name(),
      pin: pin(),
      debounceTimeInMs: debounce(),
      pinMode: selectedMode,
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
            <input
              type="number"
              value={pin()}
              min={-1}
              onInput={(event) => setPin(Number(event.currentTarget.value))}
              style={{ "margin-left": "0.5rem", width: "5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Pin mode:">
            <select
              value={pinMode()}
              onChange={(event) => setPinMode(event.currentTarget.value as ButtonPinMode)}
              style={{ "margin-left": "0.5rem" }}
            >
              <For each={BUTTON_PIN_MODE_OPTIONS}>
                {(option) => <option value={option.value}>{option.label}</option>}
              </For>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="Debounce (ms):">
            <input
              type="number"
              value={debounce()}
              min={0}
              onInput={(event) => setDebounce(Number(event.currentTarget.value))}
              style={{ "margin-left": "0.5rem", width: "5rem" }}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
