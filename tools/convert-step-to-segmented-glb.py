#!/usr/bin/env python3
"""Convert STEP file to a segmented GLB with named nodes per part.

Each meaningful assembly child becomes a named node in the glTF scene,
enabling per-part selection and animation in the 3D viewer.

Transform composition: every part carries the full chain of transforms
from root → parent → ... → component, composed via TopLoc_Location.Multiplied().
"""
import argparse
import json
import struct
import sys
import tempfile
from pathlib import Path

import numpy as np

# Sub-assemblies to split into individual children (depth 1 of root).
# Others are kept as a single merged mesh.
SPLIT_ASSEMBLIES = {"Structure", "3D_Printing_AND_Electronics"}

# Within SPLIT_ASSEMBLIES, these sub-sub-assemblies are kept as single
# merged parts (too many internal components to be useful individually).
MERGE_SUB_ASSEMBLIES = {
    "Electronic_support_v23_v41",  # motherboard: 633 tiny components
    "humidifier_1A_v18",           # humidifier: 81 components
    "PSU_Holder_v3",               # PSU holder: 19 components
    "Sensor_holder_v3",            # sensor holder: 20 components
    "Tobera_calefactor_v1_v1",     # heater nozzle: 3 components
    "IEC320-C14_v1",               # IEC connector
    "env_sensor_v2_v1",            # env sensor
    "Screws",                      # screws sub-assembly in Structure
}

# Parts that should remain as individual instances (not merged by name).
# Each instance gets a numbered suffix: Window_door_v1_1, Window_door_v1_2, ...
INDIVIDUAL_INSTANCES = {"Window_door_v1", "Window_door v1"}

# Within INDIVIDUAL_INSTANCES assemblies, children matching these patterns
# become separate GLB nodes (e.g., the handle on each door). Others are merged
# into the parent body mesh.
SPLIT_CHILD_PATTERNS = {"Handle", "Anillo"}

# Parts whose shape contains multiple solids that should be split into
# separate GLB nodes. Value = proximity threshold (mm) for grouping solids.
SPLIT_BY_SOLID = {
    "PETG 0.8mm": 20.0,
    "PETG_0.8mm": 20.0,
    "PE-300 5mm": 30.0,
    "PE-300_5mm": 30.0,
    "PE-300 10mm": 1.0,   # split all 12 solids individually
    "PE-300_10mm": 1.0,
}

# Parts to group as fasteners
FASTENER_KEYWORDS = (
    "m6x", "m3x", "screw", "ecrou", "fasten", "tr fasten", "din 933",
)

# Material assignment by part name pattern → material index in GLB.
# 0 = default (teal), 1 = white opaque (doors/panels), 2 = glass (methacrylate)
def get_material_index(name: str) -> int:
    lower = name.lower()
    if "handle" in lower or "pomo" in lower:
        return 3  # reddish handle
    if "window_door" in lower:
        return 1  # white opaque
    if "pe-300" in lower or "pe_300" in lower:
        return 1  # white opaque structural panels
    if "methacrylate" in lower:
        return 2  # glass gray transparent
    return 0


