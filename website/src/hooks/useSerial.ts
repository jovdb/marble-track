import { createSignal, onCleanup } from "solid-js";

const SERIAL_LOG_MAX_HISTORY = 1000;

export interface ISerialLogEntry {
  line: string;
  logType: string | null;
}

function parseLogType(line: string): string | null {
  const match = line.match(/^\[[^\]]+\]\[([^\]]+)\]\[[^\]]+\].*/);
  if (!match) {
    return null;
  }

  return match[1].trim();
}

declare global {
  interface Navigator {
    serial?: any;
  }
}

export function useSerial() {
  const [isConnected, setIsConnected] = createSignal(false);
  const [logs, setLogs] = createSignal<ISerialLogEntry[]>([]);

  let port: any = null;
  let reader: any = null;
  let pendingLine = "";

  const clear = () => setLogs([]);

  const appendLogs = (newLines: string[]) => {
    if (newLines.length === 0) {
      return;
    }

    const parsedEntries = newLines.map((line) => ({
      line,
      logType: parseLogType(line),
    }));

    setLogs((prev) => {
      const next = [...prev, ...parsedEntries];
      if (next.length <= SERIAL_LOG_MAX_HISTORY) {
        return next;
      }

      return next.slice(next.length - SERIAL_LOG_MAX_HISTORY);
    });
  };

  async function connect(options: { baudRate?: number } = { baudRate: 115200 }) {
    if (!(navigator as any).serial) {
      throw new Error("Web Serial API not supported in this browser");
    }

    port = await (navigator as any).serial.requestPort();
    await port.open({ baudRate: options.baudRate ?? 115200 });
    setIsConnected(true);
    readLoop();
  }

  async function disconnect() {
    try {
      if (reader) {
        await reader.cancel();
        reader = null;
      }
      if (port) {
        await port.close();
        port = null;
      }
    } catch (e) {
      console.error("Error closing serial", e);
    }

    if (pendingLine.length > 0) {
      appendLogs([pendingLine]);
      pendingLine = "";
    }

    setIsConnected(false);
  }

  async function readLoop() {
    if (!port || !port.readable) return;

    try {
      const textDecoder = new TextDecoderStream();
      const _readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
      const readerStream = textDecoder.readable.getReader();
      reader = readerStream;

      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        if (value) {
          pendingLine += value;

          const normalized = pendingLine.replace(/\r\n/g, "\n").replace(/\r/g, "\n");
          const parts = normalized.split("\n");
          pendingLine = parts.pop() ?? "";

          appendLogs(parts);
        }
      }

      if (pendingLine.length > 0) {
        appendLogs([pendingLine]);
        pendingLine = "";
      }
    } catch (e) {
      console.error("Serial read loop error", e);
    }
  }

  onCleanup(() => {
    // best-effort cleanup
    try {
      if (reader) reader.cancel();
      if (port) port.close();
      pendingLine = "";
    } catch {
      // Ignore cleanup errors during unmount.
    }
  });

  return {
    connect,
    disconnect,
    isConnected,
    logs,
    clear,
  } as const;
}
