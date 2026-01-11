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
    viewBox="0 0 500 500"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    class={props.class}
    style={props.style}
    xmlns="http://www.w3.org/2000/svg"
  >
    <g>
      <path
        transform="scale(1.5) translate(-75,-65)"
        d="M 152.545 291.464 C 152.545 293.186 152.186 294.376 151.469 295.036 C 150.752 295.696 149.532 296.026 147.81 296.026 L 129.475 296.026 C 127.753 296.026 126.534 295.696 125.816 295.036 C 125.099 294.376 124.74 293.186 124.74 291.464 L 124.74 186.357 C 124.74 184.692 125.027 183.43 125.601 182.569 C 128.183 178.781 131.799 174.793 136.448 170.603 C 137.308 169.915 138.054 169.571 138.686 169.571 C 139.374 169.571 140.091 169.915 140.838 170.603 C 143.363 172.784 145.486 174.865 147.208 176.845 C 148.93 178.825 150.422 180.733 151.684 182.569 C 152.258 183.487 152.545 184.75 152.545 186.357 L 152.545 291.464 Z M 175.519 214.936 L 175.519 209.771 C 175.519 207.82 175.792 206.342 176.337 205.338 C 176.882 204.334 177.786 203.229 179.049 202.024 L 197.384 185.41 C 201.631 181.565 204.385 178.739 205.648 176.931 C 206.911 175.123 207.542 173.071 207.542 170.776 C 207.542 165.094 204.558 162.254 198.589 162.254 C 195.777 162.254 193.338 162.727 191.272 163.674 C 189.206 164.621 187.513 165.582 186.194 166.558 C 184.931 167.591 183.955 167.419 183.267 166.041 L 177.413 154.936 C 177.011 154.133 177.126 153.473 177.758 152.957 C 179.536 151.465 182.406 149.772 186.366 147.878 C 190.325 145.984 195.777 145.037 202.721 145.037 C 209.837 145.037 215.834 146.945 220.713 150.762 C 225.591 154.578 228.03 159.987 228.03 166.988 C 228.03 173.071 226.265 178.337 222.736 182.784 C 219.206 187.232 214.802 191.751 209.522 196.343 L 202.033 202.884 L 202.033 203.401 L 221.487 203.401 C 222.52 203.401 223.324 203.573 223.898 203.918 C 226.193 205.41 228.661 207.562 231.3 210.374 C 231.76 210.948 231.99 211.378 231.99 211.665 C 231.99 211.952 231.76 212.383 231.3 212.957 C 228.661 215.768 226.193 217.92 223.898 219.412 C 223.209 219.757 222.405 219.929 221.487 219.929 L 181.373 219.929 C 179.135 219.929 177.599 219.599 176.767 218.939 C 175.935 218.279 175.519 216.945 175.519 214.936 Z M 303.178 274.936 C 307.367 274.936 310.653 274.505 313.035 273.644 C 315.416 272.784 317.267 271.722 318.587 270.459 C 319.907 269.197 321.055 267.934 322.03 266.672 C 322.949 265.524 323.709 264.663 324.312 264.09 C 324.914 263.516 325.76 263.114 326.851 262.884 C 327.769 262.597 329.247 262.411 331.284 262.325 C 333.322 262.239 335.359 262.196 337.396 262.196 C 339.433 262.196 340.854 262.253 341.657 262.368 C 342.862 262.425 343.694 262.655 344.153 263.056 C 344.613 263.458 344.9 264.319 345.014 265.639 C 345.187 267.475 345.287 269.656 345.316 272.181 C 345.344 274.706 345.301 276.887 345.187 278.723 C 345.129 279.297 345.029 279.785 344.885 280.187 C 344.742 280.588 344.441 281.191 343.982 281.995 C 343.408 282.913 342.289 284.247 340.624 285.997 C 338.96 287.748 336.521 289.555 333.307 291.42 C 330.093 293.286 325.947 294.878 320.868 296.198 C 315.789 297.518 309.549 298.178 302.145 298.178 C 294.226 298.178 286.823 297.159 279.936 295.122 C 273.049 293.085 267.009 289.656 261.815 284.835 C 256.622 280.015 252.576 273.501 249.678 265.295 C 246.78 257.088 245.331 246.844 245.331 234.563 C 245.331 220.905 247.698 209.298 252.432 199.743 C 257.167 190.188 263.896 182.914 272.619 177.921 C 281.342 172.928 291.672 170.432 303.609 170.432 C 311.987 170.432 318.716 171.292 323.795 173.014 C 328.874 174.736 332.734 176.759 335.373 179.083 C 338.013 181.407 339.878 183.43 340.968 185.152 C 341.37 185.783 341.657 186.343 341.829 186.83 C 342.002 187.318 342.116 188.079 342.174 189.112 C 342.346 190.489 342.389 192.497 342.303 195.137 C 342.217 197.777 342.002 200.101 341.657 202.11 C 341.428 203.372 341.055 204.205 340.538 204.606 C 340.308 204.836 339.907 205.037 339.333 205.209 C 338.759 205.381 337.898 205.467 336.751 205.467 C 334.914 205.467 332.848 205.395 330.553 205.252 C 328.257 205.109 326.535 204.951 325.388 204.779 C 324.01 204.549 323.006 204.219 322.375 203.789 C 321.743 203.358 321.112 202.684 320.481 201.766 C 319.448 200.274 317.669 198.552 315.144 196.6 C 312.619 194.649 308.458 193.674 302.662 193.674 C 294.513 193.674 287.798 196.816 282.519 203.1 C 277.239 209.384 274.599 219.556 274.599 233.616 C 274.599 247.619 277.368 258.006 282.906 264.778 C 288.444 271.55 295.201 274.936 303.178 274.936 Z"
        style="fill: #000"
      ></path>

      <polygon
        style="fill: none; stroke: #000; stroke-width: 16px;"
        points="10 10 400 10 495 280 400 490 10 490"
      ></polygon>
    </g>
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
