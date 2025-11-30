#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <omp.h>

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

//Producto punto entre vectores
static float vec3_dot(Vec3 a, Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
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
        perror("The file couldnt be opened");
        return 1;
    }

    //los primeros 80 bytes son la cabecera y nos dan igual
    unsigned char header[80];
    size_t first_bytes = fread(header, 1, 80, f);
    if (first_bytes != 80) {
        fprintf(stderr, "There was a mistake while reading the header\n");
        fclose(f);
        return 2;
    }

    // El numero de triangulos en un archivo STL viene en uint32, aqui obtenemos ese dato
    uint32_t num_triangles = 0;
    size_t total_triangles = fread(&num_triangles, sizeof(uint32_t), 1, f);
    if ( total_triangles != 1) {
        fprintf(stderr, "An error occured while reading the triangles\n");
        fclose(f);
        return 3;
    }

    if (num_triangles == 0) {
        fprintf(stderr, "The STL doesnt has triangles\n");
        fclose(f);
        return 4;
    }

    // Si malloc dice que no hay espacio entonces nuestro equipo de computo no es suficiente
    Triangle *triangles = (Triangle *) malloc(num_triangles * sizeof(Triangle));
    if (!triangles) {
        fprintf(stderr, "Not enough memory to read all triangles\n");
        fclose(f);
        return 5;
    }

    //Estructura de cada triangulo: 3 floats: normal.x, normal.y, normal.z 3 floats: v1.x,
    //v1.y, v1.z 3 floats: v2.x, v2.y, v2.z 3 floats: v3.x, v3.y, v3.z 1 uint16_t: attribute byte count
    for (uint32_t i = 0; i < num_triangles; i++) {
        // obtenemos la normal y los vertices
        float data[12];
        size_t normal_tt = fread(data, sizeof(float), 12, f);
        if ( normal_tt != 12) {
            fprintf(stderr, "An error ocurred while processing the triangle %u\n", i);
            free(triangles);
            fclose(f);
            return 6;
        }

        //attribute byte count
        uint16_t attr;
        size_t byte_tt = fread(&attr, sizeof(uint16_t), 1, f);
        if ( byte_tt != 1) {
            fprintf(stderr, "An error ocurred while processing the atribbute %u\n", i);
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

    //Los valores que vamos a modificar
    *out_tris  = triangles;
    *out_count = num_triangles;
    return 0;
}

// Interseccion rayo-triángulo de Möller–Trumbore
// origin: punto donde nace el rayo
// dir: dirección del rayo, no necesita estar normalizada
// tri: current triangulo
// t_out: opcional, distancia a lo largo del rayo, si hay intersección
static int ray_intersects_triangle(Vec3 origin, Vec3 dir,
                                   const Triangle *tri,
                                   float *t_out)
{

    //los vertices y aristas del triangulo, esto en POO es acceder a las propiedades
    Vec3 v0 = tri->v1;
    Vec3 v1 = tri->v2;
    Vec3 v2 = tri->v3;
    Vec3 e1 = vec3_sub(v1, v0);
    Vec3 e2 = vec3_sub(v2, v0);

    //Algoritmo de Moller–Trumbore, primero calculamos producto punto y cruz
    Vec3 pvec = vec3_cross(dir, e2);
    float det = vec3_dot(e1, pvec);

    // valor absoluto si es 0, entonces es 0, no hay interseccion util
    const float ALMOSTZERO = 1e-6f;
    float determinant = fabsf(det);
    if (determinant < ALMOSTZERO){
      return 0;
    }

    //invertir el determinante
    float invDet = 1.0f / det;

    //es el vector que va de v0 hasta el origin del rayo
    Vec3 tvec = vec3_sub(origin, v0);
    //u se calcula con un producto punto y se escala por invDet
    float u = vec3_dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f){
      return 0;
    }
    //coeficiente baricentrico
    Vec3 qvec = vec3_cross(tvec, e1);
    float v = vec3_dot(dir, qvec) * invDet;
    if (v < 0.0f || (u + v) > 1.0f){
      return 0;
    }

    float t = vec3_dot(e2, qvec) * invDet;
    if (t < 0.0f){
      return 0; // intersección detrás del origen del rayo
    }

    if (t_out){
      *t_out = t;
    }

    return 1;
}


