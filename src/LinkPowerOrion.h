#ifndef _LINK_POWER_ORION_H_
#define _LINK_POWER_ORION_H_

#include "LinkPower.h"

// Leakage Power Modeling and Optimization in Interconnection Networks
// X. Chen and L-S. Peh (ISLPED '03)
// link configuration: 3mm, 128-bit wide
// 0.0004W at 0.18um
// 0.0008W at 0.10um
// 0.0057W at 0.07um
//
#define LINK_STAT_POWER_180             0.0004
#define LINK_STAT_POWER_100             0.0008
#define LINK_STAT_POWER_70              0.0057
// The following constant is for 1mm 1-bit link
#define LINK_STAT_POWER                 (LINK_STAT_POWER_70/3.0/128.0)

#define LINK_DYNA_POWER_70              0.1389
#define LINK_DYNA_POWER                 (LINK_DYN_POWER_70/128.0)
#define LINK256_LOAD_CAPACITANCE        (44.448 * UNIT_PICO)

#define LINK_DYN_ENERGY                 1.50348e-10     // 3mm, 128bit wide

#ifdef LINK_DVS
// Multi-level frequency/voltage setting
// Dynamic Voltage Scaling with Links for Power Optimization for
// Interconnection Networks (HPCA 02)
/*
double g_link_dvs_freq[MAX_LINK_DVS_LEVELS] =
        {1.00*UNIT_GIGA, 0.87*UNIT_GIGA, 0.75*UNIT_GIGA, 0.64*UNIT_GIGA,
         0.54*UNIT_GIGA, 0.45*UNIT_GIGA, 0.37*UNIT_GIGA, 0.30*UNIT_GIGA,
         0.24*UNIT_GIGA, 0.19*UNIT_GIGA};
double g_link_dvs_voltage[MAX_LINK_DVS_LEVELS] =
        {2.5, 2.2, 1.95, 1.75, 1.58, 1.43, 1.30, 1.19, 1.1, 1.02};
*/

// Software-Directed Power-Aware Interconnection Networks
// Vassos Soteriou, Noel Eisley, and Li-Shiuan Peh
// CASES 2005
double g_link_dvs_freq[MAX_LINK_DVS_LEVELS] =
        {1.00*UNIT_GIGA, 0.95*UNIT_GIGA, 0.90*UNIT_GIGA, 0.85*UNIT_GIGA,
         0.80*UNIT_GIGA, 0.76*UNIT_GIGA, 0.72*UNIT_GIGA, 0.68*UNIT_GIGA,
         0.64*UNIT_GIGA, 0.60*UNIT_GIGA};
double g_link_dvs_voltage[MAX_LINK_DVS_LEVELS] =
        {2.5, 2.38, 2.27, 2.15, 2.02, 1.93, 1.84, 1.75, 1.66, 1.57};

double g_DVS_transition_energy = 0.0;
unsigned long long g_DVS_transition_count = 0;
#endif

#ifdef ORION_MODEL
  #include "SIM_power.h"
  #include "SIM_router_power.h"
  #include "SIM_power_router.h"
  using namespace orion;
#endif

class LinkPowerOrion : public LinkPower {
public:

    LinkPowerOrion(Router* router, int out_pc, int num_wires);
    ~LinkPowerOrion();

    double dynamicE();
    double staticE();
    void traverse(Flit* p_flit);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
#ifdef ORION_MODEL
    SIM_power_bus_t m_orion_bus;
#endif

#ifdef LINK_DVS
    long double m_dyn_power_link_sum;
    long double m_dyn_energy_link_sum;
    unsigned long long m_dyn_power_link_count;
#endif

};

// Output operator declaration
ostream& operator<<(ostream& out, const LinkPowerOrion& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const LinkPowerOrion& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif
