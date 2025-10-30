import { Show, For } from "solid-js";
import styles from "./WheelGraphic.module.css";

export interface WheelGraphicProps {
  angle?: number; // current UI angle (degrees)
  size?: number; // outer SVG size (pixels)
  breakpoints?: number[]; // list of breakpoint angles in degrees
  zeroPointDegree?: number; // zero offset degree
  isCalibrated?: boolean; // whether calibration complete
  isSearchingZero?: boolean; // show spinning ? mark
}

// Pure visual wheel graphic (no device store usage). All math is internal.
export function WheelGraphic(props: WheelGraphicProps) {
  const size = () => props.size ?? 100;
  const radius = () => size() / 2 - 15;
  const zeroPointDegree = () => props.zeroPointDegree ?? 0;
  const breakpoints = () => props.breakpoints ?? [];

  // use createMemo?
  const innerRadius = radius() * 0.9;
  const outerRadius = radius() * 1.1;
  const textRadius = radius() * 0.8;

  return (
    <svg class={styles["wheel-graphic__svg"]} viewBox={`0 0 ${size()} ${size()}`}>
      <g
        transform={`rotate(${-(props.angle ?? 0)})`}
        transform-origin={`${size() / 2}px ${size() / 2}px`}
      >
        <circle
          cx={size() / 2}
          cy={size() / 2}
          r={radius()}
          style={{ fill: "none", stroke: "currentColor", "stroke-width": "1" }}
        />

        <Show when={!props.isCalibrated}>
          <text
            x={size() / 2}
            y={size() / 2}
            text-anchor="middle"
            dominant-baseline="middle"
            class={props.isSearchingZero ? styles["wheel-graphic__searching"] : undefined}
            style={{
              fill: "currentColor",
              "transform-origin": "center",
              opacity: 0.5,
              "font-size": "16px",
              "font-weight": "bold",
              "font-family": "monospace",
            }}
          >
            ?
          </text>
        </Show>

        <Show when={props.isCalibrated}>
          {(() => {
            const angleRad = (-zeroPointDegree() / 180) * Math.PI;
            return (
              <line
                x1={size() / 2 + Math.sin(angleRad) * outerRadius}
                y1={size() / 2 - Math.cos(angleRad) * outerRadius}
                x2={size() / 2 + Math.sin(angleRad) * radius()}
                y2={size() / 2 - Math.cos(angleRad) * radius()}
                style={{ stroke: "currentColor", "stroke-width": "1" }}
              />
            );
          })()}
        </Show>
      </g>

      {/* Zero degree outer marker */}
      {(() => {
        const angleRad = (-zeroPointDegree() / 180) * Math.PI;
        return (
          <g>
            <line
              x1={size() / 2 + Math.sin(angleRad) * innerRadius}
              y1={size() / 2 - Math.cos(angleRad) * innerRadius}
              x2={size() / 2 + Math.sin(angleRad) * radius()}
              y2={size() / 2 - Math.cos(angleRad) * radius()}
              style={{ stroke: "currentColor", opacity: 0.5, "stroke-width": "1" }}
            />
            <text
              x={size() / 2 + Math.sin(angleRad) * textRadius}
              y={size() / 2 - Math.cos(angleRad) * textRadius}
              text-anchor="middle"
              dominant-baseline="middle"
              style={{
                fill: "currentColor",
                opacity: 0.5,
                "font-size": "6px",
                "font-weight": "bold",
                "font-family": "monospace",
              }}
            >
              0
            </text>
          </g>
        );
      })()}
      <For each={breakpoints()}>
        {(bp: number, idx) => {
          const angleRad = -((bp + zeroPointDegree()) / 180) * Math.PI;
          return (
            <g>
              <line
                x1={size() / 2 + Math.sin(angleRad) * innerRadius}
                y1={size() / 2 - Math.cos(angleRad) * innerRadius}
                x2={size() / 2 + Math.sin(angleRad) * radius()}
                y2={size() / 2 - Math.cos(angleRad) * radius()}
                style={{ stroke: "currentColor", "stroke-width": "1" }}
              />
              <text
                x={size() / 2 + Math.sin(angleRad) * textRadius}
                y={size() / 2 - Math.cos(angleRad) * textRadius}
                text-anchor="middle"
                dominant-baseline="middle"
                style={{
                  fill: "currentColor",
                  "font-size": "6px",
                  "font-weight": "bold",
                  "font-family": "monospace",
                }}
              >
                {idx() + 1}
              </text>
            </g>
          );
        }}
      </For>
    </svg>
  );
}
