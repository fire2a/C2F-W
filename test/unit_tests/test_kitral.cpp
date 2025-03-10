//
// Created by mati on 20-02-25.
//
#define CATCH_CONFIG_MAIN
#include "../../Cell2Fire/FuelModelKitral.h"
#include <catch2/catch.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("Slope effect works correctly", "[slope_effect]")
{
    REQUIRE_THAT(slope_effect(156.75, 170, 100), WithinAbs(1.0124, 0.001));
    REQUIRE_THAT(slope_effect(201.124, 192.543, 100), WithinAbs(0.992, 0.001));
    REQUIRE_THAT(slope_effect(201.124, 201.124, 100), WithinAbs(1, 0.001));
    // REQUIRE_THROWS(slope_effect(156.75, 170, 0));
}

class NativeFuelFixture
{
public:
    inputs* test_data;
    fuel_coefs* test_coefs;
    main_outs* test_outs;

public:
    NativeFuelFixture()
    {
        setup_const();
        test_data = new inputs();
        test_coefs = new fuel_coefs();
        test_outs = new main_outs();

        strcpy(test_data->fueltype, "BN03");
        test_data->nftype = 16;
        test_data->ws = 10;
        test_data->waz = 45;
        test_data->tmp = 27;
        test_data->rh = 40;
        test_data->elev = 82.7;
        test_data->cbh = 7.5;
        test_data->cbd = 0.15;
        test_data->tree_height = 21;

        // Initialize fuel_coefs
        test_coefs->nftype = 16;
        test_coefs->fmc = 0.000979;
        test_coefs->cbh = 13.8;
        test_coefs->fl = 3.544;
        test_coefs->h = 19045;

        test_outs->hffmc = 0;
        test_outs->sfc = 0;
        test_outs->csi = 0;
        test_outs->fl = 0;
        test_outs->fh = 0;
        test_outs->a = 0;
        test_outs->b = 0;
        test_outs->c = 0;
        test_outs->rss = 0;
        test_outs->angle = 0;
        test_outs->ros_active = 0;
        test_outs->cfb = 0;
        test_outs->se = 0;
        test_outs->rso = 0;
        test_outs->fmc = 0;
        test_outs->sfi = 0;
        test_outs->isi = 0;
        test_outs->be = 0;
        test_outs->sf = 0;
        test_outs->raz = 0;
        test_outs->wsv = 0;
        test_outs->ff = 0;
        test_outs->crown_intensity = 0;
        test_outs->crown_flame_length = 0;
        test_outs->max_flame_length;
        test_outs->crown = 0;
        test_outs->jd_min = 0;
        test_outs->jd = 0;

    }

};


TEST_CASE_METHOD(NativeFuelFixture, "Test rate_of_spread_k", "[rate_of_spread_k]")
{
    rate_of_spread_k(test_data, test_coefs, test_outs);
    REQUIRE_THAT(test_outs->rss, WithinAbs(1.340, 0.001));
}


TEST_CASE_METHOD(NativeFuelFixture, "Test length to breadth", "[l_to_b]")
{
    REQUIRE_THAT(l_to_b(10, test_coefs), WithinAbs(1.058, 0.001));
    REQUIRE_THAT(l_to_b(100, test_coefs), WithinAbs(17.225, 0.001));
    //REQUIRE_THROWS(l_to_b(-1, test_coefs));
    // This should throw exception in the future
}

TEST_CASE_METHOD(NativeFuelFixture, "Test backfire ros", "[backfire_ros_k]")
{
    auto sec = new snd_outs();
    sec->lb = 1.058;
    test_outs->rss = 8;
    REQUIRE_THAT(backfire_ros_k(test_outs, sec), WithinAbs(4.061, 0.001));
    sec->lb = 17.225;
    REQUIRE_THAT(backfire_ros_k(test_outs, sec), WithinAbs(0.006, 0.001));
    sec->lb = 17.225;
    test_outs->rss = 15;
    REQUIRE_THAT(backfire_ros_k(test_outs, sec), WithinAbs(0.0126, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Test flankfire ros", "[flankfire_ros_k]")
{
    REQUIRE_THAT(flankfire_ros_k(8, 4, 1.058), WithinAbs(5.671, 0.001));
    REQUIRE_THAT(flankfire_ros_k(15, 0.126, 17.225), WithinAbs(0.439, 0.001));
}

