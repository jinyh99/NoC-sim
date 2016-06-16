#include "noc.h"
#include "RouterPowerStats.h"

RouterPowerStats::RouterPowerStats(Router* p_router) : RouterPower(p_router)
{
    init();
}

RouterPowerStats::~RouterPowerStats()
{
    m_router = 0;

    for (unsigned int out_pc=0; out_pc<m_link_power_vec.size(); out_pc++) {
        delete m_link_power_vec[out_pc];
        m_link_power_vec[out_pc] = 0;
    }

    if (m_xbar_trav_tab)
        delete m_xbar_trav_tab;
}

void RouterPowerStats::init()
{
    int num_pc = m_router->num_pc();
    int num_vc = m_router->num_vc();
    int datapath_width_byte = g_cfg.flit_sz_byte;

    m_op_buf_rd_vec.resize(num_pc, 0);
    m_op_buf_wr_vec.resize(num_pc, 0);
    m_op_vc_arb = 0;
    m_op_sw_input_arb = 0;
    m_op_sw_output_arb = 0;

    switch (g_cfg.chip_tech_nano) {
    case 45:
    case 50:
        init_50nm(num_pc, num_vc, datapath_width_byte, g_cfg.router_inbuf_depth);
        break;
    default:
        assert(0);
    }

    connectLinkPowerModel();
}

void RouterPowerStats::init_50nm(int num_pc, int num_vc, int datapath_width_byte, int vc_buf_depth)
{
    assert(g_cfg.chip_freq == 4.0*UNIT_GIGA);
    assert(g_cfg.chip_voltage == 1.0);

    // set energy values for input buffer (single operation)
    switch (datapath_width_byte) {
    case 8: // 8B flit
        switch (vc_buf_depth) {
        case 4:
            m_dyn_energy_buf_rd = m_dyn_energy_buf_wr = 1.91292e-12; m_stat_energy_buf = 1.98491e-13*num_vc*num_pc; break;
        case 8:
            m_dyn_energy_buf_rd = m_dyn_energy_buf_wr = 2.29442e-12; m_stat_energy_buf = 3.4732e-13*num_vc*num_pc; break;
        case 16:
            m_dyn_energy_buf_rd = m_dyn_energy_buf_wr = 3.11342e-12; m_stat_energy_buf = 6.60985e-13*num_vc*num_pc; break;
        default:
            printf("input buffer power value N/A. datapath_width_byte=%d vc_buf_depth=%d\n", datapath_width_byte, vc_buf_depth); assert(0);
        }
        break;

    case 16: // 16B flit
        switch (vc_buf_depth) {
        case 4:
            m_dyn_energy_buf_rd = m_dyn_energy_buf_wr = 3.81129e-12; m_stat_energy_buf = 3.78853e-13*num_vc*num_pc; break;
        case 8:
            m_dyn_energy_buf_rd = m_dyn_energy_buf_wr = 4.5743e-12; m_stat_energy_buf = 6.58382e-13*num_vc*num_pc; break;
        default:
            printf("input buffer power value N/A. datapath_width_byte=%d vc_buf_depth=%d\n", datapath_width_byte, vc_buf_depth); assert(0);
        }
        break;

    case 32: // 32B flit
        switch (vc_buf_depth) {
        case 4:
            m_dyn_energy_buf_rd = m_dyn_energy_buf_wr = 9.13044e-12; m_stat_energy_buf = 1.28051e-12*num_vc*num_pc; break;
        default:
            printf("input buffer power value N/A. datapath_width_byte=%d vc_buf_depth=%d\n", datapath_width_byte, vc_buf_depth); assert(0);
        }
        break;

    default:
        printf("input buffer power value N/A. datapath_width_byte=%d\n", datapath_width_byte);
        assert(0);
    }

    // set energy value for crossbar (for one traversal)
    switch (datapath_width_byte) {
    case 8:	// 8B flit
        switch (num_pc) {
        case 5: m_dyn_energy_xbar = 2.45415E-11/num_pc; m_stat_energy_xbar = 5.59010E-12; break;
        case 6: m_dyn_energy_xbar = 3.49413E-11/num_pc; m_stat_energy_xbar = 7.85550E-12; break;
        // FIXME: num_pc=7 value is incorrect. This is just for test.
        case 7: m_dyn_energy_xbar = 5.39413E-11/num_pc; m_stat_energy_xbar = 1.15550E-11; break;
        case 8: m_dyn_energy_xbar = 6.12322e-11/num_pc; m_stat_energy_xbar = 1.35337e-11; break;
        case 9: m_dyn_energy_xbar = 7.71234E-11/num_pc; m_stat_energy_xbar = 1.69465E-11; break;
        case 10: m_dyn_energy_xbar = 9.48450E-11/num_pc; m_stat_energy_xbar = 2.07418E-11; break;
        default: printf("crossbar power value N/A. num_pc=%d\n", num_pc); assert(0);
        }
        break;

    case 16:	// 16B flit
        switch (num_pc) {
        case 5: m_dyn_energy_xbar = 8.63182E-11/num_pc; m_stat_energy_xbar = 1.11736E-11; break;
        case 6: m_dyn_energy_xbar = 1.23502e-10/num_pc; m_stat_energy_xbar = 1.57015e-11; break;
        case 8: m_dyn_energy_xbar = 2.17791e-10/num_pc; m_stat_energy_xbar = 2.70506e-11; break;
        case 9: m_dyn_energy_xbar = 2.74895e-10/num_pc; m_stat_energy_xbar = 3.38717e-11; break;
        case 10: m_dyn_energy_xbar = 3.3864e-10/num_pc; m_stat_energy_xbar = 4.14573e-11; break;
        default: printf("crossbar power value N/A. num_pc=%d\n", num_pc); assert(0);
        }
        break;

    case 32:	// 32B flit
        switch (num_pc) {
        case 5: m_dyn_energy_xbar = 3.21591E-10/num_pc; m_stat_energy_xbar = 2.23407E-11; break;
        default: printf("crossbar power value N/A. num_pc=%d\n", num_pc); assert(0);
        }
        break;

    default:
        printf("crossbar power value N/A. datapath_width_byte=%d\n", datapath_width_byte);
        assert(0);
    }

    // set energy value for input arbiter (for switch)
    switch (num_vc) {
    case 1: m_dyn_energy_sw_input_arb = 0.0; break;
    case 2: m_dyn_energy_sw_input_arb = 2.60932E-14/num_pc; break;
    case 3: m_dyn_energy_sw_input_arb = 5.7669e-14/num_pc; break;
    case 4: m_dyn_energy_sw_input_arb = 9.69707E-14/num_pc; break;
    case 8: m_dyn_energy_sw_input_arb = 3.31437E-13/num_pc; break;
    default: printf("sw input arbiter power value N/A. num_vc=%d\n", num_vc); assert(0);
    }

    // set energy value for output arbiter (for switch)
    switch (num_pc) {
    case 5: m_dyn_energy_sw_output_arb = 1.16192E-13/num_pc; break;
    case 6: m_dyn_energy_sw_output_arb = 1.67064E-13/num_pc; break;
    // FIXME: num_pc=7 value is incorrect. This is just for test.
    case 7: m_dyn_energy_sw_output_arb = 2.42741e-13/num_pc; break;
    case 8: m_dyn_energy_sw_output_arb = 3.22741e-13/num_pc; break;
    case 9: m_dyn_energy_sw_output_arb = 3.66036E-13/num_pc; break;
    case 10: m_dyn_energy_sw_output_arb = 4.47812E-13/num_pc; break;
    default: printf("sw output arbiter power value N/A. num_pc=%d\n", num_pc); assert(0);
    }
}

