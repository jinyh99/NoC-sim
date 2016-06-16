#include "noc.h"
#include "WorkloadSynthetic.h"
#include "Core.h"
#include "Topology.h"

static bool intVecGreater(const int e1, int e2)
{
    return (e1 > e2);
}

WorkloadSynthetic::WorkloadSynthetic()
{
    switch (g_cfg.wkld_type) {
    case WORKLOAD_SYNTH_SPATIAL:
        m_workload_name = "Synthetic - ";
        switch (g_cfg.wkld_synth_spatial) {
        case WORKLOAD_SYNTH_SP_UR:	m_workload_name += "uniform random"; break;
        case WORKLOAD_SYNTH_SP_NN:	m_workload_name += "nearest neighbor"; break;
        case WORKLOAD_SYNTH_SP_BC:	m_workload_name += "bit complement"; break;
        case WORKLOAD_SYNTH_SP_TP:	m_workload_name += "transpose"; break;
        case WORKLOAD_SYNTH_SP_TOR:	m_workload_name += "tornado"; break;
        default: assert(0);
        }
        break;
    case WORKLOAD_SYNTH_TRAFFIC_MATRIX:
        m_workload_name = "Synthetic Traffic Matrix";
        break;
    default: assert(0);
    }
    m_synthetic = true;
    m_containData = false;

    // packet inter-arrival time
    if (g_cfg.wkld_synth_bimodal) {
        double avg_num_flits_pkt = g_cfg.wkld_synth_num_flits_pkt*(1.0-g_cfg.wkld_synth_bimodal_single_pkt_rate) + 1.0*g_cfg.wkld_synth_bimodal_single_pkt_rate;
        m_pkt_inter_arrv_time = 1.0/g_cfg.wkld_synth_load*avg_num_flits_pkt;
    } else {
        m_pkt_inter_arrv_time = 1.0/g_cfg.wkld_synth_load*g_cfg.wkld_synth_num_flits_pkt;
    }

    // network radix
    m_network_radix = (int) sqrt(g_cfg.core_num);

    for (int i=0; i<g_cfg.core_num; i++) {
        m_stream_temporal_vec.push_back(new stream());
        m_stream_spatial_vec.push_back(new stream());

        if (g_cfg.wkld_synth_bimodal)
            m_stream_pktlen_bimodality_vec.push_back(new stream());

        // multicast
        m_stream_multicast_ratio_vec.push_back(new stream());
        m_stream_multicast_destnum_vec.push_back(new stream());
    }

    m_SS_OnMode_vec.resize(g_cfg.core_num, false);
    m_SS_last_OnTimeStamp_vec.resize(g_cfg.core_num, 0.0);


    // make configuration string
    buildConfigStr();
}

WorkloadSynthetic::~WorkloadSynthetic()
{
    for (unsigned int i=0; i<m_stream_temporal_vec.size(); i++) {
        delete m_stream_temporal_vec[i];
        delete m_stream_spatial_vec[i];
        if (g_cfg.wkld_synth_bimodal)
            delete m_stream_pktlen_bimodality_vec[i];

        // multicast
        delete m_stream_multicast_ratio_vec[i];
        delete m_stream_multicast_destnum_vec[i];
    }
    m_stream_temporal_vec.clear();
    m_stream_spatial_vec.clear();
    m_stream_pktlen_bimodality_vec.clear();
    m_stream_multicast_ratio_vec.clear();
    m_stream_multicast_destnum_vec.clear();
}

