# IncuNest Wokwi Chips

Mono-repo para simular IncuNest en Wokwi con custom chips, ejemplos y visor 3D.

## Estructura

- `chips/`: modelos de sensores y actuadores (Wokwi Chips API en C)
- `examples/`: diagramas Wokwi listos para simular
- `viewer-3d/`: app Next.js para visualizar el ensamblaje 3D
- `SIMULATION_GUIDE.md`: guía consolidada completa

## Validación rápida

```bash
node tools/validate-configs.mjs
```

## Generación de binarios de chips

```bash
./tools/build-chips.sh
```

- Si tienes `wokwi-chip-builder`, compila los `.c` a `.chip.wasm`.
- Si no está instalado, genera placeholders `.chip.wasm` válidos para que Wokwi no falle por binario faltante.

## Generación del modelo GLB para el visor 3D

```bash
python3 -m pip install --user cadquery trimesh numpy
python3 tools/convert-step-to-glb.py \
  --input Incunest_v15/Mechanical/IN3_structure_v15.step \
  --output viewer-3d/public/models/incubator.glb
```

## Uso con VS Code + Wokwi

1. Abre `examples/full-incubator-demo/` en VS Code.
2. Instala y activa la extensión Wokwi.
3. Ejecuta `./tools/build-chips.sh` para generar/compilar `.chip.wasm`.
4. Ejecuta `Wokwi: Start Simulator`.
