export interface IDeviceState {
  type?: string;
  children?: IDeviceState[];
}

export type IDeviceConfig = object;

declare global {
  // These interfaces are augmented by device-specific files
  // eslint-disable-next-line @typescript-eslint/no-empty-object-type
  export interface IDeviceStates {}

  // eslint-disable-next-line @typescript-eslint/no-empty-object-type
  export interface IDeviceConfigs extends Record<string, any> {}
}
