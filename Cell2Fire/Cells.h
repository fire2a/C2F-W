#ifndef CELLS
#define CELLS

// include stuff
#include "ReadArgs.h"
#include <math.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

std::vector<int> adjacentCells(int cell, int nrows, int ncols);
/*
 *   Weather structure
 */
typedef struct
{
    float ws, waz, rh, tmp, apcp, ffmc, dmc, dc, isi, bui,
        fwi;  // David: some variables only used on C2FK and not on C2FSB and
              // viceversa
} weatherDF;  // David: Moved here to simplify inclusion

typedef struct
{
    char fueltype[5];
    float ws, saz, cur, ps, cbh, ccf, cbd, elev, tmp, rh, lat, lon, ffmc, bui, gfl,
        tree_height;  // David: some variables only used on C2FK and not on C2FSB and
                      // viceversa
    int waz, nftype, FMC, time, pattern, mon, jd, jd_min, pc, pdf;
} inputs;  // David: Moved here to simplify inclusion

typedef struct
{
    char fueltype[4];
    float p1, p2, p3;    // hros coef
    float q1, q2, q3;    // flame length coef
    float q, bui0, cfl;  // fbp params
    float cbh, fmc, fl, h;
    int nftype;
} fuel_coefs;

typedef struct
{
    float hffmc, sfc, csi, fl, fh, a, b, c, rss, angle, ros_active, cfb, se, rso, fmc, sfi, isi, be, sf, raz, wsv, ff,
        crown_intensity, crown_flame_length, max_flame_length;
    char covertype;
    int crown, jd_min, jd;
} main_outs;

typedef struct
{
    float ros, dist, rost, cfb, fc, cfc, time, rss, isi;
    char fd;
    double fi;
} fire_struc;

typedef struct
{
    float lb, area, perm, pgr, lbt;
} snd_outs;

class Cells
{
    // TODO: find where to put the enums
  public:
    // immutable
    int id;
    int fType;
    int realId;
    double _ctr2ctrdist;
    double area;
    double perimeter;

    int coord[2];  // (x, y) — was vector<int>, saves 24 bytes heap overhead per cell
    // std::unordered_map<std::string, int> adjacents; // CP: dictionary {string:
    // [int array]}

    static constexpr const char* FTypeD[3]  = {"NonBurnable", "Normal", "Burnable"};
    static constexpr const char* StatusD[5] = {"Available", "Burning", "Burnt", "Harvested", "Non Fuel"};

    // mutable
    int status;
    int hPeriod;
    int fireStarts;
    int harvestStarts;
    int fireStartsSeason;
    int burntP;
    int tYears;

    // Flat neighbor arrays replacing 5 unordered_maps (max 8 neighbors per cell)
    int nb_count;           // number of valid neighbors
    int nb_ids[8];          // neighbor cell IDs
    int nb_angles[8];       // angle to each neighbor
    double nb_ros[8];       // ROS value per neighbor direction (replaces ROSAngleDir)
    double nb_progress[8];  // fire progress per neighbor (replaces fireProgress)
    double nb_dist[8];      // distance to center per neighbor (replaces distToCenter)
    bool nb_available[8];   // whether neighbor is burnable (was in ROSAngleDir)

    // Lookup helpers — linear scan over max 8 elements
    int slotByNb(int nb) const {
        for (int i = 0; i < nb_count; i++)
            if (nb_ids[i] == nb) return i;
        return -1;
    }
    int slotByAngle(int angle) const {
        for (int i = 0; i < nb_count; i++)
            if (nb_angles[i] == angle) return i;
        return -1;
    }
    bool hasAvailableNeighbor() const {
        for (int i = 0; i < nb_count; i++)
            if (nb_available[i]) return true;
        return false;
    }

    // TODO: reference to shared object

    // constructor and methods here
    Cells(int _id,
          double _area,
          std::vector<int> _coord,
          int _fType,
          double _perimeter,
          int _status,
          int _realId);

    void initializeFireFields(std::vector<std::vector<int>>& coordCells,
                              std::unordered_set<int>& availSet,
                              int cols,
                              int rows);  // TODO: need TYPE
    void ros_distr_old(double thetafire, double forward, double flank, double back);
    static double rhoTheta(double theta, double a, double b);
    void ros_distr_V2(double thetafire, double a, double b, double c, double EFactor);

    std::vector<int> manageFire(int period,
                                std::unordered_set<int>& AvailSet,
                                inputs df[],
                                fuel_coefs* coef,
                                std::vector<std::vector<int>>& coordCells,
                                arguments* args,
                                weatherDF* wdf_ptr,
                                std::vector<double>* FSCell,
                                std::vector<float>* crownMetrics,
                                bool& activeCrown,
                                double randomROS,
                                int perimeterCells,
                                std::vector<int>& crownState,
                                std::vector<float>& crownFraction,
                                std::vector<float>& surfFraction,
                                std::vector<float>& Intensities,
                                std::vector<float>& RateOfSpreads,
                                std::vector<float>& SurfaceFlameLengths,
                                std::vector<float>& CrownFlameLengths,
                                std::vector<float>& CrownIntensities,
                                std::vector<float>& MaxFlameLengths);

    bool get_burned(int period,
                    int season,
                    int NMsg,
                    inputs df[],
                    fuel_coefs* coef,
                    arguments* args,
                    weatherDF* wdf_ptr,
                    bool& activeCrown,
                    int perimeterCells);

    // void set_Adj(std::unordered_map<std::string, int> & adjacentCells);

    void setStatus(int status_int);

    std::string getStatus();

    bool ignition(int period,
                  int year,
                  std::vector<int>& ignitionPoints,
                  inputs* df_ptr,
                  fuel_coefs* coef,
                  arguments* args,
                  weatherDF* wdf_ptr,
                  bool& activeCrown,
                  int perimeterCells);

    void harvested(int id, int period);

    void print_info();

  private:
    static double allocate(double offset, double base, double ros1, double ros2);
    static float slope_effect(float elev_i, float elev_j, int cellsize);
};

#endif
