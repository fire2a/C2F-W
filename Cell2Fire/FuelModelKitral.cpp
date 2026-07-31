/* coding: utf-8
__version__ = "3.0"
__author__ = "Jaime Carrasco-Barra"
__maintainer__ = "Jaime Carrasco-Barra, Matilde Rivas, David Palacios"
*/
#include "FuelModelKitral.h"
#include <cstdint>   // int16_t: no llega transitivamente en MinGW/MSVC
#include "Cells.h"
#include "FuelModelUtils.h"
#include "ReadArgs.h"
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// ---- Helpers de humedad del combustible (para --fch-mode) ----
static float emc_simard(float T, float H) {  // EMC Simard (%), T en C, H=RH%
    if (H < 10.0f)  return 0.03229f + 0.281073f * H - 0.000578f * H * T;
    if (H <= 50.0f) return 2.22749f + 0.160107f * H - 0.014784f * T;
    return 21.0606f + 0.005565f * H * H - 0.00035f * H * T - 0.483199f * H;
}
static float eta_M(float m) {  // damping de humedad de Rothermel, Mx=30%
    float r = m / 30.0f; if (r > 1.0f) r = 1.0f;
    float e = 1.0f - 2.59f * r + 5.11f * r * r - 3.52f * r * r * r;
    return e > 0.0f ? e : 0.0f;
}
static float ff_moist(float m) {  // funcion de humedad fina del FFMC/ISI (FBP)
    return 91.9f * exp(-0.1386f * m) * (1.0f + pow(m, 5.31f) / 4.93e7f);
}

using namespace std;

/*
        Global coefficients
*/
std::unordered_map<int, std::vector<float>> fmcs;
std::unordered_map<int, std::vector<float>> cbhs;
std::unordered_map<int, std::vector<float>> fls_david;
std::unordered_map<int, std::vector<float>> hs;
std::unordered_map<int, std::vector<float>> cbds;

/*
        Functions
*/

