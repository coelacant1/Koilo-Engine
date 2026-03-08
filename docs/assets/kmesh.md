# KoiloMesh Format

Version 1, little-endian. Stores triangle meshes with optional UVs and morph targets (blend shapes). Two representations: ASCII for development, binary for deployment.

- `.kmesh` - ASCII
- `.kmeshb` - Binary

The loader auto-detects format by reading the first bytes of the file.

---

## ASCII Format

```
KMESH 1
NAME <mesh_name>

VERTICES <count>
<x> <y> <z>
...

TRIANGLES <count>
<a> <b> <c>
...

UVS <count>
<u> <v>
...

UVTRIANGLES <count>
<a> <b> <c>
...

MORPH <name> <affected_count>
INDICES <idx0> <idx1> ...
VECTORS
<dx> <dy> <dz>
...

END
```

Sections can appear in any order after the header. Lines starting with `#` are comments. `NAME`, `UVS`, `UVTRIANGLES`, and `MORPH` sections are optional.

### Example

```
KMESH 1
NAME Cube

VERTICES 8
-10.0 -10.0 -10.0
 10.0 -10.0 -10.0
 10.0  10.0 -10.0
-10.0  10.0 -10.0
-10.0 -10.0  10.0
 10.0 -10.0  10.0
 10.0  10.0  10.0
-10.0  10.0  10.0

TRIANGLES 12
0 1 2
0 2 3
4 5 6
4 6 7
0 4 5
0 5 1
1 5 6
1 6 2
2 6 7
2 7 3
3 7 4
3 4 0

MORPH Expand 8
INDICES 0 1 2 3 4 5 6 7
VECTORS
-5.0 -5.0 -5.0
 5.0 -5.0 -5.0
 5.0  5.0 -5.0
-5.0  5.0 -5.0
-5.0 -5.0  5.0
 5.0 -5.0  5.0
 5.0  5.0  5.0
-5.0  5.0  5.0

END
```

Vertices are `X Y Z` floats (right-handed, Y-up). Triangle indices are 0-based, CCW winding. UV coordinates are `U V` floats in [0, 1]. UV triangles use a separate index list so UV and position topologies can differ.

Morph targets use sparse representation - only affected vertices are stored. Multiple morphs can be active simultaneously with independent weights in [0, 1]. Final position: `baseVertex + Σ(delta[i] × weight[i])`.

---

## Binary Format

Magic bytes `KOIM`. Auto-detected by the loader.

### Header (32 bytes)

```
Offset  Size  Type      Field
0x00    4     char[4]   Magic ("KOIM")
0x04    4     uint32    Version (1)
0x08    4     uint32    Vertex count
0x0C    4     uint32    Triangle count
0x10    4     uint32    Morph count
0x14    4     uint32    Flags (bit 0: UVs, bit 1: normals)
0x18    8     -         Reserved
```

### Body

```
Vertices     vertex_count × 12 bytes    [float x, float y, float z]
Triangles    tri_count × 12 bytes       [uint32 v0, uint32 v1, uint32 v2]
```

### Morph Data (repeated per morph)

```
1 byte       Name length (uint8)
N bytes      Name (ASCII, no null terminator)
4 bytes      Affected vertex count (uint32)

Per affected vertex (16 bytes each):
  4 bytes    Vertex index (uint32)
  4 bytes    Delta X (float)
  4 bytes    Delta Y (float)
  4 bytes    Delta Z (float)
```

### End Marker

```
4 bytes      "ENDM"
```

### Size Comparison

| Mesh | ASCII | Binary | Reduction |
|------|-------|--------|-----------|
| NukudeFace (55v, 26 morphs) | ~18 KB | ~8 KB | ~55% |
| Simple Cube (8v, 1 morph) | ~400 B | ~250 B | ~38% |

---

## Loader API

```cpp
KoiloMeshLoader loader;
loader.Load("face.kmesh");

uint32_t verts  = loader.GetVertexCount();
uint32_t tris   = loader.GetTriangleCount();
uint32_t morphs = loader.GetMorphCount();
const float*    v = loader.GetVertices();     // x,y,z interleaved
const uint32_t* t = loader.GetTriangles();    // v0,v1,v2 triplets
const float*    u = loader.GetUVs();          // u,v interleaved
const uint32_t* ut = loader.GetUVTriangles(); // UV index triplets
bool hasUV = loader.HasUVs();

const MorphTarget* m = loader.GetMorph("Anger");
// m->name, m->indices, m->deltaX, m->deltaY, m->deltaZ
```

`MorphableMesh` wraps the loader with blend weight control:

```cpp
MorphableMesh mesh;
mesh.Load("face.kmesh");
mesh.SetMorphWeight("Anger", 1.0f);
mesh.SetMorphWeight("Blink", 0.5f);
mesh.Update();
Mesh* m = mesh.GetMesh();
```

---

## Converter

`tools/asset-conversion/convert_to_kmesh.py` converts FBX and OBJ files to KoiloMesh format.

```bash
python3 tools/asset-conversion/convert_to_kmesh.py input.fbx -o output.kmesh --format ascii
python3 tools/asset-conversion/convert_to_kmesh.py input.obj -o output.kmeshb --format binary
```

Supporting files in the same directory: `FBXReader.py` (FBX ASCII parser).

---

## Validation

The loader enforces:
- Magic bytes match (`KMESH` for ASCII, `KOIM` for binary)
- Version == 1
- Vertex and triangle counts match declared values
- Binary end marker `ENDM` present
- File not truncated mid-section

---

## Limits

| Constraint | Value |
|------------|-------|
| Morph name length | 255 chars (uint8 prefix) |
| Max vertices / triangles | 2³² − 1 (uint32 indices) |
| BlendshapeController default capacity | 16 morphs |
| Normals | Flag exists, not yet read by loader |
| Multi-mesh | Not supported (single mesh per file) |
