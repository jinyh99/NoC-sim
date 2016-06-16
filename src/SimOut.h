#ifndef _SIM_OUT_H_
#define _SIM_OUT_H_

// Simulation results

#include "Packet.h"
#include "Config.h"
extern Config g_cfg;

class SimOut {
public:
    SimOut();
    ~SimOut();

    void init();
    void recordPkt(Packet* p_pkt, double extra_lat_NIout, bool profile_perf);
    void resetStats();
    void resetStatsPeriodic();

public:
    // #injected/ejected packets in simulation
    unsigned long long m_num_pkt_inj;
    unsigned long long m_num_pkt_ejt;
    unsigned long long m_num_flit_inj;
    unsigned long long m_num_flit_ejt;

    unsigned long long m_num_pkt_spurious;	// #generated packets for other purposes
    unsigned long long m_num_pkt_inj_multicast;	// multicast packets
    unsigned long long m_sum_multicast_dest;	// sum of multicast destinations

    // #injected/ejected packets/flits used for warmup
    unsigned long long m_num_pkt_inj_warmup;
    unsigned long long m_num_pkt_ejt_warmup;
    unsigned long long m_num_flit_inj_warmup;
    unsigned long long m_num_flit_ejt_warmup;

    unsigned long long m_num_pkt_in_network;	// #in-transit packets
    unsigned long long m_num_flit_in_network;	// #in-transit flits

    // simulator execution start/end times
    time_t m_start_time;
    time_t m_end_time;
    int m_elapsed_time;
    int m_num_CSIM_process;

    double m_clk_warmup_end;
    double m_clk_sim_end;

    // latency:
    //   T_t = T_q (queuing) + T_n (network) + T_e (extra)
    //       = T_h (router) + T_w (wire) + T_s (serialization) + T_c (contention)
    //   T_c (contention) = T_t - T_h - T_w - T_s 
    //   T_t > T_ni
    //       T_ni = T_t - T_q (no T_ni) ???
    table* m_pkt_T_t_tab;	// total
    table* m_pkt_T_ni_tab;	// queuing(starting from injection pkt buffer) + network traversal
    table* m_pkt_T_q_tab;	// queuing
    table* m_pkt_T_n_tab;	// network traversal
    table* m_pkt_T_e_tab;	// extra (decoding, ...)

    // latency breakdown
    table* m_pkt_T_h_tab;	// router
    table* m_pkt_T_w_tab;	// wire
    table* m_pkt_T_s_tab;	// serialization

    // latency breakdown for packet type
    map< unsigned int, table* > m_pkttype_T_t_map;	// total
    map< unsigned int, table* > m_pkttype_T_q_map;	// queuing
    map< unsigned int, table* > m_pkttype_T_h_map;	// router
    map< unsigned int, table* > m_pkttype_T_w_map;	// wire
    map< unsigned int, table* > m_pkttype_T_s_map;	// serialization

    // per-source, per-destination, per-flow latency
    vector< table* > m_pkt_T_t_src_tab_vec;	// per-source queing + network
    vector< table* > m_pkt_T_t_dest_tab_vec;	// per-destination queing + network
    vector< vector < table* > > m_pkt_T_t_c2c_tab_vec;	// X[src_core][dest_core]: queing + network
    vector< vector < table* > > m_pkt_T_s_c2c_tab_vec;	// X[src_core][dest_core]: serialization

    // for generating packet and flit identifiers uniquely
    unsigned long long m_next_pkt_id;
    unsigned long long m_next_flit_id;

    // hop count
    table* m_hop_count_tab;

    bool m_warmup_phase;// true : warmup
			// false: measurement after warmup

    unsigned long long m_num_instr_executed;	// for trace workload

