"use client";

import { Suspense, useCallback, useEffect, useMemo, useRef, useState } from "react";
import { Canvas, useFrame } from "@react-three/fiber";
import { CameraControls, Center, ContactShadows, Environment, Html, useGLTF } from "@react-three/drei";
import { Group, Mesh, MeshPhysicalMaterial } from "three";
import type CameraControlsImpl from "camera-controls";

import { useIncubatorStore } from "@/stores/incubator-store";

const MODEL_BASE_ROTATION_X = -Math.PI / 2;
const MODEL_FLOAT_ROTATION_SPEED = 0.22;
const MODEL_FLOAT_ROTATION_AMPLITUDE = 0.08;
const MODEL_SCALE = 0.01;

function TwinShell({ glow }: { glow: number }) {
  const groupRef = useRef<Group>(null);
  const intensity = useMemo(() => Math.max(0.2, Math.min(1, glow / 40)), [glow]);

  useFrame((state) => {
    if (!groupRef.current) return;
    groupRef.current.rotation.y =
      Math.sin(state.clock.elapsedTime * MODEL_FLOAT_ROTATION_SPEED) * MODEL_FLOAT_ROTATION_AMPLITUDE;
  });

  return (
    <group ref={groupRef} rotation={[MODEL_BASE_ROTATION_X, 0, 0]}>
      <mesh position={[0, 0.75, 0]}>
        <boxGeometry args={[3.3, 1.7, 1.9]} />
        <meshPhysicalMaterial
          color="#4ff8d7"
          roughness={0.02}
          metalness={0.1}
          transmission={0.86}
          thickness={0.3}
          emissive="#00ffcc"
          emissiveIntensity={0.12 + intensity * 0.18}
        />
      </mesh>
      <mesh position={[0, -0.25, 0]}>
        <boxGeometry args={[3.6, 0.4, 2.2]} />
        <meshStandardMaterial color="#0f2435" metalness={0.35} roughness={0.45} />
      </mesh>
      <mesh position={[0, 0.65, 0.95]}>
        <planeGeometry args={[2.4, 0.9]} />
        <meshStandardMaterial color="#09111b" emissive="#55ffe3" emissiveIntensity={0.22} />
      </mesh>
      <mesh position={[0, 1.75, 0]} rotation={[Math.PI / 2, 0, 0]}>
        <ringGeometry args={[0.15, 0.38, 48]} />
        <meshBasicMaterial color="#ffe48f" />
      </mesh>
    </group>
  );
}

function IncubatorModelGlb() {
  const { scene } = useGLTF("/models/incubator.glb");

  const translucentScene = useMemo(() => {
    const clonedScene = scene.clone(true);
    clonedScene.traverse((node) => {
      if (!(node instanceof Mesh)) return;
      const originalMaterials = Array.isArray(node.material) ? node.material : [node.material];
      const preparedMaterials = originalMaterials.map((material) => {
        const nextMaterial =
          material && "clone" in material
            ? material.clone()
            : new MeshPhysicalMaterial({ color: "#8be9d7", roughness: 0.35, metalness: 0.05 });

        nextMaterial.transparent = true;
        nextMaterial.opacity = 0.42;
        if ("transmission" in nextMaterial) {
          nextMaterial.transmission = Math.max(nextMaterial.transmission ?? 0, 0.45);
        }
        if ("thickness" in nextMaterial) {
          nextMaterial.thickness = 0.08;
        }
        nextMaterial.depthWrite = false;
        nextMaterial.needsUpdate = true;
        return nextMaterial;
      });

      node.material = Array.isArray(node.material) ? preparedMaterials : preparedMaterials[0];
    });

    return clonedScene;
  }, [scene]);

  return (
    <Center>
      <group scale={MODEL_SCALE} rotation={[MODEL_BASE_ROTATION_X, 0, 0]}>
        <primitive object={translucentScene} />
      </group>
    </Center>
  );
}

function SceneContent({
  hasGlbModel,
  controlsRef
}: {
  hasGlbModel: boolean;
  controlsRef: { current: CameraControlsImpl | null };
}) {
  const telemetry = useIncubatorStore((s) => s.telemetry);
  const glow = telemetry.chamberTemp;

  return (
    <>
      <ambientLight intensity={0.35} />
      <directionalLight position={[6, 7, 4]} intensity={1.25} color="#9ffcff" />
      <pointLight
        position={[0, 2.5, 0]}
        intensity={1.6}
        color={telemetry.alarm === "NO_ALARMS" ? "#67ffe9" : "#ff6a7a"}
      />
      <Suspense fallback={<TwinShell glow={glow} />}>
        {hasGlbModel ? <IncubatorModelGlb /> : <TwinShell glow={glow} />}
      </Suspense>
      <ContactShadows position={[0, -1.05, 0]} opacity={0.45} scale={8} blur={2.2} />
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
      <Html position={[-1.8, 1.7, 0]} transform>
        <div className="viewer-chip">
          <span>CHAMBER</span>
          <strong>{telemetry.chamberTemp.toFixed(1)}°C</strong>
        </div>
      </Html>
      <Html position={[1.8, 1.7, 0]} transform>
        <div className="viewer-chip">
          <span>HUM</span>
          <strong>{telemetry.humidity.toFixed(0)}%</strong>
        </div>
      </Html>
    </>
  );
}

