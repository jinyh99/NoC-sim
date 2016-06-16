#include "noc.h"
#include "LinkPowerRepeatedValue.h"
#include "Router.h"

LinkPowerRepeatedValue::LinkPowerRepeatedValue(Router* router, int out_pc, int num_wires)
  : LinkPowerRepeated(router, out_pc, num_wires)
{
    m_link_trav_data_vec.resize(g_cfg.flit_sz_64bit_multiple, 0LL);
    m_intra_trans = 0;
    m_inter_trans = 0;
}

LinkPowerRepeatedValue::~LinkPowerRepeatedValue()
{
}

double LinkPowerRepeatedValue::report_dynamic_energy()
{
    // sum of power :)
    double total_dynamic_power =
           m_intra_dynamic_power * ((double) m_intra_trans) +
           m_inter_dynamic_power * ((double) m_inter_trans);

    double total_dynamic_energy = total_dynamic_power / g_cfg.chip_freq;
// printf("cycle=%.0lf router=%d out_pc=%d dynamic_energy(J)=%lf dynamic_power(W)=%lg m_op_trav=%d #wires=%d\n", simtime(), m_router->id(), m_out_pc, total_dynamic_energy, total_dynamic_power, m_op_trav, m_num_wires);

    return total_dynamic_energy;
}

double LinkPowerRepeatedValue::report_static_energy()
{
    return (m_leakage_power / g_cfg.chip_freq) * m_num_wires;
}

void LinkPowerRepeatedValue::traverse(Flit* p_flit)
{
    unsigned long long old_d, new_d;
    int intra_trans = 0;
    int inter_trans = 0;

    for (int i=0; i<g_cfg.flit_sz_64bit_multiple; i++) {
        old_d = m_link_trav_data_vec[i];
        new_d = p_flit->m_flitData[i];

        // intra transitions
        intra_trans += compute_bitcount_64bit(old_d ^ new_d);

        // inter-wire transitions
        inter_trans += compute_interbit_trans_64bit(old_d, new_d);

// printf("router=%d out_pc=%d i=%d clk=%.0lf old_d=%016llX, new_d=%016llX, intra=%d inter=%d\n", m_router->id(), m_out_pc, i, simtime(), old_d, new_d, compute_bitcount_64bit(old_d ^ new_d), compute_interbit_trans_64bit(old_d, new_d));

        m_link_trav_data_vec[i] = new_d;
    }

    m_intra_trans += (unsigned long long) intra_trans;
    m_inter_trans += (unsigned long long) inter_trans;
    m_op_trav++;
}

void LinkPowerRepeatedValue::printStats(ostream& out) const
{
}

void LinkPowerRepeatedValue::print(ostream& out) const
{
}

