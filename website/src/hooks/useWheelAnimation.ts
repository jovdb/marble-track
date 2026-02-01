import { createEffect, createSignal, onCleanup, untrack } from "solid-js";
import { IWheelState } from "../stores/Wheel";

type AnimationPlan = {
  startAngle: number;
  targetAngle: number;
  direction: 1 | -1;
  distance: number;
  acceleration: number;
  cruiseSpeed: number;
  accelDuration: number;
  cruiseDuration: number;
  decelDuration: number;
  accelDistance: number;
  cruiseDistance: number;
  totalDuration: number;
};

export function useWheelAnimation(deviceState: () => IWheelState | undefined) {
  const [uiAngle, setUiAngle] = createSignal<undefined | number>(undefined);

  let animationFrame: number | null = null;
  let animationStartTime: number | null = null;
  let currentAnimation: AnimationPlan | null = null;
  let activeMovementKey: string | null = null;
  let completedMovementKey: string | null = null;

  createEffect(() => {
    const angle = deviceState()?.currentAngle;
    if (angle !== undefined && angle !== null) setUiAngle(angle);
  });

  createEffect(() => {
    const state = deviceState();
    if (!state) {
      currentAnimation = null;
      activeMovementKey = null;
      completedMovementKey = null;
      animationStartTime = null;
      return;
    }

    const status = state.state;
    const targetAngle = state.targetAngle;
    const speedRpm = state.speedRpm;
    const accelerationRpm = state.acceleration ?? 0;

    if (
      status !== "MOVING" ||
      targetAngle === undefined ||
      targetAngle === null ||
      speedRpm === undefined ||
      speedRpm === null ||
      speedRpm <= 0
    ) {
      currentAnimation = null;
      animationStartTime = null;
      activeMovementKey = null;
      if (status !== "MOVING") {
        completedMovementKey = null;
      }
      return;
    }

    const movementKey = `${targetAngle}:${speedRpm}:${accelerationRpm}`;

    if (movementKey === completedMovementKey) {
      return;
    }

    if (movementKey === activeMovementKey && currentAnimation) {
      return;
    }

    const startingAngle = state.currentAngle ?? untrack(() => uiAngle()) ?? targetAngle;

    const plan = createAnimationPlan(startingAngle, targetAngle, speedRpm, accelerationRpm);

    if (!plan) {
      completedMovementKey = movementKey;
      currentAnimation = null;
      animationStartTime = null;
      setUiAngle(targetAngle);
      return;
    }

    currentAnimation = plan;
    activeMovementKey = movementKey;
    animationStartTime = null;
  });

  function createAnimationPlan(
    startAngle: number,
    targetAngle: number,
    speedRpm: number,
    accelerationRpm: number
  ): AnimationPlan | null {
    if (!Number.isFinite(startAngle) || !Number.isFinite(targetAngle)) {
      return null;
    }

    const distanceRaw = targetAngle - startAngle;
    if (!Number.isFinite(distanceRaw) || Math.abs(distanceRaw) < 0.001) {
      return null;
    }

    const direction: 1 | -1 = distanceRaw >= 0 ? 1 : -1;
    const distance = Math.abs(distanceRaw);
    const cruiseSpeed = Math.abs(speedRpm) * 6; // rpm -> deg/s

    if (cruiseSpeed <= 0) {
      return null;
    }

    const acceleration = Math.abs(accelerationRpm) * 6; // rpm/s -> deg/s^2

    if (acceleration <= 0.0001) {
      const cruiseDuration = distance / cruiseSpeed;
      if (!Number.isFinite(cruiseDuration) || cruiseDuration <= 0) {
        return null;
      }
      return {
        startAngle,
        targetAngle,
        direction,
        distance,
        acceleration: 0,
        cruiseSpeed,
        accelDuration: 0,
        cruiseDuration,
        decelDuration: 0,
        accelDistance: 0,
        cruiseDistance: distance,
        totalDuration: cruiseDuration,
      };
    }

    let effectiveCruiseSpeed = cruiseSpeed;
    let accelDuration = effectiveCruiseSpeed / acceleration;
    let accelDistance = 0.5 * acceleration * accelDuration * accelDuration;
    let cruiseDistance = distance - 2 * accelDistance;
    let cruiseDuration = 0;

    if (cruiseDistance < 0) {
      effectiveCruiseSpeed = Math.sqrt(distance * acceleration);
      accelDuration = effectiveCruiseSpeed / acceleration;
      accelDistance = 0.5 * acceleration * accelDuration * accelDuration;
      cruiseDistance = 0;
      cruiseDuration = 0;
    } else {
      cruiseDuration = cruiseDistance / effectiveCruiseSpeed;
    }

    const decelDuration = accelDuration;
    const totalDuration = accelDuration + cruiseDuration + decelDuration;

    if (!Number.isFinite(totalDuration) || totalDuration <= 0) {
      return null;
    }

    return {
      startAngle,
      targetAngle,
      direction,
      distance,
      acceleration,
      cruiseSpeed: effectiveCruiseSpeed,
      accelDuration,
      cruiseDuration,
      decelDuration,
      accelDistance,
      cruiseDistance,
      totalDuration,
    };
  }

  function animateWheel(time: number) {
    const plan = currentAnimation;
    if (plan) {
      if (animationStartTime === null) {
        animationStartTime = time;
      }

      const elapsedSeconds = (time - animationStartTime) / 1000;
      const clampedElapsed = Math.min(elapsedSeconds, plan.totalDuration);

      let travelled = 0;

      if (plan.acceleration <= 0) {
        travelled = plan.cruiseSpeed * clampedElapsed;
      } else if (clampedElapsed <= plan.accelDuration) {
        travelled = 0.5 * plan.acceleration * clampedElapsed * clampedElapsed;
      } else if (clampedElapsed <= plan.accelDuration + plan.cruiseDuration) {
        const cruiseTime = clampedElapsed - plan.accelDuration;
        travelled = plan.accelDistance + plan.cruiseSpeed * cruiseTime;
      } else {
        const decelTime = clampedElapsed - plan.accelDuration - plan.cruiseDuration;
        const decelDistance =
          plan.cruiseSpeed * decelTime - 0.5 * plan.acceleration * decelTime * decelTime;
        travelled = plan.accelDistance + plan.cruiseDistance + decelDistance;
      }

      travelled = Math.min(plan.distance, travelled);
      const nextAngle = plan.startAngle + plan.direction * travelled;
      setUiAngle(nextAngle);

      if (elapsedSeconds >= plan.totalDuration - 0.0001) {
        setUiAngle(plan.targetAngle);
        completedMovementKey = activeMovementKey;
        currentAnimation = null;
        activeMovementKey = null;
        animationStartTime = null;
      }
    } else {
      animationStartTime = null;
    }

    animationFrame = requestAnimationFrame(animateWheel);
  }

  animationFrame = requestAnimationFrame(animateWheel);
  onCleanup(() => {
    if (animationFrame) cancelAnimationFrame(animationFrame);
  });

  return uiAngle;
}