    // periodic interval profiling
    table* m_periodic_pkt_T_t_tab;    // total
    table* m_periodic_pkt_T_q_tab;    // queuing
    table* m_periodic_pkt_T_hws_tab;  // T_h + T_w + T_s
    unsigned long long m_periodic_ejt_pkt;
    unsigned long long m_periodic_ejt_flit;
    unsigned long long m_periodic_inj_pkt;
    unsigned long long m_periodic_inj_flit;
    unsigned long long m_periodic_instr_executed;	// for the previous interval
};

inline SimOut::SimOut()
{
    m_num_pkt_inj = m_num_pkt_ejt = 0;
    m_num_flit_inj = m_num_flit_ejt = 0;
    m_num_pkt_in_network = m_num_flit_in_network = 0;
    m_num_pkt_spurious = 0;
    m_num_pkt_inj_warmup = m_num_pkt_ejt_warmup = 0;
    m_num_flit_inj_warmup = m_num_flit_ejt_warmup = 0;
    m_num_pkt_inj_multicast = 0;
    m_sum_multicast_dest = 0;

    m_clk_warmup_end = 0.0;
    m_clk_sim_end = 0.0;
    m_next_pkt_id = 0;
    m_next_flit_id = 0;
    m_warmup_phase = true;
    m_num_instr_executed = 0;

    m_start_time = time((time_t *)NULL);
    m_end_time = time((time_t *)NULL);
    m_num_CSIM_process = 0;

    m_pkt_T_t_tab = new table("simout_pkt_T_t");
    m_pkt_T_ni_tab = new table("simout_pkt_T_ni");
    m_pkt_T_q_tab = new table("simout_pkt_T_q");
    m_pkt_T_n_tab = new table("simout_pkt_T_n");
    m_pkt_T_e_tab = new table("simout_pkt_T_e");
    m_pkt_T_h_tab = new table("simout_pkt_T_h");
    m_pkt_T_s_tab = new table("simout_pkt_T_s");
    m_pkt_T_w_tab = new table("simout_pkt_T_w");

    m_periodic_pkt_T_t_tab = new table("simout_profile_pkt_T_t");
    m_periodic_pkt_T_q_tab = new table("simout_profile_pkt_T_q");
    m_periodic_pkt_T_hws_tab = new table("simout_profile_pkt_T_hws");

    m_hop_count_tab = new table ("simout_hop_count");
}

inline SimOut::~SimOut()
{
    if (m_pkt_T_t_tab) delete m_pkt_T_t_tab;
    if (m_pkt_T_ni_tab) delete m_pkt_T_ni_tab;
    if (m_pkt_T_q_tab) delete m_pkt_T_q_tab;
    if (m_pkt_T_n_tab) delete m_pkt_T_n_tab;
    if (m_pkt_T_e_tab) delete m_pkt_T_e_tab;
    if (m_pkt_T_h_tab) delete m_pkt_T_h_tab;
    if (m_pkt_T_s_tab) delete m_pkt_T_s_tab;
    if (m_pkt_T_w_tab) delete m_pkt_T_w_tab;

    for (unsigned int i=0; i<m_pkt_T_t_src_tab_vec.size(); i++)
        delete m_pkt_T_t_src_tab_vec[i];
    for (unsigned int i=0; i<m_pkt_T_t_dest_tab_vec.size(); i++)
        delete m_pkt_T_t_dest_tab_vec[i];
    for (unsigned int s=0; s<m_pkt_T_t_c2c_tab_vec.size(); s++)
    for (unsigned int d=0; d<m_pkt_T_t_c2c_tab_vec[s].size(); d++) {
        delete m_pkt_T_t_c2c_tab_vec[s][d];
        delete m_pkt_T_s_c2c_tab_vec[s][d];
    }

    if (m_periodic_pkt_T_t_tab) delete m_periodic_pkt_T_t_tab;
    if (m_periodic_pkt_T_q_tab) delete m_periodic_pkt_T_q_tab;
    if (m_periodic_pkt_T_hws_tab) delete m_periodic_pkt_T_hws_tab;

    if (m_hop_count_tab) delete m_hop_count_tab;

    for (map< unsigned int, table* >::iterator pos = m_pkttype_T_t_map.begin();
        pos != m_pkttype_T_t_map.end(); ++pos) {
        unsigned int pkt_type = pos->first;
        delete m_pkttype_T_t_map[pkt_type];
        delete m_pkttype_T_q_map[pkt_type];
        delete m_pkttype_T_h_map[pkt_type];
        delete m_pkttype_T_s_map[pkt_type];
        delete m_pkttype_T_w_map[pkt_type];
    }
}