def main():
    parser = argparse.ArgumentParser(description="STEP → segmented GLB")
    parser.add_argument("--input", default="Incunest_v15/Mechanical/IN3_structure_v15.step")
    parser.add_argument("--output", default="viewer-3d/public/models/incubator-parts.glb")
    parser.add_argument("--tolerance", type=float, default=2.0)
    args = parser.parse_args()

    from OCP.STEPCAFControl import STEPCAFControl_Reader
    from OCP.TDocStd import TDocStd_Document
    from OCP.TCollection import TCollection_ExtendedString
    from OCP.XCAFDoc import XCAFDoc_DocumentTool
    from OCP.TDF import TDF_LabelSequence, TDF_Label
    from OCP.TDataStd import TDataStd_Name

    print(f"Reading {args.input}...")
    doc = TDocStd_Document(TCollection_ExtendedString("STEP"))
    reader = STEPCAFControl_Reader()
    reader.SetNameMode(True)
    reader.SetColorMode(True)
    reader.ReadFile(args.input)
    reader.Transfer(doc)

    st = XCAFDoc_DocumentTool.ShapeTool_s(doc.Main())
    free_labels = TDF_LabelSequence()
    st.GetFreeShapes(free_labels)

    root_label = free_labels.Value(1)
    root_comps = TDF_LabelSequence()
    st.GetComponents_s(root_label, root_comps)

    print(f"Found {root_comps.Length()} top-level components")

    parts = []

    for i in range(1, root_comps.Length() + 1):
        comp = root_comps.Value(i)
        ref = TDF_Label()
        if st.GetReferredShape_s(comp, ref):
            name = get_label_name(st, ref)
        else:
            name = get_label_name(st, comp)
            ref = comp

        shape = st.GetShape_s(comp)
        if shape.IsNull():
            print(f"  Skipping {name}: null shape")
            continue

        is_asm = st.IsAssembly_s(ref)

        if is_asm and name in SPLIT_ASSEMBLIES:
            # Split into individual children with correct transforms
            parent_loc = st.GetLocation_s(comp)
            sub_parts = extract_split_assembly(st, ref, name, parent_loc, args.tolerance)
            parts.extend(sub_parts)
        else:
            # Keep as single merged part (applies to Bottle, Matress, CrowPanel, USB-C, etc.)
            mesh_data = tessellate_shape(shape, args.tolerance)
            if mesh_data and len(mesh_data[0]) > 0:
                parts.append({"name": sanitize_name(name), "vertices": mesh_data[0], "indices": mesh_data[1]})
                print(f"  Part: {name} → {len(mesh_data[0])//3} vertices")

    print(f"\nTotal parts: {len(parts)}")
    write_glb(parts, args.output)
    size_mb = Path(args.output).stat().st_size / 1024 / 1024
    print(f"Written {args.output} ({size_mb:.1f} MB, {len(parts)} nodes)")


def get_label_name(st, label):
    """Get the name attribute from an XDE label."""
    from OCP.TDataStd import TDataStd_Name
    name_attr = TDataStd_Name()
    if label.FindAttribute(TDataStd_Name.GetID_s(), name_attr):
        return name_attr.Get().ToExtString()
    return "unnamed"


def collect_all_shapes(st, ref_label, world_loc):
    """Recursively collect all leaf shapes under an assembly with composed transforms."""
    from OCP.TDF import TDF_LabelSequence, TDF_Label

    sub_comps = TDF_LabelSequence()
    st.GetComponents_s(ref_label, sub_comps)
    shapes = []

    for i in range(1, sub_comps.Length() + 1):
        sub = sub_comps.Value(i)
        sub_ref = TDF_Label()
        if st.GetReferredShape_s(sub, sub_ref):
            pass
        else:
            sub_ref = sub

        sub_loc = st.GetLocation_s(sub)
        child_world_loc = world_loc.Multiplied(sub_loc)

        if st.IsAssembly_s(sub_ref):
            shapes.extend(collect_all_shapes(st, sub_ref, child_world_loc))
        else:
            shape = st.GetShape_s(sub_ref)
            if shape is not None and not shape.IsNull():
                shapes.append(shape.Located(child_world_loc))

    return shapes


def merge_shapes_to_mesh(shapes, tolerance):
    """Merge multiple shapes into a single tessellated mesh."""
    from OCP.BRep import BRep_Builder
    from OCP.TopoDS import TopoDS_Compound

    if not shapes:
        return None

    builder = BRep_Builder()
    compound = TopoDS_Compound()
    builder.MakeCompound(compound)
    for s in shapes:
        builder.Add(compound, s)
    return tessellate_shape(compound, tolerance)


