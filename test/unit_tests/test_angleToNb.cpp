//
// Unit test for angleToNb mapping and wind direction conversion
// Tests the fix for issue #207: slope effect not working on kitral due to head cell calculation
//
#include "../../Cell2Fire/Cells.h"
// catch v3 or v2 compatibility
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#else
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#endif
#include <cmath>
#include <unordered_set>

using Catch::Matchers::WithinAbs;

/**
 * @brief Helper function to convert meteorological azimuth to grid angle
 * 
 * angleToNb coordinate system (from initializeFireFields):
 *   0° = South (a < 0, b = 0)
 *  90° = East (a = 0, b < 0)
 * 180° = North (a > 0, b = 0)
 * 270° = West (a = 0, b > 0)
 * 
 * Meteorological convention:
 *   0° = North, 90° = East, 180° = South, 270° = West
 * 
 * Conversion: grid_angle = (180 - met_angle + 360) % 360
 * Then snap to nearest 45° increment
 */
int
meteorological_to_grid_angle(int waz)
{
    // Convert meteorological to grid system
    int head_angle = (static_cast<int>(180.0 - waz) + 360) % 360;

    // int head_angle = waz - 90;
    // if (head_angle < 0)
    //     head_angle += 360;
    
    // Snap to nearest 45° increment
    head_angle = std::round(head_angle / 45.0) * 45.0;
    
    // Handle wraparound
    if (head_angle >= 360)
        head_angle = 0;

    return head_angle;
}

/**
 * @brief Fixture for testing Cell angleToNb mapping
 */
class AngleToNbFixture
{
  public:
    Cells* test_cell;
    std::vector<std::vector<int>> coordCells;
    std::unordered_set<int> availSet;
    int rows = 5;
    int cols = 5;
    int center_cell_realId = 13;  // Center of 5x5 grid (1-indexed)

    AngleToNbFixture()
    {
        // Create a 5x5 grid with center cell at position 13
        // Grid layout (realId):
        //  1  2  3  4  5
        //  6  7  8  9 10
        // 11 12 13 14 15
        // 16 17 18 19 20
        // 21 22 23 24 25

        coordCells.resize(rows * cols);
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                int idx = i * cols + j;
                coordCells[idx] = { i, j };  // row, col coordinates
                availSet.insert(idx + 1);    // 1-indexed realId
            }
        }

        // Create center cell (id=12, realId=13)
        std::vector<int> coord = { 2, 2 };  // center position
        test_cell = new Cells(12,           // id (0-indexed)
                              100.0,        // area (10m x 10m)
                              coord,        // coordinates
                              1,            // fType (Normal)
                              "Normal",     // fType2
                              40.0,         // perimeter (4 * 10m)
                              0,            // status (Available)
                              13            // realId (1-indexed)
        );

        // Initialize fire fields to populate angleToNb
        test_cell->initializeFireFields(coordCells, availSet, cols, rows);
    }

    ~AngleToNbFixture()
    {
        delete test_cell;
    }
};

TEST_CASE_METHOD(AngleToNbFixture, "angleToNb mapping for 8 cardinal and diagonal directions", "[angleToNb]")
{
    // Expected neighbors for center cell (realId=13) in a 5x5 grid:
    //  7  8  9
    // 12 13 14
    // 17 18 19
    //
    // angleToNb coordinate system:
    //   0° = South, 90° = East, 180° = North, 270° = West

    SECTION("South direction (0 degrees grid angle)")
    {
        REQUIRE(test_cell->angleToNb.count(0) > 0);
        REQUIRE(test_cell->angleToNb[0] == 18);  // South neighbor
    }

    SECTION("Southeast direction (45 degrees grid angle)")
    {
        REQUIRE(test_cell->angleToNb.count(45) > 0);
        REQUIRE(test_cell->angleToNb[45] == 19);  // SE neighbor
    }

    SECTION("East direction (90 degrees grid angle)")
    {
        REQUIRE(test_cell->angleToNb.count(90) > 0);
        REQUIRE(test_cell->angleToNb[90] == 14);  // East neighbor
    }

    SECTION("Northeast direction (135 degrees grid angle)")
    {
        REQUIRE(test_cell->angleToNb.count(135) > 0);
        REQUIRE(test_cell->angleToNb[135] == 9);  // NE neighbor
    }

    SECTION("North direction (180 degrees grid angle)")
    {
        REQUIRE(test_cell->angleToNb.count(180) > 0);
        REQUIRE(test_cell->angleToNb[180] == 8);  // North neighbor
    }

    SECTION("Northwest direction (225 degrees grid angle)")
    {
        REQUIRE(test_cell->angleToNb.count(225) > 0);
        REQUIRE(test_cell->angleToNb[225] == 7);  // NW neighbor
    }

    SECTION("West direction (270 degrees grid angle)")
    {
        REQUIRE(test_cell->angleToNb.count(270) > 0);
        REQUIRE(test_cell->angleToNb[270] == 12);  // West neighbor
    }

    SECTION("Southwest direction (315 degrees grid angle)")
    {
        REQUIRE(test_cell->angleToNb.count(315) > 0);
        REQUIRE(test_cell->angleToNb[315] == 17);  // SW neighbor
    }
}

