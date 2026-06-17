import { For, createEffect, createMemo } from "solid-js";
import { ESP32_AVAILABLE_PINS, getUsedPins } from "../utils/esp32Pins";
import { useDevices } from "../stores/Devices";
import { PinConfig } from "../interfaces/WebSockets";
import { IIoExpanderConfig } from "../stores/IoExpander";
import { IPwmExpanderConfig } from "../stores/PwmExpander";

interface PinSelectProps {
  value: PinConfig;
  onChange: (value: PinConfig) => void;
  disabled?: boolean;
  style?: string | Record<string, string>;
  class?: string;
  title?: string;
  excludeDeviceId?: string;
  /** Show IoExpander pins (PCF8574, PCF8575, MCP23017) */
  showExpanderPins?: boolean;
  /** Show PwmExpander pins (PCA9685) */
  showPwmExpanderPins?: boolean;
  availableGpioPins?: number[];
}

export default function PinSelect(props: PinSelectProps) {
  const [devicesStore, { getDeviceConfig }] = useDevices();
  const devicesState = () => devicesStore;
  const usedPins = createMemo(() => getUsedPins(devicesState().devices, props.excludeDeviceId));
  const availablePins = createMemo(() => props.availableGpioPins ?? ESP32_AVAILABLE_PINS);
  const getPinUsage = (pinKey: string) => usedPins().get(pinKey);

  // List of expander device ids (does NOT depend on their config). We only
  // recompute this when devices are added/removed, so requesting their
  // configs below cannot create a feedback loop with the device store.
  const expanderDeviceIds = createMemo(() =>
    Object.values(devicesState().devices)
      .filter((device) => {
        if (device.type === "ioexpander") return props.showExpanderPins;
        if (device.type === "pwmexpander") return props.showPwmExpanderPins;
        return false;
      })
      .map((device) => device.id)
  );

  // Track which expander configs we've already requested so we never spam the
  // websocket when the store updates (e.g. when the expander config arrives,
  // which would otherwise re-fire this effect and cause the <select> to
  // re-render mid-update — visible as the pin combobox toggling between the
  // expander pin and "Disabled" before settling on "Disabled").
  const requestedExpanderConfigs = new Set<string>();
  createEffect(() => {
    for (const id of expanderDeviceIds()) {
      if (!requestedExpanderConfigs.has(id)) {
        requestedExpanderConfigs.add(id);
        getDeviceConfig(id);
      }
    }
  });

  const expanderPinOptions = createMemo(() => {
    if (!props.showExpanderPins && !props.showPwmExpanderPins) return [];

    const options: { value: PinConfig; label: string }[] = [];
    Object.values(devicesState().devices).forEach((device) => {
      if (device.type === "ioexpander" && props.showExpanderPins) {
        if (device.config) {
          const config = device.config as IIoExpanderConfig | undefined;
          const expanderType = config?.expanderType || "PCF8574";
          let pinCount = 8;
          if (expanderType === "PCF8575" || expanderType === "MCP23017") {
            pinCount = 16;
          }

          // Log expander pins when available
          for (let pin = 0; pin < pinCount; pin++) {
            const deviceName = config?.name || device.id;
            const pinString = `${deviceName}:${pin}`;
            const usedBy = getPinUsage(`${device.id}:${pin}`);
            options.push({
              value: {
                pin: pin,
                expanderId: device.id, // Use the expander device ID
              },
              label: usedBy ? `${pinString} (used by '${usedBy}')` : pinString,
            });
          }
        }
      } else if (device.type === "pwmexpander" && props.showPwmExpanderPins) {
        const config = device.config as IPwmExpanderConfig | undefined;
        // PCA9685 always has 16 channels (0-15)
        for (let ch = 0; ch < 16; ch++) {
          const deviceName = config?.name || device.id;
          const pinString = `${deviceName}:${ch}`;
          const usedBy = getPinUsage(`${device.id}:${ch}`);
          options.push({
            value: {
              pin: ch,
              expanderId: device.id,
            },
            label: usedBy ? `${pinString} (used by '${usedBy}')` : pinString,
          });
        }
      }
    });
    return options;
  });

  const getSelectedValue = () => {
    if (props.value.expanderId === "") {
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
      props.onChange({ pin: -1, expanderId: "" });
      return;
    }
    // Check if it's a GPIO pin
    const pinNum = parseInt(selectedValue);
    if (!isNaN(pinNum) && availablePins().includes(pinNum)) {
      props.onChange({ pin: pinNum, expanderId: "" });
      return;
    }
    // Otherwise it's an I2C pin
    try {
      const parsed = JSON.parse(selectedValue) as PinConfig;
      props.onChange(parsed);
    } catch {
      props.onChange({ pin: -1, expanderId: "" });
    }
  };

  return (
    <select
      ref={(el) => {
        // The expander <option> the current value points to may not exist
        // yet when the popup first opens (we have to wait for the expander's
        // config to arrive over the websocket). Setting `<select>.value` to
        // a non-existent option is silently ignored by the browser, so the
        // select would stay on "Disabled" forever even after the option
        // shows up. Re-apply the value whenever either the selected pin or
        // the option list changes so the select catches up.
        createEffect(() => {
          const desired = getSelectedValue();
          // Track option-list dependencies so this effect re-runs when they update.
          availablePins();
          expanderPinOptions();
          if (el && el.value !== desired) {
            el.value = desired;
          }
        });
      }}
      value={getSelectedValue()}
      onChange={handleChange}
      disabled={props.disabled}
      style={props.style}
      class={props.class}
      title={props.title}
    >
      <option value={-1}>Disabled</option>
      <For each={availablePins()}>
        {(pinNum) => {
          const deviceId = getPinUsage(String(pinNum));
          return (
            <option value={pinNum}>
              {pinNum}
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
