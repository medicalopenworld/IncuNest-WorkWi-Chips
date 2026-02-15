"use client";

import { Suspense, useCallback, useMemo, useRef, useState } from "react";
import { Canvas, ThreeEvent, useFrame } from "@react-three/fiber";
import { CameraControls, Center, ContactShadows, Environment, Html, useGLTF } from "@react-three/drei";
import { Group, Mesh, MeshStandardMaterial, Color } from "three";
import type CameraControlsImpl from "camera-controls";

const MODEL_BASE_ROTATION_X = -Math.PI / 2;
const MODEL_SCALE = 0.01;

/* ── Labels ─────────────────────────────────────────────────── */

const PART_LABELS: Record<string, string> = {
  "Structure__PE-300_10mm_base": "Base inferior (PE-300 10mm)",
  "Structure__PE-300_10mm_shelf": "Bandeja/cama interior (extraer tras colchón)",
  "Structure__PE-300_10mm_side_R": "Pared lateral derecha (PE-300 10mm)",
  "Structure__PE-300_10mm_side_L": "Pared lateral izquierda (PE-300 10mm)",
  "Structure__PE-300_10mm_side_R_inner": "Pared interior derecha (PE-300 10mm)",
  "Structure__PE-300_10mm_side_L_inner": "Pared interior izquierda (PE-300 10mm)",
  "Structure__PE-300_10mm_wall_front_47": "Pared frontal inferior (PE-300 10mm)",
  "Structure__PE-300_10mm_wall_front_121": "Pared frontal superior (PE-300 10mm)",
  "Structure__PE-300_10mm_wall_back_80": "Pared trasera inferior (PE-300 10mm)",
  "Structure__PE-300_10mm_wall_back_177": "Pared trasera superior (PE-300 10mm)",
  "Structure__PE-300_10mm_ledge_back_385": "Reborde superior trasero (PE-300 10mm)",
  "Structure__PE-300_10mm_ledge_front_385": "Reborde superior frontal (PE-300 10mm)",
  Structure__Methacrylate: "Metacrilato (paredes transparentes)",
  "Structure__PE-300_5mm_1": "Panel PE-300 5mm (superior trasero, fijo)",
  "Structure__PE-300_5mm_2": "Panel PE-300 5mm (trasero)",
  "Structure__PE-300_5mm_3": "Panel PE-300 5mm (superior frontal, abatible)",
  "Structure__PE-300_5mm_4": "Panel PE-300 5mm (tapa frontal con pomo)",
  "Structure__PETG_08mm_1": "Lámina PETG 0.8mm (pieza 1)",
  "Structure__PETG_08mm_2": "Lámina PETG 0.8mm (pieza 2)",
  Structure__Threaded_Rods: "Varillas roscadas",
  Structure__Fasteners: "Tornillería estructural",
  Bottle_v4_v1_v1: "Botella de agua (pulsar para extraer)",
  "3D_Printing_AND_Electronics__humidifier_1A_v18": "Humidificador",
  "3D_Printing_AND_Electronics__Distanciador_cama_v3_v1": "Distanciador cama",
  "3D_Printing_AND_Electronics__PomoV3_v1": "Pomo (tirar para abrir tapa)",
  "3D_Printing_AND_Electronics__Window_door_v1_1": "Compuerta circular 1 (panel)",
  "3D_Printing_AND_Electronics__Window_door_v1_1_Handle_v1": "Pomo compuerta 1",
  "3D_Printing_AND_Electronics__Window_door_v1_1_Anillo_v5_v1": "Anillo compuerta 1",
  "3D_Printing_AND_Electronics__Window_door_v1_2": "Compuerta circular 2 (panel)",
  "3D_Printing_AND_Electronics__Window_door_v1_2_Handle_v1": "Pomo compuerta 2",
  "3D_Printing_AND_Electronics__Window_door_v1_2_Anillo_v5_v1": "Anillo compuerta 2",
  "3D_Printing_AND_Electronics__Window_door_v1_3": "Compuerta circular 3 (panel)",
  "3D_Printing_AND_Electronics__Window_door_v1_3_Handle_v1": "Pomo compuerta 3",
  "3D_Printing_AND_Electronics__Window_door_v1_3_Anillo_v5_v1": "Anillo compuerta 3",
  "3D_Printing_AND_Electronics__Window_door_v1_4": "Compuerta circular 4 (panel)",
  "3D_Printing_AND_Electronics__Window_door_v1_4_Handle_v1": "Pomo compuerta 4",
  "3D_Printing_AND_Electronics__Window_door_v1_4_Anillo_v5_v1": "Anillo compuerta 4",
  "3D_Printing_AND_Electronics__Tobera_calefactor_v1_v1": "Tobera calefactor",
  "3D_Printing_AND_Electronics__TapaCablesV4_v10": "Tapa de cables",
  "3D_Printing_AND_Electronics__PSU_Holder_v3": "Soporte fuente alimentación",
  "3D_Printing_AND_Electronics__Sensor_holder_v3": "Soporte sensores",
  "3D_Printing_AND_Electronics__Electronic_support_v23_v41": "Placa electrónica (MotherBoard)",
  "3D_Printing_AND_Electronics__Window_blocker": "Bloqueador ventana",
  "3D_Printing_AND_Electronics__Cables_Entry_Nut_v1": "Tuerca entrada cables",
  "3D_Printing_AND_Electronics__Cables_Entry_v1": "Entrada cables",
  "3D_Printing_AND_Electronics__SOLID": "Elemento sólido 3D",
  Matress: "Colchón",
  "CrowPanel_7.0_Acrylic_Case_v14_v3": "CrowPanel 7.0\" (Display)",
  "USB-C_Adapter_90_Degree_Sideways_v1": "Adaptador USB-C 90°",
  IncuNest_Assembly: "Ensamblaje completo",
};