inline void SimOut::init()
{
    char table_name[64];

    for (int i=0; i<g_cfg.core_num; i++) {
        sprintf(table_name, "simout_pkt_T_t_src_vec_%d\n", i);
        m_pkt_T_t_src_tab_vec.push_back(new table (table_name));

        sprintf(table_name, "simout_pkt_T_t_dest_vec_%d\n", i);
        m_pkt_T_t_dest_tab_vec.push_back(new table (table_name));
    }

    m_pkt_T_t_c2c_tab_vec.resize(g_cfg.core_num);
    m_pkt_T_s_c2c_tab_vec.resize(g_cfg.core_num);
    for (int s=0; s<g_cfg.core_num; s++) {
        m_pkt_T_t_c2c_tab_vec[s].resize(g_cfg.core_num);
        m_pkt_T_s_c2c_tab_vec[s].resize(g_cfg.core_num);
        for (int d=0; d<g_cfg.core_num; d++) {
            sprintf(table_name, "simout_pkt_T_t_c2c_%d_%d\n", s, d);
            m_pkt_T_t_c2c_tab_vec[s][d] = new table (table_name);
            assert(m_pkt_T_t_c2c_tab_vec[s][d]);

            sprintf(table_name, "simout_pkt_T_s_c2c_%d_%d\n", s, d);
            m_pkt_T_s_c2c_tab_vec[s][d] = new table (table_name);
            assert(m_pkt_T_s_c2c_tab_vec[s][d]);
        }
    }
}

inline void SimOut::recordPkt(Packet* p_pkt, double extra_lat_NIout, bool profile_perf)
{
    double T_t = (simtime() - p_pkt->m_clk_gen) + extra_lat_NIout;
    double T_n = (simtime() - p_pkt->m_clk_enter_net) + extra_lat_NIout;
    double T_ni = (simtime() - p_pkt->m_clk_store_NIin) + extra_lat_NIout;
    int T_q = (int) (p_pkt->m_clk_enter_net - p_pkt->m_clk_gen);
    int T_h = p_pkt->m_hops*g_cfg.router_num_pipelines;
    int T_s = p_pkt->m_num_flits;
    int T_w = p_pkt->m_wire_delay;
    // int T_c = ((int) T_t) - T_h - T_w - T_s;

// printf("p=%06lld p_pkt->(m_clk_gen=%.0lf m_clk_store_NIin=%.0lf m_clk_enter_net=%.0lf) T_t=%.0lf T_q=%d T_ni=%.0lf\n", p_pkt->id(), p_pkt->m_clk_gen, p_pkt->m_clk_store_NIin, p_pkt->m_clk_enter_net, T_t, T_q, T_ni);

    m_pkt_T_t_tab->tabulate(T_t);
    m_pkt_T_ni_tab->tabulate(T_ni);
    m_pkt_T_q_tab->tabulate(T_q);
    m_pkt_T_n_tab->tabulate(T_n);
    m_pkt_T_e_tab->tabulate(extra_lat_NIout);
    m_pkt_T_h_tab->tabulate(T_h);
    m_pkt_T_w_tab->tabulate(T_w);
    m_pkt_T_s_tab->tabulate(T_s);

    m_pkt_T_t_src_tab_vec[p_pkt->getSrcCoreID()]->tabulate(T_t);
    m_pkt_T_t_dest_tab_vec[p_pkt->getDestCoreID()]->tabulate(T_t);

    m_pkt_T_t_c2c_tab_vec[p_pkt->getSrcCoreID()][p_pkt->getDestCoreID()]->tabulate(T_t);
    m_pkt_T_s_c2c_tab_vec[p_pkt->getSrcCoreID()][p_pkt->getDestCoreID()]->tabulate(T_s);

    // hop count
    m_hop_count_tab->tabulate(p_pkt->m_hops);

    if (profile_perf) {
        m_periodic_pkt_T_t_tab->tabulate(T_t);
        m_periodic_pkt_T_q_tab->tabulate(T_q);
        m_periodic_pkt_T_hws_tab->tabulate(T_h + T_w + T_s);
    }

    if (m_pkttype_T_t_map.find(p_pkt->m_packet_type) == m_pkttype_T_t_map.end()) {
        m_pkttype_T_t_map.insert( make_pair(p_pkt->m_packet_type, new table()) );
        m_pkttype_T_q_map.insert( make_pair(p_pkt->m_packet_type, new table()) );
        m_pkttype_T_h_map.insert( make_pair(p_pkt->m_packet_type, new table()) );
        m_pkttype_T_w_map.insert( make_pair(p_pkt->m_packet_type, new table()) );
        m_pkttype_T_s_map.insert( make_pair(p_pkt->m_packet_type, new table()) );
    }

    m_pkttype_T_t_map[p_pkt->m_packet_type]->tabulate(T_t);
    m_pkttype_T_q_map[p_pkt->m_packet_type]->tabulate(T_q);
    m_pkttype_T_h_map[p_pkt->m_packet_type]->tabulate(T_h);
    m_pkttype_T_w_map[p_pkt->m_packet_type]->tabulate(T_w);
    m_pkttype_T_s_map[p_pkt->m_packet_type]->tabulate(T_s);
}

