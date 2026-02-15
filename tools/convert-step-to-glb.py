#!/usr/bin/env python3
"""
Convert an IncuNest STEP model to GLB for the Next.js viewer.

Usage:
  python3 tools/convert-step-to-glb.py \
    --input Incunest_v15/Mechanical/IN3_structure_v15.step \
    --output viewer-3d/public/models/incubator.glb
"""

from __future__ import annotations

import argparse
from pathlib import Path

from cadquery import exporters, importers
import trimesh


def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(description="Convert STEP file to GLB")
  parser.add_argument("--input", required=True, help="Input .step file path")
  parser.add_argument("--output", required=True, help="Output .glb file path")
  parser.add_argument(
      "--tolerance",
      type=float,
      default=2.0,
      help="STL tessellation tolerance (higher = lower poly, default: 2.0)",
  )
  parser.add_argument(
      "--angular-tolerance",
      type=float,
      default=0.5,
      help="STL angular tolerance (default: 0.5)",
  )
  parser.add_argument(
      "--keep-stl",
      action="store_true",
      help="Keep temporary STL file used during conversion",
  )
  return parser.parse_args()


def main() -> None:
  args = parse_args()
  input_path = Path(args.input).resolve()
  output_path = Path(args.output).resolve()
  output_path.parent.mkdir(parents=True, exist_ok=True)
  temp_stl = output_path.with_suffix(".stl")

  if not input_path.exists():
    raise FileNotFoundError(f"STEP file not found: {input_path}")

  print(f"[1/4] Loading STEP: {input_path}")
  shape = importers.importStep(str(input_path))

  print(f"[2/4] Exporting STL: {temp_stl}")
  exporters.export(
      shape,
      str(temp_stl),
      tolerance=args.tolerance,
      angularTolerance=args.angular_tolerance,
  )

  print("[3/4] Converting STL -> GLB")
  mesh = trimesh.load_mesh(str(temp_stl))
  if isinstance(mesh, trimesh.Scene):
    mesh = trimesh.util.concatenate(tuple(mesh.dump()))
  mesh.export(str(output_path))

  if not args.keep_stl and temp_stl.exists():
    temp_stl.unlink()

  print(f"[4/4] Done: {output_path} ({output_path.stat().st_size} bytes)")


if __name__ == "__main__":
  main()
