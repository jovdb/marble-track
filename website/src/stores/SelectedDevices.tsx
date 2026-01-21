import { createContext, createSignal, JSX, useContext } from "solid-js";

export type ISelectedDevicesStore = {
  selectedDevices: () => Set<string>;
};

export type ISelectedDevicesActions = {
  setSelectedDevices: (value: Set<string> | ((prev: Set<string>) => Set<string>)) => void;
  clearSelectedDevices: () => void;
};

const SelectedDevicesContext = createContext<[ISelectedDevicesStore, ISelectedDevicesActions]>([
  { selectedDevices: () => new Set() },
  { setSelectedDevices: () => undefined, clearSelectedDevices: () => undefined },
]);

export function SelectedDevicesProvider(props: { children: JSX.Element }) {
  const [selectedDevices, setSelectedDevices] = createSignal<Set<string>>(new Set());

  const store: ISelectedDevicesStore = {
    selectedDevices,
  };

  const actions: ISelectedDevicesActions = {
    setSelectedDevices,
    clearSelectedDevices: () => setSelectedDevices(new Set()),
  };

  return (
    <SelectedDevicesContext.Provider value={[store, actions]}>
      {props.children}
    </SelectedDevicesContext.Provider>
  );
}

export function useSelectedDevices() {
  return useContext(SelectedDevicesContext);
}