void
setup_const()
{
    // printf("dlw debug, enter fuel_coefs\n");
    /*   fuel type 1 */
    int PCH1 = 1;
    std::vector<float> fmc_pch1;
    std::vector<float> cbh_pch1;
    std::vector<float> fl_pch1;
    std::vector<float> h_pch1;
    std::vector<float> cbd_pch1;
    fmc_pch1.push_back(0.018880);
    cbh_pch1.push_back(0);
    fl_pch1.push_back(0.918);
    h_pch1.push_back(16434);
    cbd_pch1.push_back(0);
    fmcs.insert(std::make_pair(PCH1, fmc_pch1));
    cbhs.insert(std::make_pair(PCH1, cbh_pch1));
    fls_david.insert(std::make_pair(PCH1, fl_pch1));
    hs.insert(std::make_pair(PCH1, h_pch1));
    cbds.insert(std::make_pair(PCH1, cbd_pch1));

    int PCH2 = 2;
    std::vector<float> fmc_pch2;
    std::vector<float> cbh_pch2;
    std::vector<float> fl_pch2;
    std::vector<float> h_pch2;
    std::vector<float> cbd_pch2;
    fmc_pch2.push_back(0.016027);
    cbh_pch2.push_back(0);
    fl_pch2.push_back(0.617);
    h_pch2.push_back(16434);
    cbd_pch2.push_back(0);
    fmcs.insert(std::make_pair(PCH2, fmc_pch2));
    cbhs.insert(std::make_pair(PCH2, cbh_pch2));
    fls_david.insert(std::make_pair(PCH2, fl_pch2));
    hs.insert(std::make_pair(PCH2, h_pch2));
    cbds.insert(std::make_pair(PCH2, cbd_pch2));

    int PCH3 = 3;
    std::vector<float> fmc_pch3;
    std::vector<float> cbh_pch3;
    std::vector<float> fl_pch3;
    std::vector<float> h_pch3;
    std::vector<float> cbd_pch3;
    fmc_pch3.push_back(0.010235);
    cbh_pch3.push_back(0);
    fl_pch3.push_back(0.684);
    h_pch3.push_back(16434);
    cbd_pch3.push_back(0);
    fmcs.insert(std::make_pair(PCH3, fmc_pch3));
    cbhs.insert(std::make_pair(PCH3, cbh_pch3));
    fls_david.insert(std::make_pair(PCH3, fl_pch3));
    hs.insert(std::make_pair(PCH3, h_pch3));
    cbds.insert(std::make_pair(PCH3, cbd_pch3));

    int PCH4 = 4;
    std::vector<float> fmc_pch4;
    std::vector<float> cbh_pch4;
    std::vector<float> fl_pch4;
    std::vector<float> h_pch4;
    std::vector<float> cbd_pch4;
    fmc_pch4.push_back(0.008690);
    cbh_pch4.push_back(0);
    fl_pch4.push_back(0.527);
    h_pch4.push_back(16434);
    cbd_pch4.push_back(0);
    fmcs.insert(std::make_pair(PCH4, fmc_pch4));
    cbhs.insert(std::make_pair(PCH4, cbh_pch4));
    fls_david.insert(std::make_pair(PCH4, fl_pch4));
    hs.insert(std::make_pair(PCH4, h_pch4));
    cbds.insert(std::make_pair(PCH4, cbd_pch4));

    int PCH5 = 5;
    std::vector<float> fmc_pch5;
    std::vector<float> cbh_pch5;
    std::vector<float> fl_pch5;
    std::vector<float> h_pch5;
    std::vector<float> cbd_pch5;
    fmc_pch5.push_back(0.001009);
    cbh_pch5.push_back(0);
    fl_pch5.push_back(0.649);
    h_pch5.push_back(15899);
    cbd_pch5.push_back(0);
    fmcs.insert(std::make_pair(PCH5, fmc_pch5));
    cbhs.insert(std::make_pair(PCH5, cbh_pch5));
    fls_david.insert(std::make_pair(PCH5, fl_pch5));
    hs.insert(std::make_pair(PCH5, h_pch5));
    cbds.insert(std::make_pair(PCH5, cbd_pch5));

    int MT01 = 6;
    std::vector<float> fmc_mt01;
    std::vector<float> cbh_mt01;
    std::vector<float> fl_mt01;
    std::vector<float> h_mt01;
    std::vector<float> cbd_mt01;
    fmc_mt01.push_back(0.007603);
    cbh_mt01.push_back(0);
    fl_mt01.push_back(2.923);
    h_mt01.push_back(19635);
    cbd_mt01.push_back(0);
    fmcs.insert(std::make_pair(MT01, fmc_mt01));
    cbhs.insert(std::make_pair(MT01, cbh_mt01));
    fls_david.insert(std::make_pair(MT01, fl_mt01));
    hs.insert(std::make_pair(MT01, h_mt01));
    cbds.insert(std::make_pair(MT01, cbd_mt01));

    int MT02 = 7;
    std::vector<float> fmc_mt02;
    std::vector<float> cbh_mt02;
    std::vector<float> fl_mt02;
    std::vector<float> h_mt02;
    std::vector<float> cbd_mt02;
    fmc_mt02.push_back(0.008147);
    cbh_mt02.push_back(0);
    fl_mt02.push_back(1.910);
    h_mt02.push_back(19635);
    cbd_mt02.push_back(0);
    fmcs.insert(std::make_pair(MT02, fmc_mt02));
    cbhs.insert(std::make_pair(MT02, cbh_mt02));
    fls_david.insert(std::make_pair(MT02, fl_mt02));
    hs.insert(std::make_pair(MT02, h_mt02));
    cbds.insert(std::make_pair(MT02, cbd_mt02));

    int MT03 = 8;
    std::vector<float> fmc_mt03;
    std::vector<float> cbh_mt03;
    std::vector<float> fl_mt03;
    std::vector<float> h_mt03;
    std::vector<float> cbd_mt03;
    fmc_mt03.push_back(0.001672);
    cbh_mt03.push_back(0);
    fl_mt03.push_back(3.308);
    h_mt03.push_back(19129);
    cbd_mt03.push_back(0);
    fmcs.insert(std::make_pair(MT03, fmc_mt03));
    cbhs.insert(std::make_pair(MT03, cbh_mt03));
    fls_david.insert(std::make_pair(MT03, fl_mt03));
    hs.insert(std::make_pair(MT03, h_mt03));
    cbds.insert(std::make_pair(MT03, cbd_mt03));

    int MT04 = 9;
    std::vector<float> fmc_mt04;
    std::vector<float> cbh_mt04;
    std::vector<float> fl_mt04;
    std::vector<float> h_mt04;
    std::vector<float> cbd_mt04;
    fmc_mt04.push_back(0.004886);
    cbh_mt04.push_back(0);
    fl_mt04.push_back(1.383);
    h_mt04.push_back(19129);
    cbd_mt04.push_back(0);
    fmcs.insert(std::make_pair(MT04, fmc_mt04));
    cbhs.insert(std::make_pair(MT04, cbh_mt04));
    fls_david.insert(std::make_pair(MT04, fl_mt04));
    hs.insert(std::make_pair(MT04, h_mt04));
    cbds.insert(std::make_pair(MT04, cbd_mt04));

    int MT05 = 10;
    std::vector<float> fmc_mt05;
    std::vector<float> cbh_mt05;
    std::vector<float> fl_mt05;
    std::vector<float> h_mt05;
    std::vector<float> cbd_mt05;
    fmc_mt05.push_back(0.010321);
    cbh_mt05.push_back(0);
    fl_mt05.push_back(3.029);
    h_mt05.push_back(20920);
    cbd_mt05.push_back(0);
    fmcs.insert(std::make_pair(MT05, fmc_mt05));
    cbhs.insert(std::make_pair(MT05, cbh_mt05));
    fls_david.insert(std::make_pair(MT05, fl_mt05));
    hs.insert(std::make_pair(MT05, h_mt05));
    cbds.insert(std::make_pair(MT05, cbd_mt05));

    int MT06 = 11;
    std::vector<float> fmc_mt06;
    std::vector<float> cbh_mt06;
    std::vector<float> fl_mt06;
    std::vector<float> h_mt06;
    std::vector<float> cbd_mt06;
    fmc_mt06.push_back(0.009234);
    cbh_mt06.push_back(0);
    fl_mt06.push_back(3.529);
    h_mt06.push_back(21284);
    cbd_mt06.push_back(0);
    fmcs.insert(std::make_pair(MT06, fmc_mt06));
    cbhs.insert(std::make_pair(MT06, cbh_mt06));
    fls_david.insert(std::make_pair(MT06, fl_mt06));
    hs.insert(std::make_pair(MT06, h_mt06));
    cbds.insert(std::make_pair(MT06, cbd_mt06));

    int MT07 = 12;
    std::vector<float> fmc_mt07;
    std::vector<float> cbh_mt07;
    std::vector<float> fl_mt07;
    std::vector<float> h_mt07;
    std::vector<float> cbd_mt07;
    fmc_mt07.push_back(0.001787);
    cbh_mt07.push_back(0);
    fl_mt07.push_back(3.189);
    h_mt07.push_back(18828);
    cbd_mt07.push_back(0);
    fmcs.insert(std::make_pair(MT07, fmc_mt07));
    cbhs.insert(std::make_pair(MT07, cbh_mt07));
    fls_david.insert(std::make_pair(MT07, fl_mt07));
    hs.insert(std::make_pair(MT07, h_mt07));
    cbds.insert(std::make_pair(MT07, cbd_mt07));

    int MT08 = 13;
    std::vector<float> fmc_mt08;
    std::vector<float> cbh_mt08;
    std::vector<float> fl_mt08;
    std::vector<float> h_mt08;
    std::vector<float> cbd_mt08;
    fmc_mt08.push_back(0.004342);
    cbh_mt08.push_back(0);
    fl_mt08.push_back(1.903);
    h_mt08.push_back(18828);
    cbd_mt08.push_back(0);
    fmcs.insert(std::make_pair(MT08, fmc_mt08));
    cbhs.insert(std::make_pair(MT08, cbh_mt08));
    fls_david.insert(std::make_pair(MT08, fl_mt08));
    hs.insert(std::make_pair(MT08, h_mt08));
    cbds.insert(std::make_pair(MT08, cbd_mt08));

    int BN01 = 14;
    std::vector<float> fmc_bn01;
    std::vector<float> cbh_bn01;
    std::vector<float> fl_bn01;
    std::vector<float> h_bn01;
    std::vector<float> cbd_bn01;
    fmc_bn01.push_back(0.002249);
    cbh_bn01.push_back(0);
    fl_bn01.push_back(2.624);
    h_bn01.push_back(19246);
    cbd_bn01.push_back(0);
    fmcs.insert(std::make_pair(BN01, fmc_bn01));
    cbhs.insert(std::make_pair(BN01, cbh_bn01));
    fls_david.insert(std::make_pair(BN01, fl_bn01));
    hs.insert(std::make_pair(BN01, h_bn01));
    cbds.insert(std::make_pair(BN01, cbd_bn01));

    int BN02 = 15;
    std::vector<float> fmc_bn02;
    std::vector<float> cbh_bn02;
    std::vector<float> fl_bn02;
    std::vector<float> h_bn02;
    std::vector<float> cbd_bn02;
    fmc_bn02.push_back(0.001441);
    cbh_bn02.push_back(0);
    fl_bn02.push_back(2.310);
    h_bn02.push_back(19037);
    cbd_bn02.push_back(0);
    fmcs.insert(std::make_pair(BN02, fmc_bn02));
    cbhs.insert(std::make_pair(BN02, cbh_bn02));
    fls_david.insert(std::make_pair(BN02, fl_bn02));
    hs.insert(std::make_pair(BN02, h_bn02));
    cbds.insert(std::make_pair(BN02, cbd_bn02));

    int BN03 = 16;
    std::vector<float> fmc_bn03;
    std::vector<float> cbh_bn03;
    std::vector<float> fl_bn03;
    std::vector<float> h_bn03;
    std::vector<float> cbd_bn03;
    fmc_bn03.push_back(0.000979);
    cbh_bn03.push_back(0);
    fl_bn03.push_back(3.544);
    h_bn03.push_back(19045);
    cbd_bn03.push_back(0);
    fmcs.insert(std::make_pair(BN03, fmc_bn03));
    cbhs.insert(std::make_pair(BN03, cbh_bn03));
    fls_david.insert(std::make_pair(BN03, fl_bn03));
    hs.insert(std::make_pair(BN03, h_bn03));
    cbds.insert(std::make_pair(BN03, cbd_bn03));

    int BN04 = 17;
    std::vector<float> fmc_bn04;
    std::vector<float> cbh_bn04;
    std::vector<float> fl_bn04;
    std::vector<float> h_bn04;
    std::vector<float> cbd_bn04;
    fmc_bn04.push_back(0.001556);
    cbh_bn04.push_back(0);
    fl_bn04.push_back(2.164);
    h_bn04.push_back(19045);
    cbd_bn04.push_back(0);
    fmcs.insert(std::make_pair(BN04, fmc_bn04));
    cbhs.insert(std::make_pair(BN04, cbh_bn04));
    fls_david.insert(std::make_pair(BN04, fl_bn04));
    hs.insert(std::make_pair(BN04, h_bn04));
    cbds.insert(std::make_pair(BN04, cbd_bn04));

    int BN05 = 18;
    std::vector<float> fmc_bn05;
    std::vector<float> cbh_bn05;
    std::vector<float> fl_bn05;
    std::vector<float> h_bn05;
    std::vector<float> cbd_bn05;
    fmc_bn05.push_back(0.002365);
    cbh_bn05.push_back(0);
    fl_bn05.push_back(1.954);
    h_bn05.push_back(19045);
    cbd_bn05.push_back(0);
    fmcs.insert(std::make_pair(BN05, fmc_bn05));
    cbhs.insert(std::make_pair(BN05, cbh_bn05));
    fls_david.insert(std::make_pair(BN05, fl_bn05));
    hs.insert(std::make_pair(BN05, h_bn05));
    cbds.insert(std::make_pair(BN05, cbd_bn05));

    int PL01 = 19;
    std::vector<float> fmc_pl01;
    std::vector<float> cbh_pl01;
    std::vector<float> fl_pl01;
    std::vector<float> h_pl01;
    std::vector<float> cbd_pl01;
    fmc_pl01.push_back(0.013174);
    cbh_pl01.push_back(0.93);
    fl_pl01.push_back(0.838);
    h_pl01.push_back(18405);
    cbd_pl01.push_back(0.03);
    fmcs.insert(std::make_pair(PL01, fmc_pl01));
    cbhs.insert(std::make_pair(PL01, cbh_pl01));
    fls_david.insert(std::make_pair(PL01, fl_pl01));
    hs.insert(std::make_pair(PL01, h_pl01));
    cbds.insert(std::make_pair(PL01, cbd_pl01));

    int PL02 = 20;
    std::vector<float> fmc_pl02;
    std::vector<float> cbh_pl02;
    std::vector<float> fl_pl02;
    std::vector<float> h_pl02;
    std::vector<float> cbd_pl02;
    fmc_pl02.push_back(0.005973);
    cbh_pl02.push_back(7.59);
    fl_pl02.push_back(3.019);
    h_pl02.push_back(20376);
    cbd_pl02.push_back(0.125);
    fmcs.insert(std::make_pair(PL02, fmc_pl02));
    cbhs.insert(std::make_pair(PL02, cbh_pl02));
    fls_david.insert(std::make_pair(PL02, fl_pl02));
    hs.insert(std::make_pair(PL02, h_pl02));
    cbds.insert(std::make_pair(PL02, cbd_pl02));

    int PL03 = 21;
    std::vector<float> fmc_pl03;
    std::vector<float> cbh_pl03;
    std::vector<float> fl_pl03;
    std::vector<float> h_pl03;
    std::vector<float> cbd_pl03;
    fmc_pl03.push_back(0.002481);
    cbh_pl03.push_back(11.05);
    fl_pl03.push_back(3.333);
    h_pl03.push_back(20376);
    cbd_pl03.push_back(0.218);
    fmcs.insert(std::make_pair(PL03, fmc_pl03));
    cbhs.insert(std::make_pair(PL03, cbh_pl03));
    fls_david.insert(std::make_pair(PL03, fl_pl03));
    hs.insert(std::make_pair(PL03, h_pl03));
    cbds.insert(std::make_pair(PL03, cbd_pl03));

    int PL04 = 22;
    std::vector<float> fmc_pl04;
    std::vector<float> cbh_pl04;
    std::vector<float> fl_pl04;
    std::vector<float> h_pl04;
    std::vector<float> cbd_pl04;
    fmc_pl04.push_back(0.002712);
    cbh_pl04.push_back(12.1);
    fl_pl04.push_back(3.249);
    h_pl04.push_back(20376);
    cbd_pl04.push_back(0.274);
    fmcs.insert(std::make_pair(PL04, fmc_pl04));
    cbhs.insert(std::make_pair(PL04, cbh_pl04));
    fls_david.insert(std::make_pair(PL04, fl_pl04));
    hs.insert(std::make_pair(PL04, h_pl04));
    cbds.insert(std::make_pair(PL04, cbd_pl04));

    int PL05 = 23;
    std::vector<float> fmc_pl05;
    std::vector<float> cbh_pl05;
    std::vector<float> fl_pl05;
    std::vector<float> h_pl05;
    std::vector<float> cbd_pl05;
    fmc_pl05.push_back(0.006516);
    cbh_pl05.push_back(0);
    fl_pl05.push_back(4.087);
    h_pl05.push_back(20376);
    cbd_pl05.push_back(0);
    fmcs.insert(std::make_pair(PL05, fmc_pl05));
    cbhs.insert(std::make_pair(PL05, cbh_pl05));
    fls_david.insert(std::make_pair(PL05, fl_pl05));
    hs.insert(std::make_pair(PL05, h_pl05));
    cbds.insert(std::make_pair(PL05, cbd_pl05));

    int PL06 = 24;
    std::vector<float> fmc_pl06;
    std::vector<float> cbh_pl06;
    std::vector<float> fl_pl06;
    std::vector<float> h_pl06;
    std::vector<float> cbd_pl06;
    fmc_pl06.push_back(0.003255);
    cbh_pl06.push_back(0);
    fl_pl06.push_back(3.714);
    h_pl06.push_back(20376);
    cbd_pl06.push_back(0);
    fmcs.insert(std::make_pair(PL06, fmc_pl06));
    cbhs.insert(std::make_pair(PL06, cbh_pl06));
    fls_david.insert(std::make_pair(PL06, fl_pl06));
    hs.insert(std::make_pair(PL06, h_pl06));
    cbds.insert(std::make_pair(PL06, cbd_pl06));

    int PL07 = 25;
    std::vector<float> fmc_pl07;
    std::vector<float> cbh_pl07;
    std::vector<float> fl_pl07;
    std::vector<float> h_pl07;
    std::vector<float> cbd_pl07;
    fmc_pl07.push_back(0.002596);
    cbh_pl07.push_back(0);
    fl_pl07.push_back(4.063);
    h_pl07.push_back(20376);
    cbd_pl07.push_back(0);
    fmcs.insert(std::make_pair(PL07, fmc_pl07));
    cbhs.insert(std::make_pair(PL07, cbh_pl07));
    fls_david.insert(std::make_pair(PL07, fl_pl07));
    hs.insert(std::make_pair(PL07, h_pl07));
    cbds.insert(std::make_pair(PL07, cbd_pl07));

    int PL08 = 26;
    std::vector<float> fmc_pl08;
    std::vector<float> cbh_pl08;
    std::vector<float> fl_pl08;
    std::vector<float> h_pl08;
    std::vector<float> cbd_pl08;
    fmc_pl08.push_back(0.009777);
    cbh_pl08.push_back(0);
    fl_pl08.push_back(0.905);
    h_pl08.push_back(18292);
    cbd_pl08.push_back(0);
    fmcs.insert(std::make_pair(PL08, fmc_pl08));
    cbhs.insert(std::make_pair(PL08, cbh_pl08));
    fls_david.insert(std::make_pair(PL08, fl_pl08));
    hs.insert(std::make_pair(PL08, h_pl08));
    cbds.insert(std::make_pair(PL08, cbd_pl08));

    int PL09 = 27;
    std::vector<float> fmc_pl09;
    std::vector<float> cbh_pl09;
    std::vector<float> fl_pl09;
    std::vector<float> h_pl09;
    std::vector<float> cbd_pl09;
    fmc_pl09.push_back(0.005429);
    cbh_pl09.push_back(0);
    fl_pl09.push_back(3.164);
    h_pl09.push_back(20150);
    cbd_pl09.push_back(0);
    fmcs.insert(std::make_pair(PL09, fmc_pl09));
    cbhs.insert(std::make_pair(PL09, cbh_pl09));
    fls_david.insert(std::make_pair(PL09, fl_pl09));
    hs.insert(std::make_pair(PL09, h_pl09));
    cbds.insert(std::make_pair(PL09, cbd_pl09));

    int PL10 = 28;
    std::vector<float> fmc_pl10;
    std::vector<float> cbh_pl10;
    std::vector<float> fl_pl10;
    std::vector<float> h_pl10;
    std::vector<float> cbd_pl10;
    fmc_pl10.push_back(0.003799);
    cbh_pl10.push_back(0);
    fl_pl10.push_back(2.742);
    h_pl10.push_back(20150);
    cbd_pl10.push_back(0);
    fmcs.insert(std::make_pair(PL10, fmc_pl10));
    cbhs.insert(std::make_pair(PL10, cbh_pl10));
    fls_david.insert(std::make_pair(PL10, fl_pl10));
    hs.insert(std::make_pair(PL10, h_pl10));
    cbds.insert(std::make_pair(PL10, cbd_pl10));

    int PL11 = 29;
    std::vector<float> fmc_pl11;
    std::vector<float> cbh_pl11;
    std::vector<float> fl_pl11;
    std::vector<float> h_pl11;
    std::vector<float> cbd_pl11;
    fmc_pl11.push_back(0.001325);
    cbh_pl11.push_back(0);
    fl_pl11.push_back(2.464);
    h_pl11.push_back(19597);
    cbd_pl11.push_back(0);
    fmcs.insert(std::make_pair(PL11, fmc_pl11));
    cbhs.insert(std::make_pair(PL11, cbh_pl11));
    fls_david.insert(std::make_pair(PL11, fl_pl11));
    hs.insert(std::make_pair(PL11, h_pl11));
    cbds.insert(std::make_pair(PL11, cbd_pl11));

    int DX01 = 30;
    std::vector<float> fmc_dx01;
    std::vector<float> cbh_dx01;
    std::vector<float> fl_dx01;
    std::vector<float> h_dx01;
    std::vector<float> cbd_dx01;
    fmc_dx01.push_back(0.002134);
    cbh_dx01.push_back(0);
    fl_dx01.push_back(8.25);
    h_dx01.push_back(19857);
    cbd_dx01.push_back(0);
    fmcs.insert(std::make_pair(DX01, fmc_dx01));
    cbhs.insert(std::make_pair(DX01, cbh_dx01));
    fls_david.insert(std::make_pair(DX01, fl_dx01));
    hs.insert(std::make_pair(DX01, h_dx01));
    cbds.insert(std::make_pair(DX01, cbd_dx01));

    int DX02 = 31;
    std::vector<float> fmc_dx02;
    std::vector<float> cbh_dx02;
    std::vector<float> fl_dx02;
    std::vector<float> h_dx02;
    std::vector<float> cbd_dx02;
    fmc_dx02.push_back(0.001903);
    cbh_dx02.push_back(0);
    fl_dx02.push_back(7.125);
    h_dx02.push_back(19463);
    cbd_dx02.push_back(0);
    fmcs.insert(std::make_pair(DX02, fmc_dx02));
    cbhs.insert(std::make_pair(DX02, cbh_dx02));
    fls_david.insert(std::make_pair(DX02, fl_dx02));
    hs.insert(std::make_pair(DX02, h_dx02));
    cbds.insert(std::make_pair(DX02, cbd_dx02));
}

