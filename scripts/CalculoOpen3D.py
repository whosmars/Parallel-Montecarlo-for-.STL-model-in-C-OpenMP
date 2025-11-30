import open3d as o3d
import numpy as np
import sys
import time

start = time.perf_counter()

if len(sys.argv) < 2:
    print("Uso: python3 script.py archivo.stl")
    sys.exit(1)
mesh_path = sys.argv[1]
print(f"Archivo recibido por terminal: {mesh_path}")

# 1. Cargar la malla STL
mesh = o3d.io.read_triangle_mesh(mesh_path)
if not mesh.has_triangles():
    raise RuntimeError(f"La malla '{mesh_path}' no tiene triángulos.")
print(f"Archivo cargado: {mesh_path}")

# limpieza básica de la malla
mesh.remove_duplicated_vertices()
mesh.remove_duplicated_triangles()
mesh.remove_degenerate_triangles()
mesh.remove_non_manifold_edges()
mesh.remove_unreferenced_vertices()

# opcional orientar triángulos de forma consistente)
mesh.orient_triangles()

# checar info básica de la malla
print(f"Número de vértices  : {len(mesh.vertices)}")
print(f"Número de triángulos: {len(mesh.triangles)}")
print("¿La malla es watertight?:", mesh.is_watertight())

# area superficial
area = mesh.get_surface_area()
print(f"Área superficial ≈ {area} unidades²")

# volumen (si tu Open3D lo soporta)
try:
    if mesh.is_watertight():
        volume = mesh.get_volume()
        print(f"Volumen ≈ {volume} unidades³")
        end = time.perf_counter()
        print(f"Tiempo transcurrido: {end - start:.6f} segundos")
    else:
        print("La malla NO es watertight, evitando get_volume().")
        end = time.perf_counter()
        print(f"Tiempo transcurrido: {end - start:.6f} segundos")
except Exception as e:
    print("No se pudo calcular el volumen:", e)
