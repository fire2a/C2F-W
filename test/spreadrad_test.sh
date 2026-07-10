#!/bin/bash
set -euo pipefail

# Focused regression check for the spread neighborhood radius (--SpreadRad).
# $1 is the binary suffix (same convention as test/test.sh).
#
# Invariants:
#   * default run == "--SpreadRad 1" (radius 1 is the legacy default)
#   * "--SpreadRad 2" is reported and burns at least as many cells as radius 1

BIN="../Cell2Fire/Cell2Fire${1:-}"
if [ ! -x "$BIN" ]; then
  echo "Binary not found or not executable: $BIN"
  exit 1
fi

WORKDIR="spreadrad_test_results"
rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"

run_case () {
  local label="$1"
  local spread_arg="$2"
  local out="$WORKDIR/$label"
  mkdir -p "$out"
  # Keep this short but deterministic.
  "$BIN" \
    --input-instance-folder model/kitral-asc \
    --output-folder "$out" \
    --nsims 30 \
    --max-fire-periods 60 \
    --sim K \
    --seed 123 \
    --ignitionsLog \
    $spread_arg > "$WORKDIR/$label.log"
}

# Per-simulation burned-cell counts, as a comma-separated sequence.
# C2F-W prints a per-sim summary table with a "Burnt   N   X%" row.
burned_sequence () {
  local logfile="$1"
  grep -E '^[[:space:]]+Burnt[[:space:]]' "$logfile" | awk '{print $2}' | paste -sd,
}

sum_burned () {
  local logfile="$1"
  grep -E '^[[:space:]]+Burnt[[:space:]]' "$logfile" | awk '{s += $2} END {print s + 0}'
}

run_case "default" ""
run_case "spread1" "--SpreadRad 1"
run_case "spread2" "--SpreadRad 2"

SEQ_DEFAULT="$(burned_sequence "$WORKDIR/default.log")"
SEQ_SPREAD1="$(burned_sequence "$WORKDIR/spread1.log")"
S_SPREAD1="$(sum_burned "$WORKDIR/spread1.log")"
S_SPREAD2="$(sum_burned "$WORKDIR/spread2.log")"

if [ -z "$SEQ_DEFAULT" ] || [ -z "$SEQ_SPREAD1" ]; then
  echo "Failed to parse burned-cell counts from logs"
  exit 1
fi

if [ "$SEQ_DEFAULT" != "$SEQ_SPREAD1" ]; then
  echo "SpreadRad regression failed: default and --SpreadRad 1 differ"
  echo "  default : $SEQ_DEFAULT"
  echo "  spread1 : $SEQ_SPREAD1"
  exit 1
fi

if ! grep -q "SpreadRadius: 2" "$WORKDIR/spread2.log"; then
  echo "SpreadRad=2 run did not report the spread radius in output"
  exit 1
fi

# Usually radius 2 burns at least as many cells as radius 1 for this fixed case.
if [ "$S_SPREAD2" -lt "$S_SPREAD1" ]; then
  echo "Unexpected spread result: spread2 total burned=$S_SPREAD2 < spread1 total burned=$S_SPREAD1"
  exit 1
fi

echo "SpreadRad test passed: default==spread1 (total=$S_SPREAD1), spread2 total=$S_SPREAD2"
rm -rf "$WORKDIR"
