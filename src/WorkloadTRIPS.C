#include "noc.h"
#include "WorkloadTRIPS.h"

WorkloadTRIPS::WorkloadTRIPS() : WorkloadTrace()
{
    m_workload_name = "TRIPS Trace";
    m_containData = false;
}

WorkloadTRIPS::~WorkloadTRIPS()
{
}

vector< Packet* > WorkloadTRIPS::readTrace()
{
    char line_buf[1024];
    int src_x, src_y;
    int src_id;
    int dest_x, dest_y;
    int dest_id;
    double injection_tm;
    double injection_cycle;
    vector< Packet* > pkt_vec;

    // read one line trace
    assert(m_trace_fp);

    if (feof(m_trace_fp))
        return pkt_vec;

    strcpy(line_buf, "");
    char* tmp=0;
    tmp = fgets(line_buf, 1024, m_trace_fp);
    if (strlen(line_buf) == 0) { // end of trace ?
        pkt_vec.push_back(0);
        return pkt_vec;
    }

    // separate string by space
    vector< string > substr_vec = split_str(string(line_buf), ' ');

    int network_radix = (int) sqrt(g_cfg.router_num);	// #nodes/dimension

    // source node
    src_x = substr_vec[1].at(1) - '0';
    src_y = substr_vec[1].at(3) - '0';
    src_id = src_y * network_radix + src_x;

    // destination node
    dest_x = substr_vec[2].at(1) - '0';
    dest_y = substr_vec[2].at(3) - '0';
    dest_id = dest_y * network_radix + dest_x;

    // injection time
    injection_tm = atof(substr_vec[3].c_str());
    injection_cycle = injection_tm * 8.0;

    // packet data

// printf("%d %d %d, %d %d %d inj_tm=%lf %llX\n", src_x, src_y, src_id, dest_x, dest_y, dest_id, injection_tm, pkt_data);

    m_num_proc_traces++;

    // make one packet
    Packet* p_pkt = g_PacketPool.alloc();
    assert(p_pkt);
    p_pkt->setID(g_sim.m_next_pkt_id++);
    p_pkt->m_start_flit_id = g_sim.m_next_flit_id;
    p_pkt->m_num_flits = g_cfg.wkld_synth_num_flits_pkt;
    g_sim.m_next_flit_id += p_pkt->m_num_flits;
    p_pkt->setSrcRouterID(src_id);
    p_pkt->addDestRouterID(dest_id);
    p_pkt->m_clk_gen = injection_cycle;
    p_pkt->m_NI_in_pos = 0;
    p_pkt->m_NI_out_pos = 0;

    // spatial distribution
    m_spat_pattern_pkt_vec[src_id][dest_id]++;
    m_spat_pattern_flit_vec[src_id][dest_id] += p_pkt->m_num_flits;

    // throughput profile
    if (g_cfg.profile_perf) {
        g_sim.m_periodic_inj_pkt++;
        g_sim.m_periodic_inj_flit += p_pkt->m_num_flits;
    }

#ifdef _DEBUG_ROUTER
printf("TRIPS GEN mesg[p=%lld] %d->%d #flits=%d clk=%.1lf\n", p_pkt->id(), p_pkt->getSrcRouterID(), p_pkt->getDestRouterID(), p_pkt->m_num_flits, clock);
#endif

    pkt_vec.push_back(p_pkt);
    return pkt_vec;
}

bool WorkloadTRIPS::openTraceFile()
{
    char trace_path_name[256];
    sprintf(trace_path_name, "%s/%s.pkt", m_trace_dir_name.c_str(), m_benchmark_name.c_str());

    m_trace_fp = fopen(trace_path_name, "r");
    if (m_trace_fp == 0) {
        fprintf(stderr, "file[%s] open failed.\n", trace_path_name);
        return false;
    }

    return true;
}

void WorkloadTRIPS::printStats(ostream& out) const
{
}

void WorkloadTRIPS::print(ostream& out) const
{
}


void config_TRIPS_network()
{
    // simulation termination condition
    // end_clk is set when the last trace in file is read.
    g_cfg.sim_end_cond = SIM_END_BY_CYCLE;

    // network
    g_cfg.router_num = 25;

    // router
    g_cfg.router_inbuf_depth = 8;
    // g_cfg.router_num_pc = 5;
    // g_cfg.router_num_vc = 4;

    // link
    g_cfg.link_length = 3;    // 3mm

    // clock frequency
    g_cfg.chip_freq = 2.0 * UNIT_GIGA; // 2GHz
}
