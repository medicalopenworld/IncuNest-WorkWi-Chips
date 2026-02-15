#!/usr/bin/env python3
"""Diagnostic: print the complete transform chain for every part in a STEP assembly.

For each component label, we print:
  1. The label entry path
  2. Whether it's a component, assembly, or simple shape
  3. The location (transform) stored on that label
  4. The cumulative transform chain from root to leaf
  5. Comparison: GetShape_s() bounding box vs. GetShape_s() + manual location composition

This answers the key question:
  Does st.GetShape_s(component_label) include the component's location
  transform in the tessellated vertices, or must we manually compose
  transform chains?
"""
import sys
import numpy as np

def main():
    step_path = sys.argv[1] if len(sys.argv) > 1 else "Incunest_v15/Mechanical/IN3_structure_v15.step"

    from OCP.STEPCAFControl import STEPCAFControl_Reader
    from OCP.TDocStd import TDocStd_Document
    from OCP.TCollection import TCollection_ExtendedString
    from OCP.XCAFDoc import XCAFDoc_DocumentTool, XCAFDoc_Location
    from OCP.TDF import TDF_LabelSequence, TDF_Label
    from OCP.TDataStd import TDataStd_Name
    from OCP.BRepMesh import BRepMesh_IncrementalMesh
    from OCP.TopLoc import TopLoc_Location
    from OCP.BRep import BRep_Tool
    from OCP.TopAbs import TopAbs_FACE
    from OCP.TopExp import TopExp_Explorer
    from OCP.TopoDS import TopoDS
    from OCP.BRepBndLib import BRepBndLib
    from OCP.Bnd import Bnd_Box
    from OCP.gp import gp_Trsf, gp_XYZ

    print(f"Reading {step_path}...")
    doc = TDocStd_Document(TCollection_ExtendedString("STEP"))
    reader = STEPCAFControl_Reader()
    reader.SetNameMode(True)
    reader.SetColorMode(True)
    status = reader.ReadFile(step_path)
    print(f"ReadFile status: {status}")
    reader.Transfer(doc)

    st = XCAFDoc_DocumentTool.ShapeTool_s(doc.Main())

    free_labels = TDF_LabelSequence()
    st.GetFreeShapes(free_labels)
    print(f"\nFree shapes (roots): {free_labels.Length()}")

    def get_name(label):
        name_attr = TDataStd_Name()
        if label.FindAttribute(TDataStd_Name.GetID_s(), name_attr):
            return name_attr.Get().ToExtString()
        return "<unnamed>"

    def trsf_to_str(trsf):
        """Format a gp_Trsf as translation + rotation summary."""
        tx = trsf.Value(1, 4)
        ty = trsf.Value(2, 4)
        tz = trsf.Value(3, 4)
        # Rotation matrix (3x3)
        r = [[trsf.Value(i, j) for j in range(1, 4)] for i in range(1, 4)]
        is_identity = (abs(tx) < 1e-9 and abs(ty) < 1e-9 and abs(tz) < 1e-9 and
                       abs(r[0][0] - 1) < 1e-9 and abs(r[1][1] - 1) < 1e-9 and abs(r[2][2] - 1) < 1e-9)
        if is_identity:
            return "IDENTITY"
        rot_str = ""
        if not (abs(r[0][0] - 1) < 1e-9 and abs(r[1][1] - 1) < 1e-9 and abs(r[2][2] - 1) < 1e-9):
            rot_str = f" rot=[{r[0][0]:.4f},{r[0][1]:.4f},{r[0][2]:.4f} / {r[1][0]:.4f},{r[1][1]:.4f},{r[1][2]:.4f} / {r[2][0]:.4f},{r[2][1]:.4f},{r[2][2]:.4f}]"
        return f"T=({tx:.2f}, {ty:.2f}, {tz:.2f}){rot_str}"

    def get_bbox(shape):
        """Get bounding box of a shape."""
        bbox = Bnd_Box()
        BRepBndLib.Add_s(shape, bbox)
        if bbox.IsVoid():
            return None
        xmin, ymin, zmin, xmax, ymax, zmax = bbox.Get()
        return (xmin, ymin, zmin, xmax, ymax, zmax)

    def bbox_center(bb):
        if bb is None:
            return "N/A"
        cx = (bb[0] + bb[3]) / 2
        cy = (bb[1] + bb[4]) / 2
        cz = (bb[2] + bb[5]) / 2
        return f"center=({cx:.1f}, {cy:.1f}, {cz:.1f}) size=({bb[3]-bb[0]:.1f} x {bb[4]-bb[1]:.1f} x {bb[5]-bb[2]:.1f})"

    def label_entry(label):
        """Get the TDF label entry string."""
        from OCP.TCollection import TCollection_AsciiString
        entry = TCollection_AsciiString()
        from OCP.TDF import TDF_Tool
        TDF_Tool.Entry_s(label, entry)
        return entry.ToCString()

    # ---- KEY TEST: Compare GetShape_s with and without location ----
    print("\n" + "="*100)
    print("KEY TEST: Does GetShape_s() include the component's location transform?")
    print("="*100)

    root_label = free_labels.Value(1)
    root_name = get_name(root_label)
    print(f"\nRoot: {root_name} [{label_entry(root_label)}]")
    print(f"  IsAssembly: {st.IsAssembly_s(root_label)}")

    root_comps = TDF_LabelSequence()
    st.GetComponents_s(root_label, root_comps)
    print(f"  Direct children: {root_comps.Length()}")

    for i in range(1, root_comps.Length() + 1):
        comp = root_comps.Value(i)
        ref = TDF_Label()
        has_ref = st.GetReferredShape_s(comp, ref)
        name = get_name(ref) if has_ref else get_name(comp)

        print(f"\n  [{i}] {name} [{label_entry(comp)}]")
        print(f"      IsComponent: {st.IsComponent_s(comp)}")
        print(f"      IsAssembly(ref): {st.IsAssembly_s(ref) if has_ref else 'N/A'}")
        print(f"      IsSimpleShape(ref): {st.IsSimpleShape_s(ref) if has_ref else 'N/A'}")
        if has_ref:
            print(f"      Ref label: [{label_entry(ref)}]")

        # Location on the component label
        comp_loc = st.GetLocation_s(comp)
        comp_trsf = comp_loc.Transformation()
        print(f"      Component location: {trsf_to_str(comp_trsf)}")

        # Shape via GetShape_s(comp) - what the current script uses
        shape_from_comp = st.GetShape_s(comp)
        bb_comp = get_bbox(shape_from_comp) if not shape_from_comp.IsNull() else None
        print(f"      GetShape_s(comp) bbox: {bbox_center(bb_comp)}")

        # Shape's own Location()
        if not shape_from_comp.IsNull():
            shape_loc = shape_from_comp.Location()
            shape_trsf = shape_loc.Transformation()
            print(f"      shape.Location(): {trsf_to_str(shape_trsf)}")

        # If ref exists, get shape from ref (without component location)
        if has_ref:
            shape_from_ref = st.GetShape_s(ref)
            bb_ref = get_bbox(shape_from_ref) if not shape_from_ref.IsNull() else None
            print(f"      GetShape_s(ref) bbox:  {bbox_center(bb_ref)}")
            if not shape_from_ref.IsNull():
                ref_shape_loc = shape_from_ref.Location()
                print(f"      ref shape.Location(): {trsf_to_str(ref_shape_loc.Transformation())}")

        # For assemblies, recurse one level into sub-components
        if has_ref and st.IsAssembly_s(ref):
            sub_comps = TDF_LabelSequence()
            st.GetComponents_s(ref, sub_comps)
            print(f"      Sub-components: {sub_comps.Length()}")

            # Show first few sub-components
            limit = min(sub_comps.Length(), 5)
            for j in range(1, limit + 1):
                sub = sub_comps.Value(j)
                sub_ref = TDF_Label()
                has_sub_ref = st.GetReferredShape_s(sub, sub_ref)
                sub_name = get_name(sub_ref) if has_sub_ref else get_name(sub)

                sub_loc = st.GetLocation_s(sub)
                sub_trsf = sub_loc.Transformation()

                # Shape from sub (component label) - should include sub's location
                sub_shape = st.GetShape_s(sub)
                sub_bb = get_bbox(sub_shape) if not sub_shape.IsNull() else None

                print(f"        [{j}] {sub_name}")
                print(f"            Sub location: {trsf_to_str(sub_trsf)}")
                print(f"            GetShape_s(sub) bbox: {bbox_center(sub_bb)}")

                # Now test: does GetShape_s(sub) include the PARENT's location (comp_loc)?
                # If parts are correctly placed in world coords, bbox should match
                # what we'd get by composing parent_loc * sub_loc * ref_shape
                if not sub_shape.IsNull():
                    sub_shape_loc = sub_shape.Location()
                    print(f"            sub shape.Location(): {trsf_to_str(sub_shape_loc.Transformation())}")

                    # Manual composition test
                    if has_sub_ref:
                        ref_shape = st.GetShape_s(sub_ref)
                        if not ref_shape.IsNull():
                            ref_bb = get_bbox(ref_shape)
                            print(f"            GetShape_s(sub_ref) bbox: {bbox_center(ref_bb)}")

            if sub_comps.Length() > limit:
                print(f"        ... and {sub_comps.Length() - limit} more sub-components")

    # ---- DEEP DIVE: Full transform chain analysis ----
    print("\n" + "="*100)
    print("FULL TRANSFORM CHAIN ANALYSIS")
    print("="*100)
    print("Walking entire tree to check if GetShape_s(comp) at any level")
    print("includes ancestor transforms or only the component's own location.\n")

    all_parts_info = []

    def walk_assembly(label, depth=0, parent_path="", parent_world_trsf=None):
        """Recursively walk the assembly tree."""
        indent = "  " * depth
        comps = TDF_LabelSequence()
        st.GetComponents_s(label, comps)

        for i in range(1, comps.Length() + 1):
            comp = comps.Value(i)
            ref = TDF_Label()
            has_ref = st.GetReferredShape_s(comp, ref)
            name = get_name(ref) if has_ref else get_name(comp)
            path = f"{parent_path}/{name}" if parent_path else name

            # This component's location
            comp_loc = st.GetLocation_s(comp)
            comp_trsf = comp_loc.Transformation()

            # Compose world transform: parent_world * comp_local
            if parent_world_trsf is not None:
                world_trsf = gp_Trsf()
                world_trsf.Multiply(parent_world_trsf)
                world_trsf.Multiply(comp_trsf)
            else:
                world_trsf = gp_Trsf()
                world_trsf.Multiply(comp_trsf)

            shape = st.GetShape_s(comp)
            if not shape.IsNull():
                bb = get_bbox(shape)
                shape_loc_trsf = shape.Location().Transformation()

                # Extract world translation
                wx = world_trsf.Value(1, 4)
                wy = world_trsf.Value(2, 4)
                wz = world_trsf.Value(3, 4)

                # Extract shape location translation
                sx = shape_loc_trsf.Value(1, 4)
                sy = shape_loc_trsf.Value(2, 4)
                sz = shape_loc_trsf.Value(3, 4)

                info = {
                    'path': path,
                    'depth': depth,
                    'comp_loc': trsf_to_str(comp_trsf),
                    'world_trsf': trsf_to_str(world_trsf),
                    'shape_loc': trsf_to_str(shape_loc_trsf),
                    'bbox': bbox_center(bb),
                    'world_t': (wx, wy, wz),
                    'shape_t': (sx, sy, sz),
                }
                all_parts_info.append(info)

                if depth <= 2:
                    print(f"{indent}[{path}]")
                    print(f"{indent}  comp_location: {trsf_to_str(comp_trsf)}")
                    print(f"{indent}  world_transform (composed): {trsf_to_str(world_trsf)}")
                    print(f"{indent}  shape.Location(): {trsf_to_str(shape_loc_trsf)}")
                    print(f"{indent}  bbox: {bbox_center(bb)}")
                    # KEY comparison
                    match = (abs(wx - sx) < 0.1 and abs(wy - sy) < 0.1 and abs(wz - sz) < 0.1)
                    print(f"{indent}  world_T vs shape_loc_T MATCH: {match}")

            # Recurse into sub-assembly
            if has_ref and st.IsAssembly_s(ref):
                walk_assembly(ref, depth + 1, path, world_trsf)

    # Start from root
    walk_assembly(root_label, depth=0, parent_path=root_name, parent_world_trsf=None)

    # Summary
    print("\n" + "="*100)
    print("SUMMARY: Transform consistency check")
    print("="*100)

    match_count = 0
    mismatch_count = 0
    for info in all_parts_info:
        wx, wy, wz = info['world_t']
        sx, sy, sz = info['shape_t']
        match = (abs(wx - sx) < 0.1 and abs(wy - sy) < 0.1 and abs(wz - sz) < 0.1)
        if match:
            match_count += 1
        else:
            mismatch_count += 1

    print(f"Total parts analyzed: {len(all_parts_info)}")
    print(f"  world_transform == shape.Location(): {match_count}")
    print(f"  world_transform != shape.Location(): {mismatch_count}")

    if mismatch_count > 0:
        print("\nMISMATCHES (shape.Location() does NOT include full ancestor chain):")
        for info in all_parts_info:
            wx, wy, wz = info['world_t']
            sx, sy, sz = info['shape_t']
            match = (abs(wx - sx) < 0.1 and abs(wy - sy) < 0.1 and abs(wz - sz) < 0.1)
            if not match:
                print(f"  {info['path']}")
                print(f"    composed world: T=({wx:.2f}, {wy:.2f}, {wz:.2f})")
                print(f"    shape.Location: T=({sx:.2f}, {sy:.2f}, {sz:.2f})")
    else:
        print("\nAll shape locations include the full transform chain!")
        print("This means GetShape_s(comp) DOES include ancestor locations.")

    # Additional test: Does GetShape_s on the component label vs reference label differ?
    print("\n" + "="*100)
    print("TEST: GetShape_s(comp_label) vs GetShape_s(ref_label) for depth>0 components")
    print("="*100)
    root_comps = TDF_LabelSequence()
    st.GetComponents_s(root_label, root_comps)

    for i in range(1, min(root_comps.Length() + 1, 4)):  # First 3 top-level
        comp = root_comps.Value(i)
        ref = TDF_Label()
        has_ref = st.GetReferredShape_s(comp, ref)
        name = get_name(ref) if has_ref else get_name(comp)

        if has_ref and st.IsAssembly_s(ref):
            sub_comps = TDF_LabelSequence()
            st.GetComponents_s(ref, sub_comps)
            print(f"\nAssembly: {name} (testing first 3 sub-components)")

            for j in range(1, min(sub_comps.Length() + 1, 4)):
                sub = sub_comps.Value(j)
                sub_ref = TDF_Label()
                has_sub_ref = st.GetReferredShape_s(sub, sub_ref)
                sub_name = get_name(sub_ref) if has_sub_ref else get_name(sub)

                shape_comp = st.GetShape_s(sub)
                bb_comp = get_bbox(shape_comp) if not shape_comp.IsNull() else None

                if has_sub_ref:
                    shape_ref = st.GetShape_s(sub_ref)
                    bb_ref = get_bbox(shape_ref) if not shape_ref.IsNull() else None
                else:
                    bb_ref = None

                print(f"  {sub_name}:")
                print(f"    GetShape_s(comp):  {bbox_center(bb_comp)}")
                print(f"    GetShape_s(ref):   {bbox_center(bb_ref)}")
                if bb_comp and bb_ref:
                    same = all(abs(a - b) < 0.1 for a, b in zip(bb_comp, bb_ref))
                    print(f"    Same bbox? {same}")
                    if not same:
                        print(f"    → GetShape_s(comp) INCLUDES the component's own location!")
                        print(f"    → GetShape_s(ref) gives the shape in the REFERENCE frame (no location)")

    print("\nDiagnostic complete.")


if __name__ == "__main__":
    main()
