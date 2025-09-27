import { JSX, onMount } from "solid-js";
import { useDevice } from "../../stores/Devices";
import styles from "./DeviceConfig.module.css";

interface DeviceConfigProps {
  id: string;
  onSave: () => void;
  onClose?: () => void;
  children?: JSX.Element | JSX.Element[];
}

export default function DeviceConfig(props: DeviceConfigProps) {
  const deviceTuple = useDevice(props.id);

  onMount(() => {
    const actions = deviceTuple[1];
    actions.getDeviceConfig();
  });

  return (
    <section class={styles["device-config"]}>
      <form
        onSubmit={(event) => {
          event.preventDefault();
          props.onSave();
        }}
      >
        <div class={styles["device-config__fields"]}>{props.children}</div>
        <div class={styles["device-config__actions"]}>
          {props.onClose && (
            <button
              type="button"
              class={styles["device-config__close"]}
              onClick={props.onClose}
            >
              Close
            </button>
          )}
          <button type="submit" class={styles["device-config__submit"]}>
            Save
          </button>
        </div>
      </form>
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
      <td
        class={`${styles["device-config__cell"]} ${styles["device-config__cell--value"]}`}
      >
        <div class={styles["device-config__value"]}>{props.children}</div>
      </td>
    </>
  );
}
