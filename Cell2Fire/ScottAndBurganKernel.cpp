/* coding: utf-8
__version__ = "3.0"
__author__ = "Jaime Carrasco-Barra"
__maintainer__ = "Jaime Carrasco-Barra, Matilde Rivas, David Palacios"
*/
#include "ScottAndBurganKernel.h"
#include <cstdint>   // int16_t: no llega transitivamente en MinGW/MSVC
#include "Cells.h"
#include "FuelModelUtils.h"
#include "ReadArgs.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

/**
 * Global coefficients
 */
int16_t HEAT_YIELD = 18000;  // unidad kJ/kg


// ===================== Rothermel S&B con humedad CONTINUA (operativo) =====================
// Reemplaza la forma cerrada p/q por el modelo Rothermel de superficie evaluado con la
// humedad real (5 clases). Validado <1% vs firelab/behave (dominio publico) y paquete R.
namespace {
struct FuelSB { double depth,mx,hD,hL,l1,l10,l100,lh,lw,s1,sh,sw; int dyn; };
static std::unordered_map<int,FuelSB>& sbTable(){
    static std::unordered_map<int,FuelSB> T;
    if(T.empty()){
    // Modelos originales de Anderson (1982), NFFL 1-13. Se incluyen porque hay
    // cartografia historica y de LANDFIRE codificada asi; Scott & Burgan (2005)
    // los sucede pero no los reemplaza en los datos existentes. Cargas en
    // toneladas/acre, como el resto de la tabla. Ninguno es dinamico.
    T[1]=FuelSB{1.0,0.12,8000.0,8000.0,0.7405,0.0,0.0,0.0,0.0,3500.0,1500.0,1500.0,0};   // FM1: Short grass
    T[2]=FuelSB{1.0,0.15,8000.0,8000.0,2.0038,1.0019,0.5009,0.5009,0.0,3000.0,1500.0,1500.0,0};   // FM2: Timber grass and understory
    T[3]=FuelSB{2.5,0.25,8000.0,8000.0,3.0056,0.0,0.0,0.0,0.0,1500.0,1500.0,1500.0,0};   // FM3: Tall grass
    T[4]=FuelSB{6.0,0.2,8000.0,8000.0,5.0094,4.0075,2.0038,0.0,5.0094,2000.0,1500.0,1500.0,0};   // FM4: Chaparral
    T[5]=FuelSB{2.0,0.2,8000.0,8000.0,1.0019,0.5009,0.0,0.0,2.0038,2000.0,1500.0,1500.0,0};   // FM5: Brush
    T[6]=FuelSB{2.5,0.25,8000.0,8000.0,1.5028,2.5047,2.0038,0.0,0.0,1750.0,1500.0,1500.0,0};   // FM6: Dormant brush, hardwood slash
    T[7]=FuelSB{2.5,0.4,8000.0,8000.0,1.1326,1.8731,1.5028,0.0,0.3703,1750.0,1500.0,1500.0,0};   // FM7: Southern rough
    T[8]=FuelSB{0.2,0.3,8000.0,8000.0,1.5028,1.0019,2.5047,0.0,0.0,2000.0,1500.0,1500.0,0};   // FM8: Short needle litter
    T[9]=FuelSB{0.2,0.25,8000.0,8000.0,2.9185,0.4138,0.1525,0.0,0.0,2500.0,1500.0,1500.0,0};   // FM9: Long needle or hardwood litter
    T[10]=FuelSB{1.0,0.25,8000.0,8000.0,3.0056,2.0038,5.0094,0.0,2.0038,2000.0,1500.0,1500.0,0};   // FM10: Timber litter & understory
    T[11]=FuelSB{1.0,0.15,8000.0,8000.0,1.5028,4.5085,5.5103,0.0,0.0,1500.0,1500.0,1500.0,0};   // FM11: Light logging slash
    T[12]=FuelSB{2.3,0.2,8000.0,8000.0,4.0075,14.0263,16.531,0.0,0.0,1500.0,1500.0,1500.0,0};   // FM12: Medium logging slash
    T[13]=FuelSB{3.0,0.25,8000.0,8000.0,7.0132,23.0432,28.0526,0.0,0.0,1500.0,1500.0,1500.0,0};   // FM13: Heavy logging slash
    T[101]=FuelSB{0.4,0.15,8000.0,8000.0,0.1,0.0,0.0,0.3,0.0,2200.0,2000.0,1500.0,1};
    T[102]=FuelSB{1.0,0.15,8000.0,8000.0,0.1,0.0,0.0,1.0,0.0,2000.0,1800.0,1500.0,1};
    T[103]=FuelSB{2.0,0.3,8000.0,8000.0,0.1,0.4,0.0,1.5,0.0,1500.0,1300.0,1500.0,1};
    T[104]=FuelSB{2.0,0.15,8000.0,8000.0,0.25,0.0,0.0,1.9,0.0,2000.0,1800.0,1500.0,1};
    T[105]=FuelSB{1.5,0.4,8000.0,8000.0,0.4,0.0,0.0,2.5,0.0,1800.0,1600.0,1500.0,1};
    T[106]=FuelSB{1.5,0.4,9000.0,9000.0,0.1,0.0,0.0,3.4,0.0,2200.0,2000.0,1500.0,1};
    T[107]=FuelSB{3.0,0.15,8000.0,8000.0,1.0,0.0,0.0,5.4,0.0,2000.0,1800.0,1500.0,1};
    T[108]=FuelSB{4.0,0.3,8000.0,8000.0,0.5,1.0,0.0,7.3,0.0,1500.0,1300.0,1500.0,1};
    T[109]=FuelSB{5.0,0.4,8000.0,8000.0,1.0,1.0,0.0,9.0,0.0,1800.0,1600.0,1500.0,1};
    T[110]=FuelSB{0.35,24.0,19000.0,19000.0,0.3,0.0,0.0,1.2,0.0,6000.0,6000.0,6000.0,1};
    T[111]=FuelSB{0.6,24.0,19000.0,19000.0,0.5,0.1,0.0,2.5,0.3,4000.0,6000.0,4000.0,1};
    T[121]=FuelSB{0.9,0.15,8000.0,8000.0,0.2,0.0,0.0,0.5,0.65,2000.0,1800.0,1800.0,1};
    T[122]=FuelSB{1.5,0.15,8000.0,8000.0,0.5,0.5,0.0,0.6,1.0,2000.0,1800.0,1800.0,1};
    T[123]=FuelSB{1.8,0.4,8000.0,8000.0,0.3,0.25,0.0,1.45,1.25,1800.0,1600.0,1600.0,1};
    T[124]=FuelSB{2.1,0.4,8000.0,8000.0,1.9,0.3,0.1,3.4,7.1,1800.0,1600.0,1600.0,1};
    T[141]=FuelSB{1.0,0.15,8000.0,8000.0,0.25,0.25,0.0,0.15,1.3,2000.0,1800.0,1600.0,1};
    T[142]=FuelSB{1.0,0.15,8000.0,8000.0,1.35,2.4,0.75,0.0,3.85,2000.0,1800.0,1600.0,1};
    T[143]=FuelSB{2.4,0.4,8000.0,8000.0,0.45,3.0,0.0,0.0,6.2,1600.0,1800.0,1400.0,1};
    T[144]=FuelSB{3.0,0.3,8000.0,8000.0,0.85,1.15,0.2,0.0,2.55,2000.0,1800.0,1600.0,1};
    T[145]=FuelSB{6.0,0.15,8000.0,8000.0,3.6,2.1,0.0,0.0,2.9,750.0,1800.0,1600.0,1};
    T[146]=FuelSB{2.0,0.3,8000.0,8000.0,2.9,1.45,0.0,0.0,1.4,750.0,1800.0,1600.0,1};
    T[147]=FuelSB{6.0,0.15,8000.0,8000.0,3.5,5.3,2.2,0.0,3.4,750.0,1800.0,1600.0,1};
    T[148]=FuelSB{3.0,0.4,8000.0,8000.0,2.05,3.4,0.85,0.0,4.35,750.0,1800.0,1600.0,1};
    T[149]=FuelSB{4.4,0.4,8000.0,8000.0,4.5,2.45,0.0,1.55,7.0,750.0,1800.0,1500.0,1};
    T[150]=FuelSB{4.0,20.0,8000.0,8000.0,1.3,1.0,1.0,2.0,2.0,640.0,2200.0,640.0,0};
    T[151]=FuelSB{3.0,13.0,10000.0,10000.0,2.0,3.0,1.0,0.5,2.0,640.0,2200.0,640.0,0};
    T[152]=FuelSB{6.0,15.0,8000.0,8000.0,2.2,4.8,1.8,3.0,2.8,500.0,1500.0,500.0,0};
    T[153]=FuelSB{3.0,15.0,9211.0,9211.0,3.0,4.5,1.1,1.4,5.0,350.0,1500.0,250.0,0};
    T[154]=FuelSB{3.0,25.0,9200.0,9200.0,5.5,0.8,0.1,0.75,2.5,640.0,1500.0,640.0,0};
    T[155]=FuelSB{0.55,25.0,19500.0,19500.0,1.0,1.0,0.0,1.5,5.5,4500.0,8500.0,4000.0,1};
    T[156]=FuelSB{0.9,20.0,20500.0,20500.0,4.0,0.5,0.0,0.0,7.0,3000.0,3000.0,3000.0,0};
    T[157]=FuelSB{0.5,35.0,21000.0,21000.0,6.0,0.5,0.0,0.0,7.5,4500.0,4500.0,4500.0,0};
    T[158]=FuelSB{1.7,24.0,20500.0,20500.0,6.0,4.0,0.0,0.0,13.0,2500.0,3000.0,3000.0,0};
    T[159]=FuelSB{1.05,35.0,21000.0,21000.0,9.5,2.5,0.0,0.0,14.5,3500.0,4000.0,4000.0,0};
    T[161]=FuelSB{0.6,0.2,8000.0,8000.0,0.2,0.9,1.5,0.2,0.9,2000.0,1800.0,1600.0,1};
    T[162]=FuelSB{1.0,0.3,8000.0,8000.0,0.95,1.8,1.25,0.0,0.2,2000.0,1800.0,1600.0,1};
    T[163]=FuelSB{1.3,0.3,8000.0,8000.0,1.1,0.15,0.25,0.65,1.1,1800.0,1600.0,1400.0,1};
    T[164]=FuelSB{0.5,0.12,8000.0,8000.0,4.5,0.0,0.0,0.0,2.0,2300.0,1800.0,2000.0,1};
    T[165]=FuelSB{1.0,0.25,8000.0,8000.0,4.0,4.0,3.0,0.0,3.0,1500.0,1800.0,750.0,1};
    T[166]=FuelSB{0.4,26.0,21000.0,20500.0,1.37,2.89,1.59,0.0,1.84,4500.0,4200.0,5000.0,0};
    T[167]=FuelSB{0.1,30.0,20500.0,20500.0,2.71,1.0,0.0,0.66,0.1,5500.0,8000.0,4500.0,1};
    T[168]=FuelSB{0.3,35.0,19500.0,19500.0,4.5,1.5,0.5,2.35,0.48,6000.0,8000.0,4500.0,1};
    T[169]=FuelSB{0.63,30.0,20000.0,20000.0,4.54,1.87,0.61,0.0,9.08,6000.0,4921.0,5000.0,0};
    T[170]=FuelSB{0.5,27.0,20500.0,20500.0,5.65,1.5,0.48,0.0,7.89,5000.0,4921.0,5500.0,0};
    T[171]=FuelSB{0.5,40.0,20500.0,21500.0,7.21,3.0,0.0,0.0,6.89,5500.0,5500.0,6000.0,0};
    T[172]=FuelSB{0.64,32.0,21000.0,21000.0,8.37,3.81,0.0,0.0,4.51,4700.0,4200.0,5000.0,0};
    T[181]=FuelSB{0.2,0.3,8000.0,8000.0,1.0,2.2,3.6,0.0,0.0,2000.0,1800.0,1600.0,1};
    T[182]=FuelSB{0.2,0.25,8000.0,8000.0,1.4,2.3,2.2,0.0,0.0,2000.0,1800.0,1600.0,1};
    T[183]=FuelSB{0.3,0.2,8000.0,8000.0,0.5,2.2,2.8,0.0,0.0,2000.0,1800.0,1600.0,1};
    T[184]=FuelSB{0.4,0.25,8000.0,8000.0,0.5,1.5,4.2,0.0,0.0,2000.0,1800.0,1600.0,1};
    T[185]=FuelSB{0.6,0.25,8000.0,8000.0,1.15,2.5,4.4,0.0,0.0,2000.0,1800.0,160.0,1};
    T[186]=FuelSB{0.3,0.25,8000.0,8000.0,2.4,1.2,1.2,0.0,0.0,2000.0,1800.0,1600.0,1};
    T[187]=FuelSB{0.4,0.25,8000.0,8000.0,0.3,1.4,8.1,0.0,0.0,2000.0,1800.0,1600.0,1};
    T[188]=FuelSB{0.3,0.35,8000.0,8000.0,5.8,1.4,1.1,0.0,0.0,1800.0,1800.0,1600.0,1};
    T[189]=FuelSB{0.6,0.35,8000.0,8000.0,6.65,3.3,4.15,0.0,0.0,1800.0,1800.0,1600.0,1};
    T[190]=FuelSB{0.05,28.0,20500.0,20500.0,3.75,2.0,1.0,0.0,1.18,6500.0,4921.0,4500.0,0};
    T[191]=FuelSB{0.15,25.0,20500.0,20500.0,2.67,1.27,0.69,0.0,1.16,4500.0,5500.0,5000.0,0};
    T[192]=FuelSB{0.1,45.0,20500.0,21500.0,6.5,1.5,0.0,0.0,0.0,5500.0,5500.0,5500.0,0};
    T[193]=FuelSB{0.32,26.0,21000.0,20500.0,4.63,2.96,1.27,0.0,1.12,4200.0,4200.0,5000.0,0};
    T[201]=FuelSB{1.0,0.25,8000.0,8000.0,1.5,3.0,11.0,0.0,0.0,2000.0,1800.0,1600.0,1};
    T[202]=FuelSB{1.0,0.25,8000.0,8000.0,4.5,4.25,4.0,0.0,0.0,2000.0,1800.0,1600.0,1};
    T[203]=FuelSB{1.2,0.25,8000.0,8000.0,5.5,2.75,3.0,0.0,0.0,2000.0,1800.0,1600.0,1};
    T[204]=FuelSB{2.7,0.25,8000.0,8000.0,5.25,3.5,5.25,0.0,0.0,2000.0,1800.0,1600.0,1};
    }
        // --- Modelos de combustible de Portugal (Fernandes et al.) ---
    T[211]=FuelSB{1.0499,0.260,9028.4,9028.4,2.0654,1.3204,0.5665,0.0000,0.4996,1280.2,1500.0,1524.0,0}; // F-EUC
    T[212]=FuelSB{0.4921,0.250,8813.4,8813.4,1.1911,0.5665,0.3078,0.0000,0.5175,1371.6,1500.0,1524.0,0}; // F-FOL
    T[213]=FuelSB{0.3281,0.450,8813.4,8813.4,2.8996,0.6691,0.0000,0.0000,0.0000,1676.4,1500.0,1500.0,0}; // F-PIN
    T[214]=FuelSB{0.1640,0.280,8813.4,8813.4,1.6728,0.8922,0.4461,0.0000,0.5264,1981.2,1500.0,1371.6,0}; // F-RAC
    T[221]=FuelSB{2.0669,0.300,8598.5,8598.5,2.0252,0.8342,0.2721,0.0000,4.0505,1828.8,1500.0,1524.0,0}; // M-CAD
    T[222]=FuelSB{1.6404,0.250,8813.4,8813.4,2.5204,0.6691,0.2141,0.0000,3.5197,1524.0,1500.0,1676.4,0}; // M-ESC
    T[223]=FuelSB{2.0997,0.320,9028.4,9028.4,3.7338,1.6996,0.0000,0.0000,2.0119,1432.6,1500.0,1524.0,0}; // M-EUC
    T[224]=FuelSB{1.3123,0.260,9028.4,9028.4,0.6111,1.2892,0.7093,0.0000,0.8208,1371.6,1500.0,1524.0,0}; // M-EUCd
    T[225]=FuelSB{0.9843,0.350,9028.4,9028.4,2.0074,0.6691,0.2230,1.0483,0.2141,1828.8,2438.4,1371.6,0}; // M-F
    T[226]=FuelSB{0.3281,0.300,8813.4,8813.4,1.2089,0.4461,0.0000,0.2677,0.0446,1676.4,2438.4,1371.6,0}; // M-H
    T[227]=FuelSB{1.6404,0.400,9028.4,9028.4,3.2163,1.3383,0.0000,0.0000,3.0736,1676.4,1500.0,1828.8,0}; // M-PIN
    T[231]=FuelSB{1.9685,0.240,8168.5,8168.5,0.2900,0.0669,0.0000,1.0483,0.1784,1219.2,1676.4,1219.2,1}; // V-Ha
    T[232]=FuelSB{1.1483,0.240,8383.5,8383.5,0.1338,0.0000,0.0000,0.5353,0.0000,1828.8,1828.8,1500.0,1}; // V-Hb
    T[233]=FuelSB{3.4449,0.350,9028.4,9028.4,4.2379,1.1152,0.0000,0.0000,6.4683,1066.8,1500.0,1219.2,1}; // V-MAa
    T[234]=FuelSB{1.6404,0.350,9028.4,9028.4,2.6765,0.2230,0.0000,0.0000,3.3457,1066.8,1500.0,1371.6,1}; // V-MAb
    T[235]=FuelSB{1.8045,0.250,8383.5,8383.5,0.4461,0.4461,0.0000,0.6691,2.4535,1371.6,2590.8,1219.2,1}; // V-MH
    T[236]=FuelSB{5.5774,0.250,8813.4,8813.4,2.6765,1.7844,0.0000,0.0000,5.7992,762.0,1500.0,914.4,1}; // V-MMa
    T[237]=FuelSB{2.9528,0.200,8813.4,8813.4,1.7844,0.2230,0.0000,0.0000,3.1226,914.4,1500.0,914.4,1}; // V-MMb
    return T;
}
static inline double etaM_sb(double r){ if(r>1)r=1; return 1-2.59*r+5.11*r*r-3.52*r*r*r; }
// ROS [m/min] y flame length [m]; viento midflame en km/h, pendiente fraccion
static void rothermel_s(int fmid,double m1,double m10,double m100,double mlh,double mlw,
                        double ws10m_kmh,double waz,double cover,double canHt_m,
                        double slope,double saz,
                        double* rosOut,double* flOut,double* razOut,double* effWind10mOut){
    *rosOut=0; *flOut=0;
    auto it=sbTable().find(fmid); if(it==sbTable().end()) return;
    const FuelSB& fm=it->second;
    const double RHO_P=32.0,S_T=0.0555,S_E=0.010,TON_AC=0.0459137,KMH2FT=54.6806649;
    double w1=fm.l1*TON_AC,w10=fm.l10*TON_AC,w100=fm.l100*TON_AC,wlh=fm.lh*TON_AC,wlw=fm.lw*TON_AC;
    double s1=fm.s1,s10=109,s100=30,slh=fm.sh,slw=fm.sw;
    if(fm.dyn && wlh>0){
        double kt=(1.20-mlh)/0.90; if(kt<0)kt=0; if(kt>1)kt=1;
        double f1=w1*s1/RHO_P,f4=(wlh*kt)*slh/RHO_P;
        if(f1+f4>0) s1=(f1*s1+f4*slh)/(f1+f4);
        w1+=wlh*kt; wlh-=wlh*kt;
    }
    double w[5],s[5],mf[5]; int lv[5],n=0;
    auto add=[&](double ww,double ss,double mm,int l){ if(ww>0){w[n]=ww;s[n]=ss;mf[n]=mm;lv[n]=l;n++;} };
    add(w1,s1,m1,0); add(w10,s10,m10,0); add(w100,s100,m100,0); add(wlh,slh,mlh,1); add(wlw,slw,mlw,1);
    if(n==0) return;
    double A[5],Ad=0,Al=0,Wt=0;
    for(int i=0;i<n;i++){A[i]=s[i]*w[i]/RHO_P; if(lv[i])Al+=A[i]; else Ad+=A[i]; Wt+=w[i];}
    double At=Ad+Al; if(At<=0) return;
    double Fd=Ad/At,Fl=Al/At,sigD=0,sigL=0,wnD=0,wnL=0,MfD=0,MfL=0;
    for(int i=0;i<n;i++){
        double f=lv[i]?(Al>0?A[i]/Al:0):(Ad>0?A[i]/Ad:0), wn=w[i]*(1-S_T);
        if(lv[i]){sigL+=f*s[i];wnL+=f*wn;MfL+=f*mf[i];} else {sigD+=f*s[i];wnD+=f*wn;MfD+=f*mf[i];}
    }
    double sig=Fd*sigD+Fl*sigL; if(sig<=0) return;
    double rho_b=Wt/fm.depth,beta=rho_b/RHO_P,beta_op=3.348*pow(sig,-0.8189),rpr=beta/beta_op;
    double MxD=fm.mx,MxL=MxD;
    if(Al>0&&Ad>0){
        double num=0,den=0,fn=0,fd2=0;
        for(int i=0;i<n;i++){ if(!lv[i]){double e=exp(-138.0/s[i]);num+=w[i]*e;fn+=mf[i]*w[i]*e;fd2+=w[i]*e;} else den+=w[i]*exp(-500.0/s[i]); }
        double Wp=(den>0?num/den:0),mff=(fd2>0?fn/fd2:0);
        MxL=2.9*Wp*(1-mff/MxD)-0.226; if(MxL<MxD)MxL=MxD;
    }
    double etaMd=(MxD>0?etaM_sb(MfD/MxD):0),etaMl=(MxL>0?etaM_sb(MfL/MxL):0);
    double eta_s=0.174*pow(S_E,-0.19); if(eta_s>1)eta_s=1;
    double Gmax=pow(sig,1.5)/(495.0+0.0594*pow(sig,1.5)),Aexp=133.0*pow(sig,-0.7913);
    double Gamma=Gmax*pow(rpr,Aexp)*exp(Aexp*(1-rpr));
    double IR=Gamma*(wnD*fm.hD*etaMd*eta_s+wnL*fm.hL*etaMl*eta_s);
    double xi=exp((0.792+0.681*sqrt(sig))*(beta+0.1))/(192.0+0.2595*sig);
    double C=7.47*exp(-0.133*pow(sig,0.55)),B=0.02526*pow(sig,0.54),E=0.715*exp(-3.59e-4*sig);
    double hsD=0,hsL=0;
    for(int i=0;i<n;i++){double f=lv[i]?(Al>0?A[i]/Al:0):(Ad>0?A[i]/Ad:0),t=exp(-138.0/s[i])*(250.0+1116.0*mf[i]); if(lv[i])hsL+=f*t; else hsD+=f*t;}
    double hsink=rho_b*(Fd*hsD+Fl*hsL); if(hsink<=0||IR<=0) return;
    // Wind Adjustment Factor (Andrews 2012, como behave/FARSITE): 10m -> 20ft -> midflame
    double canHt_ft = canHt_m * 3.280839895;   // SI: tree height en metros -> ft (constantes WAF en ft)
    double waf, crownFrac = cover * 3.14159265358979 / 12.0;
    if (cover < 1e-7 || crownFrac < 0.05 || canHt_ft < 6.0)
        waf = 1.83 / log((20.0 + 0.36 * fm.depth) / (0.13 * fm.depth));        // unsheltered (fuel bed depth)
    else
        waf = 0.555 / (sqrt(crownFrac * canHt_ft) * log((20.0 + 0.36 * canHt_ft) / (0.13 * canHt_ft)));  // sheltered
    if (waf < 0.0) waf = 0.0; if (waf > 1.0) waf = 1.0;
    double U = (ws10m_kmh / 1.15) * waf * KMH2FT;   // 10-m wind -> 20ft -> midflame -> ft/min
    double phiw=U>0?C*pow(U,B)*pow(rpr,-E):0.0, phis=5.275*pow(beta,-0.3)*slope*slope;
    double R0ft=IR*xi/hsink;                       // ROS sin viento ni pendiente
    // --- Combinación vectorial viento-pendiente (behave/FARSITE SurfaceFireSpread) ---
    const double PI_=3.14159265358979;
    double splitRad=(waz - saz)*PI_/180.0;         // viento relativo al upslope (saz = upslope azimuth)
    double vx=phis + phiw*cos(splitRad);           // componente a lo largo del upslope
    double vy=phiw*sin(splitRad);                  // componente perpendicular
    double phiEff=sqrt(vx*vx+vy*vy);               // coeficiente efectivo combinado
    double Umax=0.9*IR, phiMax=C*pow(Umax,B)*pow(rpr,-E);   // límite de viento efectivo (behave)
    if(phiEff>phiMax) phiEff=phiMax;
    double Rft=R0ft*(1.0+phiEff); if(Rft<0)Rft=0;
    *rosOut=Rft*0.3048;
    double off=atan2(vy,vx)*180.0/PI_;             // offset desde upslope
    double raz=saz+off; while(raz<0)raz+=360.0; while(raz>=360.0)raz-=360.0;
    *razOut=raz;                                    // dirección de máximo avance (viento+pendiente)
    double Ueff_ft=(phiEff>0)?pow(phiEff/(C*pow(rpr,-E)),1.0/B):0.0;   // viento efectivo midflame [ft/min]
    *effWind10mOut=(waf>0)?(Ueff_ft/KMH2FT)*1.15/waf:ws10m_kmh;        // equivalente a 10 m [km/h] para l_to_b
    double tau=384.0/sig,HPA=IR*tau,IB=HPA*Rft/60.0;
    *flOut=0.45*pow(IB,0.46)*0.3048;
}

// Carga total de combustible (tons/acre) y poder calorifico (BTU/lb) desde sbTable.
// Reemplaza el antiguo fm_parameters (que solo tenia 101-204); ahora cubre todos los
// modelos de sbTable, incluidos los de Portugal (211-237).
static inline bool sbLoadHeat(int fmid, double& wa, double& H){
    auto it = sbTable().find(fmid);
    if(it==sbTable().end()){ wa=0.0; H=8000.0; return false; }
    const FuelSB& f = it->second;
    wa = f.l1 + f.l10 + f.l100 + f.lh + f.lw;
    H  = f.hD;
    return (wa>0.0);
}
} // namespace

