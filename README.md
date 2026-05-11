# L10: Herramientas de perfilado de programas

## Introducción

En el laboratorio anterior la medición de rendimiento se hizo con herramientas generales como `time`, `/usr/bin/time -v`, `clock_gettime` y `getrusage`. Esa aproximación permite saber cuánto tarda un programa y cuánta memoria consume, pero no responde con detalle una pregunta más profunda: dentro del programa, qué funciones concentran el trabajo y de dónde viene el consumo observado.

Este laboratorio introduce herramientas especializadas de perfilado para responder justamente esa pregunta. Se trabajará con cuatro programas en C que implementan la misma funcionalidad: reciben una lista de valores por `stdin`, la ordenan y reportan estadísticas básicas. La diferencia entre ellos no está en la salida, sino en su implementación interna:

- `profiling_001`
- `profiling_002`
- `profiling_003`
- `profiling_004`

En una primera idea de este laboratorio se consideró entregar los binarios ya compilados, pero eso limitaría el uso de `gprof` y haría más incómoda la preparación del entorno. Por esa razón, en esta versión los estudiantes compilarán directamente los programas desde su código fuente. En este contexto, conocer la implementación no reduce el valor pedagógico de la actividad: el objetivo no es "adivinar" qué programa debería ganar, sino justificar con evidencia experimental qué programa conviene más según tiempo de ejecución y comportamiento de memoria.

### Pre-requisitos

- Tener iniciada la máquina virtual Lubuntu y clonar este repositorio
- Tener disponible `gcc`, `gprof`, `perf`, `valgrind`, `awk`, `sed`, `head` y `bash`
- Haber trabajado previamente con terminal, compilación en C y redirección de entrada/salida
- Haber completado los laboratorios anteriores de medición de rendimiento y debugging con `valgrind`
- Entender la diferencia entre `stdout` y `stderr`

### Objetivo general

- Usar herramientas especializadas de perfilado para comparar cuatro implementaciones equivalentes y recomendar cuál conviene en distintos escenarios según tiempo de ejecución y memoria.

### Objetivos específicos

- Compilar programas en C con soporte de perfilado usando `-pg`
- Interpretar la salida de `gprof` en sus secciones de `flat profile` y `call graph`
- Medir contadores de ejecución con `perf stat`
- Inspeccionar hotspots de CPU con `perf record` y `perf report`
- Observar consumo de memoria con `massif`, distinguiendo heap y stack
- Relacionar el comportamiento medido con la estructura interna del programa
- Comparar el efecto de distintos tipos de input: aleatorio, ordenado, inverso y con muchos repetidos
- Formular una recomendación final sustentada en datos y no solo en intuición teórica

### Metodología

El laboratorio sigue una ruta de comparación guiada. Primero se compilan los cuatro programas desde su código fuente, generando dos familias de binarios:

- una familia con `-pg` para trabajar específicamente con `gprof`
- una familia sin `-pg` para realizar comparaciones de rendimiento y memoria sin el costo adicional de la instrumentación de `gprof`

Luego se generan inputs sintéticos de distintos tipos, se ejecutan los programas sobre esos mismos datasets y se observan sus perfiles con varias herramientas complementarias:

- `gprof` para atribución de tiempo por función
- `perf stat` para comparar costo total y contadores relevantes del procesador
- `perf record` y `perf report` para inspeccionar hotspots
- `massif` para distinguir comportamiento de heap y stack

La actividad no pide demostrar equivalencia funcional como objetivo central, porque ese punto ya fue trabajado en laboratorios anteriores. Aquí el foco está en observar, medir e interpretar.

### Estructura del repositorio

Este laboratorio usa una carpeta `code` dentro del propio repositorio:

- `code/profiling_001.c`
- `code/profiling_002.c`
- `code/profiling_003.c`
- `code/profiling_004.c`

Cada programa:

- lee un entero `n`
- luego lee `n` números reales
- ordena la lista
- imprime el `Id de programa` y un resumen estadístico

### Contexto