def extract_split_assembly(st, ref_label, parent_name, parent_world_loc, tolerance):
    """Extract children of an assembly as separate named parts.

    - Children whose name matches MERGE_SUB_ASSEMBLIES are merged into one part.
    - Fasteners are grouped together.
    - Others become individual parts (with transforms correctly composed).
    - Duplicate-named parts (same prototype placed multiple times) are merged
      into a single node to keep node count manageable.
    """
    from OCP.TDF import TDF_LabelSequence, TDF_Label
    from collections import defaultdict

    sub_comps = TDF_LabelSequence()
    st.GetComponents_s(ref_label, sub_comps)

    parts = []
    fastener_shapes = []
    # Group same-named parts together (e.g., 4x Window_door)
    named_groups = defaultdict(list)
    # Counter for parts that must stay individual
    instance_counters = defaultdict(int)

    for i in range(1, sub_comps.Length() + 1):
        sub = sub_comps.Value(i)
        sub_ref = TDF_Label()
        if st.GetReferredShape_s(sub, sub_ref):
            name = get_label_name(st, sub_ref)
        else:
            name = get_label_name(st, sub)
            sub_ref = sub

        sub_loc = st.GetLocation_s(sub)
        world_loc = parent_world_loc.Multiplied(sub_loc)
        full_name = f"{parent_name}/{name}"

        # Check if fastener
        lower_name = name.lower()
        if any(k in lower_name for k in FASTENER_KEYWORDS):
            shape = st.GetShape_s(sub_ref)
            if shape is not None and not shape.IsNull():
                fastener_shapes.append(shape.Located(world_loc))
            continue

        # Check if this part should be kept as individual instances
        name_key = name.replace(" ", "_")
        if name_key in INDIVIDUAL_INSTANCES or name in INDIVIDUAL_INSTANCES:
            instance_counters[name] += 1
            inst_num = instance_counters[name]
            inst_name = f"{full_name}_{inst_num}"

            if st.IsAssembly_s(sub_ref) and SPLIT_CHILD_PATTERNS:
                # Split children: matching patterns become separate nodes,
                # others merge into the body mesh.
                child_comps = TDF_LabelSequence()
                st.GetComponents_s(sub_ref, child_comps)
                body_shapes = []
                for ci in range(1, child_comps.Length() + 1):
                    ch = child_comps.Value(ci)
                    ch_ref = TDF_Label()
                    if st.GetReferredShape_s(ch, ch_ref):
                        ch_name = get_label_name(st, ch_ref)
                    else:
                        ch_name = get_label_name(st, ch)
                        ch_ref = ch
                    ch_loc = st.GetLocation_s(ch)
                    ch_world = world_loc.Multiplied(ch_loc)

                    matched = any(pat.lower() in ch_name.lower() for pat in SPLIT_CHILD_PATTERNS)
                    if matched:
                        # Separate node for this child
                        if st.IsAssembly_s(ch_ref):
                            ch_shapes = collect_all_shapes(st, ch_ref, ch_world)
                        else:
                            s = st.GetShape_s(ch_ref)
                            ch_shapes = [s.Located(ch_world)] if s and not s.IsNull() else []
                        ch_mesh = merge_shapes_to_mesh(ch_shapes, tolerance)
                        if ch_mesh and len(ch_mesh[0]) > 0:
                            ch_node_name = sanitize_name(f"{inst_name}_{ch_name.replace(' ', '_')}")
                            parts.append({"name": ch_node_name, "vertices": ch_mesh[0], "indices": ch_mesh[1]})
                            print(f"  Part: {inst_name}/{ch_name} → {len(ch_mesh[0])//3} vertices")
                    else:
                        # Merge into body
                        if st.IsAssembly_s(ch_ref):
                            body_shapes.extend(collect_all_shapes(st, ch_ref, ch_world))
                        else:
                            s = st.GetShape_s(ch_ref)
                            if s and not s.IsNull():
                                body_shapes.append(s.Located(ch_world))

                body_mesh = merge_shapes_to_mesh(body_shapes, tolerance)
                if body_mesh and len(body_mesh[0]) > 0:
                    parts.append({"name": sanitize_name(inst_name), "vertices": body_mesh[0], "indices": body_mesh[1]})
                    print(f"  Part: {inst_name} (body) → {len(body_mesh[0])//3} vertices")
            else:
                if st.IsAssembly_s(sub_ref):
                    all_shapes = collect_all_shapes(st, sub_ref, world_loc)
                else:
                    shape = st.GetShape_s(sub_ref)
                    all_shapes = [shape.Located(world_loc)] if shape is not None and not shape.IsNull() else []
                mesh_data = merge_shapes_to_mesh(all_shapes, tolerance)
                if mesh_data and len(mesh_data[0]) > 0:
                    parts.append({"name": sanitize_name(inst_name), "vertices": mesh_data[0], "indices": mesh_data[1]})
                    print(f"  Part: {inst_name} → {len(mesh_data[0])//3} vertices")
            continue

        # Check if this part should be split by its internal solids
        if name_key in SPLIT_BY_SOLID or name in SPLIT_BY_SOLID:
            prox_threshold = SPLIT_BY_SOLID.get(name_key) or SPLIT_BY_SOLID.get(name, 20.0)
            shape = st.GetShape_s(sub_ref)
            if shape is not None and not shape.IsNull():
                shape = shape.Located(world_loc)
                from OCP.TopAbs import TopAbs_SOLID
                from OCP.TopExp import TopExp_Explorer as TE2
                from OCP.Bnd import Bnd_Box
                from OCP.BRepBndLib import BRepBndLib
                solid_groups: list[list] = []
                exp = TE2(shape, TopAbs_SOLID)
                while exp.More():
                    solid = exp.Current()
                    bb = Bnd_Box()
                    BRepBndLib.Add_s(solid, bb)
                    xn, yn, zn, xx, yx, zx = bb.Get()
                    cx, cy, cz = (xn+xx)/2, (yn+yx)/2, (zn+zx)/2
                    merged = False
                    for grp in solid_groups:
                        gcx, gcy, gcz = grp[0]
                        dist = ((cx-gcx)**2 + (cy-gcy)**2 + (cz-gcz)**2) ** 0.5
                        if dist < prox_threshold:
                            grp[1].append(solid)
                            merged = True
                            break
                    if not merged:
                        solid_groups.append([(cx, cy, cz), [solid]])
                    exp.Next()
                for gi, (center, solids) in enumerate(solid_groups, 1):
                    mesh_data = merge_shapes_to_mesh(solids, tolerance)
                    if mesh_data and len(mesh_data[0]) > 0:
                        # For PE-300 10mm, classify solids by geometry
                        suffix = str(gi)
                        if "PE-300 10" in name or "PE-300_10" in name:
                            from OCP.BRep import BRep_Builder as BB2
                            from OCP.TopoDS import TopoDS_Compound as TC2
                            bb2 = Bnd_Box()
                            for s in solids:
                                BRepBndLib.Add_s(s, bb2)
                            xn2, yn2, zn2, xx2, yx2, zx2 = bb2.Get()
                            dx, dy, dz = xx2-xn2, yx2-yn2, zx2-zn2
                            cy = (yn2+yx2)/2
                            cz = (zn2+zx2)/2
                            if dz < 15 and dx > 300 and dy > 300:
                                suffix = "base" if cz < 50 else "shelf"
                            elif dx < 15:
                                suffix = f"side_{'R' if center[0] > 200 else 'L'}{'_inner' if dy < 300 else ''}"
                            elif dy < 15:
                                suffix = f"wall_{'back' if center[1] > 300 else 'front'}_{int(cz)}"
                            else:
                                suffix = f"ledge_{'back' if cy > 250 else 'front'}_{int(cz)}"
                        sname = f"{full_name}_{suffix}"
                        parts.append({"name": sanitize_name(sname), "vertices": mesh_data[0], "indices": mesh_data[1]})
                        print(f"  Part: {sname} → {len(mesh_data[0])//3} vertices")
            continue

        # Check if this sub-assembly should be merged into one part
        if st.IsAssembly_s(sub_ref) and name_key in MERGE_SUB_ASSEMBLIES:
            all_shapes = collect_all_shapes(st, sub_ref, world_loc)
            for s in all_shapes:
                named_groups[full_name].append(s)
            continue

        # Leaf part or simple assembly — get shape
        if st.IsAssembly_s(sub_ref):
            all_shapes = collect_all_shapes(st, sub_ref, world_loc)
            for s in all_shapes:
                named_groups[full_name].append(s)
        else:
            shape = st.GetShape_s(sub_ref)
            if shape is not None and not shape.IsNull():
                named_groups[full_name].append(shape.Located(world_loc))

    # Tessellate each named group
    for full_name, shapes in named_groups.items():
        mesh_data = merge_shapes_to_mesh(shapes, tolerance)
        if mesh_data and len(mesh_data[0]) > 0:
            count = len(shapes)
            suffix = f" ({count}×)" if count > 1 else ""
            parts.append({"name": sanitize_name(full_name), "vertices": mesh_data[0], "indices": mesh_data[1]})
            print(f"  Part: {full_name} → {len(mesh_data[0])//3} vertices{suffix}")

    # Merge all fasteners
    if fastener_shapes:
        mesh_data = merge_shapes_to_mesh(fastener_shapes, tolerance)
        if mesh_data and len(mesh_data[0]) > 0:
            parts.append({
                "name": sanitize_name(f"{parent_name}/Fasteners"),
                "vertices": mesh_data[0],
                "indices": mesh_data[1],
            })
            print(f"  Part: {parent_name}/Fasteners → {len(mesh_data[0])//3} vertices (merged)")

    return parts