void WorkloadSynthetic::buildConfigStr()
{
    m_workload_config = "";

    m_workload_config += "  self-similar: ";
    m_workload_config += g_cfg.wkld_synth_ss ? "Yes\n" : "No\n";

    m_workload_config += "  injection load(flit/cycle/core): " + double2str(g_cfg.wkld_synth_load, 4) + "\n";

    m_workload_config += "  injection load(pkt/cycle/core): " + double2str(g_cfg.wkld_synth_load/g_cfg.wkld_synth_num_flits_pkt, 4) + "\n";

    m_workload_config += "  packet inter-arrival time(cycle): " + double2str(m_pkt_inter_arrv_time, 4) + "\n";

    m_workload_config += "  #flits per packet: " + int2str(g_cfg.wkld_synth_num_flits_pkt) + "\n";

    m_workload_config += "  packet length bimodality: ";
    m_workload_config += g_cfg.wkld_synth_bimodal ? string("Yes") : string("No");

    m_workload_config += "  single_flit_pkt_rate: " + double2str(g_cfg.wkld_synth_bimodal_single_pkt_rate, 4) + "\n";

    m_workload_config += "  multicast ratio: " + double2str(g_cfg.wkld_synth_multicast_ratio, 4) + "\n";
    m_workload_config += "  #multicast destinations: " + int2str(g_cfg.wkld_synth_multicast_destnum) + "\n";
}

Packet* WorkloadSynthetic::genPacket(int src_core_id)
{
    Packet* p_pkt = g_PacketPool.alloc();
    assert(p_pkt);

    p_pkt->setID(g_sim.m_next_pkt_id++);
    p_pkt->m_start_flit_id = g_sim.m_next_flit_id;
    p_pkt->m_clk_gen = simtime();
    p_pkt->m_NI_in_pos = 0;
    p_pkt->m_NI_out_pos = 0;

    // determine the number of flits in a packet.
    int num_flits = g_cfg.wkld_synth_num_flits_pkt;
    if (g_cfg.wkld_synth_bimodal) {
        double prob = m_stream_temporal_vec[src_core_id]->uniform(0.0, 1.0);
        if (prob <= g_cfg.wkld_synth_bimodal_single_pkt_rate) {
            num_flits = 1;
            p_pkt->m_packet_type = PACKET_TYPE_UNICAST_SHORT;
        } else {
            p_pkt->m_packet_type = PACKET_TYPE_UNICAST_LONG;
        }
    }
    p_pkt->m_num_flits = num_flits;
    g_sim.m_next_flit_id += num_flits;

    // determine source core and router
    p_pkt->setSrcCoreID(src_core_id);
    int src_router_id = INVALID_ROUTER_ID;
    switch (g_cfg.net_topology) {
    case TOPOLOGY_MESH:
    case TOPOLOGY_TORUS:
    case TOPOLOGY_HMESH:
        src_router_id = src_core_id;
        break;
    case TOPOLOGY_DMESH:
        src_router_id = getRouterRelativeID(src_core_id, (int) sqrt((double) g_cfg.core_num), (int) sqrt((double) (g_cfg.router_num/g_cfg.net_networks)));
        break;
    case TOPOLOGY_FAT_TREE:
    case TOPOLOGY_FLBFLY:
      {
        pair< int, int > src_pair = g_Topology->core2router(src_core_id);
        src_router_id = src_pair.first;
        p_pkt->m_NI_in_pos = src_pair.second;
      }
        break;
    default:
        assert(0);
    }
    p_pkt->setSrcRouterID(src_router_id);

    // determine unicast or multicast
    bool is_multicast = false;
    if (g_cfg.wkld_synth_multicast_ratio > 0.0)
        is_multicast = (m_stream_multicast_ratio_vec[src_core_id]->uniform(0.0, 1.0) < g_cfg.wkld_synth_multicast_ratio) ? true: false;

    // determine destination cores
    vector< int > dest_core_id_vec;
    if (is_multicast) {
       dest_core_id_vec = spatialPatternMulticast(src_core_id);
       p_pkt->m_packet_type = PACKET_TYPE_MULTICAST_LONG;
    } else {
       dest_core_id_vec.push_back(spatialPattern(src_core_id));
       p_pkt->m_packet_type = PACKET_TYPE_UNICAST_LONG;
    }
    p_pkt->addDestCoreIDVec(dest_core_id_vec);

    // determine destination routers
    IntSet dest_router_id_set;
    for (unsigned int i=0; i<dest_core_id_vec.size(); i++) {
        int dest_core_id = dest_core_id_vec[i];

        switch (g_cfg.net_topology) {
        case TOPOLOGY_MESH:
        case TOPOLOGY_TORUS:
        case TOPOLOGY_HMESH:
            dest_router_id_set.insert(dest_core_id);
            break;
        case TOPOLOGY_DMESH:
            dest_router_id_set.insert(getRouterRelativeID(dest_core_id, (int) sqrt((double) g_cfg.core_num), (int) sqrt((double) (g_cfg.router_num/g_cfg.net_networks))));
            break;
        case TOPOLOGY_FAT_TREE:
        case TOPOLOGY_FLBFLY:
          {
            pair< int, int > dest_pair = g_Topology->core2router(dest_core_id);
            dest_router_id_set.insert(dest_pair.first);
            p_pkt->m_NI_out_pos = dest_pair.second;
          }
            break;

        default:
            assert(0);
        }
    }
    vector< int > dest_router_id_vec(dest_router_id_set.begin(), dest_router_id_set.end());
    sort(dest_router_id_vec.begin(), dest_router_id_vec.end(), intVecGreater);
    p_pkt->addDestRouterIDVec(dest_router_id_vec);

    // spatial distribution
    for (unsigned int i=0; i<dest_core_id_vec.size(); i++) {
        int dest_core_id = dest_core_id_vec[i];
        m_spat_pattern_pkt_vec[src_core_id][dest_core_id]++;
        m_spat_pattern_flit_vec[src_core_id][dest_core_id] += p_pkt->m_num_flits;
    }

    // throughput profile
    if (g_cfg.profile_perf) {
        g_sim.m_periodic_inj_pkt++;
        g_sim.m_periodic_inj_flit += p_pkt->m_num_flits;
    }

#ifdef _DEBUG_ROUTER
    string dest_core_id_str = "";
    if (p_pkt->getDestCoreNum() == 1) {
        dest_core_id_str = int2str(p_pkt->getDestCoreID());
    } else {
        vector< int > dest_core_id_vec = p_pkt->getDestCoreIDVec();
        for (int i=0; i<dest_core_id_vec.size(); i++) {
            dest_core_id_str += int2str(dest_core_id_vec[i]);
            dest_core_id_str += ",";
        }
    }

    string dest_router_id_str = "";
    if (p_pkt->getDestRouterNum() == 1) {
        dest_router_id_str = int2str(p_pkt->getDestRouterID());
    } else {
        vector< int > dest_router_id_vec = p_pkt->getDestRouterIDVec();
        for (int i=0; i<dest_router_id_vec.size(); i++) {
            dest_router_id_str += int2str(dest_router_id_vec[i]);
            dest_router_id_str += ",";
        }
    }

    printf("GEN clk=%.1lf p=%lld #flits=%d ", simtime(), p_pkt->id(), p_pkt->m_num_flits);
    printf("core(%d->%s) router(%d->%s)\n", p_pkt->getSrcCoreID(), dest_core_id_str.c_str(), p_pkt->getSrcRouterID(), dest_router_id_str.c_str());
#endif

    return p_pkt;
}

