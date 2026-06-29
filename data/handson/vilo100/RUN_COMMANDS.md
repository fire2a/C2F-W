# Cell2Fire-W (S&B continuo) — instancia `vilo100` y comandos

Buffer **100×100** recortado de `data/ScottAndBurgan/Vilopriu_2013-asc`
(508×610, celda 20 m), centrado en la ignición original. Incluye **todas las
capas**, incluidas las de fuego de copas (`cbd`, `cbh`, `ccf`, `fcc`).

## Contenido

```
vilo100/
├── fuels.asc          # modelos Scott & Burgan
├── elevation.asc      # elevación (m)
├── slope.asc          # pendiente (%)  ── usada por la pendiente FARSITE
├── saz.asc            # azimut upslope ── usada por la pendiente FARSITE
├── cbd.asc            # canopy bulk density (copa)
├── cbh.asc            # canopy base height (copa)
├── ccf.asc            # canopy cover fraction (sombreado/WAF protegido)
├── fcc.asc            # canopy fuel / cobertura
├── Ignitions.csv      # ignición al centro del buffer (celda 5051)
├── spain_lookup_table.csv
├── Data.csv           # generado desde las capas (borrar para regenerar)
├── Weather.csv             # = Weather_scenario.csv (por defecto)
├── Weather_scenario.csv    # FireScenario (DkLk heredado)
├── Weather_direct.csv      # humedad continua m1h..mlw (%)
├── Weather_ffmc.csv        # FFMC
└── Weather_conditioning.csv# T(°C), RH(%)
```

> Cada modo usa un `Weather.csv` con columnas distintas. Para cambiar de modo,
> copia la variante deseada sobre `Weather.csv` (ver comandos abajo).

## Compilación (IMPORTANTE)

Se agregó el campo `MoistureMode` al struct `arguments` (`ReadArgs.h`). El Makefile
no rastrea dependencias de headers, así que hay que recompilar limpio:

```bash
cd C2F-W/Cell2Fire
make clean && make
```

(Un rebuild parcial deja `Cell2Fire.o` con el struct viejo → corrupción de heap
`free(): invalid pointer` en el cierre del programa.)

## Parámetros comunes

| Flag | Significado |
|---|---|
| `--sim S` | módulo Scott & Burgan (Rothermel continuo) |
| `--moisture-mode <m>` | `direct` \| `scenario` \| `ffmc` \| `conditioning` |
| `--nsims 1` | nº de simulaciones |
| `--nweathers 1` | nº de archivos de clima |
| `--Fire-Period-Length 5.0` | paso de propagación (min) |
| `--Weather-Period-Length 60` | duración de cada fila de clima (min) |
| `--max-fire-periods 84` | nº máximo de pasos (84×5 min = 7 h) |
| `--ignitions` | usar `Ignitions.csv` |
| `--seed 1` | semilla |

Salidas: `--out-ros` (RateOfSpread/), `--final-grid` (Grids/Grids1/ForestGrid0.csv,
matriz 0/1), `--out-fl` (SurfaceFlameLength/), `--out-intensity` (SurfaceIntensity/),
`--out-crown`, `--grids`. La **pendiente FARSITE** (slope.asc + saz.asc + combinación
vectorial viento-pendiente) actúa automáticamente en cualquier modo.

---

## Comandos por modo (desde `C2F-W/Cell2Fire/`)

Reemplaza `RUTA/vilo100` por la ruta a la instancia y `Cell2Fire` por tu binario.

### 1) `scenario` — DkLk heredado (formato original Vilopriu)

```bash
cp RUTA/vilo100/Weather_scenario.csv RUTA/vilo100/Weather.csv
./Cell2Fire --input-instance-folder RUTA/vilo100/ --output-folder RUTA/out_scenario/ \
  --sim S --moisture-mode scenario \
  --nsims 1 --nweathers 1 --Fire-Period-Length 5.0 --Weather-Period-Length 60 \
  --max-fire-periods 84 --ignitions --out-ros --final-grid --seed 1
# -> 6050 celdas quemadas (60.50%)
```

### 2) `direct` — humedad continua leída de columnas (modo 1)

```bash
cp RUTA/vilo100/Weather_direct.csv RUTA/vilo100/Weather.csv
./Cell2Fire --input-instance-folder RUTA/vilo100/ --output-folder RUTA/out_direct/ \
  --sim S --moisture-mode direct \
  --nsims 1 --nweathers 1 --Fire-Period-Length 5.0 --Weather-Period-Length 60 \
  --max-fire-periods 84 --ignitions --out-ros --final-grid --seed 1
# -> 6050 (idéntico a scenario: D1L1 == escenario 1)
```

### 3) `ffmc` — humedad fina invertida desde FFMC (modo 2a)

```bash
cp RUTA/vilo100/Weather_ffmc.csv RUTA/vilo100/Weather.csv
./Cell2Fire --input-instance-folder RUTA/vilo100/ --output-folder RUTA/out_ffmc/ \
  --sim S --moisture-mode ffmc \
  --nsims 1 --nweathers 1 --Fire-Period-Length 5.0 --Weather-Period-Length 60 \
  --max-fire-periods 84 --ignitions --out-ros --final-grid --seed 1
# FFMC=92 -> m1h~8.7% (más húmedo) -> 3070 (30.70%)
```

### 4) `conditioning` — EMC (Simard) + relajación por time-lag (modo 2b)

```bash
cp RUTA/vilo100/Weather_conditioning.csv RUTA/vilo100/Weather.csv
./Cell2Fire --input-instance-folder RUTA/vilo100/ --output-folder RUTA/out_conditioning/ \
  --sim S --moisture-mode conditioning \
  --nsims 1 --nweathers 1 --Fire-Period-Length 5.0 --Weather-Period-Length 60 \
  --max-fire-periods 84 --ignitions --out-ros --final-grid --seed 1
# T~16-18°C, RH~30-45% -> EMC~6% -> 3501 (35.01%)
```

---

## Notas

- Si modificas alguna capa `.asc`, **borra `Data.csv`** para que se regenere.
- `--moisture-mode` solo aplica a `--sim S`.
- `direct` sin la flag es el comportamiento por defecto (retrocompatible: si el
  Weather tiene ≥9 columnas usa humedad continua; si tiene `FireScenario`, DkLk).
- Para leer el ROS de cabeza de la salida:
  `awk 'NR>6{for(i=1;i<=NF;i++)if($i+0>0)print $i}' out_*/RateOfSpread/ROSFile1.asc | sort -un | tail -1`
