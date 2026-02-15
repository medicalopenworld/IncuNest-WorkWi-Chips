"use client";

import { useState } from "react";
import { StatusPanel } from "@/components/status-panel";
import { IncubatorViewer } from "@/components/incubator-viewer";
import { PartsViewer } from "@/components/parts-viewer";

const helpSteps = [
  "Asegura el modelo en viewer-3d/public/models/incubator.glb",
  "Inicia Wokwi en examples/full-incubator-demo/",
  "Conecta telemetría real por RFC2217/WebSocket (puerto 4000) cuando quieras sustituir datos simulados"
] as const;

type ViewMode = "overview" | "parts";

export default function HomePage() {
  const [viewMode, setViewMode] = useState<ViewMode>("overview");

  return (
    <main className="page-shell page-shell--immersive">
      <header className="topbar">
        <div className="topbar__brand">
          <p className="eyebrow">IncuNest Digital Twin</p>
          <h1>Simulador 3D clínico para la incubadora virtual de IncuNest</h1>
          <p className="hero-copy">
            Explora el gemelo digital con navegación libre y monitoriza el estado térmico en tiempo real.
          </p>
        </div>

        <div className="topbar__actions">
          <nav className="view-toggle" aria-label="Modo de vista">
            <button
              type="button"
              className={viewMode === "overview" ? "view-toggle__btn view-toggle__btn--active" : "view-toggle__btn"}
              onClick={() => setViewMode("overview")}
            >
              Vista general
            </button>
            <button
              type="button"
              className={viewMode === "parts" ? "view-toggle__btn view-toggle__btn--active" : "view-toggle__btn"}
              onClick={() => setViewMode("parts")}
            >
              Piezas
            </button>
          </nav>

          <details className="help-tooltip">
            <summary aria-label="Mostrar ayuda" title="Ayuda" />
            <div className="help-tooltip__content">
              <h2>Guía rápida</h2>
              <ol>
                {helpSteps.map((step) => (
                  <li key={step}>{step}</li>
                ))}
              </ol>
              <p className="help-tooltip__note">
                Controles 3D: clic izquierdo = girar, clic derecho/arrastrar = desplazar, rueda = zoom.
              </p>
              <p className="help-tooltip__note">
                En vista &quot;Piezas&quot;, haz clic en cualquier pieza para ver su nombre y seleccionarla.
              </p>
            </div>
          </details>
        </div>
      </header>

      <StatusPanel orientation="horizontal" />

      <section className="viewer-section">
        {viewMode === "overview" ? <IncubatorViewer /> : <PartsViewer />}
      </section>
    </main>
  );
}