int WorkloadSynthetic::spatialPattern(int src_core_id)
{
    switch (g_cfg.wkld_synth_spatial) {
    case WORKLOAD_SYNTH_SP_UR:
        return spatialUR(src_core_id);
    case WORKLOAD_SYNTH_SP_NN:
        return spatialNN(src_core_id);
    case WORKLOAD_SYNTH_SP_BC:
        return spatialBC(src_core_id);
    case WORKLOAD_SYNTH_SP_TP:
        return spatialTP(src_core_id);
    case WORKLOAD_SYNTH_SP_TOR:
        return spatialTOR(src_core_id);
    default:
        break;
    }

    assert(0);
    return INVALID_ROUTER_ID;
}

int WorkloadSynthetic::spatialUR(int src_core_id)
{
    // int src_x = src_core_id % m_network_radix;
    // int src_y = src_core_id / m_network_radix;

    return (src_core_id + m_stream_spatial_vec[src_core_id]->random(0L, (long)(g_cfg.core_num - 1))) % g_cfg.core_num;
}

int WorkloadSynthetic::spatialNN(int src_core_id)
{
    int src_x = src_core_id % m_network_radix;
    int src_y = src_core_id / m_network_radix;
    int dst_id, dst_x, dst_y;

    // 2 candidates
    if (src_core_id==0 || src_core_id==m_network_radix-1 || src_core_id==m_network_radix*(m_network_radix-1) || src_core_id==m_network_radix*m_network_radix-1) {
        int index = m_stream_spatial_vec[src_core_id]->random(0L, (long)(g_cfg.core_num - 1)) % 2;
        if (src_core_id==0)
            dst_id = (index == 0) ? 1 : m_network_radix;
        else if (src_core_id==m_network_radix-1)
            dst_id = (index == 0) ? m_network_radix-2 : m_network_radix*2-1;
        else if (src_core_id==m_network_radix*(m_network_radix-1))
            dst_id = (index == 0) ? m_network_radix*(m_network_radix-2) : m_network_radix*(m_network_radix-1)+1;
        else	// src_core_id==m_network_radix*m_network_radix-1
            dst_id = (index == 0) ? m_network_radix*(m_network_radix-1)-1 : m_network_radix*m_network_radix-2;

    // 3 candidates
    } else if (src_x == 0) {
        int index = m_stream_spatial_vec[src_core_id]->random(0L, (long)(g_cfg.core_num - 1)) % 3;
        switch (index) {
        case 0:
            dst_x = src_x+1; dst_y = src_y; break;
        case 1:
            dst_x = src_x; dst_y = src_y+1; break;
        default:
            dst_x = src_x; dst_y = src_y-1; break;
        }
        dst_id = dst_x + dst_y*m_network_radix;
    } else if (src_y == 0) {
        int index = m_stream_spatial_vec[src_core_id]->random(0L, (long)(g_cfg.core_num - 1)) % 3;
        switch (index) {
        case 0:	// west
            dst_x = src_x-1; dst_y = src_y; break;
        case 1: // east
            dst_x = src_x+1; dst_y = src_y; break;
        default: // north
            dst_x = src_x; dst_y = src_y+1; break;
        }
        dst_id = dst_x + dst_y*m_network_radix;
    } else if (src_x == m_network_radix-1) {
        int index = m_stream_spatial_vec[src_core_id]->random(0L, (long)(g_cfg.core_num - 1)) % 3;
        switch (index) {
        case 0:	// west
            dst_x = src_x-1; dst_y = src_y; break;
        case 1: // north
            dst_x = src_x; dst_y = src_y+1; break;
        default: // south
            dst_x = src_x; dst_y = src_y-1; break;
        }
        dst_id = dst_x + dst_y*m_network_radix;
    } else if (src_y == m_network_radix-1) {
        int index = m_stream_spatial_vec[src_core_id]->random(0L, (long)(g_cfg.core_num - 1)) % 3;
        switch (index) {
        case 0:	// west
            dst_x = src_x-1; dst_y = src_y; break;
        case 1: // east
            dst_x = src_x+1; dst_y = src_y; break;
        default: // south
            dst_x = src_x; dst_y = src_y-1; break;
        }
        dst_id = dst_x + dst_y*m_network_radix;

    // 4 candidates
    } else {
        int index = m_stream_spatial_vec[src_core_id]->random(0L, (long)(g_cfg.core_num - 1)) % 4;
        switch (index) {
        case 0:	// west
            dst_x = src_x-1; dst_y = src_y; break;
        case 1: // east
            dst_x = src_x+1; dst_y = src_y; break;
        case 2: // north
            dst_x = src_x; dst_y = src_y+1; break;
        default: // south
            dst_x = src_x; dst_y = src_y-1; break;
        }
        dst_id = dst_x + dst_y*m_network_radix;
    }

    return dst_id;
}

