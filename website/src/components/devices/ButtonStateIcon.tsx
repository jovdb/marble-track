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
      stroke-width="1.8"
      stroke-linecap="round"
      stroke="currentcolor"
      xmlns="http://www.w3.org/2000/svg"
      class={props.class}
      style={props.style}
    >
      <g>
        <line x1="1" x2="23" y1="18" y2="18"></line>
        <line x1="1" x2="23" y1="21" y2="21"></line>
        <line x1="1" x2="1" y1="18" y2="21"></line>
        <line x1="23" x2="23" y1="18" y2="21"></line>
        <line x1="1" x2="3" y1="18" y2="9"></line>
        <line x1="23" x2="21" y1="18" y2="9"></line>
        <line x1="3" x2="21" y1="9" y2="9"></line>
      </g>
      <g>
        <ellipse cx="12" cy="12.5" rx="7" ry="3" fill="#fff"></ellipse>
        <rect x="4.5" y="6" width="14" height="6.5" fill="#fff" stroke="none"></rect>
        <ellipse cx="12" cy="6" rx="7" ry="3" fill="#fff"></ellipse>
        <line x1="5" x2="5" y1="6" y2="12.5" fill="none"></line>
        <line x1="19" x2="19" y1="6" y2="12.5" fill="none"></line>
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
      stroke-width="1.8"
      stroke-linecap="round"
      stroke="currentcolor"
      xmlns="http://www.w3.org/2000/svg"
      class={props.class}
      style={props.style}
    >
      <g>
        <line x1="1" x2="23" y1="18" y2="18"></line>
        <line x1="1" x2="23" y1="21" y2="21"></line>
        <line x1="1" x2="1" y1="18" y2="21"></line>
        <line x1="23" x2="23" y1="18" y2="21"></line>
        <line x1="1" x2="3" y1="18" y2="9"></line>
        <line x1="23" x2="21" y1="18" y2="9"></line>
        <line x1="3" x2="21" y1="9" y2="9"></line>
      </g>
      <g>
        <ellipse cx="12" cy="12.5" rx="7" ry="3" fill="#fff"></ellipse>
        <rect x="4.5" y="10" width="14" height="2.5" fill="#fff" stroke="none"></rect>
        <ellipse cx="12" cy="10" rx="7" ry="3" fill="#fff"></ellipse>
        <line x1="5" x2="5" y1="10" y2="12.5" fill="none"></line>
        <line x1="19" x2="19" y1="10" y2="12.5" fill="none"></line>
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
