"use client";

import { useEffect, useRef } from "react";
import { useIncubatorStore, IncubatorTelemetry, initialTelemetry } from "@/stores/incubator-store";

const WS_URL = process.env.NEXT_PUBLIC_WOKWI_WS ?? "ws://localhost:8081";
const RECONNECT_DELAY = 3000;

/**
 * Connects to the Wokwi bridge WebSocket and updates the store
 * with real telemetry data from the simulator.
 *
 * Falls back to simulated telemetry if connection fails.
 */
export function useWokwiTelemetry() {
  const setTelemetry = useIncubatorStore((s) => s.setTelemetry);
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const lastTelemetryRef = useRef<IncubatorTelemetry>(initialTelemetry);

  useEffect(() => {
    let mounted = true;

    function connect() {
      if (!mounted) return;
      const ws = new WebSocket(WS_URL);
      wsRef.current = ws;

      ws.onopen = () => {
        console.log("[Wokwi] Connected to bridge:", WS_URL);
      };

      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          // Merge partial updates with previous state
          const merged: IncubatorTelemetry = {
            ...lastTelemetryRef.current,
            ...data,
          };
          lastTelemetryRef.current = merged;
          setTelemetry(merged);
        } catch {
          // ignore non-JSON messages
        }
      };

      ws.onclose = () => {
        console.log("[Wokwi] Disconnected. Reconnecting in 3s...");
        if (mounted) {
          reconnectRef.current = setTimeout(connect, RECONNECT_DELAY);
        }
      };

      ws.onerror = () => {
        ws.close();
      };
    }

    connect();

    return () => {
      mounted = false;
      if (reconnectRef.current) clearTimeout(reconnectRef.current);
      if (wsRef.current) wsRef.current.close();
    };
  }, [setTelemetry]);
}
