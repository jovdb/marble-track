import { createSignal, createEffect, onCleanup } from "solid-js";
import { useLed } from "../../stores/Led";

function LightbulbOffIcon(props: { width?: number; height?: number; class?: string; style?: any }) {
  return (
    <svg
      id="Layer_1"
      height={props.height || 512}
      viewBox="0 0 24 24"
      width={props.width || 512}
      xmlns="http://www.w3.org/2000/svg"
      data-name="Layer 1"
      class={props.class}
      style={props.style}
    >
      <path d="m17.994 2.286a9 9 0 0 0 -14.919 5.536 8.938 8.938 0 0 0 2.793 7.761 6.263 6.263 0 0 1 2.132 4.566v.161a3.694 3.694 0 0 0 3.69 3.69h.62a3.694 3.694 0 0 0 3.69-3.69v-.549a5.323 5.323 0 0 1 1.932-4 8.994 8.994 0 0 0 .062-13.477zm-5.684 19.714h-.62a1.692 1.692 0 0 1 -1.69-1.69s-.007-.26-.008-.31h4.008v.31a1.692 1.692 0 0 1 -1.69 1.69zm4.3-7.741a7.667 7.667 0 0 0 -2.364 3.741h-1.246v-7.184a3 3 0 0 0 2-2.816 1 1 0 0 0 -2 0 1 1 0 0 1 -2 0 1 1 0 0 0 -2 0 3 3 0 0 0 2 2.816v7.184h-1.322a8.634 8.634 0 0 0 -2.448-3.881 7 7 0 0 1 3.951-12.073 7.452 7.452 0 0 1 .828-.046 6.921 6.921 0 0 1 4.652 1.778 6.993 6.993 0 0 1 -.048 10.481z" />
    </svg>
  );
}

function LightbulbOnIcon(props: { width?: number; height?: number; class?: string; style?: any }) {
  return (
    <svg
      id="Layer_1"
      height={props.height || 512}
      viewBox="0 0 24 24"
      width={props.width || 512}
      xmlns="http://www.w3.org/2000/svg"
      data-name="Layer 1"
      class={props.class}
      style={props.style}
    >
      <path d="m5.868 15.583a8.938 8.938 0 0 1 -2.793-7.761 9 9 0 1 1 14.857 7.941 5.741 5.741 0 0 0 -1.594 2.237h-3.338v-7.184a3 3 0 0 0 2-2.816 1 1 0 0 0 -2 0 1 1 0 0 1 -2 0 1 1 0 0 0 -2 0 3 3 0 0 0 2 2.816v7.184h-3.437a6.839 6.839 0 0 0 -1.695-2.417zm2.132 4.417v.31a3.694 3.694 0 0 0 3.69 3.69h.62a3.694 3.694 0 0 0 3.69-3.69v-.31z" />
    </svg>
  );
}

export function LedState(props: { id: string }) {
  const [device] = useLed(props.id);
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

  return (
    <div
      style={{
        padding: "10px",
        display: "flex",
        "justify-content": "center",
        "align-items": "center",
        height: "60px",
      }}
    >
      {isOn() ? (
        <LightbulbOnIcon width={40} height={40} style={{ fill: "#440000" }} />
      ) : (
        <LightbulbOffIcon width={40} height={40} />
      )}
    </div>
  );
}
