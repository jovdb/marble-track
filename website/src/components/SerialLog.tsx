import { Component, createEffect, createMemo, createSignal, For, onMount } from "solid-js";
import styles from "./SerialLog.module.css";
import { useSerial } from "../hooks/useSerial";
import type { ISerialLogEntry } from "../hooks/useSerial";
import { useSystemInfo } from "../stores/SystemInfo";
import { CollapsibleSection } from "./CollapsibleSection";
import { BugIcon } from "./icons/Icons";

type SerialFilterType = "debug" | "info" | "warning" | "error" | "ws-recv" | "ws-send";

interface ISerialFilterOption {
  value: SerialFilterType;
  label: string;
}

const SERIAL_FILTER_OPTIONS: ISerialFilterOption[] = [
  { value: "debug", label: "Debug" },
  { value: "info", label: "Info" },
  { value: "warning", label: "Warning" },
  { value: "error", label: "Error" },
  { value: "ws-recv", label: "WS ESP32 Received" },
  { value: "ws-send", label: "WS ESP32 Send" },
];

function getFilterTypeForLogType(logType: string | null): SerialFilterType {
  if (!logType) {
    return "info";
  }

  const normalizedType = logType.toUpperCase();

  if (normalizedType === "E" || normalizedType === "ERROR") {
    return "error";
  }

  if (normalizedType === "W" || normalizedType === "WARN" || normalizedType === "WARNING") {
    return "warning";
  }

  if (normalizedType === "D" || normalizedType === "DEBUG") {
    return "debug";
  }

  if (normalizedType === "WS_RECV") {
    return "ws-recv";
  }

  if (normalizedType === "WS_SEND" || normalizedType === "WE_SEND") {
    return "ws-send";
  }

  return "info";
}

function getLogTypeClass(logType: string | null): string | undefined {
  if (!logType) {
    return undefined;
  }

  const normalizedType = logType.toUpperCase();

  if (normalizedType === "E" || normalizedType === "ERROR") {
    return styles["serial-log__line--error"];
  }

  if (normalizedType === "W" || normalizedType === "WARN" || normalizedType === "WARNING") {
    return styles["serial-log__line--warning"];
  }

  if (normalizedType === "D" || normalizedType === "DEBUG") {
    return styles["serial-log__line--debug"];
  }

  if (normalizedType === "WS_RECV") {
    return styles["serial-log__line--ws-recv"];
  }

  if (normalizedType === "WS_SEND" || normalizedType === "WE_SEND") {
    return styles["serial-log__line--ws-send"];
  }

  return undefined;
}

