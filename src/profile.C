#include "noc.h"
#include "main.h"
#include "Router.h"
#include "RouterPower.h"
#include "RouterPowerOrion.h"
#include "RouterPowerStats.h"
#include "Workload.h"

static double g_last_profile_clk = .0;
////////////////////////////////////////////////////////////////////////
// Performance

void profile_set_start_clk()
{
    g_last_profile_clk = simtime();
}

void profile_perf_reset()
{
    g_sim.resetStatsPeriodic();
}

void profile_perf_header_print()
{
    fprintf(g_cfg.profile_perf_fp, "clk ");
    fprintf(g_cfg.profile_perf_fp, "T_t T_c T_n ");
    fprintf(g_cfg.profile_perf_fp, "pkt_ld flit_ld ");
    fprintf(g_cfg.profile_perf_fp, "pkt_thrp flit_thrp ");
    fprintf(g_cfg.profile_perf_fp, "ejt_pkt# ejt_flit# ");
    fprintf(g_cfg.profile_perf_fp, "instr/cycle instr#");
    fprintf(g_cfg.profile_perf_fp, "\n");
}

void profile_perf_print()
{
    double num_cycles = simtime() - g_last_profile_clk;

    double T_t = g_sim.m_periodic_pkt_T_t_tab->mean();	// latency
    double T_c = g_sim.m_periodic_pkt_T_t_tab->mean() - g_sim.m_periodic_pkt_T_hws_tab->mean() + 1.0; // contention latency
    double T_n = g_sim.m_periodic_pkt_T_t_tab->mean() - g_sim.m_periodic_pkt_T_q_tab->mean(); // network latency
    double avg_offered_pkt_load = ((double) g_sim.m_periodic_inj_pkt)/num_cycles/((double) g_cfg.router_num);
    double avg_offered_flit_load = ((double) g_sim.m_periodic_inj_flit)/num_cycles/((double) g_cfg.router_num);
    double avg_accepted_pkt_load = ((double) g_sim.m_periodic_ejt_pkt)/num_cycles/((double) g_cfg.router_num);
    double avg_accepted_flit_load = ((double) g_sim.m_periodic_ejt_flit)/num_cycles/((double) g_cfg.router_num);
    double avg_instr_load = ((double) (g_sim.m_num_instr_executed - g_sim.m_periodic_instr_executed))/num_cycles;

    fprintf(g_cfg.profile_perf_fp,
        "%.0lf %.2lf %.2lf %.2lf %.4lf %.4lf %.4lf %.4lf %lld %lld %.2lf %lld\n",
        simtime(),
        T_t,
        T_c,
        T_n,
        avg_offered_pkt_load,
        avg_offered_flit_load,
        avg_accepted_pkt_load,
        avg_accepted_flit_load,
        g_sim.m_periodic_ejt_pkt,
        g_sim.m_periodic_ejt_flit,
        avg_instr_load,
        g_sim.m_num_instr_executed
    );
    fflush(g_cfg.profile_perf_fp);
}

////////////////////////////////////////////////////////////////////////
// Power

void profile_power_header_print()
{
    // clock
    fprintf(g_cfg.profile_power_fp, "CLK\t");

/*
    // router
    for (int i=0; i<g_cfg.router_num; i++) {
        fprintf(g_cfg.profile_power_fp, "R-%d\t", i);
    }

    // links
    for (int i=0; i<g_cfg.router_num; i++) {
        for (int pc=0; pc<g_Router_vec[i]->num_pc(); pc++) {
            if (g_Router_vec[i]->isEjectChannel(pc))
                continue;	// Do not report if this link is used for core connection.

            Link& link = g_Router_vec[i]->getLink(pc);

            if (link.valid) {
                fprintf(g_cfg.profile_power_fp, "L%s-%d\t", link.m_link_name.c_str(), i);
            }
        }
    }
*/

/*
    // core
    for (i=0; i<g_cfg.core_num; i++) {
        fprintf(g_cfg.profile_power_fp, "CA-%d\t", i);
        fprintf(g_cfg.profile_power_fp, "CB-%d\t", i);
        fprintf(g_cfg.profile_power_fp, "CC-%d\t", i);
    }
*/

    // power for routers
    fprintf(g_cfg.profile_power_fp, "#D-BUF\tD-XBAR\tD-RTER\tS-RTER\t");
    // power for links
    fprintf(g_cfg.profile_power_fp, "D-LINK\tS-LINK\t");
    // power for network
    fprintf(g_cfg.profile_power_fp, "NET\t");
    fprintf(g_cfg.profile_power_fp, "instr#");

    fprintf(g_cfg.profile_power_fp, "\n");
}

