// Base types for better type safety
export type DeviceType =
  | "button"
  | "buzzer"
  | "gate"
  | "led"
  | "lift"
  | "pwmmotor"
  | "stepper"
  | "wheel";
export type NetworkMode = "ap" | "sta" | "apsta";
export type EncryptionType = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8;

// Common response patterns
interface _IWsErrorResponse {
  error: string;
}

interface _IWsSuccessResponse {
  success: boolean;
}

// Device-related types
export interface DeviceInfo {
  id: string;
  type: DeviceType;
  pins?: number[];
  children?: DeviceInfo[];
}

export interface NetworkInfo {
  ssid: string;
  rssi: number;
  encryption: EncryptionType;
  channel: number;
  bssid: string;
  hidden: boolean;
}

export interface NetworkStatus {
  mode: NetworkMode;
  connected: boolean;
  ssid: string;
  ip: string;
  rssi?: number;
  clients?: number;
}

interface IWsMessageBase<TType extends string = string> {
  type: TType;
}

export interface IWsDeviceMessage extends IWsMessageBase<"device-fn"> {
  deviceType: DeviceType;
  deviceId: string;
  fn: string;
  args?: Record<string, unknown>;
}

export type IWsReceiveDevicesListMessage =
  | (IWsMessageBase<"devices-list"> & _IWsErrorResponse)
  | (IWsMessageBase<"devices-list"> & {
      devices: DeviceInfo[];
    });

export type IWsReceiveDeviceStateMessage =
  | (IWsMessageBase<"device-state"> &
      _IWsErrorResponse & {
        deviceId: string;
      })
  | (IWsMessageBase<"device-state"> & {
      deviceId: string;
      state: Record<string, unknown>;
    });

export type IWsReceiveDeviceConfigMessage =
  | (IWsMessageBase<"device-config"> &
      _IWsErrorResponse & {
        deviceId: string;
      })
  | (IWsMessageBase<"device-config"> & {
      deviceId: string;
      /** only when a config changes, devices need to be reloaded */
      triggerBy: "set" | "get";
      config: Record<string, unknown>;
    });

export type IWsReceiveDeviceReadConfigMessage =
  | (IWsMessageBase<"device-read-config"> &
      _IWsErrorResponse & {
        deviceId: string;
      })
  | (IWsMessageBase<"device-read-config"> & {
      deviceId: string;
      config: Record<string, unknown>;
    });

export type IWsReceiveDeviceSaveConfigMessage =
  | (IWsMessageBase<"device-save-config"> &
      _IWsErrorResponse & {
        deviceId: string;
      })
  | (IWsMessageBase<"device-save-config"> & {
      deviceId: string;
      config: Record<string, unknown>;
    });

// Heartbeat messages
export type IWsReceivePongMessage = IWsMessageBase<"pong"> & {
  timestamp?: number;
};

export type IWsReceiveAddDeviceMessage =
  | (IWsMessageBase<"add-device"> & _IWsErrorResponse & { deviceId?: string })
  | (IWsMessageBase<"add-device"> & _IWsSuccessResponse & { deviceId: string });

export type IWsReceiveRemoveDeviceMessage =
  | (IWsMessageBase<"remove-device"> & _IWsErrorResponse & { deviceId?: string })
  | (IWsMessageBase<"remove-device"> & _IWsSuccessResponse & { deviceId: string });

export type IWsReceiveGetNetworkConfigMessage =
  | (IWsMessageBase<"network-config"> & _IWsErrorResponse)
  | (IWsMessageBase<"network-config"> & { ssid: string }); // Note: password omitted for security

export type IWsReceiveSetNetworkConfigMessage =
  | (IWsMessageBase<"set-network-config"> & _IWsErrorResponse)
  | (IWsMessageBase<"set-network-config"> & _IWsSuccessResponse);

export type IWsReceiveGetNetworksMessage =
  | (IWsMessageBase<"networks"> & _IWsErrorResponse)
  | (IWsMessageBase<"networks"> & {
      count: number;
      networks: NetworkInfo[];
    });