inline void SimOut::resetStats()
{
    m_pkt_T_t_tab->reset();
    m_pkt_T_ni_tab->reset();
    m_pkt_T_q_tab->reset();
    m_pkt_T_n_tab->reset();
    m_pkt_T_e_tab->reset();
    m_pkt_T_h_tab->reset();
    m_pkt_T_w_tab->reset();
    m_pkt_T_s_tab->reset();

    for (unsigned int i=0; i<m_pkt_T_t_src_tab_vec.size(); i++)
        m_pkt_T_t_src_tab_vec[i]->reset();
    for (unsigned int i=0; i<m_pkt_T_t_dest_tab_vec.size(); i++)
        m_pkt_T_t_dest_tab_vec[i]->reset();
    for (unsigned int s=0; s<m_pkt_T_t_c2c_tab_vec.size(); s++)
    for (unsigned int d=0; d<m_pkt_T_t_c2c_tab_vec[s].size(); d++) {
        m_pkt_T_t_c2c_tab_vec[s][d]->reset();
        m_pkt_T_s_c2c_tab_vec[s][d]->reset();
    }

    for (map< unsigned int, table* >::iterator pos = m_pkttype_T_t_map.begin();
        pos != m_pkttype_T_t_map.end(); ++pos) {
        unsigned int pkt_type = pos->first;
        m_pkttype_T_t_map[pkt_type]->reset();
        m_pkttype_T_q_map[pkt_type]->reset();
        m_pkttype_T_h_map[pkt_type]->reset();
        m_pkttype_T_w_map[pkt_type]->reset();
        m_pkttype_T_s_map[pkt_type]->reset();
    }

    m_hop_count_tab->reset();
}

inline void SimOut::resetStatsPeriodic()
{
    m_periodic_pkt_T_t_tab->reset();
    m_periodic_pkt_T_q_tab->reset();
    m_periodic_pkt_T_hws_tab->reset();
    m_periodic_ejt_pkt = 0;
    m_periodic_ejt_flit = 0;
    m_periodic_inj_pkt = 0;
    m_periodic_inj_flit = 0;
    m_periodic_instr_executed = m_num_instr_executed;
}

#endif // #ifndef _SIM_OUT_H_
