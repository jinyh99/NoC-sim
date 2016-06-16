#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "noc.h"
#include "WorkloadTiledCMPValue.h"
#include "gzlib.h"

static int g_NI_in_last_pos = 0;
static int g_NI_out_last_pos = 0; 

// #define _DEBUG_TRACE_TILED_CMP_VALUE

WorkloadTiledCMPValue::WorkloadTiledCMPValue() : WorkloadTiledCMP()
{
    m_workload_name = "Cache Coherence Trace with Value";
    m_containData = true;
}

WorkloadTiledCMPValue::~WorkloadTiledCMPValue()
{
}

void WorkloadTiledCMPValue::printStats(ostream& out) const
{
}

void WorkloadTiledCMPValue::print(ostream& out) const
{
}

vector< Packet* > WorkloadTiledCMPValue::readTrace()
{
    PktTraceValue tr;
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
        if (! openTraceFile() )	{ // no trace file ?
            pkt_vec.push_back(0);
            return pkt_vec;
        }
        fprintf(stderr, "benchmark=%s trace_file_id=%d successfully open.\n", m_benchmark_name.c_str(), m_trace_file_id-1);

        igzstream_read(m_trace_fp, (char*) &tr, sizeof(PktTraceValue));
        // assert(tr->cycle != 0);
    }
    // printTrace(cout, tr);

    // post processing
    int src_tile_id = (int) tr.src_mach_num;
    int dest_tile_id = (int) tr.dest_mach_num;
    g_sim.m_num_instr_executed = 0;

    int num_flits = (int) ceil( tr.sz_bytes * BITS_IN_BYTE
                                / ( (double) g_cfg.link_width ));
#ifdef _DEBUG_TRACE_TILED_CMP_VALUE
    printf("DEBUG_TILED_CMP_VALUE: src_tile=%d dest_tile=%d bytes=%d #flits=%d cycle=%lld cur_cycle=%.0lf\n", src_tile_id, dest_tile_id, tr.sz_bytes, num_flits, tr.cycle, simtime());
#endif

    // make one packet
    Packet* p_pkt = g_PacketPool.alloc();
    assert(p_pkt);
    p_pkt->setID(g_sim.m_next_pkt_id++);
    p_pkt->m_start_flit_id = g_sim.m_next_flit_id;
    p_pkt->m_num_flits = num_flits;
    g_sim.m_next_flit_id += num_flits;
    p_pkt->setSrcRouterID(src_tile_id);
    p_pkt->addDestRouterID(dest_tile_id);
    p_pkt->setSrcCoreID(src_tile_id);
    p_pkt->addDestCoreID(dest_tile_id);
    p_pkt->m_clk_gen = (double) tr.cycle;
    switch(tr.src_mach_type) {
    case MachineType_L1Cache: p_pkt->m_NI_in_pos = 0; break;
    case MachineType_L2Cache: p_pkt->m_NI_in_pos = 1; break;
    case MachineType_Directory: p_pkt->m_NI_in_pos = 2; break;
    default: assert(0);
    }
    switch (tr.dest_mach_type) {
    case MachineType_L1Cache: p_pkt->m_NI_out_pos = 0; break;
    case MachineType_L2Cache: p_pkt->m_NI_out_pos = 1; break;
    case MachineType_Directory: p_pkt->m_NI_out_pos = 2; break;
    default: assert(0);
    }

    // port multiplexing
    if (g_cfg.NI_port_mux) {
        p_pkt->m_NI_in_pos = g_NI_in_last_pos;
        p_pkt->m_NI_out_pos = g_NI_out_last_pos;
        g_NI_in_last_pos = (g_NI_in_last_pos + 1) % g_cfg.core_num_NIs;
        g_NI_out_last_pos = (g_NI_out_last_pos + 1) % g_cfg.core_num_NIs;
    }

    // packet type
    if (tr.sz_bytes == (int) CONTROL_MESSAGE_SIZE) {
        p_pkt->m_packet_type = PACKET_TYPE_UNICAST_SHORT;
    } else {
        assert(tr.sz_bytes == (int) DATA_MESSAGE_SIZE);
        p_pkt->m_packet_type = PACKET_TYPE_UNICAST_LONG;
    }

    // set packet data
    assert(((int) tr.sz_bytes)%8 == 0);
    int pkt_64bitdata_sz = tr.sz_bytes/8;
    p_pkt->m_packetData_vec.resize(pkt_64bitdata_sz);
    if (pkt_64bitdata_sz == 1) {
        // address only
        p_pkt->m_packetData_vec[0] = tr.addr_value;
#ifdef _DEBUG_TRACE_TILED_CMP_VALUE
printf("  addr[flit0]=%016llX\n", tr.addr_value);
#endif
    } else {
        // address + data block
        p_pkt->m_packetData_vec[0] = tr.addr_value;
#ifdef _DEBUG_TRACE_TILED_CMP_VALUE
printf("  addr[flit0]=%016llX\n", tr.addr_value);
#endif

        unsigned int data_value_pos = 0;
        for (int i=1; i<pkt_64bitdata_sz; i++) {
            unsigned long long data64bit = 0x0;
#ifdef _DEBUG_TRACE_TILED_CMP_VALUE
printf("  ");
#endif
            for (int j=0; j<8; j++) {
#ifdef _DEBUG_TRACE_TILED_CMP_VALUE
printf("%02X ", tr.data_value[data_value_pos]);
#endif
                assert(data_value_pos < MAX_PKT_TRACE_DATA_VALUE_SZ);
                data64bit |= tr.data_value[data_value_pos];

                if (j!=7)
                    data64bit <<= 8;

                ++data_value_pos;
            }

            p_pkt->m_packetData_vec[i] = data64bit;
#ifdef _DEBUG_TRACE_TILED_CMP_VALUE
printf("data[flit%d]=%016llX\n", i, data64bit);
#endif
        }
    }

    // spatial distribution
    m_spat_pattern_pkt_vec[src_tile_id][dest_tile_id]++;
    m_spat_pattern_flit_vec[src_tile_id][dest_tile_id] += num_flits;

    // throughput profile
    if (g_cfg.profile_perf) {
        g_sim.m_periodic_inj_pkt++;
        g_sim.m_periodic_inj_flit += num_flits;
    }

    m_num_proc_traces++;

    pkt_vec.push_back(p_pkt);
    return pkt_vec;
}