Un módulo de análisis del Imperio Galáctico debe ordenar grandes lotes de mediciones provenientes de sondas desplegadas en varios sistemas. Cuatro equipos de ingeniería entregaron prototipos distintos. Todos producen la misma salida observable, pero no todos consumen los mismos recursos.

El área de operaciones necesita una recomendación sustentada. No basta con afirmar que una versión es "mejor en teoría": se necesita evidencia experimental sobre tiempo, hotspots y memoria para decidir qué versión conviene usar según el escenario operativo.

## Actividad

### 1. Preparar el entorno y compilar los programas

Entrar a la carpeta `code` del laboratorio:

```bash
cd code
ls
```

Deberían aparecer los cuatro archivos fuente `profiling_001.c` a `profiling_004.c`. En esta actividad se compilarán dos familias de binarios:

- una familia con `-pg` para trabajar con `gprof`
- una familia sin `-pg` para trabajar con `perf`, `massif` y `callgrind`

Primero compilar la familia para `gprof`. Esta herramienta requiere que el programa haya sido compilado con `-pg`. Además conviene usar símbolos y sin optimizaciones fuertes, de modo que el reporte sea fácil de interpretar:

```bash
for id in 001 002 003 004; do
  gcc -Wall -Wextra -std=c11 -g -O0 -pg \
    -o profiling_${id} profiling_${id}.c
done
```

Las flags relevantes aquí son:

- `-g`: agrega símbolos para que los reportes muestren nombres de funciones y líneas con precisión
- `-O0`: evita optimizaciones que podrían hacer menos claro el perfil observado
- `-pg`: agrega la instrumentación necesaria para que el programa genere `gmon.out`

> **Importante**: `gprof` crea un archivo llamado `gmon.out` cada vez que se ejecuta el programa. Si se
> corre otro binario en la misma carpeta sin guardar o renombrar ese archivo, la medición anterior se
> pierde.

Luego compilar la familia para comparación de rendimiento y memoria. La instrumentación de `-pg` altera el costo de ejecución. Por eso, para `perf` y `massif` conviene usar otra familia de binarios sin `-pg`:

```bash
for id in 001 002 003 004; do
  gcc -Wall -Wextra -std=c11 -g \
    -o profiling_${id}_perf profiling_${id}.c
done
```

### 2. Generar datasets de prueba

Generar cuatro datasets grandes de tamaño `20000`, todos con el mismo formato de entrada, pero con distribuciones distintas. Para evitar problemas con el locale numérico de la máquina, se fuerza `LC_NUMERIC=C` en todos los comandos de generación:

```bash
LC_NUMERIC=C awk 'BEGIN { srand(42); n=20000; print n; for(i=1;i<=n;i++) printf "%.4f\n", rand()*100000 }' \
  > /tmp/random_20000.in

LC_NUMERIC=C awk 'BEGIN { n=20000; print n; for(i=1;i<=n;i++) printf "%.4f\n", i/10.0 }' \
  > /tmp/sorted_20000.in

LC_NUMERIC=C awk 'BEGIN { n=20000; print n; for(i=n;i>=1;i--) printf "%.4f\n", i/10.0 }' \
  > /tmp/reverse_20000.in

LC_NUMERIC=C awk 'BEGIN { n=20000; print n; for(i=1;i<=n;i++) printf "%.4f\n", (i % 17) * 10.0 }' \
  > /tmp/duplicates_20000.in
```

Para herramientas más pesadas como `massif`, también conviene generar un input intermedio:

```bash
LC_NUMERIC=C awk 'BEGIN { srand(42); n=5000; print n; for(i=1;i<=n;i++) printf "%.4f\n", rand()*100000 }' \
  > /tmp/random_5000.in
```

La razón de usar varios tipos de input es que una misma implementación puede comportarse distinto según la distribución de datos.

### 3. Primer contacto con gprof

Ejecutar una primera corrida de `profiling_001` con el input aleatorio:

```bash
./profiling_001 < /tmp/random_20000.in > /tmp/out_001.txt
mv gmon.out /tmp/gmon_001.out
gprof ./profiling_001 /tmp/gmon_001.out | head -n 40
```

