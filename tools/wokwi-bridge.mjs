#!/usr/bin/env node
/**
 * Wokwi ↔ 3D Viewer Bridge
 *
 * Connects to Wokwi's RFC2217 serial port (TCP) and forwards
 * telemetry to the Next.js viewer via WebSocket.
 *
 * Usage:
 *   node tools/wokwi-bridge.mjs [--serial-port 4000] [--ws-port 8081]
 *
 * The firmware may output data in different formats:
 *   1) JSON lines: {"temp":34.5,"hum":60,...}
 *   2) Key=Value lines: TEMP=34.5 HUM=60 ...
 *   3) CSV: 34.5,36.1,60,1200,42,...
 *
 * The bridge auto-detects the format and maps to telemetry fields.
 */

import { createServer } from "net";
import { connect } from "net";
import { WebSocketServer } from "ws";

const SERIAL_HOST = "127.0.0.1";
const SERIAL_PORT = parseInt(
  process.env.WOKWI_SERIAL_PORT ??
  process.argv.find((_, i, a) => a[i - 1] === "--serial-port") ??
  "4000"
);
const WS_PORT = parseInt(
  process.env.WOKWI_WS_PORT ??
  process.argv.find((_, i, a) => a[i - 1] === "--ws-port") ??
  "8081"
);

// --- WebSocket server (broadcast to all viewer clients) ---
const wss = new WebSocketServer({ port: WS_PORT });
const clients = new Set();

wss.on("connection", (ws) => {
  clients.add(ws);
  console.log(`[WS] Client connected (${clients.size} total)`);
  ws.on("close", () => {
    clients.delete(ws);
    console.log(`[WS] Client disconnected (${clients.size} total)`);
  });
});

function broadcast(data) {
  const msg = JSON.stringify(data);
  for (const ws of clients) {
    if (ws.readyState === 1) ws.send(msg);
  }
}

console.log(`[WS] Server listening on ws://localhost:${WS_PORT}`);

// --- Field mapping from various firmware output names ---
const FIELD_MAP = {
  // JSON keys the firmware might use
  temp: "chamberTemp", temperature: "chamberTemp", chamber_temp: "chamberTemp",
  air_temp: "chamberTemp", chambertemp: "chamberTemp", t_air: "chamberTemp",
  skin: "skinTemp", skin_temp: "skinTemp", skintemp: "skinTemp", t_skin: "skinTemp",
  hum: "humidity", humidity: "humidity", rh: "humidity",
  fan: "fanRpm", fan_rpm: "fanRpm", fanrpm: "fanRpm",
  heater: "heaterDuty", heater_duty: "heaterDuty", heaterduty: "heaterDuty",
  photo: "phototherapyDuty", phototherapy: "phototherapyDuty",
  door: "doorOpen", door_open: "doorOpen",
  alarm: "alarm", alarms: "alarm",
};

function mapFields(raw) {
  const mapped = {};
  for (const [key, value] of Object.entries(raw)) {
    const normalized = key.toLowerCase().replace(/[^a-z0-9_]/g, "");
    const target = FIELD_MAP[normalized];
    if (target) {
      if (target === "doorOpen") {
        mapped[target] = value === true || value === 1 || value === "1" || value === "open";
      } else if (target === "alarm") {
        const ALARM_NAMES = ["NO_ALARMS", "TEMPERATURE_ALARM", "HUMIDITY_ALARM", "FAN_ISSUE_ALARM", "HEATER_ALARM", "POWER_ALARM"];
        const code = parseInt(value);
        mapped[target] = (!isNaN(code) && ALARM_NAMES[code]) ? ALARM_NAMES[code] : String(value);
      } else {
        mapped[target] = parseFloat(value) || 0;
      }
    }
  }
  mapped.timestampMs = Date.now();
  return mapped;
}

function tryParseJSON(line) {
  try {
    const obj = JSON.parse(line);
    if (typeof obj === "object" && obj !== null) return mapFields(obj);
  } catch { /* not JSON */ }
  return null;
}

function tryParseKeyValue(line) {
  // e.g. TEMP=34.5 HUM=60 FAN=1200
  const pairs = line.match(/(\w+)\s*[=:]\s*([\d.]+|true|false|open|closed|\w+)/gi);
  if (pairs && pairs.length >= 2) {
    const obj = {};
    for (const pair of pairs) {
      const [k, v] = pair.split(/\s*[=:]\s*/);
      obj[k] = v;
    }
    return mapFields(obj);
  }
  return null;
}

function tryParseCSV(line) {
  // Assume order: chamberTemp, skinTemp, humidity, fanRpm, heaterDuty, doorOpen, alarm
  const parts = line.split(",").map((s) => s.trim());
  if (parts.length >= 5 && parts.every((p) => !isNaN(parseFloat(p)) || p === "true" || p === "false")) {
    return {
      chamberTemp: parseFloat(parts[0]) || 0,
      skinTemp: parseFloat(parts[1]) || 0,
      humidity: parseFloat(parts[2]) || 0,
      fanRpm: parseFloat(parts[3]) || 0,
      heaterDuty: parseFloat(parts[4]) || 0,
      doorOpen: parts[5] === "1" || parts[5] === "true",
      alarm: parts[6] || "NO_ALARMS",
      timestampMs: Date.now(),
    };
  }
  return null;
}

function parseLine(line) {
  const trimmed = line.trim();
  if (!trimmed) return null;
  return tryParseJSON(trimmed) || tryParseKeyValue(trimmed) || tryParseCSV(trimmed);
}

// --- TCP connection to Wokwi RFC2217 serial port ---
let buffer = "";
let reconnectTimer = null;

function connectSerial() {
  console.log(`[Serial] Connecting to ${SERIAL_HOST}:${SERIAL_PORT}...`);

  const socket = connect({ host: SERIAL_HOST, port: SERIAL_PORT }, () => {
    console.log(`[Serial] Connected to Wokwi serial on port ${SERIAL_PORT}`);
    buffer = "";
  });

  socket.setEncoding("utf8");

  socket.on("data", (chunk) => {
    buffer += chunk;
    const lines = buffer.split("\n");
    buffer = lines.pop() ?? "";
    for (const line of lines) {
      const telemetry = parseLine(line);
      if (telemetry && Object.keys(telemetry).length > 1) {
        console.log(`[Telemetry]`, telemetry);
        broadcast(telemetry);
      }
    }
  });

  socket.on("error", (err) => {
    console.log(`[Serial] Error: ${err.message}`);
  });

  socket.on("close", () => {
    console.log("[Serial] Connection closed. Reconnecting in 3s...");
    if (reconnectTimer) clearTimeout(reconnectTimer);
    reconnectTimer = setTimeout(connectSerial, 3000);
  });
}

connectSerial();

// Graceful shutdown
process.on("SIGINT", () => {
  console.log("\n[Bridge] Shutting down...");
  wss.close();
  process.exit(0);
});