function getLabel(name: string): string {
  for (const [key, label] of Object.entries(PART_LABELS)) {
    if (name.includes(key) || name.replace(/-/g, "_").includes(key)) return label;
  }
  return name.replace(/__/g, " / ").replace(/_/g, " ");
}

/* ── Colors ─────────────────────────────────────────────────── */

const COLOR_HOVER = new Color("#55ffcc");
const COLOR_SELECTED = new Color("#ffd966");
const COLOR_DEFAULT_TEAL = new Color(0.7, 0.85, 0.82);
const COLOR_DOOR_WHITE = new Color(0.95, 0.95, 0.95);
const COLOR_GLASS_GRAY = new Color(0.75, 0.78, 0.80);
const COLOR_POMO_RED = new Color(0.78, 0.22, 0.18);
const COLOR_MATTRESS = new Color(0.85, 0.82, 0.72);     // beige/cream
const COLOR_ELECTRONICS = new Color(0.15, 0.45, 0.25);  // PCB green
const COLOR_METAL = new Color(0.72, 0.72, 0.72);        // metallic gray
const COLOR_PLASTIC_3D = new Color(0.88, 0.86, 0.80);   // light beige 3D print
const COLOR_BOTTLE = new Color(0.6, 0.75, 0.9);         // light blue

function isPomo(n: string) { return n.includes("PomoV3") || n.includes("Handle_v1"); }
function isDoor(n: string) { return (n.includes("Window_door") || n.includes("Anillo")) && !n.includes("Handle_v1"); }
function isPanel(n: string) { return n.includes("PE-300") || n.includes("PE_300"); }
function isGlass(n: string) { return n.includes("Methacrylate"); }
function isPETG(n: string) { return n.includes("PETG"); }

function getBaseColor(name: string): Color {
  if (isPomo(name)) return COLOR_POMO_RED;
  if (isDoor(name) || isPanel(name)) return COLOR_DOOR_WHITE;
  if (isGlass(name)) return COLOR_GLASS_GRAY;
  if (isPETG(name)) return COLOR_GLASS_GRAY;
  if (name.includes("Matress")) return COLOR_MATTRESS;
  if (name.includes("PE-300_10mm_shelf")) return new Color(0.85, 0.75, 0.55); // warm wood tone for shelf
  if (name.includes("Electronic_support")) return COLOR_ELECTRONICS;
  if (name.includes("Bottle")) return COLOR_BOTTLE;
  if (name.includes("Fasteners") || name.includes("Threaded_Rods")) return COLOR_METAL;
  if (name.includes("USB-C") || name.includes("CrowPanel") || name.includes("Cables_Entry") || name.includes("IEC")) return COLOR_METAL;
  if (name.includes("Distanciador") || name.includes("Sensor_holder") || name.includes("Tobera") || name.includes("TapaCables") || name.includes("PSU_Holder") || name.includes("Window_blocker") || name.includes("humidifier")) return COLOR_PLASTIC_3D;
  return COLOR_DEFAULT_TEAL;
}
function getBaseOpacity(name: string): number {
  if (isGlass(name)) return 0.3;
  if (isPETG(name)) return 0.4;
  if (name.includes("Bottle")) return 0.7;
  return 1.0;
}
function isSolid(name: string): boolean {
  return !isGlass(name) && !isPETG(name) && !name.includes("Bottle");
}

