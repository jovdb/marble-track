// Base types for better type safety
export type DeviceType =
  | "button"
  | "buzzer"
  | "gate"
  | "ioexpander"
  | "i2c"
  | "led"
  | "lift"
  | "marblecontroller"
  | "servo"
  | "stepper"
  | "wheel";
export type NetworkMode = "ap" | "sta" | "apsta";
export type EncryptionType = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8;

// Pin types
export type PinType = "GPIO" | "PCF8574" | "PCF8575" | "MCP23017";

export interface PinConfig {
  pin: number;
  expanderId: string;
}

// Deserialize pin configuration from device config
export function deserializePinConfig(pin: number | Record<string, any>): PinConfig {
  // Handle plain number format (legacy or simple GPIO pins)
  if (typeof pin === "number") {
    return {
      pin: pin,
      expanderId: "",
    };
  }

  // Handle object format
  const pinNum = typeof pin.pin === "number" ? pin.pin : -1;
  const expanderId = typeof pin.expanderId === "string" ? pin.expanderId : "";

  return {
    pin: pinNum,
    expanderId,
  };
}

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
  /** Generic features mirrored from firmware mixins */
  features?: string[];
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

interface IWsMessageBase<TType extends string = string> {
  type: TType;
}

export interface IWsDeviceMessage extends IWsMessageBase<"device-fn"> {
  deviceId: string;
  deviceType: DeviceType;
  fn: string;
  args?: Record<string, unknown>;
}

export interface IWsDeviceError<T extends string> extends IWsMessageBase<T> {
  deviceId: string;
  success: false;
  message: string;
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
  | IWsDeviceError<"device-state">
  | (IWsMessageBase<"device-state"> & {
      deviceId: string;
      state: Record<string, unknown>;
      isChanged?: boolean;
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
      isChanged?: boolean;
    });

export type IWsReceiveDeviceReadConfigMessage =
  | (IWsMessageBase<"device-read-config"> &
      _IWsErrorResponse & {
        deviceId: string;
      })
  | IWsDeviceError<"device-read-config">
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

export type IWsReceiveDeviceErrorMessage = IWsMessageBase<"device-error"> & {
  deviceId: string;
  error: string;
};

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

export type IWsReceiveReorderDevicesMessage =
  | (IWsMessageBase<"reorder-devices"> & _IWsErrorResponse)
  | (IWsMessageBase<"reorder-devices"> & _IWsSuccessResponse);

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

export type IWsReceiveExpanderAddressesMessage =
  | (IWsMessageBase<"expander-addresses"> & _IWsErrorResponse)
  | (IWsMessageBase<"expander-addresses"> & {
      addresses: number[];
    });

// Individual message type (non-batch)
export type IWsReceiveSingleMessage =
  | IWsReceiveDevicesListMessage
  | IWsReceiveDeviceStateMessage
  | IWsReceiveDeviceConfigMessage
  | IWsReceiveDeviceReadConfigMessage
  | IWsReceiveDeviceSaveConfigMessage
  | IWsReceiveDeviceErrorMessage
  | IWsReceiveAddDeviceMessage
  | IWsReceiveRemoveDeviceMessage
  | IWsReceiveReorderDevicesMessage
  | IWsReceiveGetNetworkConfigMessage
  | IWsReceiveSetNetworkConfigMessage
  | IWsReceiveGetNetworksMessage
  | IWsReceiveGetNetworkStatusMessage
  | IWsReceiveDevicesConfigMessage
  | IWsReceiveStepsPerRevolutionMessage
  | IWsReceiveExpanderAddressesMessage
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

export type IWsSendReorderDevicesMessage = IWsMessageBase<"reorder-devices"> & {
  deviceIds: string[];
};

export type IWsSendGetNetworkConfigMessage = IWsMessageBase<"network-config">;

export type IWsSendGetNetworksMessage = IWsMessageBase<"networks">;

export type IWsSendSetNetworkConfigMessage = IWsMessageBase<"set-network-config"> & {
  ssid: string;
  password: string;
};

export type IWsSendGetNetworkStatusMessage = IWsMessageBase<"network-status">;

export type IWsSendGetExpanderAddressesMessage = IWsMessageBase<"expander-addresses"> & {
  i2cDeviceId: string;
};

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
  | IWsSendReorderDevicesMessage
  | IWsSendGetNetworkConfigMessage
  | IWsSendSetNetworkConfigMessage
  | IWsSendGetNetworksMessage
  | IWsSendGetNetworkStatusMessage
  | IWsSendGetExpanderAddressesMessage
  | IWsSendPingMessage;