void RouterPowerStats::record_buffer_read(Flit* p_flit, int pc)
{
    m_op_buf_rd_vec[pc]++;
}

void RouterPowerStats::record_buffer_write(Flit* p_flit, int pc)
{
    m_op_buf_wr_vec[pc]++;
}

void RouterPowerStats::record_vc_arb(int out_pc, unsigned int req, unsigned int grant)
{
    m_op_vc_arb++;
}

void RouterPowerStats::record_v1_sw_arb(int in_pc, unsigned int req, unsigned int grant)
{
    m_op_sw_input_arb++;
}

void RouterPowerStats::record_p1_sw_arb(int out_pc, unsigned int req, unsigned int grant)
{
    m_op_sw_output_arb++;
}

void RouterPowerStats::record_link_trav(Flit* p_flit, int out_pc)
{
    m_link_power_vec[out_pc]->traverse(p_flit);
}

double RouterPowerStats::dynamicE_buffer()
{
    return ((m_dyn_energy_buf_rd * total_op_buf_rd()) +
            (m_dyn_energy_buf_wr * total_op_buf_wr())) * g_cfg.power_router_avf;
}

double RouterPowerStats::dynamicE_vc_arb()
{
    return m_dyn_energy_vc_arb * m_op_vc_arb * g_cfg.power_router_avf;
}

double RouterPowerStats::dynamicE_sw_arb()
{
    return (m_dyn_energy_sw_input_arb * m_op_sw_input_arb + 
            m_dyn_energy_sw_output_arb * m_op_sw_output_arb) * g_cfg.power_router_avf;
}

double RouterPowerStats::dynamicE_xbar()
{
    return m_dyn_energy_xbar * m_xbar_trav_tab->sum() * g_cfg.power_router_avf;
}