export function IncubatorViewer() {
  const [hasGlbModel, setHasGlbModel] = useState(false);
  const [checked, setChecked] = useState(false);
  const controlsRef = useRef<CameraControlsImpl | null>(null);

  const zoomIn = useCallback(() => {
    void controlsRef.current?.dolly(1.4, false);
  }, []);

  const zoomOut = useCallback(() => {
    void controlsRef.current?.dolly(-1.4, false);
  }, []);

  const rotateLeft = useCallback(() => {
    void controlsRef.current?.rotate(0.2, 0, false);
  }, []);

  const rotateRight = useCallback(() => {
    void controlsRef.current?.rotate(-0.2, 0, false);
  }, []);

  const nudgeView = useCallback((x: number, y: number) => {
    void controlsRef.current?.truck(x, y, false);
  }, []);

  const resetView = useCallback(() => {
    void controlsRef.current?.reset(false);
  }, []);

  useEffect(() => {
    let mounted = true;
    fetch("/models/incubator.glb", { method: "HEAD" })
      .then((res) => {
        if (mounted) {
          setHasGlbModel(res.ok);
          setChecked(true);
        }
      })
      .catch(() => {
        if (mounted) {
          setHasGlbModel(false);
          setChecked(true);
        }
      });

    return () => {
      mounted = false;
    };
  }, []);

  return (
    <div className="viewer-wrap">
      <Canvas camera={{ position: [8, 6, 8], fov: 42, near: 0.01, far: 240 }} dpr={[1, 2]}>
        <SceneContent hasGlbModel={hasGlbModel && checked} controlsRef={controlsRef} />
      </Canvas>

      <div className="viewer-corner-controls" aria-label="Controles de cámara">
        <p className="viewer-corner-controls__title">Controles</p>
        <div className="viewer-corner-controls__row viewer-corner-controls__row--three">
          <button type="button" title="Zoom in" aria-label="Zoom in" data-tooltip="Acercar zoom" onClick={zoomIn}>
            +
          </button>
          <button
            type="button"
            title="Reset vista"
            aria-label="Restablecer vista"
            data-tooltip="Restablecer vista"
            onClick={resetView}
          >
            ⟲
          </button>
          <button type="button" title="Zoom out" aria-label="Zoom out" data-tooltip="Alejar zoom" onClick={zoomOut}>
            −
          </button>
        </div>
        <div className="viewer-corner-controls__row viewer-corner-controls__row--two">
          <button
            type="button"
            title="Girar izquierda"
            aria-label="Girar izquierda"
            data-tooltip="Girar izquierda"
            onClick={rotateLeft}
          >
            ↺
          </button>
          <button
            type="button"
            title="Girar derecha"
            aria-label="Girar derecha"
            data-tooltip="Girar derecha"
            onClick={rotateRight}
          >
            ↻
          </button>
        </div>
        <div className="viewer-corner-controls__row viewer-corner-controls__row--four">
          <button
            type="button"
            title="Mover izquierda"
            aria-label="Mover izquierda"
            data-tooltip="Mover izquierda"
            onClick={() => nudgeView(-0.45, 0)}
          >
            ←
          </button>
          <button
            type="button"
            title="Mover arriba"
            aria-label="Mover arriba"
            data-tooltip="Mover arriba"
            onClick={() => nudgeView(0, 0.35)}
          >
            ↑
          </button>
          <button
            type="button"
            title="Mover abajo"
            aria-label="Mover abajo"
            data-tooltip="Mover abajo"
            onClick={() => nudgeView(0, -0.35)}
          >
            ↓
          </button>
          <button
            type="button"
            title="Mover derecha"
            aria-label="Mover derecha"
            data-tooltip="Mover derecha"
            onClick={() => nudgeView(0.45, 0)}
          >
            →
          </button>
        </div>
      </div>

      {!hasGlbModel && checked ? (
        <p className="viewer-note">
          Modelo GLB no encontrado en <code>viewer-3d/public/models/incubator.glb</code>. Se usa casco
          virtual de respaldo.
        </p>
      ) : null}
    </div>
  );
}