/**
 * @brief Sets default value for crown constants when no raster files are provided for them.
 * @param data Cell data
 */
void
setup_crown_const(inputs* data)
{
    if (data->cbd == -9999)
    {
        data->cbd = cbds[data->nftype][0];
    }
    if (data->cbh == -9999)
    {
        data->cbh = cbhs[data->nftype][0];
    }
}

float
rate_of_spread_k(inputs* data,
                 fuel_coefs* ptr,
                 main_outs* at,
                 weatherDF* wdf_ptr,
                 arguments* args)  // incluir efecto pendiente aqui y no afuera
{
    float p1, p2, p3, ws, tmp, rh, ch, fmc, fch, fv, ps, fp;
    // se = slope_effect(inp) ;
    ws = wdf_ptr->ws;
    tmp = wdf_ptr->tmp;
    rh = wdf_ptr->rh;
    ps = at->se;  // hacerlo con elevaciones
    p1 = -12.86;
    p2 = 0.04316;
    p3 = 13.8;
    float midpoint = 57.09;
    float steepness = 0.081;
    float sigmoid = 1.0 / (1.0 + exp(-steepness * (rh - midpoint)));
    ch = 4 + 16 * sigmoid - 0.00982 * tmp;
    fmc = fmcs[data->nftype][0] * 60;  // factor de propagacion en m/min
    fch = min(51.43, 52.3342 * pow(ch, -1.3035));       // KITRAL (RH/T)
    fv = p1 * exp(-p2 * ws) + p3;                        // KITRAL wind factor

    // ================= Modo de humedad seleccionable (--fch-mode) =================
    // Cada modo define (fch, phi_w). La pendiente (phi_s) y la combinacion vectorial B
    // son comunes. Todo anclado a la referencia KITRAL (T=25, RH=40, ws=30 km/h).
    // kitral: fch(RH/T), viento fv           | emc : EMC(Simard)->etaM(Rothermel)
    // ffmc  : FFMC->humedad->ff (memoria)     | isi : ffmc + viento FBP fW (reemplaza fv)
    // bui   : isi + efecto buildup BE(BUI)   (sequia estacional profunda)
    const float fch0 = 4.174f;   // fch KITRAL en (T=25, RH=40)
    const float m0   = 8.0f;     // humedad de combustible de referencia (%)
    const std::string& mode = args->FchMode;
    float phi_w = fv;            // por defecto: viento KITRAL
    if (mode == "emc") {
        float m = emc_simard(tmp, rh);
        fch = fch0 * eta_M(m) / eta_M(m0);
    } else if (mode == "ffmc" || mode == "isi" || mode == "bui") {
        float F = wdf_ptr->ffmc;
        float m = (F <= 0.0f) ? emc_simard(tmp, rh) : 147.2f * (101.0f - F) / (59.5f + F);
        fch = fch0 * ff_moist(m) / ff_moist(m0);        // humedad fina (con memoria FFMC)
        if (mode == "isi" || mode == "bui") {
            // viento via funcion FBP fW (exponencial), anclada a fv(30) en ws=30
            float fv30 = p1 * exp(-p2 * 30.0f) + p3;
            phi_w = fv30 * exp(0.05039f * (ws - 30.0f));
        }
        if (mode == "bui") {                            // efecto buildup: sequia estacional
            float BUI = wdf_ptr->bui;
            if (BUI > 0.0f) {
                const float q = 0.85f, BUI0 = 60.0f;    // buildup effect estilo FBP
                float BE = exp(50.0f * log(q) * (1.0f / BUI - 1.0f / BUI0));
                fch *= BE;
            }
        }
    }

    // ========== Acondicionamiento espacial de humedad (sunny/shade) ==========
    // La humedad del combustible fino muerto se condiciona por celda segun la
    // exposicion solar (pendiente + aspecto) y el sombreado de dosel (ccf).
    // Umbrias/sombreadas -> mas humedas -> fch menor -> ROS menor. La humedad
    // "expuesta" base es EMC(T,RH). Solo frena celdas sombreadas (expuesta = base).
    if (args->FmcShading) {
        const float dMax = 4.0f;                       // maximo aumento de humedad por sombra (%)
        // ---- posicion solar DINAMICA (cualquier hemisferio) desde lat + fecha/hora ----
        float lat_deg = args->HasLatitude ? args->Latitude : data->lat;   // --latitude o lat por celda
        float lat_r = lat_deg * M_PI / 180.0f;
        float doy   = (wdf_ptr->doy  > 0.0f) ? wdf_ptr->doy  : 15.0f;
        float hour  = (wdf_ptr->hour > 0.0f) ? wdf_ptr->hour : 12.0f;
        float decl  = 23.45f * M_PI / 180.0f * sin(2.0f * M_PI * (284.0f + doy) / 365.0f);
        float Hang  = (hour - 12.0f) * 15.0f * M_PI / 180.0f;             // angulo horario (hora solar aprox)
        float sin_alt = sin(lat_r) * sin(decl) + cos(lat_r) * cos(decl) * cos(Hang);
        sin_alt = (sin_alt < -1.0f) ? -1.0f : (sin_alt > 1.0f ? 1.0f : sin_alt);
        float sun_alt = asin(sin_alt);
        float cosAz = (sin(decl) - sin_alt * sin(lat_r)) / (cos(sun_alt) * cos(lat_r) + 1e-6f);
        cosAz = (cosAz < -1.0f) ? -1.0f : (cosAz > 1.0f ? 1.0f : cosAz);
        float sun_az = acos(cosAz);                                       // 0..pi medido desde el NORTE
        if (sin(Hang) > 0.0f) sun_az = 2.0f * M_PI - sun_az;             // tarde -> lado oeste (auto ambos hemisf.)
        // ---- geometria ladera-sol ----
        float slope_r = atan(data->ps / 100.0f);
        float aspect  = fmod(data->saz + 180.0f, 360.0f) * M_PI / 180.0f; // direccion que enfrenta la ladera
        float cos_i;
        if (sun_alt <= 0.0f) cos_i = 0.0f;                                // sol bajo el horizonte (noche): sin secado
        else cos_i = cos(slope_r) * sin(sun_alt) + sin(slope_r) * cos(sun_alt) * cos(sun_az - aspect);
        float S = cos_i; if (S < 0.0f) S = 0.0f; if (S > 1.0f) S = 1.0f;  // exposicion topografica [0,1]
        float cc = data->ccf; if (cc < 0.0f) cc = 0.0f; if (cc > 1.0f) cc = 1.0f;
        float E = S * (1.0f - cc);                     // exposicion efectiva del combustible al sol
        float m_base = emc_simard(tmp, rh);            // humedad expuesta (base)
        float m_eff  = m_base + dMax * (1.0f - E);     // sombra/umbria -> mas humedo
        fch *= eta_M(m_eff) / eta_M(m_base);           // ratio de damping (<=1) frena celdas sombreadas
    }

    // ================= B (S&B-like): viento y pendiente como vectores =================
    float slope_deg = atan(data->ps / 100.0f) * 180.0f / M_PI;   // slope.asc % -> grados
    float se_k = 1.0f + 0.023322f * slope_deg + 0.00013585f * slope_deg * slope_deg;
    float phi_s = se_k - 1.0f;                          // coef. pendiente
    float th = (wdf_ptr->waz - data->saz) * M_PI / 180.0f;       // viento relativo al upslope
    float vx = phi_s + phi_w * cos(th);
    float vy = phi_w * sin(th);
    float phi_eff = sqrt(vx * vx + vy * vy);
    at->raz = fmod(data->saz + atan2(vy, vx) * 180.0f / M_PI + 360.0f, 360.0f);  // resultante
    at->rss = fmc * fch * (1.0f + phi_eff);
    return at->rss * (at->rss >= 0);
}

