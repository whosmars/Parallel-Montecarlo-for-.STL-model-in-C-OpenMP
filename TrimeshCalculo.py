import trimesh
import sys
import time

start = time.perf_counter()
if len(sys.argv) < 2:
    print("Uso: python3 script.py archivo.stl")
    sys.exit(1)
mesh_path = sys.argv[1]
print(f"Archivo recibido por terminal: {mesh_path}")

mesh = trimesh.load(mesh_path)

print("¿Malla watertight?:", mesh.is_watertight)
print("Área superficial:", mesh.area)
try:
    print("Volumen:", mesh.volume)
    end = time.perf_counter()
    print(f"Tiempo transcurrido: {end - start:.6f} segundos")
except Exception as e:
    print("No se pudo calcular volumen:", e)
