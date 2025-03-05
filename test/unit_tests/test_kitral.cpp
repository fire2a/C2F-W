//
// Created by mati on 20-02-25.
//
#define CATCH_CONFIG_MAIN
#include "../../Cell2Fire/FuelModelKitral.h"
#include "Cells.h"
#include <catch2/catch.hpp>

using Catch::Matchers::WithinAbs;

struct KitralModelFixture
{
    inputs test_data;
    fuel_coefs test_coefs;
    main_outs test_outs;

    KitralModelFixture()
    {
        test_data.nftype = 0;
        test_data.ws = 5.0f;
        test_data.tmp = 30.0f;
        test_data.rh = 50.0f;
        test_data.elev = 100.0f;

        // Initialize fuel_coefs (Example values)
        test_coefs.p1 = -12.86;
        test_coefs.p2 = 0.04316;
        test_coefs.p3 = 13.8;

        // Initialize main_outs (All zero initially)
        memset(&test_outs, 0, sizeof(test_outs));
    }
};

TEST_CASE_METHOD(KitralModelFixture, "Test rate_of_spread_k", "[rate_of_spread_k]")
{
    rate_of_spread_k(&test_data, &test_coefs, &test_outs);
    REQUIRE(test_outs.rss > 0); // Basic check
}

TEST_CASE_METHOD(KitralModelFixture, "Test fire behavior calculations", "[fire_sequence]")
{
    rate_of_spread_k(&test_data, &test_coefs, &test_outs);
    REQUIRE(test_outs.rss > 0);

    flame_length(&test_data, &test_outs);
    REQUIRE(test_outs.fl > 0);

    angleFL(&test_data, &test_outs);
    REQUIRE(test_outs.angle >= 0); // Angle should be valid
}

TEST_CASE("Slope effect works correctly", "[slope_effect]")
{
    REQUIRE_THAT(slope_effect(156.75, 170, 100), WithinAbs(1.0124, 0.001));
    REQUIRE_THAT(slope_effect(201.124, 192.543, 100), WithinAbs(0.992, 0.001));
    REQUIRE_THAT(slope_effect(201.124, 201.124, 100), WithinAbs(1, 0.001));
    // REQUIRE_THROWS(slope_effect(156.75, 170, 0));
}
