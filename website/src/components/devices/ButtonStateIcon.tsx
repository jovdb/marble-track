import { createSignal, createEffect } from "solid-js";
import { useButton } from "../../stores/Button";
import { IconProps } from "../icons/Icons";

function ButtonReleasedIcon(props: IconProps) {
  return (
    <svg
      width={props.width || 24}
      height={props.height || 24}
      viewBox="0 0 24 24"
      fill="none"
      xmlns="http://www.w3.org/2000/svg"
      class={props.class}
      style={props.style}
    >
      <g>
        <ellipse cx="12" cy="16.5" rx="10" ry="3" stroke="#000" fill="#fff" />
        <rect x="2" y="10" width="20" height="6.5" stroke="none" fill="#fff" />
        <ellipse cx="12" cy="10" rx="10" ry="3" stroke="#000" fill="#fff" />
        <line x1="2" x2="2" y1="10" y2="16.5" fill="none" stroke="#000" />
        <line x1="22" x2="22" y1="10" y2="16.5" fill="none" stroke="#000" />
      </g>
    </svg>
  );
}

function ButtonPressedIcon(props: IconProps) {
  return (
    <svg
      width={props.width || 24}
      height={props.height || 24}
      viewBox="0 0 24 24"
      fill="none"
      xmlns="http://www.w3.org/2000/svg"
      class={props.class}
      style={props.style}
    >
      <g>
        <ellipse cx="12" cy="16.5" rx="10" ry="3" stroke="#000" fill="#fff" />
        <rect x="2" y="14" width="20" height="2.5" stroke="none" fill="#fff" />
        <ellipse cx="12" cy="14" rx="10" ry="3" stroke="#000" fill="#fff" />
        <line x1="2" x2="2" y1="14" y2="16.5" fill="none" stroke="#000" />
        <line x1="22" x2="22" y1="14" y2="16.5" fill="none" stroke="#000" />
      </g>
    </svg>
  );
}

export function ButtonStateIcon(props: { deviceId: string } & IconProps) {
  const [device] = useButton(props.deviceId);
  const [isPressed, setIsPressed] = createSignal(false);

  createEffect(() => {
    const state = device?.state;
    if (!state) return;

    setIsPressed(state.isPressed);
  });

  return (
    <span>
      {isPressed() ? <ButtonPressedIcon {...props} /> : <ButtonReleasedIcon {...props} />}
    </span>
  );
}
