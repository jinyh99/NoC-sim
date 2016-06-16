#ifndef _ROUTER_POWER_STATS_H_
#define _ROUTER_POWER_STATS_H_

#include "RouterPower.h"

class RouterPowerStats : public RouterPower {
public:

    RouterPowerStats(Router* p_router);
    ~RouterPowerStats();

    // dynamic energy
    double dynamicE_buffer();
    double dynamicE_vc_arb();
    double dynamicE_sw_arb();
    double dynamicE_xbar();
    double dynamicE_link(int out_pc);
    double dynamicE_all_link();

    // static energy
    double staticE_buffer() { return m_stat_energy_buf; };
    double staticE_xbar()   { return m_stat_energy_xbar; };
    double staticE_vc_arb() { return 0.0; };
    double staticE_xbar_v1_arb() { return 0.0; };
    double staticE_xbar_p1_arb() { return 0.0; };

    void record_link_trav(Flit* p_flit, int out_pc);
    void record_buffer_read(Flit* p_flit, int pc);
    void record_buffer_write(Flit* p_flit, int pc);
    void record_xbar_trav(Flit* p_flit, int in_pc, int out_pc) {};
    void record_vc_arb(int out_pc, unsigned int req, unsigned int grant);
    void record_v1_sw_arb(int in_pc, unsigned int req, unsigned int grant);
    void record_p1_sw_arb(int out_pc, unsigned int req, unsigned int grant);

private:
    void init();
    void init_50nm(int num_pc, int num_vc, int datapath_width_byte, int vc_buf_depth);

private:
    double m_dyn_energy_buf_rd;
    double m_dyn_energy_buf_wr;
    double m_dyn_energy_xbar;	// for load=1 (fully utilized)
    double m_dyn_energy_vc_arb;
    double m_dyn_energy_sw_input_arb;
    double m_dyn_energy_sw_output_arb;
    double m_stat_energy_buf;	// all buffers
    double m_stat_energy_xbar;
};

#endif // #ifndef _ROUTER_POWER_STATS_H_
