#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

//Aqui definimos la estructura de un vertice en R3
typedef struct {
    float x, y, z;
} Vec3;

//Definimos la estructura de un triangulo, que son 3 vertices, la normal y el bytecount
typedef struct {
    Vec3 normal;
    Vec3 v1, v2, v3;
    uint16_t attr;
} Triangle;

//Definimos la resta entre vectores
static Vec3 vec3_sub(Vec3 a, Vec3 b) {
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

//Definimos el producto cruz entre vectores, esto nos servira para el calculo de la normal
static Vec3 vec3_cross(Vec3 a, Vec3 b) {
    Vec3 r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

//Norma de un vector
static float vec3_length(Vec3 v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

//Calculamos el area del triangulo
static float triangle_area(const Triangle *t) {
    Vec3 a = vec3_sub(t->v2, t->v1);
    Vec3 b = vec3_sub(t->v3, t->v1);
    Vec3 c = vec3_cross(a, b);
    return 0.5f * vec3_length(c);
}

// Procesamiento de STL binario, devuelve 0 en caso de exito
// out_tris es el apuntador a arreglo malloc de Triangle
// out_count es el número de triángulos leídos
int load_stl_binary(const char *filename, Triangle **out_tris, uint32_t *out_count) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("No se pudo abrir el archivo");
        return 1;
    }

    //los primeros 80 bytes son la cabecera y nos dan igual
    unsigned char header[80];
    if (fread(header, 1, 80, f) != 80) {
        fprintf(stderr, "Error leyendo cabecera\n");
        fclose(f);
        return 2;
    }

    // El numero de triangulos en un archivo STL viene en uint32, aqui obtenemos ese dato
    uint32_t num_triangles = 0;
    if (fread(&num_triangles, sizeof(uint32_t), 1, f) != 1) {
        fprintf(stderr, "Error leyendo número de triángulos\n");
        fclose(f);
        return 3;
    }

    if (num_triangles == 0) {
        fprintf(stderr, "El STL indica 0 triángulos\n");
        fclose(f);
        return 4;
    }

    // Si malloc dice que no hay espacio entonces nuestro equipo de computo no es suficiente
    Triangle *triangles = (Triangle *) malloc(num_triangles * sizeof(Triangle));
    if (!triangles) {
        fprintf(stderr, "Sin memoria para %u triángulos\n", num_triangles);
        fclose(f);
        return 5;
    }

    //Procesamiento de todos los triangulos del archivo
    //Estructura general de cada triangulo: 3 floats: normal.x, normal.y, normal.z 3 floats: v1.x, v1.y, v1.z 3 floats: v2.x, v2.y, v2.z 3 floats: v3.x, v3.y, v3.z 1 uint16_t: attribute byte count
    for (uint32_t i = 0; i < num_triangles; i++) {
        float data[12];
        uint16_t attr;

        // obtenemos la normal y los vertices
        if (fread(data, sizeof(float), 12, f) != 12) {
            fprintf(stderr, "Error leyendo triángulo %u\n", i);
            free(triangles);
            fclose(f);
            return 6;
        }

        // attribute byte count
        if (fread(&attr, sizeof(uint16_t), 1, f) != 1) {
            fprintf(stderr, "Error leyendo atributo del triángulo %u\n", i);
            free(triangles);
            fclose(f);
            return 7;
        }

        triangles[i].normal.x = data[0];
        triangles[i].normal.y = data[1];
        triangles[i].normal.z = data[2];

        triangles[i].v1.x = data[3];
        triangles[i].v1.y = data[4];
        triangles[i].v1.z = data[5];

        triangles[i].v2.x = data[6];
        triangles[i].v2.y = data[7];
        triangles[i].v2.z = data[8];

        triangles[i].v3.x = data[9];
        triangles[i].v3.y = data[10];
        triangles[i].v3.z = data[11];

        triangles[i].attr = attr;
    }

    fclose(f);

    *out_tris  = triangles;
    *out_count = num_triangles;
    return 0;
}

// Programa principal de prueba
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s archivo.stl\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];

    Triangle *tris = NULL;
    uint32_t count = 0;

    int err = load_stl_binary(filename, &tris, &count);
    if (err != 0) {
        fprintf(stderr, "Error cargando STL (%d)\n", err);
        return 1;
    }

    printf("Triángulos leídos: %u\n", count);

    // Área total
    double total_area = 0.0;
    for (uint32_t i = 0; i < count; i++) {
        total_area += (double) triangle_area(&tris[i]);
    }
    printf("Área superficial aproximada: %.6f unidades^2\n", total_area);

    // Bounding box
    Vec3 min = tris[0].v1;
    Vec3 max = tris[0].v1;

    for (uint32_t i = 0; i < count; i++) {
        Vec3 vs[3] = { tris[i].v1, tris[i].v2, tris[i].v3 };
        for (int k = 0; k < 3; k++) {
            Vec3 v = vs[k];
            if (v.x < min.x) min.x = v.x;
            if (v.y < min.y) min.y = v.y;
            if (v.z < min.z) min.z = v.z;
            if (v.x > max.x) max.x = v.x;
            if (v.y > max.y) max.y = v.y;
            if (v.z > max.z) max.z = v.z;
        }
    }

    printf("Bounding box:\n");
    printf("  min = (%.6f, %.6f, %.6f)\n", min.x, min.y, min.z);
    printf("  max = (%.6f, %.6f, %.6f)\n", max.x, max.y, max.z);

    free(tris);
    return 0;
}