const SerialLog: Component = () => {
  const { connect, connectToPairedPort, disconnect, isConnected, logs, clear } = useSerial();
  const [systemInfo] = useSystemInfo();
  const [wrapLines, setWrapLines] = createSignal(false);
  const [autoScroll, setAutoScroll] = createSignal(true);
  const [selectedFilters, setSelectedFilters] = createSignal<Set<SerialFilterType>>(
    new Set(SERIAL_FILTER_OPTIONS.map((opt) => opt.value))
  );
  let logContainerRef: HTMLDivElement | undefined;
  const baudRate = createMemo(() => systemInfo.serialBaudRate || 115200);

  const toggleFilter = (filterType: SerialFilterType) => {
    setSelectedFilters((prev) => {
      const next = new Set(prev);
      if (next.has(filterType)) {
        next.delete(filterType);
      } else {
        next.add(filterType);
      }
      return next;
    });
  };

  const isAllChecked = createMemo(() => selectedFilters().size === SERIAL_FILTER_OPTIONS.length);

  const toggleAllFilters = () => {
    if (isAllChecked()) {
      setSelectedFilters(new Set<SerialFilterType>());
    } else {
      setSelectedFilters(new Set(SERIAL_FILTER_OPTIONS.map((opt) => opt.value)));
    }
  };

  const filteredLogs = createMemo(() => {
    const selected = selectedFilters();
    const allLogs = logs();

    return allLogs.filter((entry) => {
      const filterType = getFilterTypeForLogType(entry.logType);
      return selected.has(filterType);
    });
  });

  createEffect(() => {
    filteredLogs();

    if (!autoScroll() || !logContainerRef) {
      return;
    }

    queueMicrotask(() => {
      if (!logContainerRef) {
        return;
      }

      logContainerRef.scrollTop = logContainerRef.scrollHeight;
    });
  });

  const connectClicked = async () => {
    if (isConnected()) {
      await disconnect();
      return;
    }

    try {
      await connect({ baudRate: baudRate() });
    } catch (e) {
      console.error("Serial connect failed", e);
      alert("Serial connect failed: " + (e as Error).message);
    }
  };

  onMount(async () => {
    if (isConnected()) {
      return;
    }

    try {
      await connectToPairedPort({ baudRate: baudRate() });
    } catch (e) {
      console.warn("Serial auto-connect skipped", e);
    }
  });

  return (
    <CollapsibleSection title="USB Monitoring" icon={<BugIcon width={24} height={24} />}>
      <p class={styles["serial-log__info-text"]}>
        Connect the ESP32 via USB to this device and press Connect.
      </p>

      <div class={styles["serial-log__toolbar"]}>
        <label class={styles["serial-log__checkbox"]}>
          <input
            type="checkbox"
            checked={wrapLines()}
            onChange={(e) => setWrapLines(e.currentTarget.checked)}
          />
          <span>Wrap lines</span>
        </label>
        <label class={styles["serial-log__checkbox"]}>
          <input
            type="checkbox"
            checked={autoScroll()}
            onChange={(e) => setAutoScroll(e.currentTarget.checked)}
          />
          <span>Auto-scroll</span>
        </label>

        <div class={styles["serial-log__filter-checkboxes"]}>
          <label class={styles["serial-log__filter-checkbox"]}>
            <input type="checkbox" checked={isAllChecked()} onChange={toggleAllFilters} />
            <span>All</span>
          </label>
          <For each={SERIAL_FILTER_OPTIONS}>
            {(option) => (
              <label class={styles["serial-log__filter-checkbox"]}>
                <input
                  type="checkbox"
                  checked={selectedFilters().has(option.value)}
                  onChange={() => toggleFilter(option.value)}
                />
                <span>{option.label}</span>
              </label>
            )}
          </For>
        </div>

        <div class={styles["serial-log__actions"]}>
          <button
            class={`${styles["serial-log__button"]} ${
              isConnected()
                ? styles["serial-log__button--secondary"]
                : styles["serial-log__button--primary"]
            }`}
            onClick={connectClicked}
          >
            {isConnected() ? "Disconnect" : "Connect"}
          </button>
          <button
            class={`${styles["serial-log__button"]} ${styles["serial-log__button--secondary"]}`}
            onClick={() => clear()}
            disabled={logs().length === 0}
          >
            Clear
          </button>
        </div>
      </div>

      <div
        ref={logContainerRef}
        classList={{
          [styles["serial-log__log"]]: true,
          [styles["serial-log__log--wrapped"]]: wrapLines(),
        }}
      >
        <For each={filteredLogs()}>
          {(entry: ISerialLogEntry) => (
            <div
              classList={{
                [styles["serial-log__line"]]: true,
                [getLogTypeClass(entry.logType) ?? ""]: !!getLogTypeClass(entry.logType),
              }}
            >
              {entry.line}
            </div>
          )}
        </For>
      </div>

      <div class={styles["serial-log__status"]}>
        <span
          classList={{
            [styles["serial-log__status-badge"]]: true,
            [styles["serial-log__status-badge--connected"]]: isConnected(),
          }}
        >
          {isConnected() ? "Connected" : "Disconnected"}
        </span>
        <span>
          {filteredLogs().length} / {logs().length} entries
        </span>
      </div>
    </CollapsibleSection>
  );
};

export { SerialLog };
