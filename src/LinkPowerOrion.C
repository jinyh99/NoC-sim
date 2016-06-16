#include "noc.h"
#include "LinkPowerOrion.h"
#include "Router.h"

LinkPowerOrion::LinkPowerOrion(Router* router, int out_pc, int num_wires)
  : LinkPower(router, out_pc, num_wires)
{
    m_link_trav_data_vec.resize(g_cfg.flit_sz_64bit_multiple, 0LL);

#ifdef ORION_MODEL
    // Orion supports several encoding methods:
    // IDENT_ENC, TRANS_ENC, BUSINV_ENC, BUS_MAX_ENC
    SIM_bus_init(&m_orion_bus,
                 GENERIC_BUS,    // model
                 IDENT_ENC,      // encoding
                 64,             // width
                 0,              // grp_width
                 1,              // n_snd -> # of senders
                 1,              // n_rcv -> # of receivers
                 m_router->getLink(out_pc).m_length_mm * 1000,       // length(um)
                 0);             // time -> rise and fall time, 0 means using default transistor sizes
#endif

#ifdef LINK_DVS
    m_dyn_power_link_sum = 0.0;
    m_dyn_energy_link_sum = 0.0;
    m_dyn_power_link_count = 0;
#endif
}

LinkPowerOrion::~LinkPowerOrion()
{
}

double LinkPowerOrion::dynamicE()
{
#ifdef LINK_DVS
/*
    if (dyn_power_link_count[out_pc] == 0)
        return 0.0;

    return dyn_power_link_sum[out_pc] / ((double) dyn_power_link_count[out_pc]);
*/
    return m_dyn_energy_link_sum;
#endif

#ifdef ORION_MODEL
    return SIM_bus_report(&m_orion_bus);
#else
    return LINK_DYN_ENERGY * m_op_trav;
#endif
}

void LinkPowerOrion::traverse(Flit* p_flit)
{
#ifdef LINK_DVS
    Link& out_link = m_router->getLink(m_out_pc);

    // E = C * V^2
    // P = C * V^2 * f

    double link_energy = LINK256_LOAD_CAPACITANCE * (out_link.dvs_voltage * out_link.dvs_voltage);
    m_dyn_energy_link_sum += link_energy;
    m_dyn_power_link_sum += link_energy * out_link.dvs_freq;
    m_dyn_power_link_count++;
// printf("router=%d out_pc=%d link_power=%lg link_energy=%lg total_power=%llg\n", p_router->id, out_pc, link_energy*out_link.dvs_freq, link_energy, dyn_power_link_sum[out_pc]);
#else // #ifdef LINK_DVS

#ifdef ORION_MODEL
    unsigned long long old_d, new_d;

    assert(m_out_pc < m_router->num_pc());

    for (int i=0; i<g_cfg.flit_sz_64bit_multiple; i++) {
        old_d = m_link_trav_data_vec[i];
        new_d = p_flit->m_flitData[i];
        SIM_bus_record(&m_orion_bus, old_d, new_d);
        m_link_trav_data_vec[i] = p_flit->m_flitData[i];
    }
#endif // #ifdef ORION_MODEL
#endif // #ifdef LINK_DVS

    m_op_trav++;
}

double LinkPowerOrion::staticE()
{
    double wire_leakage_energy = (LINK_STAT_POWER / g_cfg.chip_freq)
                               *m_router->getLink(m_out_pc).m_length_mm;

    return wire_leakage_energy * m_num_wires;
}

void LinkPowerOrion::printStats(ostream& out) const
{
}

void LinkPowerOrion::print(ostream& out) const
{
}
