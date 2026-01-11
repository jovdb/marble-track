// Device PNG Icons
import { JSX } from "solid-js";

// Import PNG icons
import ledIcon from "../../assets/icons/led.png";
import servoIcon from "../../assets/icons/servo.png";
import buttonIcon from "../../assets/icons/button.png";
import buzzerIcon from "../../assets/icons/buzzer1.png";
import stepperIcon from "../../assets/icons/stepper.png";
import devicesIcon from "../../assets/icons/devices.png";
import websocketIcon from "../../assets/icons/websocket.png";
import dotsIcon from "../../assets/icons/dots.png";

// Icon component props
export interface IconProps {
  width?: number;
  height?: number;
  class?: string;
  style?: JSX.CSSProperties;
  alt?: string;
}

export const LedIcon = (props: IconProps) => (
  <img
    src={ledIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "LED"}
  />
);

export const ServoIcon = (props: IconProps) => (
  <img
    src={servoIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "Servo"}
  />
);

export const ButtonIcon = (props: IconProps) => (
  <img
    src={buttonIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "Button"}
  />
);

export const BuzzerIcon = (props: IconProps) => (
  <img
    src={buzzerIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "Buzzer"}
  />
);

export const StepperIcon = (props: IconProps) => (
  <img
    src={stepperIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "Stepper Motor"}
  />
);

export const WheelIcon = (props: IconProps) => (
  <svg
    width={props.width || 24}
    height={props.height || 24}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <circle cx="12" cy="12" r="9" />
  </svg>
);

export const LiftIcon = (props: IconProps) => (
  <svg
    width={props.width || 24}
    height={props.height || 24}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="1"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <path stroke="none" d="M0 0h24v24H0z" fill="none" />
    <path d="M5 4m0 1a1 1 0 0 1 1 -1h12a1 1 0 0 1 1 1v14a1 1 0 0 1 -1 1h-12a1 1 0 0 1 -1 -1z" />
    <path d="M10 10l2 -2l2 2" />
    <path d="M10 14l2 2l2 -2" />
  </svg>
);

export const IoExpanderIcon = (props: IconProps) => (
  <svg
    width={props.width || 24}
    height={props.height || 24}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <path stroke="none" d="M0 0h24v24H0z" fill="none" />
    {/* IC chip body */}
    <rect x="6" y="4" width="12" height="16" rx="1" />
    {/* Left pins */}
    <path d="M6 8h-3" />
    <path d="M6 12h-3" />
    <path d="M6 16h-3" />
    {/* Right pins */}
    <path d="M18 8h3" />
    <path d="M18 12h3" />
    <path d="M18 16h3" />
    {/* Chip notch */}
    <path d="M10 4v1a1 1 0 0 0 1 1h2a1 1 0 0 0 1-1v-1" />
  </svg>
);

export const I2cIcon = (props: IconProps) => (
  <svg
    width={props.width || 24}
    height={props.height || 24}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <path stroke="none" d="M0 0h24v24H0z" fill="none" />
    {/* I2C bus lines */}
    <path d="M4 12h16" />
    <path d="M8 8v8" />
    <path d="M16 8v8" />
    {/* SDA/SCL labels */}
    <text x="6" y="6" font-size="4" fill="currentColor">
      SDA
    </text>
    <text x="14" y="6" font-size="4" fill="currentColor">
      SCL
    </text>
  </svg>
);

export const ClipboardIcon = (props: IconProps) => (
  <img
    src={devicesIcon}
    width={props.width || 20}
    height={props.height || 20}
    class={props.class}
    style={props.style}
    alt={props.alt || "Devices"}
  />
);

export const RadioIcon = (props: IconProps) => (
  <img
    src={websocketIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "WebSocket"}
  />
);

export const BroadcastIcon = (props: IconProps) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
  >
    <path stroke="none" d="M0 0h24v24H0z" fill="none" />
    <path d="M12 12m-1 0a1 1 0 1 0 2 0a1 1 0 1 0 -2 0" />
    <path d="M16.616 13.924a5 5 0 1 0 -9.23 0" />
    <path d="M20.307 15.469a9 9 0 1 0 -16.615 0" />
    <path d="M9 21l3 -9l3 9" />
    <path d="M10 19h4" />
  </svg>
);

export const MessageIcon = (props: IconProps) => (
  <svg
    width={props.width || 20}
    height={props.height || 20}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    class={props.class}
    style={props.style}
  >
    <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z" />
    <path d="M8 10h.01" />
    <path d="M12 10h.01" />
    <path d="M16 10h.01" />
  </svg>
);

export const IncomingMessageIcon = (props: IconProps) => (
  <svg
    width={props.width || 20}
    height={props.height || 20}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="3"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
  >
    <path d="M9 18l6-6-6-6" />
  </svg>
);

export const OutgoingMessageIcon = (props: IconProps) => (
  <svg
    width={props.width || 20}
    height={props.height || 20}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="3"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
  >
    <path d="M15 18l-6-6 6-6" />
  </svg>
);

export const DotsIcon = (props: IconProps) => (
  <img
    src={dotsIcon}
    width={props.width || 24}
    height={props.height || 24}
    class={props.class}
    style={props.style}
    alt={props.alt || "WebSocket"}
  />
);

