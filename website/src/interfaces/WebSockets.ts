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
  | (IWsMessageBase<"add-device"> & { error: string })
  | (IWsMessageBase<"add-device"> & { success: boolean; deviceId: string });

export type IWsReceiveRemoveDeviceMessage =
  | (IWsMessageBase<"remove-device"> & { error: string })
  | (IWsMessageBase<"remove-device"> & { success: boolean; deviceId: string });

export type IWsReceiveGetNetworkConfigMessage =
  | (IWsMessageBase<"get-network-config"> & { error: string })
  | (IWsMessageBase<"get-network-config"> & { ssid: string; password: string });

export type IWsReceiveSetNetworkConfigMessage =
  | (IWsMessageBase<"set-network-config"> & { error: string })
  | (IWsMessageBase<"set-network-config"> & { success: boolean });

export type IWsReceiveGetNetworksMessage =
  | (IWsMessageBase<"get-networks"> & { error: string })
  | (IWsMessageBase<"get-networks"> & {
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

export type IWsSendGetNetworkConfigMessage = IWsMessageBase<"get-network-config">;

export type IWsSendGetNetworksMessage = IWsMessageBase<"get-networks">;

export type IWsSendSetNetworkConfigMessage = IWsMessageBase<"set-network-config"> & {
  ssid: string;
  password: string;
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
  | IWsSendGetNetworkConfigMessage
  | IWsSendSetNetworkConfigMessage
  | IWsSendGetNetworksMessage
  | IWsSendPingMessage;
