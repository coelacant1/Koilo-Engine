# Asset Conversion

Convert 3D models to KoiloMesh (`.kmesh`) format.

## Files

| Script | Purpose |
|--------|---------|
| `convert_to_kmesh.py` | FBX/OBJ → `.kmesh` (binary or ASCII) |
| `FBXReader.py` | FBX parser (used by convert_to_kmesh) |

## Usage

```bash
python convert_to_kmesh.py model.obj -o model.kmesh
python convert_to_kmesh.py model.obj -o model.kmesh --format ascii
python convert_to_kmesh.py model.fbx -o model.kmesh
python convert_to_kmesh.py model.fbx -o model.kmesh --no-converter  # ASCII FBX
```

FBX conversion requires `FbxFormatConverter.exe` (Autodesk FBX SDK) in this directory for binary FBX files. Use `--no-converter` if FBX is already ASCII.

## KoiloMesh Format

**Binary** (default): 32-byte header (`KLOM` magic), vertex/triangle/morph data, `ENDM` marker.

**ASCII**: OBJ-like text with `v`/`f` lines plus `m` lines for morph deltas. Loader auto-detects format.
