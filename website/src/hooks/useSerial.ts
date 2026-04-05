import { createSignal, onCleanup } from "solid-js";

const SERIAL_LOG_MAX_HISTORY = 1000;

export interface ISerialLogEntry {
  line: string;
  logType: string | null;
}

function isIgnorableSerialStateError(error: unknown): boolean {
  if (!(error instanceof Error)) {
    return false;
  }

  return error.name === "InvalidStateError" || error.message.includes("already closed");
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

  const flushPendingLine = () => {
    if (pendingLine.length > 0) {
      appendLogs([pendingLine]);
      pendingLine = "";
    }
  };

  const safeClosePort = async () => {
    if (!port) {
      return;
    }

    const portToClose = port;
    port = null;

    try {
      if (portToClose.readable || portToClose.writable) {
        await portToClose.close();
      }
    } catch (e) {
      if (!isIgnorableSerialStateError(e)) {
        throw e;
      }
    }
  };

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

  async function getPairedPorts() {
    const serialApi = (navigator as any).serial;
    if (!serialApi?.getPorts) {
      return [] as any[];
    }

    return (await serialApi.getPorts()) as any[];
  }

  async function attachToPort(nextPort: any, baudRate: number) {
    port = nextPort;

    try {
      if (!port.readable) {
        await port.open({ baudRate });
      }

      setIsConnected(true);
      readLoop();
    } catch (e) {
      setIsConnected(false);
      await safeClosePort();
      throw e;
    }
  }

  async function connect(options: { baudRate?: number; port?: any } = { baudRate: 115200 }) {
    const serialApi = (navigator as any).serial;
    if (!serialApi) {
      throw new Error("Web Serial API not supported in this browser");
    }

    const selectedPort = options.port ?? (await serialApi.requestPort());
    await attachToPort(selectedPort, options.baudRate ?? 115200);
  }

  async function connectToPairedPort(options: { baudRate?: number } = { baudRate: 115200 }) {
    const pairedPorts = await getPairedPorts();
    const baudRate = options.baudRate ?? 115200;
    let lastError: unknown = null;

    for (const pairedPort of pairedPorts) {
      try {
        await attachToPort(pairedPort, baudRate);
        return true;
      } catch (e) {
        lastError = e;
      }
    }

    if (lastError) {
      console.warn("Serial auto-connect skipped", lastError);
    }

    return false;
  }

  async function disconnect() {
    try {
      if (reader) {
        await reader.cancel();
        reader = null;
      }
      await safeClosePort();
    } catch (e) {
      if (!isIgnorableSerialStateError(e)) {
        console.error("Error closing serial", e);
      }
    }

    flushPendingLine();

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
    } finally {
      reader = null;
    }
  }

  onCleanup(() => {
    // best-effort cleanup
    void (async () => {
      try {
        if (reader) {
          await reader.cancel();
          reader = null;
        }

        await safeClosePort();
      } catch {
        // Ignore cleanup errors during unmount.
      } finally {
        pendingLine = "";
        setIsConnected(false);
      }
    })();
  });

  return {
    connect,
    connectToPairedPort,
    getPairedPorts,
    disconnect,
    isConnected,
    logs,
    clear,
  } as const;
}
