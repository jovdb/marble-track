import { For, createMemo } from "solid-js";
import { ESP32_AVAILABLE_PINS, getUsedPins } from "../utils/esp32Pins";
import { useDevices } from "../stores/Devices";
import { PinConfig } from "../interfaces/WebSockets";

interface PinSelectProps {
  value: PinConfig;
  onChange: (value: PinConfig) => void;
  disabled?: boolean;
  style?: string | Record<string, string>;
  class?: string;
  title?: string;
  excludeDeviceId?: string;
  showExpanderPins?: boolean;
}

export default function PinSelect(props: PinSelectProps) {
  const [devicesStore] = useDevices();
  const usedPins = createMemo(() => getUsedPins(devicesStore.devices, props.excludeDeviceId));
  const getPinUsage = (pin: number) => usedPins().get(pin);

  const expanderPinOptions = createMemo(() => {
    if (!props.showExpanderPins) return [];

    const options: { value: PinConfig; label: string }[] = [];
    Object.values(devicesStore.devices).forEach((device) => {
      if (device.type === "ioexpander" && device.config) {
        const config = device.config as any;
        const expanderType = config.expanderType || "PCF8574";
        const i2cAddress = config.i2cAddress || 0x20;
        let pinCount = 8;
        if (expanderType === "PCF8575" || expanderType === "MCP23017") {
          pinCount = 16;
        }

        // Log expander pins when available
        for (let pin = 0; pin < pinCount; pin++) {
          const pinString = `${expanderType}:0x${i2cAddress.toString(16).padStart(2, "0").toUpperCase()}:${pin}`;
          options.push({
            value: {
              pinType: expanderType as PinConfig["pinType"],
              pin: pin,
              i2cAddress: i2cAddress,
            },
            label: pinString,
          });
        }
      }
    });
    return options;
  });

  const getSelectedValue = () => {
    if (props.value.pinType === "GPIO") {
      if (props.value.pin === -1) {
        return "-1";
      }
      return props.value.pin.toString();
    }
    return JSON.stringify(props.value);
  };

  const handleChange = (event: Event) => {
    const target = event.currentTarget as HTMLSelectElement;
    const selectedValue = target.value;
    if (selectedValue === "-1") {
      props.onChange({ pinType: "GPIO", pin: -1, i2cAddress: 0x20 });
      return;
    }
    // Check if it's a GPIO pin
    const pinNum = parseInt(selectedValue);
    if (!isNaN(pinNum) && ESP32_AVAILABLE_PINS.includes(pinNum)) {
      props.onChange({ pinType: "GPIO", pin: pinNum, i2cAddress: 0x20 });
      return;
    }
    // Otherwise it's an I2C pin
    try {
      const parsed = JSON.parse(selectedValue) as PinConfig;
      props.onChange(parsed);
    } catch {
      props.onChange({ pinType: "GPIO", pin: -1, i2cAddress: 0x20 });
    }
  };

  return (
    <select
      value={getSelectedValue()}
      onChange={handleChange}
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
              GPIO:{pinNum}
              {deviceId ? ` (used by '${deviceId}')` : ""}
            </option>
          );
        }}
      </For>
      <For each={expanderPinOptions()}>
        {(option) => <option value={JSON.stringify(option.value)}>{option.label}</option>}
      </For>
    </select>
  );
}
