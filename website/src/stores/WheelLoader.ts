import { IDeviceConfig, IDeviceState, useDevice, useDevices } from "./Devices";
import { useWebSocket2 } from "../hooks/useWebSocket";

export interface IWheelLoaderState extends IDeviceState {
  state: string;
}

export interface IWheelLoaderConfig extends IDeviceConfig {
  name: string;
  innerCenter: number;
  outerCenter: number;
}

export function useWheelLoader(id: string) {
  const [device] = useDevice<IWheelLoaderState, IWheelLoaderConfig>(id);
  const [, { sendMessage, setDeviceConfig }] = useDevices();

  const actions = {
    init: () => {
      sendMessage({
        type: "device-fn",
        deviceId: id,
        deviceType: "wheelloader",
        fn: "init",
      });
    },
    loadLeft: () => {
      sendMessage({
        type: "device-fn",
        deviceId: id,
        deviceType: "wheelloader",
        fn: "loadLeft",
      });
    },
    loadRight: () => {
      sendMessage({
        type: "device-fn",
        deviceId: id,
        deviceType: "wheelloader",
        fn: "loadRight",
      });
    },
    loadAny: () => {
      sendMessage({
        type: "device-fn",
        deviceId: id,
        deviceType: "wheelloader",
        fn: "loadAny",
      });
    },
    setDeviceConfig: (deviceId: string, config: IWheelLoaderConfig) => {
      setDeviceConfig(deviceId, config);
    },
  };

  return [device, actions] as const;
}