/* ── Kinematic groups ──────────────────────────────────────── */
// Parts are in STEP coordinates (mm). After MODEL_SCALE and -90° X rotation:
//   STEP X → Three X, STEP Y → Three -Z, STEP Z → Three Y
//
// The lid assembly consists of:
//   - PE-300 5mm _3 (top panel FRONT half — abatible, rotates around hinge)
//   - PE-300 5mm _4 (front panel/tapa vertical, with pomo)
//   - PETG 0.8mm_1 (transparent sheet connecting top to front)
//   - PomoV3 (handle on front panel)
//   - Sensor_holder (mounted inside)
//
// PE-300 5mm _1 is the BACK half of the top panel and stays FIXED.
//
// The hinge is at the joint between _1 (back, fixed) and _3 (front, abatible):
//   STEP coords X≈207, Y≈273, Z≈394

const LID_GROUP_NAMES = new Set([
  "Structure__PE-300_5mm_3",
  "Structure__PE-300_5mm_4",
  "Structure__PETG_08mm_1",
  "3D_Printing_AND_Electronics__PomoV3_v1",
]);

// Pieces that trigger lid movement when clicked in "move" mode.
// Only the pomo acts as a handle — other lid parts move but don't trigger.
const LID_TRIGGER_NAMES = new Set([
  "3D_Printing_AND_Electronics__PomoV3_v1",
]);

/* ── Door (compuerta) kinematic config ─────────────────────── */
// Each door assembly has 3 pieces: Anillo (ring, stays fixed), SOLID (door panel),
// Handle (pomo, red). Only SOLID + Handle rotate; Anillo is static.
//
// The hinge is at the Y-edge OPPOSITE to the handle (farthest from handle).
// The door swings outward around the STEP Z axis at the hinge point.
//
// Door 1 (right-back):  panel Y[353-525], handle Y≈383 → hinge at Y≈525, swings back
// Door 2 (right-front): panel Y[117-289], handle Y≈259 → hinge at Y≈117, swings front
// Door 3 (left-front):  panel Y[117-289], handle Y≈259 → hinge at Y≈117, swings front
// Door 4 (left-back):   panel Y[353-525], handle Y≈383 → hinge at Y≈525, swings back
interface DoorConfig {
  body: string;    // door panel mesh name (SOLID, rotates)
  handle: string;  // handle mesh name (trigger + red, rotates with body)
  pivotX: number;  // hinge X in STEP mm (door inner face)
  pivotY: number;  // hinge Y in STEP mm (at hinge edge, opposite to handle)
  pivotZ: number;  // hinge Z in STEP mm (center height)
  angle: number;   // max opening angle (radians), sign = direction
}

const DOOR_CONFIGS: DoorConfig[] = [
  { body: "3D_Printing_AND_Electronics__Window_door_v1_1", handle: "3D_Printing_AND_Electronics__Window_door_v1_1_Handle_v1", pivotX: 417, pivotY: 525, pivotZ: 250, angle:  (90 * Math.PI) / 180 },
  { body: "3D_Printing_AND_Electronics__Window_door_v1_2", handle: "3D_Printing_AND_Electronics__Window_door_v1_2_Handle_v1", pivotX: 417, pivotY: 117, pivotZ: 250, angle: -(90 * Math.PI) / 180 },
  { body: "3D_Printing_AND_Electronics__Window_door_v1_3", handle: "3D_Printing_AND_Electronics__Window_door_v1_3_Handle_v1", pivotX: 5,   pivotY: 117, pivotZ: 250, angle:  (90 * Math.PI) / 180 },
  { body: "3D_Printing_AND_Electronics__Window_door_v1_4", handle: "3D_Printing_AND_Electronics__Window_door_v1_4_Handle_v1", pivotX: 5,   pivotY: 525, pivotZ: 250, angle: -(90 * Math.PI) / 180 },
];

// Door meshes that rotate (body + handle). Anillo stays static.
const DOOR_NAMES = new Set(DOOR_CONFIGS.flatMap((d) => [d.body, d.handle]));
// Only handles act as triggers in move mode
const DOOR_TRIGGER_NAMES = new Set(DOOR_CONFIGS.map((d) => d.handle));
// STEP coords: X≈207 (center), Y≈273 (back edge of _3), Z≈394 (top surface).
const PIVOT_X = 207 * MODEL_SCALE;
const PIVOT_Y = 273 * MODEL_SCALE;
const PIVOT_Z = 394 * MODEL_SCALE;

