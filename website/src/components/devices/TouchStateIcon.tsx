import { createEffect, createSignal } from "solid-js";
import { useTouch } from "../../stores/Touch";
import { IconProps } from "../icons/Icons";

function TouchIcon(props: IconProps & { touched?: boolean }) {
  return (
    <svg
      width={props.width || 24}
      height={props.height || 24}
      viewBox="0 0 32 32"
      fill="none"
      stroke-width="1.8"
      stroke-linecap="round"
      stroke="currentcolor"
      xmlns="http://www.w3.org/2000/svg"
      class={props.class}
      style={props.style}
    >
      <line x1="4" x2="28" y1="28" y2="28" stroke="currentColor" stroke-width="1.6"></line>
      <g
        style={`transition: transform 300ms;transform:scale(0.8) translate(3px,${props.touched ? "8" : "2"}px)`}
      >
        <path
          fill="currentColor"
          d="M28,14V8a7.0085,7.0085,0,0,0-7-7H16a6.1457,6.1457,0,0,0-4.1055,1.5664L3.8833,9.874a2.9986,2.9986,0,0,0,3.881,4.55l.0008.0012L10,12.8955V24a3,3,0,0,0,6,0h0l0-5.1843a2.939,2.939,0,0,0,3.5294-1.2171A2.963,2.963,0,0,0,21,18a2.9936,2.9936,0,0,0,2.5292-1.4014A2.963,2.963,0,0,0,25,17,3.0033,3.0033,0,0,0,28,14Zm-2,0a1,1,0,0,1-2,0V13H22v2a1,1,0,0,1-2,0V13H18v3a1,1,0,0,1-2,0V13H14V24h.0005A1,1,0,0,1,12,24V9.1045L6.6,12.8008a.9993.9993,0,0,1-1.3081-1.5044l7.9507-7.2515A4.1483,4.1483,0,0,1,16,3h5a5.0059,5.0059,0,0,1,5,5Z"
        />
      </g>
    </svg>
  );
}

export function TouchStateIcon(props: { deviceId: string } & IconProps) {
  const [device] = useTouch(props.deviceId);
  const [touched, setTouched] = createSignal(false);

  createEffect(() => {
    const state = device?.state;
    if (!state) return;

    setTouched(Boolean(state.touched));
  });

  return (
    <span>
      <TouchIcon {...props} touched={touched()} />
    </span>
  );
}
