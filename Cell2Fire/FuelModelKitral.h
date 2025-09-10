#ifndef FUELMODELKITRAL
#define FUELMODELKITRAL
#include "Cells.h"
#include "FuelModelUtils.h"
#include "ReadArgs.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

/*
               Functions
*/
// Initialize coefficients
void setup_const();

// Calculate flank fire
float flankfire_ros_k(float ros, float bros, float lb);

// Calculate rate of spread
float rate_of_spread_k(inputs* data, fuel_coefs* ptr, main_outs* at, weatherDF* wdf_ptr);

// Length-to-Breadth ratio
float l_to_b(float ws, fuel_coefs*);

// BROS
float backfire_ros_k(const main_outs* at, const snd_outs* sec);

// Flame length [m])
float flame_length(const main_outs* at);

// byram intensity
float byram_intensity(inputs* data, main_outs* at);

// Type of fire (Crown = 1, SROS = 0)
bool fire_type(inputs* data, main_outs* atr, int FMC);
// Active crown
bool checkActive(inputs* data, main_outs* at, int FMC);
// CROS adjustements
float final_rate_of_spreadPL04(main_outs* at);
float active_rate_of_spreadPL04(inputs* data, main_outs* at, weatherDF* wdf_ptr);  // En KITRAL SE USA PL04
float crownfractionburn(inputs* data, main_outs* at, int FMC);

// Back fire with CROS
float backfire_ros10_k(fire_struc* hptr, snd_outs* sec);

// Slope effect
float slope_effect(float elev_i, float elev_j, int cellsize);

// Main function to populate spread outputs based on inputs provided from main
// class
void calculate_k(inputs* data,
                 inputs* head,
                 int cellsize,
                 fuel_coefs* ptr,
                 arguments* args,
                 main_outs* at,
                 snd_outs* sec,
                 fire_struc* hptr,
                 fire_struc* fptr,
                 fire_struc* bptr,
                 bool& activeCrown,
                 weatherDF* wdf_ptr);

void determine_destiny_metrics_k(inputs* data, fuel_coefs* ptr, arguments* args, main_outs* at);

#endif