// Devuelve todos los codigos de combustible disponibles en sbTable (S&B 101-204 +
// Portugal 211-237). Sirve para construir un lookup identidad cuando el usuario no
// entrega spain_lookup_table.csv (el valor del raster ES el codigo del modelo).
std::vector<int> sbAllFuelCodes(){
    std::vector<int> v;
    for(const auto& kv : sbTable()) v.push_back(kv.first);
    std::sort(v.begin(), v.end());
    return v;
}

// TODO: citation needed

// TODO: citation needed
float
flankfire_ros_s(float ros, float bros, float lb)
{
    return ((ros + bros) / (lb * 2.0));
}

/* ----------------- Length-to-Breadth --------------------------*/
// TODO: citation needed
float
l_to_b(float ws)
{
    // if(strncmp(ft,"O1",2)==0)return( ws<1.0 ? 1.0 : (1.1*pow(ws,0.464)));
    float alpha, beta, factor;
    alpha = 0.2566;
    beta = -0.1548;
    factor = 1000.0 / 3600.0;
    return pow((0.936 * exp(alpha * factor * ws) + 0.461 * exp(beta * factor * ws) - 0.397), 0.45);
}

/* ----------------- Back Rate of Spread --------------------------*/
// TODO: citation needed
float
backfire_ros_s(main_outs* at, snd_outs* sec)
{
    float hb, bros, lb;
    // lb = l_to_b(data->fueltype,at->wsv);
    lb = sec->lb;
    hb = (lb + sqrt(pow(lb, 2) - 1.0)) / (lb - sqrt(pow(lb, 2) - 1.0));

    bros = at->rss / hb;

    return bros * (bros >= 0);
}