bool WorkloadTiledCMPValue::openTraceFile()
{
    char trace_path_name[256];
    sprintf(trace_path_name, "%s/%s/%s-tr.%04d", m_trace_dir_name.c_str(), m_benchmark_name.c_str(), m_benchmark_name.c_str(), m_trace_file_id);
// printf("openTraceFile(): trace_path_name='%s'\n", trace_path_name);

    // check the existence of file
    int fd = open(trace_path_name, O_RDONLY);
    if (fd == -1) {
        printf("no '%s' file.\n", trace_path_name);
        return false;
    }
    close(fd);
// printf("openTraceFile(): trace_path_name='%s' successfully open\n", trace_path_name);

    m_trace_fp = igzstream_open(trace_path_name);
    assert(m_trace_fp);

    m_trace_file_id++;

    return true;
}

void WorkloadTiledCMPValue::skipTraceFile()
{
    if (m_trace_skip_cycles == 0.0) // no skip ?
        return;

    PktTraceValue tr;
    int num_skipped_trace_files = 0;

    assert(m_trace_fp != 0);

    // check cycle of the first trace in each file
    while (1) {
        tr.cycle = 0;
        tr.sz_bytes = 0;
        igzstream_read(m_trace_fp, (char*) &tr, sizeof(PktTraceValue));
        assert(tr.cycle!=0 && tr.sz_bytes!=0);

        closeTraceFile();

#ifdef _DEBUG_TRACE_TILED_CMP_VALUE
        printf("DEBUG_TRACE_TILED_CMP_VALUE: tr.cycle=%lld, num_skipped_trace_files=%d\n", tr.cycle, num_skipped_trace_files);
#endif

        if (((double) tr.cycle) > m_trace_skip_cycles) {
            num_skipped_trace_files -= 1;
            break;
        }

         // open the next file
        if (openTraceFile())
            num_skipped_trace_files++;
        else
            break;
    }

    // set the trace file index to open
    m_trace_file_id = num_skipped_trace_files;
// printf("m_trace_file_id=%d\n", m_trace_file_id);

    // locate the proper trace position
    bool rc_open = openTraceFile();
    assert(rc_open == true);

    int num_lines = 0;
    while (1) {
        tr.cycle = 0;
        tr.sz_bytes = 0;
        igzstream_read(m_trace_fp, (char*) &tr, sizeof(PktTraceValue));
        if (tr.cycle==0 || tr.sz_bytes==0) {
            printf("skip_cycle=%0.lf is too big...\n", m_trace_skip_cycles);
            assert(0);
        }
        num_lines++;

        if (((double) tr.cycle) > m_trace_skip_cycles)
            break;
    }

#ifdef _DEBUG_TRACE_TILED_CMP_VALUE
    printf("DEBUG_TRACE_TILED_CMP_VALUE: positioned... file_id=%d num_lines=%d\n", m_trace_file_id-1, num_lines);
    printf("DEBUG_TRACE_TILED_CMP_VALUE:               m_trace_skip_cycles=%.0lf tr.cycle=%lld\n", m_trace_skip_cycles, tr.cycle);
#endif
}

void WorkloadTiledCMPValue::printTrace(ostream& out, const PktTraceValue & pkt)
{
    out << "cycle: " << pkt.cycle << " ";
    out << "src: [" << MachineType_to_string(pkt.src_mach_type) << ":" << pkt.src_mach_num << "] ";
    out << "dest:[" << MachineType_to_string(pkt.dest_mach_type) << ":" << pkt.dest_mach_num << "] ";
    out << "sz_bytes:" << (int) pkt.sz_bytes << " ";

    out << "addr: " << hex << "0x" << pkt.addr_value << " ";
    if (pkt.sz_bytes == (int) DATA_MESSAGE_SIZE) {
        out << "data: [";
        for (unsigned int i=0; i<MAX_PKT_TRACE_DATA_VALUE_SZ; i+=4) {
            out << hex << *((uint32*)(&(pkt.data_value[i]))) << " ";
        }
        out << "]";
    } else { 
        out << "data: NO-DATA";
    }
    out << endl;
    out << dec << flush;
}