El reporte de `gprof` tiene dos secciones principales:

- `Flat profile`: muestra cuánto tiempo se atribuye a cada función
- `Call graph`: muestra quién llama a quién y cómo se distribuye el costo entre funciones

En esta etapa conviene ubicar:

- qué función concentra la mayor parte del tiempo
- si esa función coincide con la rutina principal de ordenamiento
- cuántas llamadas aparecen y cómo se organiza la recursión o iteración

### 4. Ejecutar gprof sobre los cuatro programas

Repetir la corrida anterior para los cuatro programas usando el mismo dataset aleatorio:

```bash
for id in 001 002 003 004; do
  ./profiling_${id} < /tmp/random_20000.in > /tmp/out_${id}.txt
  mv gmon.out /tmp/gmon_${id}.out
  gprof ./profiling_${id} /tmp/gmon_${id}.out \
    > /tmp/gprof_${id}.txt
done
```

Revisar luego cada reporte:

```bash
head -n 40 /tmp/gprof_001.txt
head -n 40 /tmp/gprof_002.txt
head -n 40 /tmp/gprof_003.txt
head -n 40 /tmp/gprof_004.txt
```

Completar una tabla de observación como la siguiente:

| ID  | Función dominante | Observaciones del call graph |
|-----|-------------------|------------------------------|
| 001 | ---               | ---                          |
| 002 | ---               | ---                          |
| 003 | ---               | ---                          |
| 004 | ---               | ---                          |

La idea no es copiar el reporte completo, sino resumir las observaciones sobre la estructura de ejecución de cada programa.

### 5. Comparar costo total con perf stat

> ⚠️ **Importante**: Ejecutar el comando `sudo sysctl -w kernel.perf_event_paranoid=-1` en la misma consola de trabajo antes de comenzar con los pasos siguientes. La consola solicitará una clave que se informará en clases

Ahora usar la familia de binarios sin `-pg` para comparar tiempo y contadores de ejecución. Comenzar con el input aleatorio:

```bash
for id in 001 002 003 004; do
  /usr/lib/linux-tools-6.8.0-90/perf stat ./profiling_${id}_perf < /tmp/random_20000.in \
    > /dev/null 2> /tmp/perf_random_${id}.txt
done
```

Revisar los reportes generados:

```bash
head -n 20 /tmp/perf_random_001.txt
head -n 20 /tmp/perf_random_002.txt
head -n 20 /tmp/perf_random_003.txt
head -n 20 /tmp/perf_random_004.txt
```

En particular, observar los campos:

- `task-clock`
- `cycles`
- `instructions`
- `branches`
- `branch-misses`
- `seconds time elapsed`

Repetir la misma comparación para el input con muchos repetidos:

```bash
for id in 001 002 003 004; do
  /usr/lib/linux-tools-6.8.0-90/perf stat ./profiling_${id}_perf < /tmp/duplicates_20000.in \
    > /dev/null 2> /tmp/perf_duplicates_${id}.txt
done
```

Preguntas para guiar la interpretación:

- ¿El programa más rápido también ejecuta menos instrucciones?
- ¿Cambia el ranking cuando cambia la distribución del input?
- ¿Hay alguna implementación particularmente sensible a `branch-misses`?

> 📝 **Nota**: `perf` escribe su reporte por `stderr`. Por eso en los comandos anteriores la salida funcional del programa se manda a `/dev/null`, mientras que el reporte de `perf` se guarda en un archivo aparte

> 📝 **Nota sobre Lubuntu**: en algunas instalaciones Ubuntu/Lubuntu, `perf` puede existir pero estar restringido por permisos del kernel. Si aparece un mensaje del tipo `No permission to enable ...`, entonces el problema no es el programa, sino la configuración de la máquina. En la VM del curso esto debería estar resuelto de antemano

### 6. Observar hotspots con perf record y perf report

Elegir uno de los programas que haya parecido más interesante en `perf stat` y generar un perfil más detallado:

```bash
/usr/lib/linux-tools-6.8.0-90/perf record -g ./profiling_003_perf < /tmp/random_20000.in > /dev/null
/usr/lib/linux-tools-6.8.0-90/perf report --stdio | head -n 40
```