// TODO: citation needed

/**
 * @brief Calculates the flame length of a cell when there is crown fire.
 * @param intensity Byram intensity for crown fires
 * @return the flame length
 */

// TODO: citation needed
float
crown_flame_length(float intensity)
{
    float fl = 0.1 * pow(intensity, 0.5);
    if (fl < 0.01)
    {
        return 0;
    }
    else
    {
        return std::ceil(fl * 100.0) / 100.0;
    }
}

// TODO: citation needed
float
byram_intensity(main_outs* at, fuel_coefs* ptr)
{
    float fl, ib;
    fl = at->fl;
    ib = 259.833 * pow(fl, 2.174);
    ib = std::ceil(ib * 100.0) / 100.0;
    return ib;
}

/**
 * Calculates byram fire intensity when there is active crown fire.
 * In order for this to be calculated, the input folder must contain
 * files with CBD, CBH and tree height data for each cell.
 * @param at Structure containing the cell's output data.
 * @param data Structure containing the cell's input data.
 * @return Fire intensity.
 */
// TODO: citation needed
float
crown_byram_intensity(main_outs* at, inputs* data)
{
    float canopy_height;
    if (data->tree_height == -9999)
    {
        canopy_height = data->cbh * 2;
    }
    else
    {
        canopy_height = data->tree_height - data->cbh;
    }
    if (canopy_height < 0)
    {
        std::cerr << "Tree height is lower than canopy base height, please provide valid files.\n"
                     "Could not calculate crown Byram intensity due to invalid input data"
                  << std::endl;
        throw std::runtime_error("Could not calculate crown Byram intensity due to invalid input data");
    }
    return std::ceil((HEAT_YIELD / 60) * data->cbd * canopy_height * at->ros_active * 100.0) / 100.0;
}

