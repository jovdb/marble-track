import { createContext, createSignal, JSX, useContext } from "solid-js";

export type ISelectedDevicesStore = {
  selectedDevices: () => Set<string>;
};

export type ISelectedDevicesActions = {
  setSelectedDevices: (value: Set<string> | ((prev: Set<string>) => Set<string>)) => void;
  clearSelectedDevices: () => void;
};

const SelectedDevicesContext = createContext<
  [ISelectedDevicesStore, ISelectedDevicesActions] | undefined
>(undefined);

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
  const context = useContext(SelectedDevicesContext);
  if (!context) {
    throw new Error("useSelectedDevices must be used within SelectedDevicesProvider");
  }
  return context;
}
