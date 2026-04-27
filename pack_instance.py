#!/usr/bin/env python3
"""Pack all rasters from a Cell2Fire instance folder into a single multi-band GeoTIFF.

Band layout (0-indexed, 1-indexed in rasterio):
  0 / band 1   fuels          (integer fuel type IDs stored as float32)
  1 / band 2   elevation      (m)
  2 / band 3   saz            (slope azimuth, degrees)
  3 / band 4   slope          (%)
  4 / band 5   cur            (curing level)
  5 / band 6   cbd            (canopy bulk density)
  6 / band 7   cbh            (canopy base height, m)
  7 / band 8   ccf            (canopy cover fraction)
  8 / band 9   probabilityMap (ignition probability)
  9 / band 10  fmc            (foliar moisture content)
  10 / band 11 hm             (tree height, m)

Missing layers are filled with the NODATA value (-9999).  The output uses
PLANARCONFIG_SEPARATE (interleave='band'), float32 dtype, and DEFLATE
compression so C2F can read each band with a single TIFFReadScanline(...,
sample) call.

Usage:
    python pack_instance.py <instance_folder> [-o output.tif]
"""
import argparse
import sys
from pathlib import Path

import numpy as np

try:
    import rasterio
    from rasterio.transform import from_bounds
except ImportError:
    print("Error: rasterio is required.  Install with:  pip install rasterio", file=sys.stderr)
    sys.exit(1)


NODATA = -9999.0

BANDS = [
    "fuels",
    "elevation",
    "saz",
    "slope",
    "cur",
    "cbd",
    "cbh",
    "ccf",
    "probabilityMap",
    "fmc",
    "hm",
]


def _find_raster(folder: Path, name: str) -> "Path | None":
    for ext in (".tif", ".asc"):
        p = folder / (name + ext)
        if p.exists():
            return p
    return None


def _read_raster(path: Path) -> "tuple[np.ndarray, dict]":
    """Return (float32 2-D array with NODATA for missing values, rasterio profile)."""
    with rasterio.open(path) as src:
        data = src.read(1).astype(np.float32)
        nodata = src.nodata
        if nodata is not None:
            data[data == nodata] = NODATA
        return data, src.profile


def pack(instance_folder: Path, output: Path, skip: "set[str] | None" = None) -> None:
    """Always writes all 11 bands.  Bands in *skip* are filled with NODATA
    instead of being read from disk (the file is ignored even if it exists)."""
    folder = instance_folder.resolve()
    arrays: "list[np.ndarray]" = []
    ref_profile: "dict | None" = None
    skip = skip or set()

    for band_name in BANDS:
        path = None if band_name in skip else _find_raster(folder, band_name)
        if path is None:
            if ref_profile is None:
                if band_name == "fuels":
                    raise RuntimeError(
                        f"'fuels.tif' or 'fuels.asc' not found in {folder}. "
                        "The fuels raster is required."
                    )
                raise RuntimeError(
                    "Could not find any raster to establish grid dimensions."
                )
            reason = "skipped" if band_name in skip else "not found"
            print(f"  {band_name:<18} {reason} — filling with {NODATA}")
            data = np.full(
                (ref_profile["height"], ref_profile["width"]), NODATA, dtype=np.float32
            )
        else:
            print(f"  {band_name:<18} {path.name}")
            data, profile = _read_raster(path)
            if ref_profile is None:
                ref_profile = profile

        arrays.append(data)

    if ref_profile is None:
        raise RuntimeError("No rasters found in the instance folder.")

    out_profile = {
        "driver": "GTiff",
        "dtype": "float32",
        "width": ref_profile["width"],
        "height": ref_profile["height"],
        "count": len(BANDS),
        "crs": ref_profile.get("crs"),
        "transform": ref_profile.get("transform"),
        "nodata": NODATA,
        "compress": "deflate",
        "interleave": "band",  # → PLANARCONFIG_SEPARATE
    }

    with rasterio.open(output, "w", **out_profile) as dst:
        for i, arr in enumerate(arrays, start=1):
            dst.write(arr, i)
            dst.update_tags(i, band_name=BANDS[i - 1])

    print(
        f"\nWrote: {output}\n"
        f"       {len(BANDS)} bands, "
        f"{out_profile['width']} x {out_profile['height']} cells"
    )


def main() -> None:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("instance_folder", help="Path to the Cell2Fire instance folder")
    ap.add_argument(
        "-o",
        "--output",
        help="Output GeoTIFF path (default: <instance_folder>/instance.tif)",
    )
    ap.add_argument(
        "--no-prob-map",
        action="store_true",
        help="Exclude the probabilityMap band from the output TIF",
    )
    args = ap.parse_args()

    folder = Path(args.instance_folder)
    if not folder.is_dir():
        print(f"Error: '{folder}' is not a directory", file=sys.stderr)
        sys.exit(1)

    output = Path(args.output) if args.output else folder / "instance.tif"

    skip = {"probabilityMap"} if args.no_prob_map else set()

    print(f"Packing instance: {folder}")
    pack(folder, output, skip)


if __name__ == "__main__":
    main()
