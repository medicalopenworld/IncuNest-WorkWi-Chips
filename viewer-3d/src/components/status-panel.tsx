"use client";

import { useEffect, useMemo, useState } from "react";

import { useIncubatorStore } from "@/stores/incubator-store";

type StatusPanelProps = {
  orientation?: "horizontal" | "panel";
};

function valueClassName(value: number, min: number, max: number) {
  if (value < min || value > max) return "metric-value metric-value--alert";
  return "metric-value";
}

export function StatusPanel({ orientation = "horizontal" }: StatusPanelProps) {
  const telemetry = useIncubatorStore((s) => s.telemetry);
  const [isClient, setIsClient] = useState(false);

  useEffect(() => {
    setIsClient(true);
  }, []);

  const cards = useMemo(
    () => [
      { label: "Temp. cámara", value: `${telemetry.chamberTemp.toFixed(1)}°C`, state: valueClassName(telemetry.chamberTemp, 32, 37.8) },
      { label: "Temp. piel", value: `${telemetry.skinTemp.toFixed(1)}°C`, state: valueClassName(telemetry.skinTemp, 34, 37.8) },
      { label: "Humedad", value: `${telemetry.humidity.toFixed(0)}%`, state: valueClassName(telemetry.humidity, 35, 80) },
      { label: "Ventilador", value: `${telemetry.fanRpm.toFixed(0)} RPM`, state: valueClassName(telemetry.fanRpm, 800, 3500) },
      { label: "Heater PWM", value: `${telemetry.heaterDuty.toFixed(0)}%`, state: "metric-value" }
    ],
    [telemetry]
  );

  const formattedTimestamp =
    isClient && telemetry.timestampMs > 0
      ? new Intl.DateTimeFormat("es-ES", {
          hour: "2-digit",
          minute: "2-digit",
          second: "2-digit"
        }).format(new Date(telemetry.timestampMs))
      : "--:--:--";

  const rootClassName = orientation === "horizontal" ? "status-strip" : "status-panel";
  const metricsClassName = orientation === "horizontal" ? "metrics-row" : "metrics-grid";
  const footerClassName = orientation === "horizontal" ? "status-strip__footer" : "status-panel__footer";

  if (orientation === "horizontal") {
    return (
      <aside className={rootClassName}>
        <header className="status-panel__header">
          <h2>Telemetría</h2>
          <span className={telemetry.alarm === "NO_ALARMS" ? "alarm-badge" : "alarm-badge alarm-badge--active"}>
            {telemetry.alarm}
          </span>
          <span className="status-pill">{telemetry.doorOpen ? "🚪 Abierta" : "🔒 Cerrada"}</span>
          <span className="status-pill status-pill--time" suppressHydrationWarning>🕐 {formattedTimestamp}</span>
        </header>

        <div className={metricsClassName}>
          {cards.map((card) => (
            <article key={card.label} className="metric-card">
              <p className="metric-label">{card.label}</p>
              <p className={card.state}>{card.value}</p>
            </article>
          ))}
        </div>
      </aside>
    );
  }

  return (
    <aside className={rootClassName}>
      <header className="status-panel__header">
        <h2>Telemetría</h2>
        <span className={telemetry.alarm === "NO_ALARMS" ? "alarm-badge" : "alarm-badge alarm-badge--active"}>
          {telemetry.alarm}
        </span>
      </header>

      <div className={metricsClassName}>
        {cards.map((card) => (
          <article key={card.label} className="metric-card">
            <p className="metric-label">{card.label}</p>
            <p className={card.state}>{card.value}</p>
          </article>
        ))}
      </div>

      <footer className={footerClassName}>
        <p>Puerta: {telemetry.doorOpen ? "Abierta" : "Cerrada"}</p>
        <p suppressHydrationWarning>Actualizado: {formattedTimestamp}</p>
      </footer>
    </aside>
  );
}
