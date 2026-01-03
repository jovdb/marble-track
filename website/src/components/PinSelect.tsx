import { For, createMemo } from "solid-js";
import { ESP32_AVAILABLE_PINS, getUsedPins } from "../utils/esp32Pins";
import { useDevices } from "../stores/Devices";

interface PinSelectProps {
  value: number;
  onChange: (value: number) => void;
  disabled?: boolean;
  style?: string | Record<string, string>;
  class?: string;
  title?: string;
  excludeDeviceId?: string;
}

export default function PinSelect(props: PinSelectProps) {
  const [devicesStore] = useDevices();
  const usedPins = createMemo(() => getUsedPins(devicesStore.devices, props.excludeDeviceId));
  const getPinUsage = (pin: number) => usedPins().get(pin);

  return (
    <select
      value={props.value}
      onChange={(event) => props.onChange(Number(event.currentTarget.value))}
      disabled={props.disabled}
      style={props.style}
      class={props.class}
      title={props.title}
    >
      <option value={-1}>Disabled</option>
      <For each={ESP32_AVAILABLE_PINS}>
        {(pinNum) => {
          const deviceId = getPinUsage(pinNum);
          return (
            <option value={pinNum}>
              {pinNum}
              {deviceId ? ` (used by '${deviceId}')` : ""}
            </option>
          );
        }}
      </For>
    </select>
  );
}