// Maximum opening angle (radians) — ~80° open
const LID_MAX_ANGLE = (-80 * Math.PI) / 180;

/* ── Slide parts (move mode, click to extract/retract) ───── */
// Parts that slide out when clicked in move mode.
// direction: axis + sign in STEP coords (y = -1 → slide toward front)
interface SlideConfig {
  name: string;
  axis: "x" | "y" | "z";
  distance: number; // mm, signed
  axis2?: "x" | "y" | "z";
  distance2?: number;
  axis3?: "x" | "y" | "z";
  distance3?: number;
}
const SLIDE_CONFIGS: SlideConfig[] = [
  { name: "Matress", axis: "x", distance: 550, axis2: "z", distance2: -106, axis3: "y", distance3: -100 },
  { name: "Bottle", axis: "y", distance: -350, axis2: "z", distance2: -50 },
  { name: "humidifier", axis: "y", distance: -350, axis2: "z", distance2: 150 },
  { name: "PE-300_10mm_shelf", axis: "y", distance: -675, axis2: "z", distance2: -106, axis3: "x", distance3: 125 },
];
// Groups: clicking a trigger also slides linked parts
const SLIDE_GROUPS: Record<string, string[]> = {
  Bottle: ["humidifier"],
};
// Sequential dependencies: key can only be toggled if all prerequisites are met
// lid = lid is open, Bottle/Matress/shelf = that slide is out
const SLIDE_PREREQUISITES: Record<string, string[]> = {
  Bottle: ["lid"],
  Matress: ["lid", "Bottle"],
  "PE-300_10mm_shelf": ["lid", "Bottle", "Matress"],
};
const SLIDE_TRIGGER_NAMES = new Set(SLIDE_CONFIGS.map((s) => s.name));

// All movable triggers in move mode
function isMoveTrigger(name: string): boolean {
  return LID_TRIGGER_NAMES.has(name)
    || DOOR_TRIGGER_NAMES.has(name)
    || SLIDE_CONFIGS.some((s) => name.includes(s.name));
}

type InteractionMode = "select" | "move";

/* ── PartsMesh ─────────────────────────────────────────────── */

