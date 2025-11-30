# Compilador
CC      := gcc

# Flags de compilación (con OpenMP)
CFLAGS  := -O4 -Wall -Wextra -std=c11 -fopenmp

# Flags de link (math)
LDFLAGS := -lm

# Ejecutables
TARGET_PAR := bin/lector-parallel
TARGET_SEQ := bin/lector-single

# Fuentes
SRC_PAR := src/Montecarlo-Parallel.c
SRC_SEQ := src/Montecarlo-Single-Thread.c

# Regla por defecto: compilar ambos
all: $(TARGET_PAR) $(TARGET_SEQ)

$(TARGET_PAR): $(SRC_PAR) | bin
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TARGET_SEQ): $(SRC_SEQ) | bin
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

bin:
	mkdir -p bin

clean:
	rm -f $(TARGET_PAR) $(TARGET_SEQ)

.PHONY: all clean
