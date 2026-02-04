import { createSignal, createEffect, onCleanup } from "solid-js";
import { useLed } from "../../stores/Led";
import { IconProps } from "../icons/Icons";

function LightbulbOffIcon(props: IconProps) {
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
      <path
        d="M12 7C9.23858 7 7 9.23858 7 12C7 13.3613 7.54402 14.5955 8.42651 15.4972C8.77025 15.8484 9.05281 16.2663 9.14923 16.7482L9.67833 19.3924C9.86537 20.3272 10.6862 21 11.6395 21H12.3605C13.3138 21 14.1346 20.3272 14.3217 19.3924L14.8508 16.7482C14.9472 16.2663 15.2297 15.8484 15.5735 15.4972C16.456 14.5955 17 13.3613 17 12C17 9.23858 14.7614 7 12 7Z"
        stroke="#000000"
        stroke-width="2"
      />
      <path
        d="M10 17H14"
        stroke="#000000"
        stroke-width="2"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
    </svg>
  );
}

function LightbulbOnIcon(props: IconProps) {
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
      <path
        d="M12 7C9.23858 7 7 9.23858 7 12C7 13.3613 7.54402 14.5955 8.42651 15.4972C8.77025 15.8484 9.05281 16.2663 9.14923 16.7482L9.67833 19.3924C9.86537 20.3272 10.6862 21 11.6395 21H12.3605C13.3138 21 14.1346 20.3272 14.3217 19.3924L14.8508 16.7482C14.9472 16.2663 15.2297 15.8484 15.5735 15.4972C16.456 14.5955 17 13.3613 17 12C17 9.23858 14.7614 7 12 7Z"
        stroke="#000000"
        stroke-width="2"
      />
      <path
        d="M12 4V3"
        stroke="#000000"
        stroke-width="2"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
      <path
        d="M18 6L19 5"
        stroke="#000000"
        stroke-width="2"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
      <path
        d="M20 12H21"
        stroke="#000000"
        stroke-width="2"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
      <path
        d="M4 12H3"
        stroke="#000000"
        stroke-width="2"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
      <path
        d="M5 5L6 6"
        stroke="#000000"
        stroke-width="2"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
      <path
        d="M10 17H14"
        stroke="#000000"
        stroke-width="2"
        stroke-linecap="round"
        stroke-linejoin="round"
      />
    </svg>
  );
}

export function LedStateIcon(props: { deviceId: string } & IconProps) {
  const [device] = useLed(props.deviceId);
  const [isOn, setIsOn] = createSignal(false);

  createEffect(() => {
    const state = device?.state;
    if (!state) return;

    if (state.mode === "ON") {
      setIsOn(true);
    } else if (state.mode === "OFF") {
      setIsOn(false);
    } else if (state.mode === "BLINKING") {
      const onTime = Number(state.blinkOnTime) || 500;
      const offTime = Number(state.blinkOffTime) || 500;
      const delay = Number(state.blinkDelay) || 0;
      const cycle = onTime + offTime + delay;

      const updateBlink = () => {
        const now = Date.now();
        // Mimic ESP32 logic: unsigned long value = millis() % cycle;
        const value = now % cycle;

        // bool shouldBeOn = (value >= _state.blinkDelay && value < _state.blinkDelay + _state.blinkOnTime);
        const shouldBeOn = value >= delay && value < delay + onTime;
        setIsOn(shouldBeOn);
      };

      // Check frequently to keep visual sync reasonably tight
      const intervalId = setInterval(updateBlink, 30);

      // Run immediately
      updateBlink();

      onCleanup(() => clearInterval(intervalId));
    }
  });

  return <span>{isOn() ? <LightbulbOnIcon {...props} /> : <LightbulbOffIcon {...props} />}</span>;
}