function PartsMesh({
  mode,
  selected,
  onSelect,
  onHover,
  lidAngle,
  onLidToggle,
  doorAngles,
  onDoorToggle,
  slidesOut,
  onSlideToggle,
}: {
  mode: InteractionMode;
  selected: string | null;
  onSelect: (name: string | null) => void;
  onHover: (name: string | null) => void;
  lidAngle: number;
  onLidToggle: () => void;
  doorAngles: number[];
  onDoorToggle: (index: number) => void;
  slidesOut: Set<string>;
  onSlideToggle: (name: string) => void;
}) {
  const { scene } = useGLTF("/models/incubator-parts.glb");
  const [hovered, setHovered] = useState<string | null>(null);
  const lidGroupRef = useRef<Group>(null);
  const staticGroupRef = useRef<Group>(null);
  const doorGroupRefs = useRef<(Group | null)[]>(DOOR_CONFIGS.map(() => null));

  // Separate meshes into lid-group, doors (body+handle per door), and static
  const { lidMeshes, doorGroups, staticMeshes } = useMemo(() => {
    const clone = scene.clone(true);
    const lid: Mesh[] = [];
    const doorMap: Map<string, Mesh> = new Map();
    const stat: Mesh[] = [];
    clone.traverse((node) => {
      if (!(node instanceof Mesh)) return;
      const name = node.name;
      const solid = isSolid(name);
      const glass = isGlass(name);
      const mat = new MeshStandardMaterial({
        color: getBaseColor(name).clone(),
        transparent: !solid,
        opacity: getBaseOpacity(name),
        roughness: glass ? 0.1 : solid ? 0.35 : 0.4,
        metalness: glass ? 0.08 : solid ? 0.02 : 0.05,
        depthWrite: solid,
      });
      mat.userData.nodeName = name;
      node.material = mat;

      if (LID_GROUP_NAMES.has(name)) {
        lid.push(node);
      } else if (DOOR_NAMES.has(name)) {
        doorMap.set(name, node);
      } else {
        stat.push(node);
      }
    });
    // Group door meshes: [body, handle] per DOOR_CONFIG entry
    const groups = DOOR_CONFIGS.map((cfg) => {
      const parts: Mesh[] = [];
      const body = doorMap.get(cfg.body);
      const handle = doorMap.get(cfg.handle);
      if (body) parts.push(body);
      if (handle) parts.push(handle);
      return parts;
    });
    return { lidMeshes: lid, doorGroups: groups, staticMeshes: stat };
  }, [scene]);

  // Update materials for hover/select
  const allMeshes = useMemo(() => {
    const doorFlat = doorGroups.flat();
    return [...lidMeshes, ...doorFlat, ...staticMeshes];
  }, [lidMeshes, doorGroups, staticMeshes]);
  useMemo(() => {
    for (const mesh of allMeshes) {
      const mat = mesh.material as MeshStandardMaterial;
      const name = mat.userData.nodeName as string;
      if (name === selected) {
        mat.color.copy(COLOR_SELECTED);
        mat.opacity = 0.85;
      } else if (name === hovered) {
        mat.color.copy(COLOR_HOVER);
        mat.opacity = 0.72;
      } else {
        mat.color.copy(getBaseColor(name));
        mat.opacity = getBaseOpacity(name);
      }
      mat.needsUpdate = true;
    }
  }, [allMeshes, selected, hovered]);

  // Animate lid angle smoothly
  const currentAngle = useRef(0);
  const currentDoorAngles = useRef(DOOR_CONFIGS.map(() => 0));

  useFrame((_, delta) => {
    // Lid
    if (lidGroupRef.current) {
      const target = lidAngle;
      const diff = target - currentAngle.current;
      if (Math.abs(diff) < 0.001) {
        currentAngle.current = target;
      } else {
        currentAngle.current += diff * Math.min(1, delta * 5);
      }
      lidGroupRef.current.rotation.set(currentAngle.current, 0, 0);
    }
    // Doors — each rotates around its own Z axis
    for (let i = 0; i < DOOR_CONFIGS.length; i++) {
      const ref = doorGroupRefs.current[i];
      if (!ref) continue;
      const target = doorAngles[i];
      const cur = currentDoorAngles.current[i];
      const diff = target - cur;
      if (Math.abs(diff) < 0.001) {
        currentDoorAngles.current[i] = target;
      } else {
        currentDoorAngles.current[i] = cur + diff * Math.min(1, delta * 5);
      }
      ref.rotation.set(0, 0, currentDoorAngles.current[i]);
    }
    // Slides: animate static meshes that are toggled out
    for (const mesh of staticMeshes) {
      const name = mesh.name;
      const cfg = SLIDE_CONFIGS.find((s) => name.includes(s.name));
      if (!cfg) continue;
      const isOut = slidesOut.has(cfg.name);
      const target = isOut ? cfg.distance : 0;
      const cur = mesh.position[cfg.axis] as number;
      const diff = target - cur;
      if (Math.abs(diff) > 0.1) {
        mesh.position[cfg.axis] = cur + diff * Math.min(1, delta * 5);
      } else {
        mesh.position[cfg.axis] = target;
      }
      if (cfg.axis2 && cfg.distance2 != null) {
        const t2 = isOut ? cfg.distance2 : 0;
        const c2 = mesh.position[cfg.axis2] as number;
        const d2 = t2 - c2;
        if (Math.abs(d2) > 0.1) {
          mesh.position[cfg.axis2] = c2 + d2 * Math.min(1, delta * 5);
        } else {
          mesh.position[cfg.axis2] = t2;
        }
      }
      if (cfg.axis3 && cfg.distance3 != null) {
        const t3 = isOut ? cfg.distance3 : 0;
        const c3 = mesh.position[cfg.axis3] as number;
        const d3 = t3 - c3;
        if (Math.abs(d3) > 0.1) {
          mesh.position[cfg.axis3] = c3 + d3 * Math.min(1, delta * 5);
        } else {
          mesh.position[cfg.axis3] = t3;
        }
      }
    }
  });

  const handlePointerOver = useCallback(
    (e: ThreeEvent<PointerEvent>) => {
      e.stopPropagation();
      const name = (e.object as Mesh).name;
      if (!name || name === "IncuNest_Assembly") return;
      if (mode === "move" && !isMoveTrigger(name)) return;
      setHovered(name);
      onHover(name);
      document.body.style.cursor = mode === "move" ? "grab" : "pointer";
    },
    [onHover, mode],
  );

  const handlePointerOut = useCallback(() => {
    setHovered(null);
    onHover(null);
    document.body.style.cursor = "auto";
  }, [onHover]);

  const handleClick = useCallback(
    (e: ThreeEvent<MouseEvent>) => {
      e.stopPropagation();
      const name = (e.object as Mesh).name;
      if (!name || name === "IncuNest_Assembly") return;
      if (mode === "move") {
        if (LID_TRIGGER_NAMES.has(name)) {
          onLidToggle();
          return;
        }
        if (DOOR_TRIGGER_NAMES.has(name)) {
          const idx = DOOR_CONFIGS.findIndex((d) => d.handle === name);
          if (idx >= 0) onDoorToggle(idx);
          return;
        }
        // Slide parts (e.g. mattress)
        if (SLIDE_CONFIGS.some((s) => name.includes(s.name))) {
          onSlideToggle(name);
          return;
        }
      } else if (mode === "select") {
        onSelect(selected === name ? null : name);
      }
    },
    [onSelect, selected, mode, onLidToggle, onDoorToggle, onSlideToggle],
  );

  return (
    <Center>
      <group scale={MODEL_SCALE} rotation={[MODEL_BASE_ROTATION_X, 0, 0]}>
        {/* Static parts */}
        <group
          ref={staticGroupRef}
          onPointerOver={handlePointerOver}
          onPointerOut={handlePointerOut}
          onClick={handleClick}
        >
          {staticMeshes.map((m) => (
            <primitive key={m.uuid} object={m} />
          ))}
        </group>

        {/* Lid group — translate to pivot, rotate X, translate back */}
        <group position={[PIVOT_X / MODEL_SCALE, PIVOT_Y / MODEL_SCALE, PIVOT_Z / MODEL_SCALE]}>
          <group
            ref={lidGroupRef}
            onPointerOver={handlePointerOver}
            onPointerOut={handlePointerOut}
            onClick={handleClick}
          >
            <group position={[-PIVOT_X / MODEL_SCALE, -PIVOT_Y / MODEL_SCALE, -PIVOT_Z / MODEL_SCALE]}>
              {lidMeshes.map((m) => (
                <primitive key={m.uuid} object={m} />
              ))}
            </group>
          </group>
        </group>

        {/* Door groups — each pivots around its hinge, rotating around Z */}
        {DOOR_CONFIGS.map((cfg, i) => (
          <group key={cfg.body} position={[cfg.pivotX, cfg.pivotY, cfg.pivotZ]}>
            <group
              ref={(el) => { doorGroupRefs.current[i] = el; }}
              onPointerOver={handlePointerOver}
              onPointerOut={handlePointerOut}
              onClick={handleClick}
            >
              <group position={[-cfg.pivotX, -cfg.pivotY, -cfg.pivotZ]}>
                {doorGroups[i].map((m) => (
                  <primitive key={m.uuid} object={m} />
                ))}
              </group>
            </group>
          </group>
        ))}
      </group>
    </Center>
  );
}

