import { For, createEffect, createSignal, createMemo } from "solid-js";
import DeviceConfig, { DeviceConfigItem, DeviceConfigRow, DeviceConfigTable } from "./DeviceConfig";
import { useDevices } from "../../stores/Devices";
import { II2cConfig } from "../../stores/I2c";
import { useIoExpander } from "../../stores/IoExpander";
import { I2cAddressPicker } from "./shared/I2cAddressPicker";

const EXPANDER_TYPES = ["PCF8574", "PCF8575", "MCP23017"] as const;
type ExpanderType = (typeof EXPANDER_TYPES)[number];

interface IoExpanderConfigProps {
  id: string;
  onClose: () => void;
}

export default function IoExpanderConfig(props: IoExpanderConfigProps) {
  const [device, { setDeviceConfig }] = useIoExpander(props.id);
  const [devicesStore] = useDevices();
  const devicesState = () => devicesStore; // Wrap in a function to avoid stale closure issues
  const [name, setName] = createSignal<string>((device()?.config?.name as string) ?? "IO Expander");
  const [expanderType, setExpanderType] = createSignal<ExpanderType>(
    (device()?.config?.expanderType as ExpanderType) ?? "PCF8574"
  );
  const [i2cAddress, setI2cAddress] = createSignal<number>(
    (device()?.config?.i2cAddress as number) ?? 0x20
  );
  const [i2cDeviceId, setI2cDeviceId] = createSignal<string>(
    (device()?.config?.i2cDeviceId as string) ?? ""
  );

  // Get available I2C devices
  const i2cDevices = createMemo(() => {
    return Object.values(devicesState().devices).filter((d) => d.type === "i2c");
  });

  createEffect(() => {
    const config = device()?.config;
    if (!config) {
      return;
    }

    if (typeof config.name === "string") {
      setName(config.name);
    }
    if (
      typeof config.expanderType === "string" &&
      EXPANDER_TYPES.includes(config.expanderType as ExpanderType)
    ) {
      setExpanderType(config.expanderType as ExpanderType);
    }
    if (typeof config.i2cAddress === "number") {
      setI2cAddress(config.i2cAddress);
    }
    if (typeof config.i2cDeviceId === "string") {
      setI2cDeviceId(config.i2cDeviceId);
    }
  });

  const handleSave = () => {
    setDeviceConfig({
      name: name(),
      expanderType: expanderType(),
      i2cAddress: i2cAddress(),
      i2cDeviceId: i2cDeviceId(),
    });
  };

  return (
    <DeviceConfig device={device()} onSave={handleSave} onClose={props.onClose}>
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
          <DeviceConfigItem name="Expander Type:">
            <select
              value={expanderType()}
              onChange={(event) => setExpanderType(event.currentTarget.value as ExpanderType)}
              style={{ "margin-left": "0.5rem" }}
            >
              <For each={EXPANDER_TYPES}>{(type) => <option value={type}>{type}</option>}</For>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="I²C Bus:">
            <select
              value={i2cDeviceId()}
              onChange={(event) => setI2cDeviceId(event.currentTarget.value)}
              style={{ "margin-left": "0.5rem" }}
            >
              <option value="">Select I²C Bus...</option>
              <For each={i2cDevices()}>
                {(i2cDevice) => (
                  <option value={i2cDevice.id}>
                    {(i2cDevice.config as II2cConfig)?.name || i2cDevice.id}
                  </option>
                )}
              </For>
            </select>
          </DeviceConfigItem>
        </DeviceConfigRow>
        <DeviceConfigRow>
          <DeviceConfigItem name="I²C Address:">
            <I2cAddressPicker
              i2cDeviceId={i2cDeviceId()}
              value={i2cAddress()}
              onChange={setI2cAddress}
              defaultAddresses={[0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27]}
            />
          </DeviceConfigItem>
        </DeviceConfigRow>
      </DeviceConfigTable>
    </DeviceConfig>
  );
}
