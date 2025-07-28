#!/usr/bin/env python3
"""
STL to OBJ converter that preserves original positioning (no centering)
"""

import struct
import sys
import os
from typing import List, Tuple

def read_stl_binary(filepath: str) -> Tuple[List[Tuple[float, float, float]], List[Tuple[int, int, int]]]:
    """Read binary STL file and return vertices and faces"""
    vertices = []
    faces = []
    vertex_map = {}
    vertex_index = 0
    
    with open(filepath, 'rb') as f:
        # Skip 80-byte header
        f.read(80)
        
        # Read number of triangles
        num_triangles = struct.unpack('<I', f.read(4))[0]
        
        for i in range(num_triangles):
            # Skip normal vector (12 bytes)
            f.read(12)
            
            face_indices = []
            
            # Read 3 vertices (9 floats total)
            for j in range(3):
                x, y, z = struct.unpack('<fff', f.read(12))
                x, y, z = -y, z, -x
                vertex = (x / 100, y / 100, z / 100)
                
                # Check if vertex already exists
                if vertex not in vertex_map:
                    vertex_map[vertex] = vertex_index
                    vertices.append(vertex)
                    vertex_index += 1
                
                face_indices.append(vertex_map[vertex])
            
            faces.append(tuple(face_indices))
            
            # Skip attribute byte count (2 bytes)
            f.read(2)
    
    return vertices, faces

def is_binary_stl(filepath: str) -> bool:
    """Determine if STL file is binary or ASCII"""
    with open(filepath, 'rb') as f:
        # Read first 80 bytes (header)
        header = f.read(80)
        
        # Check if header contains "solid" (common in ASCII)
        if b'solid' in header.lower():
            # Read a bit more to be sure
            f.seek(0)
            try:
                first_line = f.read(200).decode('utf-8', errors='ignore').lower()
                if first_line.strip().startswith('solid'):
                    return False  # ASCII
            except:
                pass
        
        return True  # Binary

def write_obj(vertices: List[Tuple[float, float, float]], 
              faces: List[Tuple[int, int, int]], 
              output_path: str):
    """Write vertices and faces to OBJ file"""
    with open(output_path, 'w') as f:
        f.write("# OBJ file converted from STL\n")
        f.write("# No centering applied - preserves original coordinates\n\n")
        
        # Write vertices
        for vertex in vertices:
            f.write(f"v {vertex[0]:.6f} {vertex[1]:.6f} {vertex[2]:.6f}\n")
        
        f.write("\n")
        
        # Write faces (OBJ uses 1-based indexing)
        for face in faces:
            f.write(f"f {face[0]+1} {face[1]+1} {face[2]+1}\n")

def convert_stl_to_obj(input_path: str, output_path: str = None):
    """Convert STL file to OBJ format without centering"""
    
    if not os.path.exists(input_path):
        raise FileNotFoundError(f"Input file not found: {input_path}")
    
    if output_path is None:
        base_name = os.path.splitext(input_path)[0]
        output_path = f"{base_name}.obj"
    
    print(f"Converting {input_path} to {output_path}")
    
    # Determine file type and read accordingly
    if is_binary_stl(input_path):
        print("Detected binary STL format")
        vertices, faces = read_stl_binary(input_path)
    else:
        print("Detected ASCII STL format")
        vertices, faces = read_stl_ascii(input_path)
    
    print(f"Found {len(vertices)} unique vertices and {len(faces)} faces")
    
    # Write OBJ file
    write_obj(vertices, faces, output_path)
    print(f"Conversion complete: {output_path}")

def main():
    """Main function for command line usage"""
    if len(sys.argv) < 2:
        print("Usage: python stl_to_obj.py <input.stl> [output.obj]")
        print("If output filename is not specified, it will use the input name with .obj extension")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    try:
        convert_stl_to_obj(input_file, output_file)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