def tessellate_shape(shape, tolerance):
    """Tessellate a TopoDS_Shape and return (flat_vertices, flat_indices)."""
    from OCP.BRepMesh import BRepMesh_IncrementalMesh
    from OCP.TopAbs import TopAbs_FACE
    from OCP.TopExp import TopExp_Explorer
    from OCP.BRep import BRep_Tool
    from OCP.TopLoc import TopLoc_Location
    from OCP.TopoDS import TopoDS

    BRepMesh_IncrementalMesh(shape, tolerance, False, 0.5, True)

    all_verts = []
    all_indices = []
    offset = 0

    explorer = TopExp_Explorer(shape, TopAbs_FACE)
    while explorer.More():
        face = TopoDS.Face_s(explorer.Current())
        loc = TopLoc_Location()
        triangulation = BRep_Tool.Triangulation_s(face, loc)
        if triangulation is None:
            explorer.Next()
            continue

        trsf = loc.Transformation()
        nb_nodes = triangulation.NbNodes()
        nb_tris = triangulation.NbTriangles()

        for j in range(1, nb_nodes + 1):
            pt = triangulation.Node(j)
            pt.Transform(trsf)
            all_verts.extend([pt.X(), pt.Y(), pt.Z()])

        orient_reversed = face.Orientation() == 1  # TopAbs_REVERSED
        for j in range(1, nb_tris + 1):
            tri = triangulation.Triangle(j)
            n1, n2, n3 = tri.Get()
            if orient_reversed:
                n1, n2 = n2, n1
            all_indices.extend([offset + n1 - 1, offset + n2 - 1, offset + n3 - 1])

        offset += nb_nodes
        explorer.Next()

    if not all_verts:
        return None

    return (np.array(all_verts, dtype=np.float32), np.array(all_indices, dtype=np.uint32))


