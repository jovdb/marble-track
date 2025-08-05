import { ConsoleLogger } from "../utils/ConsoleLogger";
import { LogLevel } from "../interfaces/ILogger";

// Create a global logger instance
export const logger = new ConsoleLogger("MarbleTrack");

// Set default log level based on environment
if (import.meta.env.MODE === "development") {
  logger.setLevel("debug");
} else {
  logger.setLevel("info");
}

// Export logger factory for component-specific loggers
export const createLogger = (context: string) => logger.createChild(context);

export type { LogLevel };
