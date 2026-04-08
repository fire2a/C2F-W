# angleToNb Unit Test

## Purpose

This test validates the `angleToNb` mapping and wind direction conversion logic that fixes issue #207: "slope effect not working on kitral due to head cell calculation".

## What It Tests

1. **angleToNb mapping**: Verifies that each of the 8 cardinal and diagonal directions correctly maps to the appropriate neighbor cell
2. **Meteorological to grid angle conversion**: Tests the conversion from meteorological azimuth (0° = North) to grid coordinates (0° = East)
3. **16 wind directions**: Validates that all 16 compass directions (every 22.5°) correctly round to the nearest 45° increment
4. **Head cell selection**: Simulates the actual use case of selecting the correct neighbor for slope calculations based on wind direction
5. **Boundary handling**: Tests edge and corner cells that don't have all 8 neighbors
6. **Angle snapping**: Verifies the rounding mechanism that snaps arbitrary angles to 45° increments

## The Bug and The Fix

### Original Bug
The original code directly used the meteorological wind azimuth as an index:
```cpp
int head_cell = angleToNb[wdf_ptr->waz];  // WRONG!
```

This failed because:
- **Meteorological convention**: 0° = North, 90° = East, 180° = South, 270° = West
- **Grid convention**: 0° = East, 90° = North, 180° = West, 270° = South

### The Fix
```cpp
int head_angle = wdf_ptr->waz - 90;        // Convert meteorological → grid
if (head_angle < 0)
    head_angle += 360;                      // Keep in [0, 360) range

head_angle = std::round(head_angle / 45.0) * 45.0;  // Snap to nearest 45°

int head_cell = angleToNb[head_angle];      // Correct lookup
```

## Building and Running

### Option 1: Using the dedicated makefile
```bash
cd Cell2Fire
make -f makefile.test_angleToNb
./test_angleToNb
```

### Option 2: Using the main makefile
```bash
cd Cell2Fire
make tests
./test_cell2fire "[angleToNb]"  # Run only angleToNb tests
```

## Test Output

You should see output like:
```
All tests passed (XX assertions in X test cases)
```

## Test Cases

### 1. Basic 8-direction mapping
Tests that the 8 cardinal and diagonal directions map correctly to neighbors.

### 2. Meteorological to grid conversion
Tests conversion for all cardinal and diagonal directions.

### 3. 16 wind directions
Tests all 16 compass directions to ensure proper rounding:
- N, NNE, NE, ENE, E, ESE, SE, SSE, S, SSW, SW, WSW, W, WNW, NW, NNW

### 4. Head cell selection with 16 winds
Simulates actual usage with wind from all 16 directions, verifying the correct neighbor is selected for slope calculations.

### 5. Boundary cells
Tests corner and edge cells that have fewer than 8 neighbors.

### 6. Angle snapping
Validates the mathematical rounding to 45° increments.

## Grid Layout

The test uses a 5×5 grid with the center cell at position 13:
```
 1  2  3  4  5
 6  7  8  9 10
11 12 13 14 15
16 17 18 19 20
21 22 23 24 25
```

Neighbors of cell 13:
```
NW(7)  N(8)   NE(9)
W(12)  C(13)  E(14)
SW(17) S(18)  SE(19)
```

## Dependencies

- Catch2 testing framework
- C++14 or later
- Standard math library

## Related Files

- Source: `Cell2Fire/Cells.cpp` (lines 124-180: `initializeFireFields()`)
- Source: `Cell2Fire/Cells.cpp` (line 451: fixed `manageFire()`)
- Header: `Cell2Fire/Cells.h` (line 102: `angleToNb` declaration)
- Issue: #207
- Pull Request: #208
