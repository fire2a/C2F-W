# Belgium wildfire instances for Cell2Fire-W

Four real 2025 Belgian wildfire events, packaged as Cell2Fire-W (Scott & Burgan)
instances in the same layout as the `vilo100` hands-on instance.

| Case | Site | Date | Observed area | Terrain |
|------|------|------|---------------|---------|
| Case1_Clinge | Clinge (coastal Flanders) | 2025-03-23 | 2.5 ha | flat, 1–12 m |
| Case2_Houffalize | Houffalize (Ardennes) | 2025-04-12 | 5.2 ha | hilly, 282–490 m |
| Case3_Arlon | Arlon / Aarlen | 2025-03-18 | 10.8 ha | 343–465 m |
| Case4_Maasmechelen | Maasmechelen (Limburg) | 2025-05-22 | 17.9 ha | 51–104 m |

All instances: 601×601 cells, 10 m resolution, CRS **EPSG:3812** (Belge Lambert 2008),
ignition at the grid centre (cell 180601) on the real ignition location.
Each `CaseN/ground_truth/observed_scar.*` holds the mapped fire perimeter for validation.

Moisture is set at run time with `--moisture-scenario DkLm`. Canopy layers
(`ccf/cbd/cbh`) are zero-filled placeholders (no inventory) — surface fire only.
