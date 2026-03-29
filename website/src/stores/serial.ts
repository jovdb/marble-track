import { createSignal } from "solid-js";

// Global signal for toggling the serial log panel
export const [isSerialOpen, setIsSerialOpen] = createSignal(false);

export const openSerialPanel = () => setIsSerialOpen(true);

export const closeSerialPanel = () => setIsSerialOpen(false);

export const toggleSerialPanel = () => setIsSerialOpen(!isSerialOpen());