int WorkloadSynthetic::spatialBC(int src_core_id)
{
    int src_x = src_core_id % m_network_radix;
    int src_y = src_core_id / m_network_radix;

    return (m_network_radix - src_x - 1) +
           (m_network_radix - src_y - 1)*m_network_radix;
}

int WorkloadSynthetic::spatialTP(int src_core_id)
{
    int src_x = src_core_id % m_network_radix;
    int src_y = src_core_id / m_network_radix;

    return src_y + src_x*m_network_radix;
}

int WorkloadSynthetic::spatialTOR(int src_core_id)
{
    int src_x = src_core_id % m_network_radix;
    int src_y = src_core_id / m_network_radix;

    int dst_x = src_x + m_network_radix/2 - 1;

    if (dst_x >= m_network_radix)
        dst_x = m_network_radix - 1;

    return dst_x + src_y*m_network_radix;
}

vector< int > WorkloadSynthetic::spatialPatternMulticast(int src_core_id)
{
    vector< int > dest_core_id_vec;	// return value
    bitset< MAX_PACKET_NUM_DEST > dest_mark_bs = 0;

    // determine number of destination cores
    int num_dest_cores = (int) (m_stream_multicast_destnum_vec[src_core_id]->uniform(0.0, 1.0) * g_cfg.core_num);
    if (num_dest_cores < 2)
        num_dest_cores = 2;

    // determine each destination core
    int num_distinct = 0;
    while (num_distinct <= num_dest_cores) {
        int dest_core_id = spatialUR(src_core_id);

        if (! dest_mark_bs[dest_core_id] ) {	// different from previous ones ?
            dest_mark_bs[dest_core_id] = true;
            num_distinct++;

            dest_core_id_vec.push_back(dest_core_id);
        }
    }

    // sort numbers in ascending order
    sort(dest_core_id_vec.begin(), dest_core_id_vec.end(), intVecGreater);

    return dest_core_id_vec;
}