int
fmc_scen(inputs* data)
{
    int fmc;
    fmc = data->FMC;
    return fmc;
}

bool
fire_type(inputs* data, main_outs* at)
{
    float intensity, critical_intensity, cbh;
    int fmc;
    bool crownFire = false;

    intensity = at->sfi;
    cbh = data->cbh;
    fmc = fmc_scen(data);
    critical_intensity = pow((0.01 * cbh * (460 + 25.9 * fmc)), 1.5);
    // TODO: citation needed
    if ((intensity > critical_intensity) && cbh != 0)
        crownFire = true;

    return crownFire;
}

// TODO: citation needed
float
rate_of_spread10(inputs* data, arguments* args, float ws)
{
    const float p1 = 0.2802;
    const float p2 = 0.07786;
    const float p3 = 0.01123;
    float ros10 = 1. / (p1 * exp(-p2 * ws * 0.4) + p3);

    float ROS10Factor = args->ROS10Factor;
    float CCFFactor = args->CCFFactor;
    float CBDFactor = args->CBDFactor;
    float ccf = data->ccf;
    float cbd = data->cbd;
    float ros = ROS10Factor * ros10 + CCFFactor * ccf + CBDFactor * cbd;
    return (ros);
}

// TODO: citation needed
float
final_rate_of_spread10(inputs* data, main_outs* at)
{
    float ros_active, ros_final, ros;
    ros = at->rss;
    ros_active = at->ros_active;
    ros_final = ros + at->cfb * (ros_active - ros);
    return (ros_final);
}

