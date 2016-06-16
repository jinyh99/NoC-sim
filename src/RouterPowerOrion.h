#ifndef _ROUTER_POWER_ORION_H_
#define _ROUTER_POWER_ORION_H_

#ifdef ORION_MODEL

/**
 * NOTE: Orion can be configured in the different technology, Vdd, and frequency.
 *       You need to compile Orion again for different parameters.
 *       In Orion Makefile:
 *             -DPARM_TECH_POINT=10 -DVdd=1.8 -DPARM_Freq=1e9
 */

// extern "C" {
  #include "SIM_power.h"
  #include "SIM_router_power.h"
  #include "SIM_power_router.h"
// }

using namespace orion;

int SIM_buf_set_para(SIM_power_array_info_t *info, int is_fifo, u_int n_read_port, u_int n_write_port, u_int n_entry, u_int line_width, int outdrv);


#endif

#include "RouterPower.h"

class RouterPowerOrion : public RouterPower {
public:

    RouterPowerOrion(Router* p_router);
    ~RouterPowerOrion();

    // dynamic energy
    double dynamicE_buffer();
    double dynamicE_vc_arb();
    double dynamicE_sw_arb();
    double dynamicE_xbar();
    double dynamicE_link(int out_pc);
    double dynamicE_all_link();

    // static energy
    double staticE_buffer() { return m_router_power.in_buf.I_static/g_cfg.chip_freq; };
    double staticE_xbar()   { return m_router_power.crossbar.I_static/g_cfg.chip_freq; };
    double staticE_vc_arb() { return m_arbiter_vc_power_.I_static/g_cfg.chip_freq; };
    double staticE_xbar_v1_arb() { return m_arbiter_xbar_v1_power_.I_static/g_cfg.chip_freq; };
    double staticE_xbar_p1_arb() { return m_arbiter_xbar_p1_power_.I_static/g_cfg.chip_freq; };

    void record_link_trav(Flit* p_flit, int out_pc);
    void record_buffer_read(Flit* p_flit, int pc);
    void record_buffer_write(Flit* p_flit, int pc);
    void record_xbar_trav(Flit* p_flit, int in_pc, int out_pc);
    void record_vc_arb(int out_pc, unsigned int req, unsigned int grant);
    void record_v1_sw_arb(int in_pc, unsigned int req, unsigned int grant);
    void record_p1_sw_arb(int out_pc, unsigned int req, unsigned int grant);

private:
    void init();

private:
    SIM_power_router_info_t m_router_info;
    SIM_power_router_t m_router_power;
    SIM_power_arbiter_t m_arbiter_vc_power_;
    SIM_power_arbiter_t m_arbiter_xbar_v1_power_;
    SIM_power_arbiter_t m_arbiter_xbar_p1_power_;

    vector< vector< unsigned long long > > m_in_buf_read_vec; // X[in_pc][flit_pos]
    vector< vector< unsigned long long > > m_in_buf_write_vec; // X[in_pc][flit_pos]
    vector< vector< unsigned long long > > m_xbar_read_vec; // X[in_pc][flit_pos]
    vector< vector< unsigned long long > > m_xbar_write_vec; // X[in_pc]flit_pos]
    vector< unsigned int > m_xbar_input_vec; // X[in_pc]
    vector< unsigned int > m_vc_arb_req_vec; // X[out_pc]
    vector< unsigned int > m_vc_arb_grant_vec; // X[out_pc]
    vector< unsigned int > m_xbar_arb_v1_req_vec; // X[in_pc] // for input PC arb
    vector< unsigned int > m_xbar_arb_p1_req_vec; // X[out_pc] // for output PC arb
};

#endif // #ifndef _ROUTER_POWER_ORION_H_