TEST_CASE("Meteorological to grid angle conversion", "[angleToNb][conversion]")
{
    // Meteorological convention: 0° = North, 90° = East, 180° = South, 270° = West
    // Grid convention (angleToNb): 0° = South, 90° = East, 180° = North, 270° = West
    //
    // Conversion: grid = (180 - met) % 360

    SECTION("Cardinal directions")
    {
        REQUIRE(meteorological_to_grid_angle(0) == 180);    // Met North (0°) -> Grid North (180°)
        REQUIRE(meteorological_to_grid_angle(90) == 90);    // Met East (90°) -> Grid East (90°)
        REQUIRE(meteorological_to_grid_angle(180) == 0);    // Met South (180°) -> Grid South (0°)
        REQUIRE(meteorological_to_grid_angle(270) == 270);  // Met West (270°) -> Grid West (270°)
    }

    SECTION("Diagonal directions")
    {
        REQUIRE(meteorological_to_grid_angle(45) == 135);   // Met NE (45°) -> Grid NE (135°)
        REQUIRE(meteorological_to_grid_angle(135) == 45);   // Met SE (135°) -> Grid SE (45°)
        REQUIRE(meteorological_to_grid_angle(225) == 315);  // Met SW (225°) -> Grid SW (315°)
        REQUIRE(meteorological_to_grid_angle(315) == 225);  // Met NW (315°) -> Grid NW (225°)
    }
}

TEST_CASE("Wind direction conversion for 16 directions", "[angleToNb][wind][16directions]")
{
    // Test all 16 compass directions (every 22.5 degrees)
    // This verifies the rounding to nearest 45° works correctly

    struct WindTest
    {
        int meteorological_angle;
        int expected_grid_angle;
        std::string direction_name;
    };

    std::vector<WindTest> wind_tests = {
        //  Met  Grid  Grid/45  Rounds   Direction
        { 0, 180, "N" },      // 0° → 180° → 4.00 → 180° (N)
        { 23, 135, "NNE" },   // 23° → 157° → 3.49 → 135° (NE)
        { 45, 135, "NE" },    // 45° → 135° → 3.00 → 135° (NE)
        { 68, 90, "ENE" },    // 68° → 112° → 2.49 → 90° (E)
        { 90, 90, "E" },      // 90° → 90° → 2.00 → 90° (E)
        { 113, 45, "ESE" },   // 113° → 67° → 1.49 → 45° (SE)
        { 135, 45, "SE" },    // 135° → 45° → 1.00 → 45° (SE)
        { 158, 0, "SSE" },    // 158° → 22° → 0.49 → 0° (S)
        { 180, 0, "S" },      // 180° → 0° → 0.00 → 0° (S)
        { 203, 315, "SSW" },  // 203° → 337° → 7.49 → 315° (SW)
        { 225, 315, "SW" },   // 225° → 315° → 7.00 → 315° (SW)
        { 248, 270, "WSW" },  // 248° → 292° → 6.49 → 270° (W)
        { 270, 270, "W" },    // 270° → 270° → 6.00 → 270° (W)
        { 293, 225, "WNW" },  // 293° → 247° → 5.49 → 225° (NW)
        { 315, 225, "NW" },   // 315° → 225° → 5.00 → 225° (NW)
        { 338, 180, "NNW" },  // 338° → 202° → 4.49 → 180° (N)
    };

    for (const auto& test : wind_tests)
    {
        DYNAMIC_SECTION("Wind from " << test.direction_name << " (" << test.meteorological_angle << "°)")
        {
            int result = meteorological_to_grid_angle(test.meteorological_angle);
            REQUIRE(result == test.expected_grid_angle);
        }
    }
}