// TODO: citation needed
float
backfire_ros10_s(fire_struc* hptr, snd_outs* sec)
{
    float hb, bros, lb;
    lb = sec->lb;
    hb = (lb + sqrt(pow(lb, 2) - 1.0)) / (lb - sqrt(pow(lb, 2) - 1.0));

    bros = hptr->ros / hb;

    return bros;
}

// TODO: citation needed
float
crownfractionburn(inputs* data, main_outs* at)
{  // generar output de cfb
    float a, cbd, ros, ros0, H, wa, i0, cbh, FMC, cfb;
    FMC = fmc_scen(data);
    ;  // modificar para ingresar manualmente
    cbh = data->cbh;
    i0 = pow((0.01 * cbh * (460 + 25.9 * FMC)), 1.5);
    { double _wa, _H; sbLoadHeat(data->nftype, _wa, _H); wa = (float)_wa; H = (float)_H; }
    cbd = data->cbd;
    ros0 = 60 * i0 / (H * wa);
    ros = at->rss;
    if (cbd != 0)
    {
        a = -log(0.1) / (0.9 * (3.0 / cbd - ros0));
        // a=0.23;
    }
    else
    {
        a = 0.23;
    }
    cfb = 1 - exp(-a * (ros - ros0));
    if (cfb < 0)
    {
        cfb = 0;
    }
    return cfb;
}

