# viewer-3d

Aplicación Next.js para visualizar el entorno 3D de la incubadora y telemetría del simulador.

## Ejecutar

```bash
cd viewer-3d
npm install
npm run dev
```

## Modelo 3D

Coloca un archivo `incubator.glb` en:

`viewer-3d/public/models/incubator.glb`

Si no existe, el visor usa un modelo virtual de respaldo.

Para generarlo desde el STEP oficial:

```bash
cd ..
python3 -m pip install --user cadquery trimesh numpy
python3 tools/convert-step-to-glb.py \
  --input Incunest_v15/Mechanical/IN3_structure_v15.step \
  --output viewer-3d/public/models/incubator.glb
```

## Integración con Wokwi

1. Ejecuta `examples/full-incubator-demo` con Wokwi VS Code.
2. Habilita `rfc2217ServerPort = 4000` en `wokwi.toml`.
3. Sustituye el hook `useSimulatedTelemetry` por lectura real del puerto RFC2217/WebSocket.