/* ── Scene ─────────────────────────────────────────────────── */

function PartsScene({
  controlsRef,
  mode,
  selected,
  onSelect,
  onHover,
  lidAngle,
  onLidToggle,
  doorAngles,
  onDoorToggle,
  slidesOut,
  onSlideToggle,
}: {
  controlsRef: { current: CameraControlsImpl | null };
  mode: InteractionMode;
  selected: string | null;
  onSelect: (name: string | null) => void;
  onHover: (name: string | null) => void;
  lidAngle: number;
  onLidToggle: () => void;
  doorAngles: number[];
  onDoorToggle: (index: number) => void;
  slidesOut: Set<string>;
  onSlideToggle: (name: string) => void;
}) {
  return (
    <>
      <ambientLight intensity={0.35} />
      <directionalLight position={[6, 7, 4]} intensity={1.25} color="#9ffcff" />
      <pointLight position={[0, 2.5, 0]} intensity={1.2} color="#67ffe9" />
      <Suspense
        fallback={
          <Html center>
            <div className="viewer-chip">
              <span>Cargando piezas…</span>
            </div>
          </Html>
        }
      >
        <PartsMesh
          mode={mode}
          selected={selected}
          onSelect={onSelect}
          onHover={onHover}
          lidAngle={lidAngle}
          onLidToggle={onLidToggle}
          doorAngles={doorAngles}
          onDoorToggle={onDoorToggle}
          slidesOut={slidesOut}
          onSlideToggle={onSlideToggle}
        />
      </Suspense>
      <ContactShadows position={[0, -1.05, 0]} opacity={0.45} scale={8} blur={2.2} />
      {mode === "select" && (
        <group position={[0.5, -1, -7]}>
          <axesHelper args={[2]} />
          <Html position={[2.2, 0, 0]} center style={{ color: "#ff4444", fontSize: "12px", fontWeight: 700, pointerEvents: "none" }}>X</Html>
          <Html position={[0, 2.2, 0]} center style={{ color: "#44ff44", fontSize: "12px", fontWeight: 700, pointerEvents: "none" }}>Y</Html>
          <Html position={[0, 0, 2.2]} center style={{ color: "#4488ff", fontSize: "12px", fontWeight: 700, pointerEvents: "none" }}>Z</Html>
        </group>
      )}
      <Environment preset="night" />
      <CameraControls
        ref={controlsRef}
        makeDefault
        minDistance={0.7}
        maxDistance={40}
        minPolarAngle={0.3}
        maxPolarAngle={1.55}
        smoothTime={0.15}
      />
    </>
  );
}