// TODO: citation needed
float
flankfire_ros_k(float ros, float bros, float lb)
{
    return ((ros + bros) / (lb * 2.0));
}

/* ----------------- Length-to-Breadth --------------------------*/
// TODO: citation needed
float
l_to_b(float ws, fuel_coefs* ptr, const std::string& lb_mode)
{
    float l1, l2, lb;
    if (lb_mode == "sb") {
        // length-to-breadth de S&B (Anderson/FARSITE): elipse más alargada
        float f = 1000.0 / 3600.0;   // km/h -> m/s
        lb = pow(0.936 * exp(0.2566 * f * ws) + 0.461 * exp(-0.1548 * f * ws) - 0.397, 0.45);
        return lb;
    }
    l1 = 2.233;     // 1.411; // ptr->l1 ;
    l2 = -0.01031;  // 0.01745; // ptr->l2 ;
    lb = 1.0 + pow(l1 * exp(-l2 * ws) - l1, 2.0);
    return lb;
}

/* ----------------- Back Rate of Spread --------------------------*/
// TODO: citation needed
float
backfire_ros_k(const main_outs* at, const snd_outs* sec)
{
    float hb, bros, lb;
    // lb = l_to_b(data->fueltype,at->wsv) ;
    lb = sec->lb;
    hb = (lb + sqrt(pow(lb, 2) - 1.0)) / (lb - sqrt(pow(lb, 2) - 1.0));

    bros = at->rss / hb;

    return bros * (bros >= 0);
}

