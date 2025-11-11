# Frequency Assignment Problem (FAP) - Tabu Search

## Descripción
Implementación de Búsqueda Tabú para resolver el Problema de Asignación de Frecuencias (FAP) en redes GSM, basado en las instancias benchmark COST259.

## Archivos

- `main.cpp`: Código fuente principal
- `*.scen`: Instancias del problema (formato COST259)
- `*_log.txt`: Archivos de log generados por cada ejecución
- `*_solution.txt`: Soluciones encontradas

## Compilación

### Opción 1: Usando Makefile
```bash
make
```

## Ejecución

### Sintaxis
```bash
./fap_solver <archivo.scen>
```

### Ejemplos
```bash
# Instancia pequeña (rápida)
./fap_solver Tiny.scen

# Instancia mediana
./fap_solver Swisscom.scen

# Instancia grande
./fap_solver siemens1.scen
```

### Limpiar archivos compilados
```bash
make clean          # Solo elimina el ejecutable
make cleanall       # Elimina ejecutable + logs + soluciones
```

## Características

### Restricciones Duras
1. **Demanda exacta**: Cada celda debe tener exactamente el número de TRXs especificado
2. **Dominio válido**: Las frecuencias asignadas deben pertenecer al dominio permitido
3. **Separación co-site**: Las frecuencias en el mismo sitio deben estar separadas al menos por CO_SITE_SEPARATION

### Función Objetivo
Minimizar la interferencia total:
- Interferencia co-channel: Cuando dos celdas usan la misma frecuencia
- Interferencia adjacent-channel: Cuando dos celdas usan frecuencias adyacentes

### Algoritmo: Tabu Search con Candidate List

La implementación utiliza una **estrategia de Candidate List**, que combina:
- **Generación aleatoria**: Se genera un subset de movimientos candidatos de forma aleatoria
- **Mejor Mejora**: Se evalúan TODOS los candidatos y se selecciona el mejor
- **Control de complejidad**: Solo se evalúan CANDIDATE_LIST_SIZE movimientos por iteración

**Ventajas sobre Primera Mejora:**
- Mejor calidad de soluciones (explora más antes de decidir)
- Complejidad controlada (no evalúa todo el vecindario)
- Balance entre intensificación y diversificación

**Parámetros:**
- MAX_ITER: 10000 iteraciones
- TABU_SIZE: 15 (tenure de la lista tabú)
- CANDIDATE_LIST_SIZE: 200 (candidatos evaluados por iteración)

## Instancias Soportadas

| Instancia | Celdas | TRXs | Relaciones |
|-----------|--------|------|------------|
| Tiny.scen | 7 | 12 | 12 |
| K.scen | 264 | 267 | 27,123 |
| Swisscom.scen | 148 | 310 | 535 |
| siemens1.scen | 506 | 930 | 20,524 |
| siemens2.scen | 254 | 977 | 31,032 |
| siemens3.scen | 894 | 1,623 | 65,371 |
| siemens4.scen | 760 | 2,785 | 113,658 |

## Formato de Salida

### Log (*_log.txt)
Contiene información detallada de la ejecución:
- Parámetros de la instancia
- Evolución del costo por iteración
- Resultado final (factibilidad)

### Solución (*_solution.txt)
Contiene la asignación final de frecuencias:
- Costo total
- Estado de factibilidad
- Tabla con asignaciones: Celda → TRX → Frecuencia