/* ── Exported component ────────────────────────────────────── */

export function PartsViewer() {
  const controlsRef = useRef<CameraControlsImpl | null>(null);
  const [selected, setSelected] = useState<string | null>(null);
  const [hoveredName, setHoveredName] = useState<string | null>(null);
  const [mode, setMode] = useState<InteractionMode>("select");
  const [lidOpen, setLidOpen] = useState(false);
  const [doorsOpen, setDoorsOpen] = useState(() => DOOR_CONFIGS.map(() => false));
  const [slidesOut, setSlidesOut] = useState<Set<string>>(() => new Set());
  const [toastMsg, setToastMsg] = useState<string | null>(null);
  const toastTimer = useRef<ReturnType<typeof setTimeout> | null>(null);

  const showToast = useCallback((msg: string) => {
    setToastMsg(msg);
    if (toastTimer.current) clearTimeout(toastTimer.current);
    toastTimer.current = setTimeout(() => setToastMsg(null), 3000);
  }, []);

  const lidAngle = lidOpen ? LID_MAX_ANGLE : 0;
  const toggleLid = useCallback(() => {
    if (lidOpen) {
      // Closing lid — check if items still outside
      const order = ["PE-300_10mm_shelf", "Matress", "Bottle"];
      const stillOut = order.filter((k) => slidesOut.has(k));
      if (stillOut.length > 0) {
        const names: Record<string, string> = {
          "PE-300_10mm_shelf": "la bandeja",
          Matress: "el colchón",
          Bottle: "la botella",
        };
        const list = stillOut.map((k) => names[k]).join(", ");
        showToast(`⚠️ Primero introduce ${list} antes de cerrar la tapa`);
        return;
      }
    }
    setLidOpen((v) => !v);
  }, [lidOpen, slidesOut, showToast]);

  const doorAngles = useMemo(
    () => doorsOpen.map((open, i) => (open ? DOOR_CONFIGS[i].angle : 0)),
    [doorsOpen],
  );
  const toggleDoor = useCallback(
    (index: number) => setDoorsOpen((prev) => prev.map((v, i) => (i === index ? !v : v))),
    [],
  );
  const toggleSlide = useCallback(
    (clickedMesh: string) => {
      const clickedCfg = SLIDE_CONFIGS.find((s) => clickedMesh.includes(s.name));
      if (!clickedCfg) return;
      const triggerKey = clickedCfg.name;
      const isOut = slidesOut.has(triggerKey);
      // Check prerequisites before extracting
      if (!isOut) {
        const prereqs = SLIDE_PREREQUISITES[triggerKey] ?? [];
        for (const p of prereqs) {
          if (p === "lid" && !lidOpen) {
            showToast("⚠️ Abre la tapa primero (clic en el pomo rojo)");
            return;
          }
          if (p !== "lid" && !slidesOut.has(p)) {
            const names: Record<string, string> = {
              Bottle: "la botella",
              Matress: "el colchón",
              "PE-300_10mm_shelf": "la bandeja",
            };
            showToast(`⚠️ Primero extrae ${names[p] ?? p}`);
            return;
          }
        }
      }
      // Check reverse order before putting back (dependents must be back first)
      if (isOut) {
        const dependents = Object.entries(SLIDE_PREREQUISITES)
          .filter(([, deps]) => deps.includes(triggerKey))
          .map(([k]) => k);
        const stillOut = dependents.filter((d) => slidesOut.has(d));
        if (stillOut.length > 0) {
          const names: Record<string, string> = {
            Bottle: "la botella",
            Matress: "el colchón",
            "PE-300_10mm_shelf": "la bandeja",
          };
          const list = stillOut.map((k) => names[k] ?? k).join(", ");
          showToast(`⚠️ Primero introduce ${list}`);
          return;
        }
      }
      setSlidesOut((prev) => {
        const next = new Set(prev);
        if (isOut) next.delete(triggerKey); else next.add(triggerKey);
        const members = SLIDE_GROUPS[triggerKey] ?? [];
        for (const m of members) {
          if (isOut) next.delete(m); else next.add(m);
        }
        return next;
      });
    },
    [lidOpen, slidesOut, showToast],
  );

  const zoomIn = useCallback(() => void controlsRef.current?.dolly(1.4, false), []);
  const zoomOut = useCallback(() => void controlsRef.current?.dolly(-1.4, false), []);
  const rotateLeft = useCallback(() => void controlsRef.current?.rotate(0.2, 0, false), []);
  const rotateRight = useCallback(() => void controlsRef.current?.rotate(-0.2, 0, false), []);
  const nudgeView = useCallback((x: number, y: number) => void controlsRef.current?.truck(x, y, false), []);
  const resetView = useCallback(() => void controlsRef.current?.reset(false), []);

  const displayLabel = selected ? getLabel(selected) : hoveredName ? getLabel(hoveredName) : null;

  return (
    <div className="viewer-wrap">
      <Canvas camera={{ position: [8, 6, 8], fov: 42, near: 0.01, far: 240 }} dpr={[1, 2]}>
        <PartsScene
          controlsRef={controlsRef}
          mode={mode}
          selected={selected}
          onSelect={setSelected}
          onHover={setHoveredName}
          lidAngle={lidAngle}
          onLidToggle={toggleLid}
          doorAngles={doorAngles}
          onDoorToggle={toggleDoor}
          slidesOut={slidesOut}
          onSlideToggle={toggleSlide}
        />
      </Canvas>

      {displayLabel && (
        <div className="parts-label">
          <span>{displayLabel}</span>
        </div>
      )}

      {toastMsg && (
        <div className="viewer-toast">{toastMsg}</div>
      )}

      {/* Mode toggle */}
      <div className="viewer-mode-toggle" aria-label="Modo de interacción">
        <button
          type="button"
          className={mode === "select" ? "viewer-mode-btn viewer-mode-btn--active" : "viewer-mode-btn"}
          data-tooltip="Seleccionar piezas"
          onClick={() => setMode("select")}
        >
          🔍
        </button>
        <button
          type="button"
          className={mode === "move" ? "viewer-mode-btn viewer-mode-btn--active" : "viewer-mode-btn"}
          data-tooltip="Mover piezas (clic en pomos)"
          onClick={() => setMode("move")}
        >
          ✋
        </button>
      </div>

      <div className="viewer-corner-controls" aria-label="Controles de cámara">
        <p className="viewer-corner-controls__title">Controles</p>
        <div className="viewer-corner-controls__row viewer-corner-controls__row--three">
          <button type="button" data-tooltip="Acercar zoom" onClick={zoomIn}>+</button>
          <button type="button" data-tooltip="Restablecer vista" onClick={resetView}>⟲</button>
          <button type="button" data-tooltip="Alejar zoom" onClick={zoomOut}>−</button>
        </div>
        <div className="viewer-corner-controls__row viewer-corner-controls__row--two">
          <button type="button" data-tooltip="Girar izquierda" onClick={rotateLeft}>↺</button>
          <button type="button" data-tooltip="Girar derecha" onClick={rotateRight}>↻</button>
        </div>
        <div className="viewer-corner-controls__row viewer-corner-controls__row--four">
          <button type="button" data-tooltip="Mover izquierda" onClick={() => nudgeView(-0.45, 0)}>←</button>
          <button type="button" data-tooltip="Mover arriba" onClick={() => nudgeView(0, 0.35)}>↑</button>
          <button type="button" data-tooltip="Mover abajo" onClick={() => nudgeView(0, -0.35)}>↓</button>
          <button type="button" data-tooltip="Mover derecha" onClick={() => nudgeView(0.45, 0)}>→</button>
        </div>
      </div>
    </div>
  );
}
