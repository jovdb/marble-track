import { For, createEffect, createMemo, createSignal, onCleanup, untrack } from "solid-js";
import { Device } from "./Device";
import styles from "./Device.module.css";
import LedConfig from "./LedConfig";
import { getDeviceIcon } from "../icons/Icons";
import { useLed } from "../../stores/Led";
import { LedStateIcon } from "./LedStateIcon";

function getPatternDuration(pattern: number[]) {
  return pattern.reduce((total, duration) => total + duration, 0);
}

function patternPhase(pattern: number[], offset: number) {
  let remaining = offset;
  for (let index = 0; index < pattern.length; index += 1) {
    if (remaining < pattern[index]) {
      return { isOn: index % 2 === 0, remaining: pattern[index] - remaining };
    }
    remaining -= pattern[index];
  }
  return { isOn: false, remaining: 0 };
}

function patternsOverlap(currentPattern: number[], newPattern: number[], elapsed: number) {
  const newDuration = getPatternDuration(newPattern);
  let offset = 0;

  while (offset < elapsed) {
    const currentPhase = patternPhase(currentPattern, offset);
    const newPhase = patternPhase(newPattern, offset % newDuration);
    const segmentDuration = Math.min(currentPhase.remaining, elapsed - offset);

    if (currentPhase.isOn !== newPhase.isOn || newPhase.remaining < segmentDuration) {
      return false;
    }

    offset += segmentDuration;
  }

  return true;
}

export function Led(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, actions] = useLed(props.id);

  const mode = createMemo(() => device()?.state?.mode ?? "");
  // Status visualization removed; use DeviceJsonState below
  const isMode = (value: string) => mode() === value;
  const pattern = createMemo(() => device()?.state?.pattern ?? []);
  const patternKey = createMemo(() => pattern().join(","));
  const patternDuration = createMemo(() => pattern().reduce((total, value) => total + value, 0));
  const [patternPhase, setPatternPhase] = createSignal(0);
  let activePattern: number[] = [];
  let patternStartedAt = 0;

  createEffect(() => {
    const currentMode = mode();
    patternKey();
    const currentPattern = untrack(pattern);
    const currentDuration = currentPattern.reduce((total, value) => total + value, 0);

    if (currentMode !== "PATTERN" || currentDuration <= 0) {
      activePattern = [];
      patternStartedAt = 0;
      setPatternPhase(0);
      return;
    }

    const now = Date.now();
    const currentElapsed =
      activePattern.length > 0 ? (now - patternStartedAt) % getPatternDuration(activePattern) : 0;

    if (
      activePattern.length === 0 ||
      !patternsOverlap(activePattern, currentPattern, currentElapsed)
    ) {
      patternStartedAt = now;
    } else {
      patternStartedAt = now - currentElapsed;
    }

    activePattern = [...currentPattern];
    const updatePatternPhase = () =>
      setPatternPhase((Date.now() - patternStartedAt) % currentDuration);
    const intervalId = setInterval(updatePatternPhase, 30);
    updatePatternPhase();
    onCleanup(() => clearInterval(intervalId));
  });

  const handleTurnOn = () => actions.setLed(true);
  const handleTurnOff = () => actions.setLed(false);
  const handleBlink = () => actions.blink();

  const icon = createMemo(() => {
    const type = device()?.type;
    return type ? getDeviceIcon(type, props.id) : null;
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <LedConfig id={props.id} onClose={onClose} />}
      icon={icon()}
      stateComponent={
        <div
          style={{
            "padding-bottom": "24px",
            display: "flex",
            "justify-content": "center",
            "align-items": "center",
            height: "64px",
          }}
        >
          <LedStateIcon deviceId={props.id} patternPhase={patternPhase} width={64} height={64} />
          {mode() === "PATTERN" && pattern().length > 0 && patternDuration() > 0 && (
            <div
              style={{
                position: "relative",
                display: "flex",
                width: "180px",
                height: "8px",
                "margin-left": "12px",
                "border-radius": "4px",
                overflow: "hidden",
              }}
            >
              <For each={pattern()}>
                {(duration, index) => (
                  <div
                    style={{
                      width: `${(duration / patternDuration()) * 100}%`,
                      background:
                        index() % 2 === 0
                          ? "var(--color-accent, #e0b84f)"
                          : "var(--color-surface-muted, #62666d)",
                    }}
                  />
                )}
              </For>
              <div
                style={{
                  position: "absolute",
                  top: "-2px",
                  left: `${(patternPhase() / patternDuration()) * 100}%`,
                  width: "2px",
                  height: "12px",
                  background: "currentcolor",
                }}
              />
            </div>
          )}
        </div>
      }
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div class={styles.device__controls}>
        <button
          classList={{
            [styles.device__button]: true,
            [styles["device__button--secondary"]]: isMode("ON"),
          }}
          disabled={!mode() || isMode("ON")}
          onClick={handleTurnOn}
        >
          Turn On
        </button>
        <button
          classList={{
            [styles.device__button]: true,
            [styles["device__button--secondary"]]: isMode("OFF"),
          }}
          disabled={!mode() || isMode("OFF")}
          onClick={handleTurnOff}
        >
          Turn Off
        </button>
        <button
          classList={{
            [styles.device__button]: true,
            [styles["device__button--secondary"]]: isMode("BLINKING"),
          }}
          disabled={!mode() || isMode("BLINKING")}
          onClick={handleBlink}
        >
          Blink
        </button>
      </div>
    </Device>
  );
}