export const ChevronRightIcon = (props: IconProps) => (
  <svg
    width={props.width || 16}
    height={props.height || 16}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <polyline points="9 18 15 12 9 6" />
  </svg>
);

export const ChevronDownIcon = (props: IconProps) => (
  <svg
    width={props.width || 16}
    height={props.height || 16}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <polyline points="6 9 12 15 18 9" />
  </svg>
);

export const TrashIcon = (props: IconProps) => (
  <svg
    width={props.width || 20}
    height={props.height || 20}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <path d="M3 6h18" />
    <path d="M8 6v-2a1 1 0 0 1 1-1h6a1 1 0 0 1 1 1v2" />
    <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6" />
    <path d="M10 11v6" />
    <path d="M14 11v6" />
  </svg>
);

export function ConnectedIcon(props: IconProps) {
  return (
    <svg
      width={props.width || 24}
      height={props.height || 24}
      class={props.class}
      style={props.style}
      viewBox="0 0 24 24"
      fill="none"
      stroke="#000000"
      stroke-width="1"
      stroke-linecap="round"
      stroke-linejoin="round"
    >
      <path d="M18.364 19.364a9 9 0 1 0 -12.728 0" />
      <path d="M15.536 16.536a5 5 0 1 0 -7.072 0" />
      <path d="M12 13m-1 0a1 1 0 1 0 2 0a1 1 0 1 0 -2 0" />
    </svg>
  );
}

export function DisconnectedIcon(props: IconProps) {
  return (
    <svg
      width={props.width || 24}
      height={props.height || 24}
      class={props.class}
      style={props.style}
      viewBox="0 0 24 24"
      fill="none"
      stroke="#000000"
      stroke-width="1"
      stroke-linecap="round"
      stroke-linejoin="round"
    >
      <path d="M18.364 19.364a9 9 0 0 0 -9.721 -14.717m-2.488 1.509a9 9 0 0 0 -.519 13.208" />
      <path d="M15.536 16.536a5 5 0 0 0 -3.536 -8.536m-3 1a5 5 0 0 0 -.535 7.536" />
      <path d="M12 12a1 1 0 1 0 1 1" />
      <path d="M3 3l18 18" />
    </svg>
  );
}

export const RestartIcon = (props: IconProps) => (
  <svg
    width={props.width || 24}
    height={props.height || 24}
    viewBox="0 0 24 24"
    fill="none"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <path
      d="M18.364 8.05026L17.6569 7.34315C14.5327 4.21896 9.46734 4.21896 6.34315 7.34315C3.21895 10.4673 3.21895 15.5327 6.34315 18.6569C9.46734 21.7811 14.5327 21.7811 17.6569 18.6569C19.4737 16.84 20.234 14.3668 19.9377 12.0005M18.364 8.05026H14.1213M18.364 8.05026V3.80762"
      stroke="#1C274C"
      stroke-width="1.5"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
  </svg>
);

// Helper function to get device icon by type
export const getDeviceIcon = (type: string, props?: IconProps) => {
  switch (type.toUpperCase()) {
    case "LED":
      return <LedIcon {...props} />;
    case "BUTTON":
      return <ButtonIcon {...props} />;
    case "BUZZER":
      return <BuzzerIcon {...props} />;
    case "I2C":
      return <I2cIcon {...props} />;
    case "SERVO":
      return <ServoIcon {...props} />;
    case "STEPPER":
      return <StepperIcon {...props} />;
    case "WHEEL":
      return <WheelIcon {...props} />;
    case "LIFT":
      return <LiftIcon {...props} />;
    case "MARBLECONTROLLER":
      return <ClipboardIcon {...props} />;
    case "IOEXPANDER":
      return <IoExpanderIcon {...props} />;
    default:
      return null; // Default fallback
  }
};
// Add a icon for wifi_connected and wifi_disconnected

export const WifiConnectedIcon = (props: IconProps) => (
  <svg
    width={props.width || 24}
    height={props.height || 24}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <path stroke="none" d="M0 0h24v24H0z" fill="none" />
    <path d="M12 18l.01 0" />
    <path d="M9.172 15.172a4 4 0 0 1 5.656 0" />
    <path d="M6.343 12.343a8 8 0 0 1 11.314 0" />
    <path d="M3.515 9.515c4.686 -4.687 12.284 -4.687 17 0" />
  </svg>
);

export const WifiDisconnectedIcon = (props: IconProps) => (
  <svg
    width={props.width || 24}
    height={props.height || 24}
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <path stroke="none" d="M0 0h24v24H0z" fill="none" />
    <path d="M12 18l.01 0" />
    <path d="M9.172 15.172a4 4 0 0 1 5.656 0" />
    <path d="M6.343 12.343a7.963 7.963 0 0 1 3.864 -2.14m4.163 .155a7.965 7.965 0 0 1 3.287 2" />
    <path d="M3.515 9.515a12 12 0 0 1 3.544 -2.455m3.101 -.92a12 12 0 0 1 10.325 3.374" />
    <path d="M3 3l18 18" />
  </svg>
);
