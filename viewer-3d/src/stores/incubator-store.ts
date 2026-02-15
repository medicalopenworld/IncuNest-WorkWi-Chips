import { create } from "zustand";

export type IncubatorTelemetry = {
  chamberTemp: number;
  skinTemp: number;
  humidity: number;
  fanRpm: number;
  heaterDuty: number;
  phototherapyDuty: number;
  doorOpen: boolean;
  alarm: string;
  timestampMs: number;
};

type IncubatorStore = {
  telemetry: IncubatorTelemetry;
  setTelemetry: (next: IncubatorTelemetry) => void;
};

export const initialTelemetry: IncubatorTelemetry = {
  chamberTemp: 34.2,
  skinTemp: 36.6,
  humidity: 58,
  fanRpm: 1200,
  heaterDuty: 42,
  phototherapyDuty: 0,
  doorOpen: false,
  alarm: "NO_ALARMS",
  timestampMs: 0
};

export const useIncubatorStore = create<IncubatorStore>((set) => ({
  telemetry: initialTelemetry,
  setTelemetry: (next) => set({ telemetry: next })
}));
