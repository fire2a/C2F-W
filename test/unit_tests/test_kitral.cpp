//
// Created by mati on 20-02-25.
//
#define CATCH_CONFIG_MAIN
#include "../../Cell2Fire/FuelModelKitral.h"
#include <catch2/catch.hpp>

using Catch::Matchers::WithinAbs;

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


TEST_CASE("Slope effect works correctly", "[slope_effect]")
{
    REQUIRE_THAT(slope_effect(156.75, 180, 100), WithinAbs(1.021, 0.001));
    REQUIRE_THAT(slope_effect(201.124, 182.543, 100), WithinAbs(0.982, 0.001));
    REQUIRE_THAT(slope_effect(201.124, 201.124, 100), WithinAbs(1, 0.001));
    // REQUIRE_THROWS(slope_effect(156.75, 170, 0));
}

TEST_CASE_METHOD(NativeFuelFixture, "Rate of spread changes with slope", "[rate_of_spread_k]")
{
    test_outs->se = 1;
    rate_of_spread_k(test_data, test_coefs, test_outs);
    REQUIRE_THAT(test_outs->rss, WithinAbs(1.586, 0.001));
    test_outs->se = 0.5;
    rate_of_spread_k(test_data, test_coefs, test_outs);
    REQUIRE_THAT(test_outs->rss, WithinAbs(1.463, 0.001));
    test_outs->se = 1.5;
    rate_of_spread_k(test_data, test_coefs, test_outs);
    REQUIRE_THAT(test_outs->rss, WithinAbs(1.709, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Rate of spread changes with wind", "[rate_of_spread_k]")
{
    test_outs->se = 1.2;
    test_data->ws = 10;
    rate_of_spread_k(test_data, test_coefs, test_outs);
    REQUIRE_THAT(test_outs->rss, WithinAbs(1.635, 0.001));
    test_data->ws = 50;
    rate_of_spread_k(test_data, test_coefs, test_outs);
    REQUIRE_THAT(test_outs->rss, WithinAbs(3.324, 0.001));
    test_data->ws = 100;
    rate_of_spread_k(test_data, test_coefs, test_outs);
    REQUIRE_THAT(test_outs->rss, WithinAbs(3.648, 0.001));
}

//TODO: test reaction to temperature, humidity

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

TEST_CASE_METHOD(NativeFuelFixture, "Test byram intensity", "[byram_intensity]")
{
    test_outs->rss = 15;
    REQUIRE_THAT(byram_intensity(test_data, test_outs), WithinAbs(16873.87, 0.001));
    test_outs->rss = 5.6;
    REQUIRE_THAT(byram_intensity(test_data, test_outs), WithinAbs(6299.58, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Test surface flame length", "[flame_length]")
{
    test_outs->sfi = 16873.87;
    REQUIRE_THAT(flame_length(test_data, test_outs), WithinAbs(6.82, 0.001));
    test_outs->sfi = 6299.58;
    REQUIRE_THAT(flame_length(test_data, test_outs), WithinAbs(4.334, 0.001));
    test_outs->sfi = 40420;
    REQUIRE_THAT(flame_length(test_data, test_outs), WithinAbs(10.193, 0.001));
}


TEST_CASE_METHOD(NativeFuelFixture, "Test flame angle", "[angleFL]")
{
    test_data->ws = 10;
    test_outs->fl = 6.82;
    REQUIRE_THAT(angleFL(test_data, test_outs), WithinAbs(1.127, 0.001));
    test_outs->fl = 4.334;
    REQUIRE_THAT(angleFL(test_data, test_outs), WithinAbs(1.033, 0.001));
    test_outs->fl = 10.193;
    REQUIRE_THAT(angleFL(test_data, test_outs), WithinAbs(1.2, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Test flame angle changes with wind", "[angleFL]")
{
    test_data->ws = 100;
    test_outs->fl = 6.82;
    REQUIRE_THAT(angleFL(test_data, test_outs), WithinAbs(0.207, 0.001));
    test_outs->fl = 4.334;
    REQUIRE_THAT(angleFL(test_data, test_outs), WithinAbs(0.166, 0.001));
    test_outs->fl = 10.193;
    REQUIRE_THAT(angleFL(test_data, test_outs), WithinAbs(0.251, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Test flame height", "[flame_height]")
{
    test_data->ws = 10;
    test_outs->fl = 6.82;
    REQUIRE_THAT(flame_height(test_data, test_outs), WithinAbs(6.160, 0.001));
    test_outs->fl = 4.334;
    REQUIRE_THAT(flame_height(test_data, test_outs), WithinAbs(3.723, 0.001));
    test_outs->fl = 10.193;
    REQUIRE_THAT(flame_height(test_data, test_outs), WithinAbs(9.501, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Test flame height changes with wind", "[flame_height]")
{
    test_data->ws = 100;
    test_outs->fl = 6.82;
    float fl_100 = flame_height(test_data, test_outs);
    REQUIRE_THAT(fl_100, WithinAbs(1.405, 0.001));
    test_data->ws = 50;
    float fl_50 = flame_height(test_data, test_outs);
    REQUIRE_THAT(fl_50, WithinAbs(2.647, 0.001));
    REQUIRE(fl_50 > fl_100);
}


TEST_CASE_METHOD(NativeFuelFixture, "Active rate of spread changes with slope", "[active_rate_of_spreadPL04]")
{
    test_data->ws = 20;
    test_data->tmp = 27;
    test_data->rh = 40;
    test_outs->se = 1;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(9.977, 0.001));
    test_outs->se = 0.5;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(9.101, 0.001));
    test_outs->se = 1.8;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(11.378, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Active rate of spread changes with wind", "[active_rate_of_spreadPL04]")
{
    test_outs->se = 1.3;
    test_data->tmp = 27;
    test_data->rh = 40;
    test_data->ws = 10;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(7.496, 0.001));
    test_data->ws = 50;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(16.951, 0.001));
    test_data->ws = 100;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(22.445, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Active rate of spread changes with temperature", "[active_rate_of_spreadPL04]")
{
    test_data->ws = 20;
    test_data->rh = 40;
    test_outs->se = 1.2;
    test_data->tmp = -10;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(9.521, 0.001));
    test_data->tmp = 0;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(9.728, 0.001));
    test_data->tmp = 10;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(9.943, 0.001));
    test_data->tmp = 20;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(10.166, 0.001));
    test_data->tmp = 30;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(10.397, 0.001));
    test_data->tmp = 40;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(10.637, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Active rate of spread changes with humidity", "[active_rate_of_spreadPL04]")
{
    test_data->ws = 20;
    test_data->tmp = 26;
    test_outs->se = 1.2;
    test_data->rh = 5;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(115.518, 0.001));
    test_data->rh = 25;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(36.817, 0.001));
    test_data->rh = 45;
    REQUIRE_THAT(active_rate_of_spreadPL04(test_data, test_outs), WithinAbs(7.823, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Test check for active fire", "[checkActive]")
{
    int FMC = 100; // Default value in ReadArgs.cpp
    REQUIRE(checkActive(test_data, test_outs, FMC)==false);
    //TODO: use real cbd and cbh values
}

TEST_CASE_METHOD(NativeFuelFixture, "Test crown fraction burn", "[crownfractionburn]")
{
    int FMC = 100; // Default value in ReadArgs.cpp
    test_outs->rss = 8;
    REQUIRE_THAT(crownfractionburn(test_data, test_outs, FMC), WithinAbs(0.841, 0.001));
    test_outs->rss = 15;
    REQUIRE_THAT(crownfractionburn(test_data, test_outs, FMC), WithinAbs(0.968, 0.001));
    test_outs->rss = 30;
    REQUIRE_THAT(crownfractionburn(test_data, test_outs, FMC), WithinAbs(0.998, 0.001));
}


TEST_CASE_METHOD(NativeFuelFixture, "Test final ros changes with ros", "[final_rate_of_spreadPL04]")
{
    test_outs->rss = 4;
    test_outs->ros_active = 10.3;
    test_outs->cfb = 0.89;
    REQUIRE_THAT(final_rate_of_spreadPL04(test_outs), WithinAbs(9.607, 0.001));
    test_outs->rss = 8;
    REQUIRE_THAT(final_rate_of_spreadPL04(test_outs), WithinAbs(10.047, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Test final ros changes with ros active", "[final_rate_of_spreadPL04]")
{
    test_outs->rss = 4;
    test_outs->ros_active = 10.3;
    test_outs->cfb = 0.89;
    REQUIRE_THAT(final_rate_of_spreadPL04(test_outs), WithinAbs(9.607, 0.001));
    test_outs->ros_active = 14;
    REQUIRE_THAT(final_rate_of_spreadPL04(test_outs), WithinAbs(12.9, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Test final ros changes with cfb", "[final_rate_of_spreadPL04]")
{
    test_outs->rss = 4;
    test_outs->ros_active = 10.3;
    test_outs->cfb = 0.89;
    REQUIRE_THAT(final_rate_of_spreadPL04(test_outs), WithinAbs(9.607, 0.001));
    test_outs->cfb = 0.99;
    REQUIRE_THAT(final_rate_of_spreadPL04(test_outs), WithinAbs(10.237, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Test backfire ros in crown fire changes with lb", "[backfire_ros10_k]")
{
    auto sec = new snd_outs();
    auto hptr = new fire_struc();
    sec->lb = 1.058;
    hptr->ros = 8;
    REQUIRE_THAT(backfire_ros10_k(hptr, sec), WithinAbs(4.061, 0.001));
    sec->lb = 17.225;
    REQUIRE_THAT(backfire_ros10_k(hptr, sec), WithinAbs(0.006, 0.001));
}

TEST_CASE_METHOD(NativeFuelFixture, "Test backfire ros in crown fire changes with ros", "[backfire_ros10_k]")
{
    auto sec = new snd_outs();
    auto hptr = new fire_struc();
    sec->lb = 1.058;
    hptr->ros = 8;
    REQUIRE_THAT(backfire_ros10_k(hptr, sec), WithinAbs(4.061, 0.001));
    hptr->ros = 16;
    REQUIRE_THAT(backfire_ros10_k(hptr, sec), WithinAbs(8.122, 0.001));
}