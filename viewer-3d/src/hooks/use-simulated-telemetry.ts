"use client";

import { useEffect, useRef } from "react";

import { initialTelemetry, useIncubatorStore } from "@/stores/incubator-store";

function clamp(value: number, min: number, max: number) {
  return Math.max(min, Math.min(max, value));
}

export function useSimulatedTelemetry() {
  const setTelemetry = useIncubatorStore((s) => s.setTelemetry);
  const telemetryRef = useRef(initialTelemetry);

  useEffect(() => {
    const id = window.setInterval(() => {
      const previous = telemetryRef.current;

      const heaterDuty = clamp(previous.heaterDuty + (Math.random() * 8 - 4), 0, 100);
      const fanRpm = clamp(
        previous.fanRpm + (Math.random() * 140 - 70) + heaterDuty * 1.2,
        600,
        3200
      );
      const doorToggle = Math.random() < 0.01;
      const doorOpen = doorToggle ? !previous.doorOpen : previous.doorOpen;
      const chamberTemp = clamp(
        previous.chamberTemp + (heaterDuty / 100) * 0.15 - (doorOpen ? 0.08 : 0.02),
        28,
        39.5
      );
      const skinTemp = clamp(chamberTemp + 1.5 + (Math.random() * 0.14 - 0.07), 31, 38);
      const humidity = clamp(
        previous.humidity + (doorOpen ? -0.6 : 0.25) + (Math.random() * 0.6 - 0.3),
        25,
        90
      );
      const alarm =
        chamberTemp > 37.8
          ? "TEMPERATURE_ALARM"
          : humidity < 35
            ? "HUMIDITY_ALARM"
            : fanRpm < 800
              ? "FAN_ISSUE_ALARM"
              : "NO_ALARMS";

      const next = {
        chamberTemp,
        skinTemp,
        humidity,
        fanRpm,
        heaterDuty,
        phototherapyDuty: previous.phototherapyDuty,
        doorOpen,
        alarm,
        timestampMs: Date.now()
      };

      telemetryRef.current = next;
      setTelemetry(next);
    }, 700);

    return () => {
      window.clearInterval(id);
    };
  }, [setTelemetry]);
}
