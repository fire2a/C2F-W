#!/usr/bin/env bash
# ============================================================
# run_c2f.sh  —  Run Cell2Fire over N instances × N weather folders
# ============================================================

# ---- Binary -------------------------------------------------
BINARY="/Users/matiasvilches/Documents/F2A/source/C2F-W/Cell2Fire/Cell2Fire"

# ---- Output base folder -------------------------------------
OUT_BASE="/Users/matiasvilches/Documents/F2A/itrend_2/results"

# ---- Instance TIFs (one per line) ---------------------------
INSTANCES=(
    ""   # instance 1 path
    ""   # instance 2 path
    ""   # instance 3 path
    ""   # instance 4 path
)

# ---- Weather folders (one per line) -------------------------
WEATHER_FOLDERS=(
    ""   # weather folder 1 path
    ""   # weather folder 2 path
    ""   # weather folder 3 path
    ""   # weather folder 4 path
)

# ---- Flags common to all runs -------------------------------
COMMON_FLAGS=(
    --nthreads   1
    --nsims      1
    # --seed       123
    # --weather    rows
    # --output-messages
    # --stats
    # --cros
    # --ignitionsLog
    # --out-fl
    # --out-ros
    # --fuels-path  /path/to/fuels
)

# ============================================================
# Loop: instance[i] paired with weather_folder[i]
# ============================================================
for i in "${!INSTANCES[@]}"; do
    instance="${INSTANCES[$i]}"
    weather_folder="${WEATHER_FOLDERS[$i]}"

    if [[ -z "$instance" || -z "$weather_folder" ]]; then
        echo "Skipping empty entry at index $i" >&2
        continue
    fi

    instance_name=$(basename "$instance" .tif)
    weather_name=$(basename "$weather_folder")

    out_dir="${OUT_BASE}/${instance_name}/${weather_name}"
    mkdir -p "$out_dir"

    echo "============================================================"
    echo "Run $((i+1))"
    echo "Instance : $instance_name"
    echo "Weather  : $weather_name"
    echo "Output   : $out_dir"
    echo "============================================================"

    "$BINARY" \
        --instance-tif    "$instance" \
        --weather-folder  "$weather_folder" \
        --output-folder   "$out_dir" \
        "${COMMON_FLAGS[@]}"

    echo ""
done
