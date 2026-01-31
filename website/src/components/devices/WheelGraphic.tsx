import { Show, For, createMemo } from "solid-js";
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
  const size = createMemo(() => props.size ?? 100);
  const radius = createMemo(() => size() / 2 - 15);
  const zeroPointDegree = createMemo(() => props.zeroPointDegree ?? 0);
  const zeroPointRad = createMemo(() => (zeroPointDegree() / 180) * Math.PI);
  const breakpoints = createMemo(() => props.breakpoints ?? []);

  const innerRadius = createMemo(() => radius() * 0.9);
  const outerRadius = createMemo(() => radius() * 1.1);
  const textRadius = createMemo(() => radius() * 0.8);

  const shrink = size() * 0.1;

  return (
    <svg
      class={styles["wheel-graphic__svg"]}
      viewBox={`${shrink} ${shrink} ${size() - shrink * 2} ${size() - shrink * 2}`}
      width={size()}
      height={size()}
    >
      {/* Rotates */}
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

        {/* Zero degree inner marker */}
        <g>
          <line
            x1={size() / 2 + Math.sin(-zeroPointRad()) * innerRadius()}
            y1={size() / 2 - Math.cos(-zeroPointRad()) * innerRadius()}
            x2={size() / 2 + Math.sin(-zeroPointRad()) * radius()}
            y2={size() / 2 - Math.cos(-zeroPointRad()) * radius()}
            style={{ stroke: "currentColor", opacity: 0.5, "stroke-width": "1" }}
          />
          <text
            x={size() / 2 + Math.sin(-zeroPointRad()) * textRadius()}
            y={size() / 2 - Math.cos(-zeroPointRad()) * textRadius()}
            text-anchor="middle"
            dominant-baseline="middle"
            transform={`rotate(${props.angle ?? 0}, ${size() / 2 + Math.sin(-zeroPointRad()) * textRadius()}, ${size() / 2 - Math.cos(-zeroPointRad()) * textRadius()})`}
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

        <For each={breakpoints()}>
          {(bp: number, idx) => {
            const angleRad = ((bp - zeroPointDegree()) / 180) * Math.PI;
            return (
              <g>
                <line
                  x1={size() / 2 + Math.sin(angleRad) * innerRadius()}
                  y1={size() / 2 - Math.cos(angleRad) * innerRadius()}
                  x2={size() / 2 + Math.sin(angleRad) * radius()}
                  y2={size() / 2 - Math.cos(angleRad) * radius()}
                  style={{ stroke: "currentColor", "stroke-width": "1" }}
                />
                <text
                  x={size() / 2 + Math.sin(angleRad) * textRadius()}
                  y={size() / 2 - Math.cos(angleRad) * textRadius()}
                  text-anchor="middle"
                  dominant-baseline="middle"
                  transform={`rotate(${props.angle ?? 0}, ${size() / 2 + Math.sin(angleRad) * textRadius()}, ${size() / 2 - Math.cos(angleRad) * textRadius()})`}
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
      </g>

      {/* Doesn't rotates */}
      {/* Zero degree outer marker */}
      <line
        x1={size() / 2 + Math.sin(-zeroPointRad()) * outerRadius()}
        y1={size() / 2 - Math.cos(-zeroPointRad()) * outerRadius()}
        x2={size() / 2 + Math.sin(-zeroPointRad()) * radius()}
        y2={size() / 2 - Math.cos(-zeroPointRad()) * radius()}
        style={{ stroke: "currentColor", "stroke-width": "1" }}
      />
    </svg>
  );
}