double WorkloadSynthetic::getWaitingTime(int src_core_id)
{
    return g_cfg.wkld_synth_ss ? temporalSelfSimilar(src_core_id) : temporalPoisson(src_core_id);
}

////////////////////////////////////////////////////////////////
// Pareto distribution
// IPDPS 1999
// Performance Evaluation of the ServerNetR SAN under Self-Similar Traffic
// ACM ToN 1994
// On the Self-Similar Nature of Ethernet Traffic
////////////////////////////////////////////////////////////////

double WorkloadSynthetic::temporalSelfSimilar(int src_core_id)
{
    double off_time = 0.0;

    if (m_SS_OnMode_vec[src_core_id]) {
        if (simtime() > m_SS_last_OnTimeStamp_vec[src_core_id]) {
            double x = m_stream_temporal_vec[src_core_id]->uniform(0.0, 1.0);
            double off_time = pow((1.0 - x), -1.0/1.25);
// printf("src_core_id=%d off_time=%lf\n", src_core_id, off_time);
            if (off_time < 1.0)
                off_time = 1.0;

            hold(off_time);
            m_SS_OnMode_vec[src_core_id] = false;
        }
    } else {
        double x = m_stream_temporal_vec[src_core_id]->uniform(0.0, 1.0);
        double on_time = pow((1.0 - x), (-1.0)/1.9);
// printf("src_core_id=%d on_time=%lf\n", src_core_id, on_time);
        m_SS_last_OnTimeStamp_vec[src_core_id] = simtime() + on_time;
        m_SS_OnMode_vec[src_core_id] = true;
    }

    return (off_time + temporalPoisson(src_core_id));
}

////////////////////////////////////////////////////////////////
// Poisson distribution
////////////////////////////////////////////////////////////////

double WorkloadSynthetic::temporalPoisson(int src_core_id)
{
    double hold_time = m_stream_temporal_vec[src_core_id]->exponential(m_pkt_inter_arrv_time);

    // hold_time must be integer.
    return ((double) ((int) hold_time));
}

void WorkloadSynthetic::printStats(ostream& out) const
{
}

void WorkloadSynthetic::print(ostream& out) const
{
}