// TODO: citation needed
bool
checkActive(inputs* data, main_outs* at)  // En s&b se usa fm10
{
    float rac, cbd, H, wa, i0, cbh, fmc;
    bool active;
    // rac = at->ros_active;
    fmc = fmc_scen(data);
    ;  // modificar para ingresar manualmente
    cbh = data->cbh;
    i0 = pow((0.01 * cbh * (460 + 25.9 * fmc)), 1.5);
    { double _wa, _H; sbLoadHeat(data->nftype, _wa, _H); wa = (float)_wa; H = (float)_H; }
    cbd = data->cbd;
    rac = 60 * i0 / (H * wa);  // rate active crown

    active = cbd * rac >= 3;
    return active;
}

// ===================== Mode 2: humedad de combustible ESPACIAL (solar/topográfica) =====================
// Cadena por celda: geometría solar -> irradiancia incidente en la ladera -> sombreado (dosel)
// -> ΔT de calentamiento -> T_fuel, RH_fuel -> EMC (Simard 1968). Con I=0 se reduce a 'conditioning'.
// Constantes de calentamiento (ALPHA, HC_A, HC_B) son parámetros de calibración.
namespace m2
{
constexpr double M2PI = 3.14159265358979323846;
constexpr double D2R = M2PI / 180.0, R2D = 180.0 / M2PI;
constexpr double I_SC = 1367.0, TAU_CLEAR = 0.70, SCALE_H = 8434.5;
constexpr double ALPHA = 0.88, HC_A = 45.0, HC_B = 14.0;

inline double solar_declination(double J) { return 23.45 * std::sin(2.0 * M2PI * (284.0 + J) / 365.0); }
inline double eccentricity(double J) { return 1.0 + 0.033 * std::cos(2.0 * M2PI * J / 365.0); }
inline double hour_angle(double h) { return 15.0 * (h - 12.0); }
inline double solar_altitude(double lat, double decl, double H)
{
    double s = std::sin(lat * D2R) * std::sin(decl * D2R)
             + std::cos(lat * D2R) * std::cos(decl * D2R) * std::cos(H * D2R);
    s = std::max(-1.0, std::min(1.0, s));
    return std::asin(s) * R2D;
}
inline double solar_azimuth(double lat, double decl, double H, double beta)
{
    if (beta <= 0) return 0.0;
    double sinA = std::cos(decl * D2R) * std::sin(H * D2R) / std::cos(beta * D2R);
    double cosA = (std::sin(beta * D2R) * std::sin(lat * D2R) - std::sin(decl * D2R))
                / (std::cos(beta * D2R) * std::cos(lat * D2R));
    double A = std::atan2(sinA, cosA) * R2D + 180.0;
    return std::fmod(A + 360.0, 360.0);
}
inline double air_mass(double beta)
{
    if (beta <= 0) return 1.0e9;
    return 1.0 / (std::sin(beta * D2R) + 0.50572 * std::pow(beta + 6.07995, -1.6364));
}
inline double beam_normal(double beta, double elev, double tau, double J)
{
    if (beta <= 0) return 0.0;
    double am = air_mass(beta) * std::exp(-elev / SCALE_H);
    return I_SC * eccentricity(J) * std::pow(tau, std::pow(am, 0.678));
}
inline double cos_incidence(double beta, double A_sun, double slope_pct, double saz)
{
    double S = std::atan(slope_pct / 100.0);
    double A_face = std::fmod(saz + 180.0, 360.0);
    return std::sin(beta * D2R) * std::cos(S)
         + std::cos(beta * D2R) * std::sin(S) * std::cos((A_sun - A_face) * D2R);
}
inline double saturation_vp(double T) { return 0.6108 * std::exp(17.27 * T / (T + 237.3)); }
inline double emc_simard(double Tf_c, double RH)
{
    double Tf = Tf_c * 9.0 / 5.0 + 32.0, e;
    if (RH < 10.0)       e = 0.03229 + 0.281073 * RH - 0.000578 * RH * Tf;
    else if (RH <= 50.0) e = 2.22749 + 0.160107 * RH - 0.014784 * Tf;
    else                 e = 21.0606 + 0.005565 * RH * RH - 0.00035 * RH * Tf - 0.483199 * RH;
    return std::max(0.0, e);
}
// EMC [%] por celda. ws en m/s. ccf en % o fracción. cloud 0..1.
inline double dead_fuel_emc(double lat, double J, double h, double slope_pct, double saz,
                            double elev, double ccf, double cloud,
                            double T_air, double RH_air, double U_ms)
{
    double cc = (ccf > 1.0 ? ccf / 100.0 : ccf);
    double decl = solar_declination(J), H = hour_angle(h);
    double beta = solar_altitude(lat, decl, H);
    double I = 0.0;
    if (beta > 0)
    {
        double A = solar_azimuth(lat, decl, H, beta);
        double ci = std::max(cos_incidence(beta, A, slope_pct, saz), 0.0);
        I = beam_normal(beta, elev, TAU_CLEAR, J) * ci * (1.0 - cloud) * (1.0 - cc);
    }
    double dT = ALPHA * I / (HC_A + HC_B * std::max(U_ms, 0.0));
    double Tf = T_air + dT;
    double RHf = RH_air * saturation_vp(T_air) / saturation_vp(Tf);
    RHf = std::max(0.0, std::min(100.0, RHf));
    return emc_simard(Tf, RHf);
}
}  // namespace m2

