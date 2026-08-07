# Portillo (Chile) — instancia KITRAL con río vectorial y cicatriz observada

Incendio **EL PORTILLO**, comuna de San Fernando, provincia de Colchagua, Región de
O'Higgins. Ignición **27-01-2021 14:52**, controlado **31-01-2021 17:40**, **866 ha**.

Extiende `data/Kitral/Portillo-asc` con topografía derivada, el río como capa vectorial
y la cicatriz observada, de modo que sirva como caso de validación y no sólo de demo.

## Extensión y CRS

Rasters en **EPSG:32719** (WGS84 / UTM 19S), 500×500 celdas de 30 m,
X[335984, 350984] Y[6142449, 6157449]. Los `.prj` heredados rotulan "WGS 84" pero las
coordenadas son UTM 19S; es un etiquetado impreciso del origen, no un error del dato.

## Correr

Ignición observada, río como barrera, clima de las 8 h posteriores a la ignición:

    Cell2Fire --input-instance-folder Portillo-rio-asc/ --output-folder OUT/ \
              --sim K --lb-mode sb --nsims 1 --nweathers 1 \
              --Fire-Period-Length 5.0 --Weather-Period-Length 60 --max-fire-periods 200 \
              --ignition-shp Portillo-rio-asc/ignicion_1.shp \
              --rivers --breach-factor 2 --final-grid --seed 1

`--rivers` carga `Rivers/*.shp`. Sin `--breach-factor` el río detiene el fuego.

## Capas

| archivo | origen |
|---|---|
| `fuels.asc` | KITRAL. **Modificado**, ver abajo |
| `fuels_original.asc` | el original sin modificar, para trazabilidad |
| `elevation.asc`, `cbd/cbh/ccf/hm.asc` | de la instancia original |
| `slope.asc`, `saz.asc` | derivados de `elevation.asc` por el método de Horn (pendiente en %, azimut aguas arriba). No venían en el original y KITRAL los necesita para usar la topografía |
| `Rivers/rio_claro.shp` | Río Claro, polígono de ancho variable. Ver abajo |
| `rio_claro_eje.shp` | el eje del que deriva el polígono, tal cual viene de la fuente |
| `ignicion_1.shp` | ignición observada, 14:52 |
| `ignicion_2.gpkg` | segundo foco observado, 3 h después, al oriente del río |
| `Incendio_real/FireScar_UTM19S.shp` | cicatriz observada, reproyectada a UTM 19S |
| `Incendio_real/FireScar_CL-BI_...shp` | la misma en EPSG:4326, como se recibió |

### El río

Geometría del **Río Claro** tomada de la Red Hidrográfica de Chile de **IDE Chile**
(escala 1:25.000, licencia CC-BY), recortada a la ventana y reproyectada de Web Mercator
a UTM 19S. Es el único cauce que separa las dos igniciones observadas: cruza la recta que
las une y pasa a 120 m de la segunda.

El polígono se generó por buffer del eje con **ancho variable de 30 m aguas abajo (norte)
a 10 m en cabeceras (sur)**, siguiendo el rango de anchos reportado en terreno y el hecho
de que el Río Claro fluye al noroeste.

### La modificación de `fuels.asc`

El raster original marcaba como no combustible una franja de mediana **90 m** a lo largo
del cauce. Esa franja es un artefacto de digitalización: sobredetermina el ancho real del
río entre 3 y 9 veces, y actúa como barrera aunque no se declare ninguna.

Se devolvieron a combustible las **1733 celdas** no combustibles situadas a menos de 120 m
del eje, asignando a cada una el tipo mayoritario de su vecindario quemable (dominó el
tipo 7, con aportes de 21, 6 y 16). Quedan 12 941 celdas no combustibles legítimas de las
14 674 originales.

Efecto: sin río declarado el fuego pasa de 10 410 a 15 313 celdas quemadas, confirmando
que la franja estaba actuando de barrera implícita.

**El propósito es que la barrera sea explícita**: se declara con `--rivers` y su ancho es
el real, no el de la rasterización.

### El clima

- `Weather.csv` — 15:00 a 20:00, viento del NNW virando al N
- `Weather_rio.csv` — 19:00 a 22:00, viento del NNW sostenido
- `Weather_unido.csv` — las 4 h de `Weather.csv` (15:00–18:00) más las 4 de
  `Weather_rio.csv` (19:00–22:00)

Los dos archivos se solapan en 19:00 y 20:00 con vientos distintos. `Weather_unido.csv`
toma la versión de `Weather_rio.csv` para esas horas, asumiendo que corresponde a la fase
posterior al cruce del río. **Es una interpretación, no un dato.**

## Validación contra la cicatriz observada

Cicatriz observada: 9642 celdas, 868 ha. Simulando desde **una sola ignición** con
`Weather_unido.csv`:

| configuración | ha | Jaccard | sensibilidad | precisión |
|---|---|---|---|---|
| `--lb-mode kitral` (default), sin río | 1383 | 0.370 | 0.70 | 0.44 |
| `--lb-mode sb`, sin río | 562 | 0.423 | 0.49 | 0.76 |
| `--lb-mode sb --rivers` | 407 | 0.391 | 0.41 | 0.88 |
| `--lb-mode sb --rivers --breach-factor 2` | 541 | 0.432 | 0.49 | 0.79 |
| idem, con 10 h de clima | 1074 | **0.522** | 0.77 | 0.62 |

Tres cosas que conviene saber antes de usar esta instancia para calibrar:

**El coeficiente largo/ancho domina el error de forma.** La formulación nativa de KITRAL
da L/B = 1.56 a 28 km/h, o sea un incendio casi circular, y sobrestima los flancos. Con
`--lb-mode sb` (Anderson) el área simulada cae de 1383 a 562 ha y la precisión sube de
0.44 a 0.76.

**El perímetro observado equivale a unas 10 h de propagación libre**, no a los 4 días que
el incendio ardió. Con 16 h el modelo captura el 100 % de la cicatriz pero quema 3038 ha.
La diferencia la explica la supresión, que el simulador no modela.

**Ninguna combinación de parámetros supera J ≈ 0.52.** Barrer `--fch-mode`, `--HFactor`,
`--breach-factor` o `--spot-factor` no mejora el ajuste. Calibrar más allá de eso sería
hacer que los parámetros absorban la ausencia de un modelo de supresión.

## Atribución

- Hidrografía: **IDE Chile**, Red Hidrográfica 1:25.000, licencia CC-BY.
- Cicatriz observada y rasters de combustible: ver la instancia original
  `data/Kitral/Portillo-asc` y la fuente de la cicatriz (`FireScar_CL-BI_IDnull_u330`).