El flag `-g` agrega información de call graph. En el reporte interesa ubicar:

- la función más costosa
- qué porcentaje del tiempo total concentra
- si aparecen funciones auxiliares relevantes además de la rutina principal

Si se desea repetir el procedimiento con otro programa, el archivo `perf.data` se sobreescribe, por lo que conviene renombrarlo después de cada corrida:

```bash
mv perf.data /tmp/perf_003_random.data
```

### 7. Medir heap y stack con massif

Para distinguir uso de heap y stack, usar `massif`, la herramienta de perfilado de memoria de `valgrind`. Como es bastante más lenta que una ejecución normal, conviene usar el dataset intermedio:

```bash
valgrind --tool=massif --stacks=yes \
  --massif-out-file=/tmp/massif_001.out \
  ./profiling_001_perf < /tmp/random_5000.in > /dev/null

valgrind --tool=massif --stacks=yes \
  --massif-out-file=/tmp/massif_002.out \
  ./profiling_002_perf < /tmp/random_5000.in > /dev/null

valgrind --tool=massif --stacks=yes \
  --massif-out-file=/tmp/massif_003.out \
  ./profiling_003_perf < /tmp/random_5000.in > /dev/null

valgrind --tool=massif --stacks=yes \
  --massif-out-file=/tmp/massif_004.out \
  ./profiling_004_perf < /tmp/random_5000.in > /dev/null
```

Luego inspeccionar cada salida:

```bash
ms_print /tmp/massif_001.out | head -n 40
ms_print /tmp/massif_002.out | head -n 40
ms_print /tmp/massif_003.out | head -n 40
ms_print /tmp/massif_004.out | head -n 40
```

En Lubuntu, `ms_print` viene junto con la instalación de `valgrind`, por lo que no hace falta instalar una herramienta separada para esta parte.

En `ms_print`, los campos más útiles en este laboratorio son:

- `useful-heap`: memoria efectivamente usada por datos del programa
- `extra-heap`: sobrecosto administrativo del heap
- `stacks`: memoria usada por stack

Con esos reportes debería empezar a verse un patrón de diferencias entre programas. La tarea del laboratorio es describir ese patrón y cuantificarlo usando evidencia concreta.

### 8. Síntesis y recomendación final

Con la información reunida, completar una tabla de comparación final:

| Programa | Mejor caso observado | Tiempo | Heap/stack | Recomendación |
|----------|----------------------|--------|------------|---------------|
| 001      | ---                  | ---    | ---        | ---           |
| 002      | ---                  | ---    | ---        | ---           |
| 003      | ---                  | ---    | ---        | ---           |
| 004      | ---                  | ---    | ---        | ---           |

La conclusión esperada no debería ser una frase única del tipo "el mejor programa es X". En cambio, la recomendación debiera distinguir al menos:

- qué programa conviene si el criterio principal es tiempo de ejecución
- qué programa conviene si el criterio principal es memoria auxiliar
- si el tipo de input cambia la recomendación
- qué evidencia concreta respalda esa conclusión

## Actividad de desafío

Para quienes terminen temprano o quieran profundizar más, repetir una parte del análisis con `callgrind`:

```bash
valgrind --tool=callgrind \
  --callgrind-out-file=/tmp/callgrind_002.out \
  ./profiling_002_perf < /tmp/random_5000.in > /dev/null

callgrind_annotate /tmp/callgrind_002.out | head -n 40
```

En Lubuntu, `callgrind_annotate` viene con `valgrind`, por lo que esta parte del desafío puede hacerse completamente desde terminal.

Si además la máquina virtual tiene instalado `kcachegrind`, también se puede visualizar la salida con:

```bash
kcachegrind /tmp/callgrind_002.out
```

Comparar lo que muestra `callgrind` con lo observado en `gprof` y `perf`:

- ¿Coinciden las funciones dominantes?
- ¿Aparece más detalle en la estructura de llamadas?
- ¿Cambia la interpretación si se usa otro tipo de input?