void
calculate_s(inputs* data,
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
    float _rothermel_fl = 0;  // continuous-moisture S&B
    float _eff_wind_10m = wdf_ptr->ws;  // viento efectivo (10-m equiv) viento+pendiente
    bool crownFire = false;
    // Populate fuel coefs struct
    // ptr->fueltype = data->fueltype;
    if (args->verbose)
    {
        std::cout << "Populate fuel types " << std::endl;
        std::cout << "NfTypes:" << data->nftype << std::endl;
        std::cout << "scen:" << args->scenario << std::endl;
    }
    // (legacy p_coeff/q_coeff eliminado: la ROS/llama vienen de rothermel_s)
    ptr->nftype = data->nftype;

    // Step 1: Calculate HROS (surface) -- Rothermel con humedad continua (5 clases)
    {
        double _ros = 0, _fl = 0, _raz = 0, _effw = 0;
        double _cover = (data->ccf > 1.0 ? data->ccf / 100.0 : data->ccf);  // fraction
        // Modo 2 (spatial): humedad muerta por celda (solar/topografía + sombreado de dosel).
        // v1: las tres clases (1h/10h/100h) toman el mismo EMC estacionario por celda.
        double _m1 = wdf_ptr->m1h, _m10 = wdf_ptr->m10h, _m100 = wdf_ptr->m100h;
        // --fmc-shading: bandera unificada de acondicionamiento solar/topografico (ambos kernels).
        // --moisture-mode spatial se mantiene como alias retrocompatible.
        if (args->MoistureMode == "spatial" || args->FmcShading)
        {
            double _lat = (args->HasLatitude ? (double)args->Latitude : (double)data->lat);
            double _U = wdf_ptr->ws / 3.6;  // km/h -> m/s (proxy cerca de superficie)
            double _emc = m2::dead_fuel_emc(_lat, wdf_ptr->doy, wdf_ptr->hour,
                                            data->ps, data->saz, data->elev, data->ccf,
                                            0.0 /*cloud*/, wdf_ptr->tmp, wdf_ptr->rh, _U);
            double _e = _emc / 100.0;
            _m1 = _m10 = _m100 = _e;
        }
        rothermel_s(data->nftype, _m1, _m10, _m100,
                    wdf_ptr->mlh, wdf_ptr->mlw, wdf_ptr->ws, wdf_ptr->waz, _cover,
                    data->tree_height, data->ps / 100.0, data->saz,
                    &_ros, &_fl, &_raz, &_effw);
        at->rss = _ros * (_ros >= 0);
        at->raz = _raz;            // dir. de máximo avance (viento+pendiente, FARSITE)
        _rothermel_fl = _fl;
        _eff_wind_10m = _effw;     // viento efectivo (10-m equiv) para l_to_b
    }
    hptr->rss = at->rss;

    // Step 2: Calculate Length-to-breadth (viento efectivo: la pendiente alarga la elipse)
    sec->lb = l_to_b(_eff_wind_10m);

    // Step 3: Calculate BROS (surface)
    bptr->rss = backfire_ros_s(at, sec);

    // Step 4: Calculate central FROS (surface)
    fptr->rss = flankfire_ros_s(hptr->rss, bptr->rss, sec->lb);

    // Step 5: Ellipse components
    at->a = (hptr->rss + bptr->rss) / 2.;
    at->b = (hptr->rss + bptr->rss) / (2. * sec->lb);
    at->c = (hptr->rss - bptr->rss) / 2.;

    // Step 6: Flame Length (Rothermel/Byram, humedad continua)
    at->fl = _rothermel_fl;

    // Step 7: Flame angle
    at->angle = angleFL(wdf_ptr->ws, at);

    // Step 8: Flame Height
    at->fh = flame_height(at);

    // Step 9: Byram Intensity
    at->sfi = byram_intensity(at, ptr);

    // Step 10: Criterion for Crown Fire Initiation (no init if user does not
    // want to include it)
    if (args->AllowCROS && data->cbh != 0 && data->cbd != 0)
    {
        if (activeCrown)
        {
            at->ros_active = rate_of_spread10(data, args, wdf_ptr->ws);
            if (!checkActive(data, at))
            {
                activeCrown = false;
            }
        }
        else
        {
            crownFire = fire_type(data, at);
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
        at->ros_active = rate_of_spread10(data, args, wdf_ptr->ws);
        at->cfb = crownfractionburn(data, at);

        hptr->ros = final_rate_of_spread10(data, at);
        at->rss = hptr->ros;
        bptr->ros = backfire_ros10_s(hptr, sec);
        fptr->ros = flankfire_ros_s(hptr->ros, bptr->ros, sec->lb);
        if (args->verbose)
        {
            cout << "hptr->ros = " << hptr->ros << "\n";
            cout << "bptr->ros = " << bptr->ros << "\n";
            cout << "fptr->ros = " << fptr->ros << "\n";
        }
        at->crown_intensity = crown_byram_intensity(at, data);
        at->crown_flame_length = crown_flame_length(at->crown_intensity);

        at->a = (hptr->ros + bptr->ros) / 2.;
        at->b = (hptr->ros + bptr->ros) / (2. * sec->lb);
        at->c = (hptr->ros - bptr->rss) / 2;
        at->crown = 1;
        activeCrown = true;
    }
    else if (activeCrown)
    {
        at->cfb = crownfractionburn(data,
                                    at);  // lo calculamos igual porque lo necesitamos para el output
        hptr->ros = at->ros_active;
        at->rss = hptr->ros;
        bptr->ros = backfire_ros10_s(hptr, sec);
        fptr->ros = flankfire_ros_s(hptr->ros, bptr->ros, sec->lb);
        at->crown_intensity = crown_byram_intensity(at, data);
        at->crown_flame_length = crown_flame_length(at->crown_intensity);

        if (args->verbose)
        {
            cout << "hptr->ros = " << hptr->ros << "\n";
            cout << "bptr->ros = " << bptr->ros << "\n";
            cout << "fptr->ros = " << fptr->ros << "\n";
        }

        at->a = (hptr->ros + bptr->ros) / 2.;
        at->b = (hptr->ros + bptr->ros) / (2. * sec->lb);
        at->c = (hptr->ros - bptr->rss) / 2;
        at->crown = 1;
        // std::cout  << "ros_activo: "  <<hptr->ros <<  std::endl;
    }

    // Otherwise, use the surface values
    else
    {
        at->crown = 0;
        at->cfb = 0;
        at->crown_flame_length = 0;
        at->crown_intensity = 0;
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
    // if (hptr->ros>100){
    // cout << "hptr->rss = " << hptr->ros << "\n";

    //}

    if (args->verbose)
    {
        cout << "--------------- Inputs --------------- \n";
        cout << "ws = " << wdf_ptr->ws << "\n";
        cout << "coef data->cbh = " << data->cbh << "\n";
        cout << "coef ptr->p1 = " << ptr->p1 << "\n";
        cout << "coef ptr->p2 = " << ptr->p2 << "\n";
        cout << "coef ptr->p3 = " << ptr->p3 << "\n";
        cout << "coef ptr->q1 = " << ptr->q1 << "\n";
        cout << "coef ptr->q2 = " << ptr->q2 << "\n";
        cout << "coef ptr->q3 = " << ptr->q3 << "\n";
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
determine_destiny_metrics_s(inputs* data, fuel_coefs* ptr, arguments* args, main_outs* metrics, weatherDF* wdf_ptr)
{
    // Aux
    float ros = 0, bros = 0, lb = 0, fros = 0;
    bool crownFire = false;
    ptr->nftype = data->nftype;
    // Step 6: Flame Length -- via Rothermel/BehavePlus (consistente con calculate_s), no q_coeff
    {
        double _r = 0, _f = 0, _z = 0, _e = 0;
        double _cover = (data->ccf > 1.0 ? data->ccf / 100.0 : data->ccf);
        rothermel_s(data->nftype, wdf_ptr->m1h, wdf_ptr->m10h, wdf_ptr->m100h,
                    wdf_ptr->mlh, wdf_ptr->mlw, wdf_ptr->ws, wdf_ptr->waz, _cover,
                    data->tree_height, data->ps / 100.0, data->saz,
                    &_r, &_f, &_z, &_e);
        metrics->fl = _f;
    }
    // Step 9: Byram Intensity
    metrics->sfi = byram_intensity(metrics, ptr);
    // Set cfb value for no crown fire scenario
    metrics->cfb = 0;
    // Step 10: Criterion for Crown Fire Initiation (no init if user does not
    // want to include it)
    if (args->AllowCROS)
    {
        crownFire = fire_type(data, metrics);
        if (crownFire)
        {
            metrics->cfb = crownfractionburn(data, metrics);
            metrics->crown_intensity = crown_byram_intensity(metrics, data);
            metrics->crown_flame_length = crown_flame_length(metrics->crown_intensity);
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

    metrics->crown = crownFire;
}