def sanitize_name(name):
    """Clean name for glTF node naming."""
    return name.replace(" ", "_").replace("/", "__")[:63]


def write_glb(parts, output_path):
    """Write parts as a multi-node GLB file."""
    # Build glTF JSON + binary buffer
    nodes = []
    meshes = []
    accessors = []
    buffer_views = []
    bin_data = bytearray()

    for idx, part in enumerate(parts):
        verts = part["vertices"]
        indices = part["indices"]

        # Vertex buffer view
        verts_bytes = verts.tobytes()
        v_offset = len(bin_data)
        # Pad to 4-byte boundary
        while len(bin_data) % 4 != 0:
            bin_data.append(0)
        v_offset = len(bin_data)
        bin_data.extend(verts_bytes)

        bv_verts = len(buffer_views)
        buffer_views.append({
            "buffer": 0,
            "byteOffset": v_offset,
            "byteLength": len(verts_bytes),
            "target": 34962  # ARRAY_BUFFER
        })

        # Index buffer view
        idx_bytes = indices.tobytes()
        while len(bin_data) % 4 != 0:
            bin_data.append(0)
        i_offset = len(bin_data)
        bin_data.extend(idx_bytes)

        bv_indices = len(buffer_views)
        buffer_views.append({
            "buffer": 0,
            "byteOffset": i_offset,
            "byteLength": len(idx_bytes),
            "target": 34963  # ELEMENT_ARRAY_BUFFER
        })

        # Position accessor
        num_verts = len(verts) // 3
        v_min = [float(verts[0::3].min()), float(verts[1::3].min()), float(verts[2::3].min())]
        v_max = [float(verts[0::3].max()), float(verts[1::3].max()), float(verts[2::3].max())]

        acc_pos = len(accessors)
        accessors.append({
            "bufferView": bv_verts,
            "componentType": 5126,  # FLOAT
            "count": num_verts,
            "type": "VEC3",
            "min": v_min,
            "max": v_max
        })

        # Index accessor
        acc_idx = len(accessors)
        accessors.append({
            "bufferView": bv_indices,
            "componentType": 5125,  # UNSIGNED_INT
            "count": len(indices),
            "type": "SCALAR"
        })

        meshes.append({
            "name": part["name"],
            "primitives": [{
                "attributes": {"POSITION": acc_pos},
                "indices": acc_idx,
                "material": get_material_index(part["name"])
            }]
        })

        nodes.append({
            "name": part["name"],
            "mesh": idx
        })

    # Root node that parents all parts
    root_idx = len(nodes)
    nodes.append({
        "name": "IncuNest_Assembly",
        "children": list(range(len(parts)))
    })

    gltf = {
        "asset": {"version": "2.0", "generator": "incunest-step-converter"},
        "scene": 0,
        "scenes": [{"name": "IncuNest", "nodes": [root_idx]}],
        "nodes": nodes,
        "meshes": meshes,
        "accessors": accessors,
        "bufferViews": buffer_views,
        "buffers": [{"byteLength": len(bin_data)}],
        "materials": [
            {   # 0: default teal
                "name": "default",
                "pbrMetallicRoughness": {
                    "baseColorFactor": [0.7, 0.85, 0.82, 1.0],
                    "metallicFactor": 0.1,
                    "roughnessFactor": 0.5
                }
            },
            {   # 1: white opaque (doors)
                "name": "door_white",
                "pbrMetallicRoughness": {
                    "baseColorFactor": [0.95, 0.95, 0.95, 1.0],
                    "metallicFactor": 0.0,
                    "roughnessFactor": 0.4
                }
            },
            {   # 2: glass gray transparent (methacrylate)
                "name": "glass",
                "pbrMetallicRoughness": {
                    "baseColorFactor": [0.75, 0.78, 0.80, 0.35],
                    "metallicFactor": 0.05,
                    "roughnessFactor": 0.15
                },
                "alphaMode": "BLEND"
            },
            {   # 3: reddish handle (pomo)
                "name": "handle_red",
                "pbrMetallicRoughness": {
                    "baseColorFactor": [0.78, 0.22, 0.18, 1.0],
                    "metallicFactor": 0.05,
                    "roughnessFactor": 0.45
                }
            }
        ]
    }

    json_bytes = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    # Pad JSON to 4-byte boundary
    while len(json_bytes) % 4 != 0:
        json_bytes += b" "
    # Pad bin to 4-byte boundary
    while len(bin_data) % 4 != 0:
        bin_data.append(0)

    # GLB header: magic + version + length
    total = 12 + 8 + len(json_bytes) + 8 + len(bin_data)
    header = struct.pack("<4sII", b"glTF", 2, total)
    json_chunk_header = struct.pack("<II", len(json_bytes), 0x4E4F534A)
    bin_chunk_header = struct.pack("<II", len(bin_data), 0x004E4942)

    Path(output_path).parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "wb") as f:
        f.write(header)
        f.write(json_chunk_header)
        f.write(json_bytes)
        f.write(bin_chunk_header)
        f.write(bin_data)


if __name__ == "__main__":
    main()