// TODO: citation needed
float
slope_effect(const float elev_i, const float elev_j, const int cellsize)
{
    float ps_ij = (elev_j - elev_i) / (cellsize / 4.);  // cellsize corresponds to the perimeter of the cell
    float se;
    se = 1. + 0.023322 * ps_ij + 0.00013585 * std::pow(ps_ij, 2);

    return se;
}

// TODO: citation needed
float
flame_length(const main_outs* at)  // REVISAR ESTA ECUACI�N
{
    float ib, fl;

    ib = at->sfi;

    fl = 0.0775 * pow(ib, 0.46);
    return fl;
}

// TODO: citation needed
float
byram_intensity(inputs* data, main_outs* at)
{
    float ros, H, wa, ib;
    ros = at->rss;
    H = hs[data->nftype][0];
    wa = fls_david[data->nftype][0];
    ib = H * wa * ros / 60;
    ib = std::ceil(ib * 100.0) / 100.0;
    return ib;  // unidad de medida
}

// Longitud de llama a partir del ROS real de la celda (bypass del bug de metrics->rss=0).
// Usa la misma intensidad de Byram: ib = H * wa * ros / 60 ; fl = 0.0775 * ib^0.46.
float
flame_length_from_ros(inputs* data, float ros)
{
    if (ros <= 0.0f) return 0.0f;
    float ib = hs[data->nftype][0] * fls_david[data->nftype][0] * ros / 60.0f;
    return 0.0775f * pow(ib, 0.46f);
}

