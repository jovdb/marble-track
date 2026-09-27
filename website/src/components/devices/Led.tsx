import { For, createEffect, createMemo, createSignal, onCleanup, untrack } from "solid-js";
import { Device } from "./Device";
import styles from "./Device.module.css";
import LedConfig from "./LedConfig";
import { getDeviceIcon } from "../icons/Icons";
import { useLed } from "../../stores/Led";
import { LedStateIcon } from "./LedStateIcon";

const DEFAULT_PATTERN = "200, 200, 200, 600";

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

function parsePattern(value: string) {
  const parts = value.split(",").map((part) => part.trim());
  if (
    parts.length < 2 ||
    parts.length > 32 ||
    parts.length % 2 !== 0 ||
    parts.some((part) => !part)
  ) {
    return [];
  }

  const timings = parts.map(Number);
  return timings.every((timing) => Number.isInteger(timing) && timing > 0) ? timings : [];
}

export function Led(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, actions] = useLed(props.id);

  const mode = createMemo(() => device()?.state?.mode ?? "");
  // Status visualization removed; use DeviceJsonState below
  const isMode = (value: string) => mode() === value;
  const pattern = createMemo(() => device()?.state?.pattern ?? []);
  const patternKey = createMemo(() => pattern().join(","));
  const patternDuration = createMemo(() => pattern().reduce((total, value) => total + value, 0));
  const [patternInput, setPatternInput] = createSignal(DEFAULT_PATTERN);
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

  createEffect(() => {
    const currentMode = mode();
    const currentPattern = pattern();
    if (currentMode === "PATTERN" && currentPattern.length > 0) {
      setPatternInput(currentPattern.join(", "));
    } else if (currentMode !== "PATTERN") {
      setPatternInput(DEFAULT_PATTERN);
    }
  });

  const handleTurnOn = () => actions.setLed(true);
  const handleTurnOff = () => actions.setLed(false);
  const handleBlink = () => actions.blink();
  const handlePattern = () => {
    const timings = parsePattern(patternInput());
    if (timings.length > 0) {
      actions.pattern(timings);
    }
  };

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
            "flex-direction": "column",
            "justify-content": "center",
            "align-items": "center",
          }}
        >
          <LedStateIcon deviceId={props.id} patternPhase={patternPhase} width={64} height={64} />

          {isMode("PATTERN") && (
            <label class={styles.device__label}>
              Pattern:
              <br />
              <input
                class={styles.device__input}
                type="text"
                value={patternInput()}
                onInput={(event) => setPatternInput(event.currentTarget.value)}
                aria-label="LED pattern timings"
                style={{ width: "240px" }}
              />
            </label>
          )}
          {mode() === "PATTERN" && pattern().length > 0 && patternDuration() > 0 && (
            <div
              style={{
                position: "relative",
                display: "flex",
                width: "240px",
                height: "8px",
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
        <button
          classList={{
            [styles.device__button]: true,
            [styles["device__button--secondary"]]: isMode("PATTERN"),
          }}
          disabled={!mode() || parsePattern(patternInput()).length === 0}
          onClick={handlePattern}
        >
          Pattern
        </button>
      </div>
    </Device>
  );
}