// ---------------------------------------------------------------------
// Estimación de volumen por Monte Carlo + ray casting
//
// tris   : arreglo de triángulos
// count  : número de triángulos
// min,max: bounding box (ya calculado en main)
// samples: número de puntos aleatorios a usar
//
// Devuelve volumen aproximado.
// ---------------------------------------------------------------------
static double monte_carlo_volume(const Triangle *tris, uint32_t count,
                                 Vec3 min, Vec3 max,
                                 uint32_t samples)
{
    // Volumen de la caja delimitadora
    double dx = (double)max.x - (double)min.x;
    double dy = (double)max.y - (double)min.y;
    double dz = (double)max.z - (double)min.z;
    double box_vol = dx * dy * dz;

    //caso donde algo previamente salio mal
    if (box_vol <= 0.0 || samples == 0){
      return 0.0;
    }

    // Dirección fija del rayo
    Vec3 dir;
    dir.x = 1.0f;
    dir.y = 0.0f;
    dir.z = 0.0f;

    //si inside count nos da impar = dentro, par = fuera
    uint32_t inside_count = 0;

    for (uint32_t i = 0; i < samples; ++i) {
        // Punto aleatorio uniforme dentro del bounding box
        float rx = (float)rand() / (float)RAND_MAX;
        float ry = (float)rand() / (float)RAND_MAX;
        float rz = (float)rand() / (float)RAND_MAX;

        Vec3 p;
        p.x = min.x + rx * (max.x - min.x);
        p.y = min.y + ry * (max.y - min.y);
        p.z = min.z + rz * (max.z - min.z);

        // Contar cuántas veces el rayo desde p intersecta la malla
        uint32_t intersections = 0;
        for (uint32_t j = 0; j < count; ++j) {
            float t;
            if (ray_intersects_triangle(p, dir, &tris[j], &t)) {
                intersections++;
            }
        }

        if (intersections % 2 == 1) {
            inside_count++;
        }
    }

    double ratio = (double)inside_count / (double)samples;
    return box_vol * ratio;
}

//Calculamos la bounding box
static void compute_bounding_box(const Triangle *tris, uint32_t count,
                                 Vec3 *min_out, Vec3 *max_out)
{

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

    *min_out = min;
    *max_out = max;
}

//calcula la sumatoria del area de todos los triangulos
double compute_total_area(const Triangle *tris, uint32_t count) {
    double total_area = 0.0;
    //Parallel
    #pragma omp parallel for reduction(+:total_area)
    for (uint32_t i = 0; i < count; i++) {
        total_area += (double) triangle_area(&tris[i]);
    }

    return total_area;
}


// Programa principal de prueba
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s archivo.stl\n", argv[0]);
        return 1;
    }

    double start = omp_get_wtime(); // Tiempo inicial

    //Esta es la semilla del monecarlo
    srand((unsigned) time(NULL));

    const char *filename = argv[1];

    Triangle *tris = NULL;
    uint32_t count = 0;

    int err = load_stl_binary(filename, &tris, &count);
    if (err != 0) {
        fprintf(stderr, "Error cargando STL (%d)\n", err);
        return 1;
    }
    printf("Triángulos leídos: %u\n", count);

    //Area total
    double total_area = compute_total_area(tris, count);
    printf("Área superficial aproximada: %.6f unidades^2\n", total_area);

    // Bounding box
    Vec3 min = tris[0].v1;
    Vec3 max = tris[0].v1;
    compute_bounding_box(tris, count, &min, &max);

    //numero de muestras para Monte Carlo
    uint32_t samples = 100000;

    double vol_mc = monte_carlo_volume(tris, count, min, max, samples);

    double end = omp_get_wtime(); // Tiempo final

    printf("Volumen aproximado (Monte Carlo, %u muestras): %.6f unidades^3\n",
       samples, vol_mc);
    printf("Bounding box:\n");
    printf("  min = (%.6f, %.6f, %.6f)\n", min.x, min.y, min.z);
    printf("  max = (%.6f, %.6f, %.6f)\n", max.x, max.y, max.z);

    printf("Tiempo total de ejecucion: %.4f segundos\n", end - start);

    free(tris);

    return 0;
}
