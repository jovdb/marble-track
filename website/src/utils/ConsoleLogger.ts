import { ILogger, LogLevel } from "../interfaces/ILogger";

export class ConsoleLogger implements ILogger {
  private currentLevel: LogLevel = "debug";
  private context: string;

  constructor(context: string = "App") {
    this.context = context;
  }

  setLevel(level: LogLevel): void {
    this.currentLevel = level;
  }

  getLevel(): LogLevel {
    return this.currentLevel;
  }

  debug(message: string, ...args: any[]): void {
    this.log("debug", message, ...args);
  }

  info(message: string, ...args: any[]): void {
    this.log("info", message, ...args);
  }

  warn(message: string, ...args: any[]): void {
    this.log("warn", message, ...args);
  }

  error(message: string, ...args: any[]): void {
    this.log("error", message, ...args);
  }

  log(level: LogLevel, message: string, ...args: any[]): void {
    const levelHierarchy: Record<LogLevel, number> = {
      none: -1,
      debug: 0,
      info: 1,
      warn: 2,
      error: 3,
    };

    if (levelHierarchy[level] < levelHierarchy[this.currentLevel]) {
      return;
    }

    const timestamp =
      new Date().toLocaleTimeString("en-US", {
        hour12: false,
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
      }) +
      "." +
      new Date().getMilliseconds().toString().padStart(3, "0");
    const formattedMessage = `${timestamp}: ${this.context}: ${message}`;

    switch (level) {
      case "debug":
        console.debug(formattedMessage, ...args);
        break;
      case "info":
        console.info(formattedMessage, ...args);
        break;
      case "warn":
        console.warn(formattedMessage, ...args);
        break;
      case "error":
        console.error(formattedMessage, ...args);
        break;
    }
  }

  // Create a child logger with extended context
  createChild(childContext: string): ConsoleLogger {
    const child = new ConsoleLogger(`${this.context}:${childContext}`);
    child.setLevel(this.currentLevel);
    return child;
  }
}
