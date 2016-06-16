#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "noc.h"
#include "WorkloadSNUCAValue.h"
#include "TopologySNUCA.h"
#include "gzlib.h"

// #define _DEBUG_SNUCA_VALUE

WorkloadSNUCAValue::WorkloadSNUCAValue() : WorkloadTiledCMPValue()
{
    m_workload_name = "SNUCA Cache Coherence Trace with Value";
    m_containData = true;
}

WorkloadSNUCAValue::~WorkloadSNUCAValue()
{
}

void WorkloadSNUCAValue::printStats(ostream& out) const
{
}

void WorkloadSNUCAValue::print(ostream& out) const
{
}

vector< Packet* > WorkloadSNUCAValue::readTrace()
{
    PktTraceValue tr;
    int num_flits;
    vector< Packet* > pkt_vec;

    // read one line trace
    assert(m_trace_fp > 0);

    tr.cycle = 0;
    tr.sz_bytes = 0;
    igzstream_read(m_trace_fp, (char*) &tr, sizeof(PktTraceValue));
    if (tr.cycle==0 || tr.sz_bytes==0 || igzstream_feof(m_trace_fp)) {
        // close the current file
        closeTraceFile();

        // open the next file
        if (! openTraceFile() ) { // no trace file?
            pkt_vec.push_back(0);
            return pkt_vec;
        }

        igzstream_read(m_trace_fp, (char*) &tr, sizeof(PktTraceValue));
        // assert(tr->cycle != 0);
    }

    // post processing
    // set source/destination router-id
    int src_router_id = ((TopologySNUCA*) g_Topology)->getRouterID(tr.src_mach_type, tr.src_mach_num);
    int dest_router_id = ((TopologySNUCA*) g_Topology)->getRouterID(tr.dest_mach_type, tr.dest_mach_num);
    int src_core_id = ((TopologySNUCA*) g_Topology)->getCoreID(tr.src_mach_type, tr.src_mach_num);
    int dest_core_id = ((TopologySNUCA*) g_Topology)->getCoreID(tr.dest_mach_type, tr.dest_mach_num);
    // set input/output-NI position
    int NI_in_pos = 0;
    int NI_out_pos = 0;

    num_flits = (int) ceil( (tr.sz_bytes * BITS_IN_BYTE)
                            / ( (double) g_cfg.link_width ));

#ifdef _DEBUG_SNUCA_VALUE
string src_mach_type_str;
switch (tr.src_mach_type) {
case MachineType_L1Cache: src_mach_type_str = "L1$"; break;
case MachineType_L2Cache: src_mach_type_str = "L2$"; break;
case MachineType_Directory: src_mach_type_str = "Dir"; break;
default: assert(0);
}
string dest_mach_type_str;
switch (tr.dest_mach_type) {
case MachineType_L1Cache: dest_mach_type_str = "L1$"; break;
case MachineType_L2Cache: dest_mach_type_str = "L2$"; break;
case MachineType_Directory: dest_mach_type_str = "Dir"; break;
default: assert(0);
}
printf("SNUCA tr: router(%2d[ipc=%d[=>%2d[epc=%d]) core(%3d[%s:%3d] => %3d[%s:%3d]) sz=%d #flits=%d cycle=%lld\n", src_router_id, NI_in_pos, dest_router_id, NI_out_pos, src_core_id, src_mach_type_str.c_str(), tr.src_mach_num, dest_core_id, dest_mach_type_str.c_str(), tr.dest_mach_num, tr.sz_bytes, num_flits, tr.cycle);
#endif

    // make one packet
    Packet* p_pkt = g_PacketPool.alloc();
    assert(p_pkt);
    p_pkt->setID(g_sim.m_next_pkt_id++);
    p_pkt->m_start_flit_id = g_sim.m_next_flit_id;
    p_pkt->m_num_flits = num_flits;
    g_sim.m_next_flit_id += num_flits;
    p_pkt->setSrcRouterID(src_router_id);
    p_pkt->addDestRouterID(dest_router_id);
    p_pkt->setSrcCoreID(src_core_id);
    p_pkt->addDestCoreID(dest_core_id);
    p_pkt->m_clk_gen = (double) tr.cycle;
    p_pkt->m_NI_in_pos = NI_in_pos;
    p_pkt->m_NI_out_pos = NI_out_pos;

    // packet type
    if (tr.sz_bytes == (int) CONTROL_MESSAGE_SIZE) {
        p_pkt->m_packet_type = PACKET_TYPE_UNICAST_SHORT;
    } else {
        assert(tr.sz_bytes == (int) DATA_MESSAGE_SIZE);
        p_pkt->m_packet_type = PACKET_TYPE_UNICAST_LONG;
    }

    // set packet data
    assert(tr.sz_bytes%8 == 0);
    int pkt_data_sz = tr.sz_bytes/8;
    p_pkt->m_packetData_vec.resize(pkt_data_sz);
    if (pkt_data_sz == 1) {
        // address only
        p_pkt->m_packetData_vec[0] = tr.addr_value;
#ifdef _DEBUG_SNUCA_VALUE
printf("  addr=%016llX\n", tr.addr_value);
#endif
    } else {
        // address + data block
        p_pkt->m_packetData_vec[0] = tr.addr_value;
#ifdef _DEBUG_SNUCA_VALUE
printf("  addr=%016llX\n", tr.addr_value);
#endif

        int data_value_pos = 0;
        for (int i=1; i<pkt_data_sz; i++) {
            unsigned long long data64bit = 0x0;
#ifdef _DEBUG_SNUCA_VALUE
printf("  ");
#endif
            for (int j=0; j<7; j++) {
#ifdef _DEBUG_SNUCA_VALUE
printf("%02X ", tr.data_value[data_value_pos]);
#endif
                data64bit |= tr.data_value[data_value_pos];
                data64bit <<= 8;

                ++data_value_pos;
            }
#ifdef _DEBUG_SNUCA_VALUE
printf("%02X ", tr.data_value[data_value_pos]);
#endif
            data64bit |= tr.data_value[data_value_pos];
            ++data_value_pos;

            p_pkt->m_packetData_vec[i] = data64bit;
#ifdef _DEBUG_SNUCA_VALUE
printf("\ndata64bit[%d]=%016llX\n", i, data64bit);
#endif
        }
    }

    // spatial distribution
    m_spat_pattern_pkt_vec[src_core_id][dest_core_id]++;
    m_spat_pattern_flit_vec[src_core_id][dest_core_id] += num_flits;

    // throughput profile
    if (g_cfg.profile_perf) {
        g_sim.m_periodic_inj_pkt++;
        g_sim.m_periodic_inj_flit += num_flits;
    }

    m_num_proc_traces++;

    pkt_vec.push_back(p_pkt);
    return pkt_vec;
}