export type IWsReceiveGetNetworkStatusMessage =
  | (IWsMessageBase<"network-status"> & _IWsErrorResponse)
  | (IWsMessageBase<"network-status"> & {
      status: NetworkStatus;
    });

export type IWsReceiveDevicesConfigMessage =
  | (IWsMessageBase<"devices-config"> & _IWsErrorResponse)
  | (IWsMessageBase<"devices-config"> & { config: Record<string, unknown> });

export type IWsReceiveStepsPerRevolutionMessage = IWsMessageBase<"steps-per-revolution"> & {
  deviceId: string;
  steps: number;
};

// Individual message type (non-batch)
export type IWsReceiveSingleMessage =
  | IWsReceiveDevicesListMessage
  | IWsReceiveDeviceStateMessage
  | IWsReceiveDeviceConfigMessage
  | IWsReceiveDeviceReadConfigMessage
  | IWsReceiveDeviceSaveConfigMessage
  | IWsReceiveAddDeviceMessage
  | IWsReceiveRemoveDeviceMessage
  | IWsReceiveGetNetworkConfigMessage
  | IWsReceiveSetNetworkConfigMessage
  | IWsReceiveGetNetworksMessage
  | IWsReceiveGetNetworkStatusMessage
  | IWsReceiveDevicesConfigMessage
  | IWsReceiveStepsPerRevolutionMessage
  | IWsReceivePongMessage;

// Message is always an array of messages (batch)
export type IWsReceiveMessage = IWsReceiveSingleMessage[];

export type IWsSendRestartMessage = IWsMessageBase<"restart">;

export type IWsSendGetDevicesMessage = IWsMessageBase<"devices-list">;

export type IWsSendGetDevicesConfigMessage = IWsMessageBase<"devices-config">;

export type IWsSendSetDevicesConfigMessage = IWsMessageBase<"set-devices-config"> & {
  config: Record<string, unknown>;
};

export type IWsSendDeviceFunctionMessage = IWsDeviceMessage;

export type IWsSendDeviceGetStateMessage = IWsMessageBase<"device-state"> & {
  deviceId: string;
};

export type IWsSendDeviceReadConfigMessage = IWsMessageBase<"device-read-config"> & {
  deviceId: string;
};

export type IWsSendDeviceSaveConfigMessage = IWsMessageBase<"device-save-config"> & {
  deviceId: string;
  config: Record<string, unknown>;
};

// Heartbeat message
export type IWsSendPingMessage = IWsMessageBase<"ping"> & {
  timestamp?: number;
};

export type IWsSendAddDeviceMessage = IWsMessageBase<"add-device"> & {
  deviceType: DeviceType;
  deviceId: string;
  config?: Record<string, unknown>;
};

export type IWsSendRemoveDeviceMessage = IWsMessageBase<"remove-device"> & {
  deviceId: string;
};

export type IWsSendGetNetworkConfigMessage = IWsMessageBase<"network-config">;

export type IWsSendGetNetworksMessage = IWsMessageBase<"networks">;

export type IWsSendSetNetworkConfigMessage = IWsMessageBase<"set-network-config"> & {
  ssid: string;
  password: string;
};

export type IWsSendGetNetworkStatusMessage = IWsMessageBase<"network-status">;

export type IWsSendMessage =
  | IWsSendRestartMessage
  | IWsSendGetDevicesMessage
  | IWsSendGetDevicesConfigMessage
  | IWsSendSetDevicesConfigMessage
  | IWsSendDeviceFunctionMessage
  | IWsSendDeviceGetStateMessage
  | IWsSendDeviceReadConfigMessage
  | IWsSendDeviceSaveConfigMessage
  | IWsSendAddDeviceMessage
  | IWsSendRemoveDeviceMessage
  | IWsSendGetNetworkConfigMessage
  | IWsSendSetNetworkConfigMessage
  | IWsSendGetNetworksMessage
  | IWsSendGetNetworkStatusMessage
  | IWsSendPingMessage;