// ---- Fuego de copa: intensidad y llama IDÉNTICAS a S&B/FBP (consistencia entre sistemas) ----
static int16_t HEAT_YIELD_K = 18000;  // kJ/kg (igual que HEAT_YIELD en S&B y Portugal)

float
crown_byram_intensity_k(main_outs* at, inputs* data)
{
    float canopy_height = (data->tree_height == -9999) ? (data->cbh * 2.0f)
                                                       : (data->tree_height - data->cbh);
    if (canopy_height < 0) canopy_height = 0;
    return std::ceil((HEAT_YIELD_K / 60.0f) * data->cbd * canopy_height * at->ros_active * 100.0f) / 100.0f;
}

float
crown_flame_length_k(float intensity)
{
    float fl = 0.1f * pow(intensity, 0.5f);
    return (fl < 0.01f) ? 0.0f : std::ceil(fl * 100.0f) / 100.0f;
}

bool
fire_type(inputs* data, main_outs* at, int FMC)
{
    float intensity, critical_intensity, cbh;
    bool crownFire = false;
    intensity = at->sfi;
    cbh = data->cbh;
    critical_intensity = pow((0.01 * cbh * (460 + 25.9 * FMC)), 1.5);
    if ((intensity > critical_intensity) && cbh > 0)
        crownFire = true;
    return crownFire;
}

