import { createMemo, onCleanup, onMount } from "solid-js";
import { useBattery } from "../../stores/Battery";
import { BatteryIcon, IconProps } from "../icons/Icons";

const BATTERY_POLL_MS = 60_000;

export function BatteryStateIcon(props: { deviceId: string; poll?: boolean } & IconProps) {
  const [device, { getDeviceState }] = useBattery(props.deviceId);

  const batteryPct = createMemo(() => device?.state?.batteryPercent ?? 0);
  const batteryLevel = createMemo(() => Math.min(5, Math.max(0, Math.round(batteryPct() / 20))));

  // Do only once via prop?
  onMount(() => {
    if (props?.poll ?? false) return;
    const id = setInterval(getDeviceState, BATTERY_POLL_MS);
    onCleanup(() => clearInterval(id));
  });

  const batteryTitle = createMemo(() => {
    const d = device;
    if (!d) return `No battery device with id: '${props.deviceId}'`;
    const pct = batteryPct();
    return `Battery: ${pct.toFixed(0)}%`;
  });

  return (
    <span title={batteryTitle()}>
      <BatteryIcon level={batteryLevel} width={props.width} height={props.height} />
    </span>
  );
}
