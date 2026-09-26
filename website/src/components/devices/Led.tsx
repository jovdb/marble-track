import { For, createEffect, createMemo, createSignal, onCleanup } from "solid-js";
import { Device } from "./Device";
import styles from "./Device.module.css";
import LedConfig from "./LedConfig";
import { getDeviceIcon } from "../icons/Icons";
import { useLed } from "../../stores/Led";
import { LedStateIcon } from "./LedStateIcon";

export function Led(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, actions] = useLed(props.id);

  const mode = createMemo(() => device()?.state?.mode ?? "");
  // Status visualization removed; use DeviceJsonState below
  const isMode = (value: string) => mode() === value;
  const pattern = createMemo(() => device()?.state?.pattern ?? []);
  const patternDuration = createMemo(() => pattern().reduce((total, value) => total + value, 0));
  const patternElapsed = createMemo(() => device()?.state?.patternElapsed ?? 0);
  const [patternPhase, setPatternPhase] = createSignal(0);

  createEffect(() => {
    if (mode() !== "PATTERN" || patternDuration() <= 0) {
      setPatternPhase(0);
      return;
    }

    const patternStartedAt = Date.now() - patternElapsed();
    const updatePatternPhase = () =>
      setPatternPhase((Date.now() - patternStartedAt) % patternDuration());
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
      stateComponent={() => (
        <div
          style={{
            "padding-bottom": "24px",
            display: "flex",
            "justify-content": "center",
            "align-items": "center",
            height: "64px",
          }}
        >
          <LedStateIcon deviceId={props.id} width={64} height={64} />
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
      )}
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
