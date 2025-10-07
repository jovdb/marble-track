interface IWsMessageBase<TType extends string = string> {
  type: TType;
}

export interface IWsDeviceMessage extends IWsMessageBase<"device-fn"> {
  deviceType: string;
  deviceId: string;
  fn: string;
  args: object;
}

export type IWsReceiveDevicesListMessage =
  | (IWsMessageBase<"devices-list"> & { error: string })
  | (IWsMessageBase<"devices-list"> & {
      devices: {
        id: string;
        type: string;
      }[];
    });

export type IWsReceiveDeviceStateMessage =
  | (IWsMessageBase<"device-state"> & {
      deviceId: string;
      type: string;
      error: string;
    })
  | (IWsMessageBase<"device-state"> & {
      deviceId: string;
      type: string;
      state: any;
    });

export type IWsReceiveDeviceConfigMessage =
  | (IWsMessageBase<"device-config"> & {
      deviceId: string;
      type: string;
      error: string;
    })
  | (IWsMessageBase<"device-config"> & {
      deviceId: string;
      type: string;
      config: any;
    });

export type IWsReceiveDeviceReadConfigMessage =
  | (IWsMessageBase<"device-read-config"> & {
      deviceId: string;
      type: string;
      error: string;
    })
  | (IWsMessageBase<"device-read-config"> & {
      deviceId: string;
      type: string;
      config: any;
    });

export type IWsReceiveDeviceSaveConfigMessage =
  | (IWsMessageBase<"device-save-config"> & {
      deviceId: string;
      type: string;
      error: string;
      // TODO: config?
    })
  | (IWsMessageBase<"device-save-config"> & {
      deviceId: string;
      type: string;
      config: any;
    });

// Heartbeat messages
export type IWsReceivePongMessage = IWsMessageBase<"pong"> & {
  timestamp?: number;
};

export type IWsReceiveAddDeviceMessage =
  | (IWsMessageBase<"add-device"> & { error: string; deviceId?: string })
  | (IWsMessageBase<"add-device"> & { success: boolean; deviceId: string });

export type IWsReceiveRemoveDeviceMessage =
  | (IWsMessageBase<"remove-device"> & { error: string; deviceId?: string })
  | (IWsMessageBase<"remove-device"> & { success: boolean; deviceId: string });

export type IWsReceiveGetNetworkConfigMessage =
  | (IWsMessageBase<"network-config"> & { error: string })
  | (IWsMessageBase<"network-config"> & { ssid: string; password: string });

export type IWsReceiveSetNetworkConfigMessage =
  | (IWsMessageBase<"set-network-config"> & { error: string })
  | (IWsMessageBase<"set-network-config"> & { success: boolean });

export type IWsReceiveGetNetworksMessage =
  | (IWsMessageBase<"networks"> & { error: string })
  | (IWsMessageBase<"networks"> & {
      count: number;
      networks: Array<{
        ssid: string;
        rssi: number;
        encryption: number;
        channel: number;
        bssid: string;
        hidden: boolean;
      }>;
    });

export type IWsReceiveGetNetworkStatusMessage =
  | (IWsMessageBase<"network-status"> & { error: string })
  | (IWsMessageBase<"network-status"> & {
      status: {
        mode: string;
        connected: boolean;
        ssid: string;
        ip: string;
        rssi?: number;
        clients?: number;
      };
    });

export type IWsReceiveDevicesConfigMessage =
  | (IWsMessageBase<"devices-config"> & { error: string })
  | (IWsMessageBase<"devices-config"> & { config: any });

export type IWsReceiveMessage =
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
  | IWsReceivePongMessage;

export type IWsSendRestartMessage = IWsMessageBase<"restart">;

export type IWsSendGetDevicesMessage = IWsMessageBase<"get-devices">;

export type IWsSendGetDevicesConfigMessage = IWsMessageBase<"get-devices-config">;

export type IWsSendSetDevicesConfigMessage = IWsMessageBase<"set-devices-config"> & { config: any };

export type IWsSendDeviceFunctionMessage = IWsDeviceMessage;

export type IWsSendDeviceGetStateMessage = IWsMessageBase<"device-get-state"> & {
  deviceId: string;
};

export type IWsSendDeviceReadConfigMessage = IWsMessageBase<"device-read-config"> & {
  deviceId: string;
};

export type IWsSendDeviceSaveConfigMessage = IWsMessageBase<"device-save-config"> & {
  deviceId: string;
  config: any;
};

// Heartbeat message
export type IWsSendPingMessage = IWsMessageBase<"ping"> & {
  timestamp?: number;
};

export type IWsSendAddDeviceMessage = IWsMessageBase<"add-device"> & {
  deviceType: string;
  deviceId: string;
  config?: any;
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
