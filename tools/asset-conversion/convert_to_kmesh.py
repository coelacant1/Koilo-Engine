#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
convert_to_kmesh - Convert FBX/OBJ to KoiloMesh (.kmesh) format.

Usage:
    python convert_to_kmesh.py models/NukudeFace.fbx -o nukude.kmesh
    python convert_to_kmesh.py models/cube.obj -o cube.kmesh --format ascii
"""

import struct
import sys
import os
import argparse

# FBXReader.py lives alongside this script
try:
    script_dir = os.path.dirname(os.path.abspath(__file__))
    if script_dir not in sys.path:
        sys.path.insert(0, script_dir)
    from FBXReader import GetMorphObject
    HAS_FBX_READER = True
except Exception as e:
    HAS_FBX_READER = False

class KoiloMeshWriter:
    """Writes .kmesh binary format."""
    
    MAGIC = b'KOIM'
    VERSION = 1
    END_MARKER = b'ENDM'
    
    def __init__(self):
        self.vertices = []
        self.triangles = []
        self.morphs = []
        self.uvs = []           # Per-vertex UV coords: (u, v)
        self.uv_triangles = []  # UV index triplets per triangle
        self.has_uvs = False
        self.has_normals = False
    
    def from_morph_object(self, morph_obj):
        """Convert MorphObject (from FBXReader) to KoiloMesh data."""
        # Extract base mesh
        vertex_count = int(morph_obj.baseMesh.VertexCount / 3)
        for i in range(vertex_count):
            idx = i * 3
            if idx < len(morph_obj.baseMesh.Vertices):
                v = morph_obj.baseMesh.Vertices[idx]
                self.vertices.append((v.X, v.Y, v.Z))
        
        # Extract triangles (convert from 1-based to 0-based indices)
        max_vertex_idx = len(self.vertices) - 1
        for tri in morph_obj.baseMesh.Triangles:
            # FBX uses 1-based indexing, convert to 0-based
            # Clamp to actual number of vertices extracted
            a = min(max(0, tri.A - 1), max_vertex_idx)
            b = min(max(0, tri.B - 1), max_vertex_idx)
            c = min(max(0, tri.C - 1), max_vertex_idx)
            self.triangles.append((a, b, c))
        
        # Extract morph targets
        for shape_key in morph_obj.shapeKeys:
            morph_data = []
            vertex_count = int(shape_key.VertexCount / 3)
            
            for i in range(len(shape_key.Indexes)):
                if i < vertex_count:
                    idx = i * 3
                    if idx < len(shape_key.Vertices):
                        v = shape_key.Vertices[idx]
                        # FBX uses 1-based indexing
                        vertex_idx = max(0, shape_key.Indexes[i] - 1)
                        morph_data.append((
                            vertex_idx,
                            v.X, v.Y, v.Z
                        ))
            
            self.morphs.append((shape_key.Name, morph_data))
    
    def from_obj_file(self, filepath):
        """Load OBJ file with UVs (no morphs)."""
        with open(filepath, 'r') as f:
            data = f.read()

        raw_uvs = []
        for line in data.splitlines():
            if line.startswith('v '):
                parts = line.split()
                self.vertices.append((
                    float(parts[1]),
                    float(parts[2]),
                    float(parts[3])
                ))
            elif line.startswith('vt '):
                parts = line.split()
                # Flip V: OBJ has V=0 at bottom, .ktex stores top-to-bottom
                raw_uvs.append((float(parts[1]), 1.0 - float(parts[2])))
            elif line.startswith('f '):
                parts = line.split()
                face_verts = []
                face_uvs = []
                for p in parts[1:]:
                    indices = p.split('/')
                    face_verts.append(int(indices[0]) - 1)
                    if len(indices) > 1 and indices[1]:
                        face_uvs.append(int(indices[1]) - 1)
                # Fan-triangulate
                for i in range(1, len(face_verts) - 1):
                    self.triangles.append((face_verts[0], face_verts[i], face_verts[i + 1]))
                    if face_uvs:
                        self.uv_triangles.append((face_uvs[0], face_uvs[i], face_uvs[i + 1]))

        if raw_uvs and self.uv_triangles:
            self.uvs = raw_uvs
            self.has_uvs = True
    
    def write_ascii(self, filepath):
        """Write ASCII .kmesh file matching the engine's LoadAscii parser."""
        with open(filepath, 'w') as f:
            f.write("KMESH 1\n")
            f.write(f"VERTICES {len(self.vertices)}\n")
            for v in self.vertices:
                f.write(f"{v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            f.write(f"TRIANGLES {len(self.triangles)}\n")
            for tri in self.triangles:
                f.write(f"{tri[0]} {tri[1]} {tri[2]}\n")
            if self.has_uvs and self.uvs:
                f.write(f"UVS {len(self.uvs)}\n")
                for uv in self.uvs:
                    f.write(f"{uv[0]:.6f} {uv[1]:.6f}\n")
                f.write(f"UVTRIANGLES {len(self.uv_triangles)}\n")
                for tri in self.uv_triangles:
                    f.write(f"{tri[0]} {tri[1]} {tri[2]}\n")
            for name, data in self.morphs:
                f.write(f"MORPH {name} {len(data)}\n")
                f.write("INDICES " + " ".join(str(d[0]) for d in data) + "\n")
                f.write("VECTORS\n")
                for idx, dx, dy, dz in data:
                    f.write(f"{dx:.6f} {dy:.6f} {dz:.6f}\n")
            f.write("END\n")
        
        print(f"Wrote ASCII {filepath}")
        print(f"  Vertices: {len(self.vertices)}")
        print(f"  Triangles: {len(self.triangles)}")
        print(f"  Morphs: {len(self.morphs)}")
        
        # Calculate file size
        size = os.path.getsize(filepath)
        print(f"  File size: {size} bytes ({size/1024:.2f} KB)")
    
    def write_binary(self, filepath):
        """Write binary .kmesh file (compact)."""
        with open(filepath, 'wb') as f:
            # Header (32 bytes)
            f.write(self.MAGIC)                                     # [0-3] Magic
            f.write(struct.pack('I', self.VERSION))                 # [4-7] Version
            f.write(struct.pack('I', len(self.vertices)))           # [8-11] Vertex count
            f.write(struct.pack('I', len(self.triangles)))          # [12-15] Triangle count
            f.write(struct.pack('I', len(self.morphs)))             # [16-19] Morph count
            
            flags = 0
            if self.has_uvs: flags |= 0x01
            if self.has_normals: flags |= 0x02
            f.write(struct.pack('I', flags))                        # [20-23] Flags
            f.write(bytes(8))                                       # [24-31] Reserved
            
            # Vertices (12 bytes each)
            for v in self.vertices:
                f.write(struct.pack('fff', v[0], v[1], v[2]))
            
            # Triangles (12 bytes each)
            for tri in self.triangles:
                f.write(struct.pack('III', tri[0], tri[1], tri[2]))
            
            # Morph targets
            for name, data in self.morphs:
                # Name
                name_bytes = name.encode('utf-8')[:31]  # Max 31 chars
                f.write(struct.pack('B', len(name_bytes)))
                f.write(name_bytes)
                
                # Affected vertex count
                f.write(struct.pack('I', len(data)))
                
                # Delta data
                for idx, dx, dy, dz in data:
                    f.write(struct.pack('I', idx))
                    f.write(struct.pack('fff', dx, dy, dz))
            
            # End marker
            f.write(self.END_MARKER)
        
        print(f"Wrote binary {filepath}")
        print(f"  Vertices: {len(self.vertices)}")
        print(f"  Triangles: {len(self.triangles)}")
        print(f"  Morphs: {len(self.morphs)}")
        
        # Calculate file size
        size = os.path.getsize(filepath)
        print(f"  File size: {size} bytes ({size/1024:.2f} KB)")
    
    def write(self, filepath, format='binary'):
        """Write .kmesh file in specified format."""
        if format == 'ascii' or format == 'text':
            self.write_ascii(filepath)
        else:
            self.write_binary(filepath)


def convert_fbx_to_kmesh(fbx_path, output_path, use_fbx_converter=True, format='binary'):
    """Convert FBX to KoiloMesh (binary or ASCII)."""
    if not HAS_FBX_READER:
        print("Error: FBXReader not available. Cannot convert FBX files.")
        print("FBXReader.py must be present in the same directory as this script.")
        return False
    
    print(f"Converting FBX: {fbx_path}")
    
    # Convert to ASCII FBX if binary
    ascii_fbx = fbx_path
    if use_fbx_converter and fbx_path.endswith('.fbx'):
        ascii_fbx = fbx_path.replace('.fbx', '_ascii.fbx')
        print(f"  Converting to ASCII FBX...")
        
        # Check if FbxFormatConverter.exe exists
        converter_path = os.path.join(script_dir, "FbxFormatConverter.exe")
        if os.path.exists(converter_path):
            cmd = f'{converter_path} -c "{fbx_path}" -o "{ascii_fbx}" -ascii'
            os.system(cmd)
        else:
            print(f"  Warning: FbxFormatConverter.exe not found")
            print(f"  Assuming FBX is already ASCII...")
            ascii_fbx = fbx_path
    
    # Read ASCII FBX
    try:
        with open(ascii_fbx, 'r', encoding='utf-8', errors='ignore') as f:
            fbx_data = f.read()
    except Exception as e:
        print(f"Error reading FBX: {e}")
        return False
    
    # Parse FBX
    print(f"  Parsing FBX...")
    try:
        morph_obj = GetMorphObject(fbx_data, os.path.basename(fbx_path).replace('.fbx', ''), scale=10.0)
    except Exception as e:
        print(f"Error parsing FBX: {e}")
        return False
    
    # Convert to KoiloMesh
    writer = KoiloMeshWriter()
    writer.from_morph_object(morph_obj)
    writer.write(output_path, format=format)
    
    # Clean up temp file
    if ascii_fbx != fbx_path and os.path.exists(ascii_fbx):
        os.remove(ascii_fbx)
    
    return True


def convert_obj_to_kmesh(obj_path, output_path, format='binary'):
    """Convert OBJ to KoiloMesh (binary or ASCII, no morphs)."""
    print(f"Converting OBJ: {obj_path}")
    
    writer = KoiloMeshWriter()
    writer.from_obj_file(obj_path)
    writer.write(output_path, format=format)
    return True


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert FBX/OBJ to KoiloMesh format')
    parser.add_argument('input', help='Input file (.fbx or .obj)')
    parser.add_argument('-o', '--output', required=True, help='Output .kmesh file')
    parser.add_argument('--no-converter', action='store_true', 
                       help='Skip FbxFormatConverter (for ASCII FBX files)')
    parser.add_argument('--format', choices=['binary', 'ascii', 'text'], default='binary',
                       help='Output format: binary (compact) or ascii/text (readable). Default: binary')
    
    args = parser.parse_args()
    
    # Normalize format
    output_format = 'ascii' if args.format in ['ascii', 'text'] else 'binary'
    
    # Detect input type
    success = False
    if args.input.lower().endswith('.fbx'):
        success = convert_fbx_to_kmesh(args.input, args.output, 
                                        use_fbx_converter=not args.no_converter,
                                        format=output_format)
    elif args.input.lower().endswith('.obj'):
        success = convert_obj_to_kmesh(args.input, args.output, format=output_format)
    else:
        print(f"Error: Unsupported file type. Use .fbx or .obj")
        sys.exit(1)
    
    if success:
        print(f"\nConversion complete")
        print(f"  Output: {args.output}")
        print(f"\nTo use in KoiloScript:")
        print(f"  ASSETS {{")
        print(f"    model my_model = \"{args.output}\"")
        print(f"  }}")
    else:
        print(f"\nConversion failed")
        sys.exit(1)
