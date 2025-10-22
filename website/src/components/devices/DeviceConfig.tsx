import { JSX, onMount, createSignal, Show } from "solid-js";
import styles from "./DeviceConfig.module.css";

interface DeviceConfigProps {
  device?: { id: string; config?: Record<string, unknown> };
  onSave: () => void;
  onClose?: () => void;
  children?: JSX.Element | JSX.Element[];
  title?: string;
}

export default function DeviceConfig(props: DeviceConfigProps) {
  const [error, setError] = createSignal<string | null>(null);

  const deviceData = () => props.device;
  const isLoading = () => deviceData()?.config === undefined;
  const deviceName = () =>
    ((deviceData()?.config as Record<string, unknown>)?.name as string) ||
    deviceData()?.id ||
    "Device";

  onMount(() => {
    // Config is fetched by the parent component
  });

  const handleSubmit = (event: Event) => {
    event.preventDefault();
    setError(null);

    try {
      props.onSave();
      props.onClose?.();
    } catch (err) {
      setError(err instanceof Error ? err.message : "Failed to save configuration");
    }
  };

  return (
    <section class={styles["device-config"]} aria-labelledby="device-config-title">
      <Show when={props.title}>
        <h2 id="device-config-title" class={styles["device-config__title"]}>
          {props.title} - {deviceName()}
        </h2>
      </Show>

      <Show when={isLoading()}>
        <div class={styles["device-config__loading"]}>Loading configuration...</div>
      </Show>

      <Show when={!isLoading()}>
        <form onSubmit={handleSubmit}>
          <div class={styles["device-config__fields"]}>{props.children}</div>

          <Show when={error()}>
            <div class={styles["device-config__error"]} role="alert">
              {error()}
            </div>
          </Show>

          <div class={styles["device-config__actions"]}>
            {props.onClose && (
              <button type="button" class={styles["device-config__close"]} onClick={props.onClose}>
                Cancel
              </button>
            )}
            <button type="submit" class={styles["device-config__submit"]}>
              Save
            </button>
          </div>
        </form>
      </Show>
    </section>
  );
}

export function DeviceConfigTable(props: { children?: JSX.Element | JSX.Element[] }) {
  return (
    <table class={styles["device-config__table"]}>
      <tbody>{props.children}</tbody>
    </table>
  );
}

export function DeviceConfigRow(props: { children: JSX.Element | JSX.Element[] }) {
  return <tr class={styles["device-config__row"]}>{props.children}</tr>;
}

export function DeviceConfigItem(props: { name: string; children: JSX.Element | JSX.Element[] }) {
  return (
    <>
      <th
        scope="row"
        class={`${styles["device-config__cell"]} ${styles["device-config__cell--label"]}`}
      >
        {props.name}
      </th>
      <td class={`${styles["device-config__cell"]} ${styles["device-config__cell--value"]}`}>
        <div class={styles["device-config__value"]}>{props.children}</div>
      </td>
    </>
  );
}