// TODO: citation needed
float
crownfractionburn(inputs* data, main_outs* at, int FMC)
{
    // generar output de cfb
    float a, cbd, ros, ros0, H, wa, i0, cbh, cfb;
    cbh = data->cbh;
    i0 = pow((0.01 * cbh * (460 + 25.9 * FMC)), 1.5);
    H = hs[data->nftype][0];
    wa = fls_david[data->nftype][0];
    cbd = data->cbd;

    ros0 = 60 * i0 / (H * wa);
    ros = at->rss;
    if (cbd > 0)
    {
        a = -log(0.1) / (0.9 * (3.0 / cbd - ros0));
    }
    else
    {
        a = 0.23;
    }
    cfb = 1 - exp(-a * (ros - ros0));

    return cfb;
}

// TODO: citation needed
float
active_rate_of_spreadPL04(inputs* data, main_outs* at, weatherDF* wdf_ptr, arguments* args)  // En KITRAL SE USA PL04
{
    float p1, p2, p3, ws, tmp, rh, ch, fmc, fch, fv, ps, ros_active, rospl04, fp, ros_final, ros;

    // se = slope_effect(inp) ;
    ws = wdf_ptr->ws;
    tmp = wdf_ptr->tmp;
    rh = wdf_ptr->rh;
    ps = at->se;
    p1 = -12.86;
    p2 = 0.04316;
    p3 = 13.8;

    ch = (-2.97374 + 0.262 * rh - 0.00982 * tmp);
    fmc = 0.002712 * 60;  // factor de propagacion en m/min de PL04
    fch = (389.1624 + 14.3 * ch + 0.02 * pow(ch, 2.0))
          / (3.559 + 1.6615 * ch + 2.62392 * pow(ch, 2.0));  // es -14.3 segun el libro
    fv = p1 * exp(-p2 * ws * 0.4) + p3;
    // fp = 1.0 + 0.023322 * data->ps + 0.00013585 * pow(data->ps, 2.0);
    if (ps == 0)
    {
        rospl04 = fmc * fch * (fv);
    }
    else
    {
        rospl04 = fmc * fch * (fv + ps);
    }
    ros_active = args->ROS10Factor * rospl04;  // factor de copa activa (Scott & Reinhardt, def 3.34); checkActive aplica el criterio cbd*rac>=3
    // ros_final=3.34*rospl04
    return ros_active;
}

// TODO: citation needed
float
final_rate_of_spreadPL04(main_outs* at)  // En KITRAL SE USA PL04
{
    float ros_active, ros_final, ros;
    ros = at->rss;
    ros_active = at->ros_active;
    ros_final = ros + at->cfb * (ros_active - ros);
    return ros_final;
}

bool
checkActive(inputs* data, main_outs* at, int FMC)  // En KITRAL SE USA PL04
{
    float ros_critical, cbd, H, wa, i0, cbh;
    bool active;
    cbh = data->cbh;

    i0 = pow((0.01 * cbh * (460 + 25.9 * FMC)), 1.5);
    H = hs[data->nftype][0];
    wa = fls_david[data->nftype][0];
    ros_critical = 60 * i0 / (H * wa);
    cbd = data->cbd;

    active = cbd * ros_critical >= 3;
    return active;
}

// TODO: citation needed
float
backfire_ros10_k(fire_struc* hptr, snd_outs* sec)
{
    float hb, bros, lb;
    lb = sec->lb;
    hb = (lb + sqrt(pow(lb, 2) - 1.0)) / (lb - sqrt(pow(lb, 2) - 1.0));

    bros = hptr->ros / hb;

    return bros;
}

/***
 * fire_struc *hptr: headstruct
 * fire_struc *fptr: flankstruct
 * fire_struc *bptr: backstruct
 */
