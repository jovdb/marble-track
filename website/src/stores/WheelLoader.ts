import { useDevice } from "./Devices";
import { useWebSocket2 } from "../hooks/useWebSocket";

export function useWheelLoader(id: string) {
  const [device] = useDevice(id);
  const [, { sendMessage }] = useWebSocket2();

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
  };

  return [device, actions] as const;
}