TEST_CASE_METHOD(AngleToNbFixture, "Head cell selection with wind from 16 directions", "[angleToNb][headcell]")
{
    // This test simulates the actual use case: selecting the correct head_cell
    // for slope calculation based on wind direction

    struct HeadCellTest
    {
        int wind_azimuth;       // Meteorological wind direction
        int expected_neighbor;  // Expected neighbor cell ID
        std::string description;
    };

    std::vector<HeadCellTest> tests = {
        // Cardinal directions (wind FROM direction -> fire heads opposite)
        { 0, 8, "Wind from North -> fire heads North (grid 180°)" },
        { 90, 14, "Wind from East -> fire heads East (grid 90°)" },
        { 180, 18, "Wind from South -> fire heads South (grid 0°)" },
        { 270, 12, "Wind from West -> fire heads West (grid 270°)" },

        // Diagonal directions
        { 45, 9, "Wind from NE -> fire heads NE (grid 135°)" },
        { 135, 19, "Wind from SE -> fire heads SE (grid 45°)" },
        { 225, 17, "Wind from SW -> fire heads SW (grid 315°)" },
        { 315, 7, "Wind from NW -> fire heads NW (grid 225°)" },

        // Intermediate directions (should round to nearest 45°)
        { 22, 8, "Wind from NNE (22°) -> rounds to North (grid 180°)" },
        { 68, 14, "Wind from ENE (68°) -> rounds to East (grid 90°)" },
        { 112, 14, "Wind from ESE (112°) -> rounds to East (grid 90°)" },
        { 158, 18, "Wind from SSE (158°) -> rounds to South (grid 0°)" },
        { 202, 18, "Wind from SSW (202°) -> rounds to South (grid 0°)" },
        { 248, 12, "Wind from WSW (248°) -> rounds to West (grid 270°)" },
        { 292, 12, "Wind from WNW (292°) -> rounds to West (grid 270°)" },
        { 338, 8, "Wind from NNW (338°) -> rounds to North (grid 180°)" },
    };

    for (const auto& test : tests)
    {
        DYNAMIC_SECTION(test.description)
        {
            // Convert meteorological wind azimuth to grid angle
            int head_angle = meteorological_to_grid_angle(test.wind_azimuth);

            // Lookup the head cell using angleToNb
            REQUIRE(test_cell->angleToNb.count(head_angle) > 0);
            int head_cell = test_cell->angleToNb[head_angle];

            REQUIRE(head_cell == test.expected_neighbor);
        }
    }
}

TEST_CASE_METHOD(AngleToNbFixture, "Boundary cell handling", "[angleToNb][boundary]")
{
    // Test edge cells that don't have all 8 neighbors

    SECTION("Corner cell (top-left, realId=1)")
    {
        std::vector<int> coord = { 0, 0 };
        Cells* corner_cell = new Cells(0, 100.0, coord, 1, "Normal", 40.0, 0, 1);
        corner_cell->initializeFireFields(coordCells, availSet, cols, rows);

        // Should have only 3 neighbors: E(2), SE(7), S(6)
        // Grid angles: South=0°, SE=45°, East=90°
        REQUIRE(corner_cell->angleToNb.count(0) > 0);   // South (cell 6)
        REQUIRE(corner_cell->angleToNb.count(45) > 0);  // Southeast (cell 7)
        REQUIRE(corner_cell->angleToNb.count(90) > 0);  // East (cell 2)

        // Should NOT have NW, N, NE, W, SW neighbors
        REQUIRE(corner_cell->angleToNb.size() == 3);

        delete corner_cell;
    }

    SECTION("Edge cell (top edge, realId=3)")
    {
        std::vector<int> coord = { 0, 2 };
        Cells* edge_cell = new Cells(2, 100.0, coord, 1, "Normal", 40.0, 0, 3);
        edge_cell->initializeFireFields(coordCells, availSet, cols, rows);

        // Should have 5 neighbors: W, E, SW, S, SE (no north neighbors)
        REQUIRE(edge_cell->angleToNb.size() == 5);

        delete edge_cell;
    }
}

TEST_CASE("Angle snapping to 45 degree increments", "[angleToNb][snapping]")
{
    // Verify that the rounding mechanism works correctly

    SECTION("Angles close to 0°")
    {
        REQUIRE(std::round(0 / 45.0) * 45.0 == 0);
        REQUIRE(std::round(22 / 45.0) * 45.0 == 0);
        REQUIRE(std::round(23 / 45.0) * 45.0 == 45);
    }

    SECTION("Angles close to 45°")
    {
        REQUIRE(std::round(23 / 45.0) * 45.0 == 45);
        REQUIRE(std::round(45 / 45.0) * 45.0 == 45);
        REQUIRE(std::round(67 / 45.0) * 45.0 == 45);
        REQUIRE(std::round(68 / 45.0) * 45.0 == 90);
    }

    SECTION("Angles close to 90°")
    {
        REQUIRE(std::round(68 / 45.0) * 45.0 == 90);
        REQUIRE(std::round(90 / 45.0) * 45.0 == 90);
        REQUIRE(std::round(112 / 45.0) * 45.0 == 90);
        REQUIRE(std::round(113 / 45.0) * 45.0 == 135);
    }

    SECTION("Full circle of 16 directions - grid angles after conversion")
    {
        // These are GRID angles after (180 - met + 360) % 360 conversion
        int grid_angles_16[] = { 180, 157, 135, 112, 90, 67, 45, 22, 0, 337, 315, 292, 270, 247, 225, 202 };
        int expected_8[] = { 180, 135, 135, 90, 90, 45, 45, 0, 0, 315, 315, 270, 270, 225, 225, 180 };

        for (int i = 0; i < 16; i++)
        {
            int snapped = std::round(grid_angles_16[i] / 45.0) * 45.0;
            REQUIRE(snapped == expected_8[i]);
        }
    }
}