void
calculate_k(inputs* data,
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
            weatherDF* wdf_ptr)
{
    // Aux
    float ros, bros, lb, fros;
    int FMC;
    bool crownFire = false;
    at->cfb = 0;
    // Populate fuel coefs struct
    if (args->verbose)
    {
        std::cout << "Populate fuel types " << std::endl;
        std::cout << "NfTypes:" << data->nftype << std::endl;
    }
    // FMC = Fuel Moisture Content
    FMC = args->FMC;
    ptr->nftype = data->nftype;
    ptr->fmc = fmcs[data->nftype][0];
    ptr->cbh = data->cbh;

    ptr->fl = fls_david[data->nftype][0];

    ptr->h = hs[data->nftype][0];
    // cout << "   h " << ptr->h << "\n";
    float elevation_origin = data->elev;
    float elevation_destiny = head->elev;
    at->se = slope_effect(elevation_origin, elevation_destiny, cellsize);
    // Step 1: Calculate HROS (surface)
    at->rss = rate_of_spread_k(data, ptr, at, wdf_ptr, args);

    hptr->rss = at->rss;
    // Step 2: Calculate Length-to-breadth
    sec->lb = l_to_b(wdf_ptr->ws, ptr, args->LbMode);

    // Step 3: Calculate BROS (surface)
    bptr->rss = backfire_ros_k(at, sec);

    // Step 4: Calculate central FROS (surface)
    fptr->rss = flankfire_ros_k(hptr->rss, bptr->rss, sec->lb);

    // Step 5: Ellipse components
    at->a = (hptr->rss + bptr->rss) / 2.;
    at->b = (hptr->rss + bptr->rss) / (2. * sec->lb);
    at->c = (hptr->rss - bptr->rss) / 2.;

    // Step 6: Byram Intensity
    at->sfi = byram_intensity(data, at);

    // Step 7: Flame Length
    at->fl = flame_length(at);

    // Step 8: Flame angle
    at->angle = angleFL(wdf_ptr->ws, at);

    // Step 9: Flame Height
    at->fh = flame_height(at);

    // Step 10: Criterion for Crown Fire Initiation (no init if user does not
    // want to include it)
    if (args->AllowCROS && (data->cbh > 0))
    {
        if (activeCrown)
        {
            // si el fuego esta activo en copas chequeamos condiciones
            at->ros_active = active_rate_of_spreadPL04(data, at, wdf_ptr, args);
            if (!checkActive(data, at, FMC))
            {
                activeCrown = false;
            }
        }
        else
        {
            crownFire = fire_type(data, at, FMC);
            if (args->verbose)
            {
                cout << "Checking crown Fire conditions " << crownFire << "\n";
            }
        }
    }
    else
    {
        crownFire = false;
        activeCrown = false;
    }
    // If we have Crown fire, update the ROSs
    if (crownFire)
    {
        at->ros_active = active_rate_of_spreadPL04(data, at, wdf_ptr, args);
        at->cfb = crownfractionburn(data, at, FMC);

        hptr->ros = final_rate_of_spreadPL04(at);
        at->rss = hptr->ros;
        bptr->ros = backfire_ros10_k(hptr, sec);
        fptr->ros = flankfire_ros_k(hptr->ros, bptr->ros, sec->lb);

        if (args->verbose)
        {
            cout << "hptr->ros = " << hptr->ros << "\n";
            cout << "bptr->ros = " << bptr->ros << "\n";
            cout << "fptr->ros = " << fptr->ros << "\n";
        }

        at->a = (hptr->ros + bptr->ros) / 2.;
        at->b = (hptr->ros + bptr->ros) / (2. * sec->lb);
        at->c = (hptr->ros - bptr->rss) / 2;
        at->crown_intensity = crown_byram_intensity_k(at, data);
        at->crown_flame_length = crown_flame_length_k(at->crown_intensity);
        at->crown = 1;
        activeCrown = true;
    }
    else if (activeCrown)
    {
        at->cfb = crownfractionburn(data, at,
                                    FMC);  // lo calculamos igual porque lo necesitamos para el output
        hptr->ros = at->ros_active;
        at->rss = hptr->ros;
        bptr->ros = backfire_ros10_k(hptr, sec);
        fptr->ros = flankfire_ros_k(hptr->ros, bptr->ros, sec->lb);

        if (args->verbose)
        {
            cout << "hptr->ros = " << hptr->ros << "\n";
            cout << "bptr->ros = " << bptr->ros << "\n";
            cout << "fptr->ros = " << fptr->ros << "\n";
        }

        at->a = (hptr->ros + bptr->ros) / 2.;
        at->b = (hptr->ros + bptr->ros) / (2. * sec->lb);
        at->c = (hptr->ros - bptr->rss) / 2;
        at->crown_intensity = crown_byram_intensity_k(at, data);
        at->crown_flame_length = crown_flame_length_k(at->crown_intensity);
        at->crown = 1;
        // std::cout  << "ros_activo: "  <<hptr->ros <<  std::endl;
    }

    // Otherwise, use the surface values
    else
    {
        at->crown = 0;
        at->crown_intensity = 0;
        at->crown_flame_length = 0;
        hptr->ros = hptr->rss;
        bptr->ros = bptr->rss;
        fptr->ros = fptr->rss;
        if (args->verbose)
        {
            cout << "hptr->ros = " << hptr->ros << "\n";
            cout << "bptr->ros = " << bptr->ros << "\n";
            cout << "fptr->ros = " << fptr->ros << "\n";
        }
    }

    if (args->verbose)
    {
        cout << "--------------- Inputs --------------- \n";

        cout << "coef data->cbh = " << data->cbh << "\n";
        cout << "coef ptr->fmc = " << ptr->fmc << "\n";
        cout << "coef ptr->cbh = " << ptr->cbh << "\n";
        cout << "coef ptr->fl = " << ptr->fl << "\n";
        cout << "coef ptr->h = " << ptr->h << "\n";
        // cout << "coef ptr->q2 = " << ptr->q2 << "\n" ;
        // cout << "coef ptr->q3 = " << ptr->q3 << "\n" ;
        cout << "\n";

        cout << "---------------- Outputs --------------- \n";
        cout << "at->rss = " << at->rss << "\n";
        cout << "hptr->rss = " << hptr->rss << "\n";
        cout << "lb = " << sec->lb << "\n";
        cout << "bptr->rss = " << bptr->rss << "\n";
        cout << "fptr->rss = " << fptr->rss << "\n";
        cout << "axis a = " << at->a << "\n";
        cout << "axis b = " << at->b << "\n";
        cout << "axis c = " << at->c << "\n";
        cout << "fl = " << at->fl << "\n";
        cout << "angle = " << at->angle << "\n";
        cout << "fh = " << at->fh << "\n";
        cout << "Crown Fire = " << crownFire << "\n";
    }
}

void
determine_destiny_metrics_k(inputs* data, fuel_coefs* ptr, arguments* args, main_outs* metrics)
{
    // Hack: Initialize coefficients
    // setup_const();
    setup_crown_const(data);

    ptr->cbh = data->cbh;
    // Aux
    float ros = 0, bros = 0, lb = 0, fros = 0;
    int FMC = args->FMC;
    bool crownFire = false;
    metrics->cfb = 0;
    // ptr->q1 = q_coeff[data->nftype][0];
    // ptr->q2 = q_coeff[data->nftype][1];
    // ptr->q3 = q_coeff[data->nftype][2];
    ptr->nftype = data->nftype;
    // Step 6: Byram Intensity
    metrics->sfi = byram_intensity(data, metrics);
    // Step 7: Flame Length
    metrics->fl = flame_length(metrics);
    // Step 10: Criterion for Crown Fire Initiation (no init if user does not
    // want to include it)
    if (args->AllowCROS && data->cbh > 0)
    {
        crownFire = fire_type(data, metrics, FMC);
        if (crownFire)
        {
            metrics->cfb = crownfractionburn(data, metrics, FMC);
        }
        if (args->verbose)
        {
            cout << "Checking crown Fire conditions " << crownFire << "\n";
        }
    }
    else
    {
        crownFire = false;
    }

    metrics->crown = int(crownFire);
}