void profile_power_print()
{
    double total_dyn_energy_buffer = 0.0;
    double total_dyn_energy_vc_arb = 0.0;
    double total_dyn_energy_sw_arb = 0.0;
    double total_dyn_energy_xbar = 0.0;
    double total_dyn_energy_router = 0.0;
    double total_dyn_energy_link = 0.0;
    double total_dyn_energy_network = 0.0;
    double total_stat_energy_buffer = 0.0;
    double total_stat_energy_vc_arb = 0.0;
    double total_stat_energy_sw_arb = 0.0; 
    double total_stat_energy_xbar = 0.0;
    double total_stat_energy_router = 0.0;
    double total_stat_energy_link = 0.0;
    double total_stat_energy_network = 0.0;
    vector< double > router_dyn_energy_vec;
    vector< double > router_stat_energy_vec;
    router_dyn_energy_vec.resize(g_Router_vec.size());
    router_stat_energy_vec.resize(g_Router_vec.size());

    double num_cycles = simtime() - g_last_profile_clk;

    calc_network_energy(total_dyn_energy_buffer, total_dyn_energy_vc_arb,
                        total_dyn_energy_sw_arb, total_dyn_energy_xbar,
                        total_dyn_energy_router, total_dyn_energy_link,
                        total_dyn_energy_network,

                        total_stat_energy_buffer, total_stat_energy_vc_arb,
                        total_stat_energy_sw_arb, total_stat_energy_xbar,
                        total_stat_energy_router, total_stat_energy_link,
                        total_stat_energy_network,

                        router_dyn_energy_vec, router_stat_energy_vec,

                        num_cycles,
                        true);

    // total network energy (dynamic + static)
    double total_energy_network = total_dyn_energy_network + total_stat_energy_network;
    // interval time (second)
    double interval_time = num_cycles/g_cfg.chip_freq;

    fprintf(g_cfg.profile_power_fp, "%.0lf\t%.5lf\t%.5lf\t%.5lf\t%.5lf\t%.5lf\t%.5lf\t%.5lf\t%lld\n",
            simtime(), // current clock
            total_dyn_energy_buffer/interval_time, // dynamic power for buffer
            total_dyn_energy_xbar/interval_time, // dynamic power for crossbar
            total_dyn_energy_router/interval_time, // dynamic power for router
            total_stat_energy_router/interval_time, // static power for router
            total_dyn_energy_link/interval_time, // dynamic power for link
            total_stat_energy_link/interval_time, // static power for link
            total_energy_network/interval_time, // total network power
            g_sim.m_num_instr_executed);

    fflush(g_cfg.profile_power_fp);
}

void profile_power_reset()
{
    for (int i=0; i<g_cfg.router_num; i++) {
        delete g_Router_vec[i]->m_power_tmpl_profile;

        // select power model for periodic report
        switch (g_cfg.router_power_model) {
        case ROUTER_POWER_MODEL_STATS:
            g_Router_vec[i]->m_power_tmpl_profile = new RouterPowerStats(g_Router_vec[i]);
            break;
        case ROUTER_POWER_MODEL_ORION_CALL:
            g_Router_vec[i]->m_power_tmpl_profile = new RouterPowerOrion(g_Router_vec[i]);
            break;
        default:
            assert(0);
        }
    }
}
