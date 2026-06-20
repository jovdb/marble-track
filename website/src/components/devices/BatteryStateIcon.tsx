import { createMemo } from "solid-js";
import { useBattery } from "../../stores/Battery";
import { BatteryIcon, IconProps } from "../icons/Icons";

export function BatteryStateIcon(props: { deviceId: string; poll?: boolean } & IconProps) {
  const [device] = useBattery(props.deviceId);

  const batteryPct = createMemo(() => device()?.state?.batteryPercent ?? 0);
  const batteryLevel = createMemo(() => Math.min(5, Math.max(0, Math.round(batteryPct() / 20))));

  const batteryTitle = createMemo(() => {
    const d = device();
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
