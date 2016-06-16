#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "noc.h"
#include "WorkloadTiledCMP.h"
#include "Core.h"
#include "Topology.h"
#include "gzlib.h"

// #define _DEBUG_TRACE_TILED_CMP

/////////////////////////////////////////////////
// memory controller (must be consistent with GEMS configuration)
const static int g_64p_TILED_Dir_num = 8;
static int m_64p_TILED_Dir_map[g_64p_TILED_Dir_num] = { 2, 5, 16, 23, 40, 47, 58, 61};
const static int g_16p_TILED_Dir_num = 8;
static int m_16p_TILED_Dir_map[g_16p_TILED_Dir_num] = { 1, 3, 4, 6, 9, 11, 12, 14};
/////////////////////////////////////////////////

WorkloadTiledCMP::WorkloadTiledCMP() : WorkloadTrace()
{
#ifdef MESI_SCMP_bankdirectory
    m_workload_name = "Cache Coherence (MESI directory) Trace for a Tiled CMP";
#elif MOESI_CMP_token
    m_workload_name = "Cache Coherence (MOESI token) Trace for a Tiled CMP";
#else
    assert(0);
#endif 

    m_containData = false;

    buildConfigStr();
}

WorkloadTiledCMP::~WorkloadTiledCMP()
{
}

void WorkloadTiledCMP::buildConfigStr()
{
    m_workload_config = "";
    m_workload_config += "  benchmark: ";
      m_workload_config += g_cfg.wkld_trace_benchmark_name + "\n";
    m_workload_config += "  start cycle: " + double2str(g_cfg.sim_clk_start, 0) + "\n";
    m_workload_config += "  skip cycle: " + double2str(g_cfg.wkld_trace_skip_cycles) + "\n";
    m_workload_config += "  skip instr: " +  longlong2str(g_cfg.wkld_trace_skip_instrs) + "\n";
    m_workload_config += "  end cycle: " + double2str(g_cfg.sim_clk_end) + "\n";
    m_workload_config += "  #tiles: " + int2str(g_cfg.core_num_tile_tiledCMP) + "\n";
    m_workload_config += "  #mem controllers: " + int2str(g_cfg.core_num_mem_tiledCMP) + "\n";
}

void WorkloadTiledCMP::printStats(ostream& out) const
{
}

void WorkloadTiledCMP::print(ostream& out) const
{
}

// return values: packet vector
// vector_size=0 : trace error
// vector_size=1 : unicast (one packet)
// vector_size=1 : null pointer (end of trace)
// vector_size>1 : multicast (multiple unicasting packets)
vector< Packet* > WorkloadTiledCMP::readTrace()
{
    PktTrace tr;
    vector< Packet* > pkt_vec;
    string err_str;

    // read one line trace
    assert(m_trace_fp > 0);

    tr.cycle = 0;
    tr.msg_sz_type = 0;
    igzstream_read(m_trace_fp, (char*) &tr, sizeof(PktTrace));
    if (tr.cycle==0 || tr.msg_sz_type==0 || igzstream_feof(m_trace_fp)) {
        // close the current file
        closeTraceFile();

        // open the next file
        if (! openTraceFile() )	{ // no trace file?
            pkt_vec.push_back(0);
            return pkt_vec;
        }
        fprintf(stderr, "benchmark=%s trace_file_id=%d successfully open.\n", m_benchmark_name.c_str(), m_trace_file_id-1);

        igzstream_read(m_trace_fp, (char*) &tr, sizeof(PktTrace));
        // assert(tr->cycle != 0);
    }
#ifdef _DEBUG_TRACE_TILED_CMP
    printTrace(cout, tr);
#endif

    // post processing
    int num_flits = (int) ceil( (MessageSizeType_to_int((MessageSizeType) tr.msg_sz_type) * BITS_IN_BYTE) / ( (double) g_cfg.link_width ));
    int src_tile_id = getTileID(tr.src_mach_type, tr.src_mach_num);
    int dest_tile_id = getTileID(tr.dest_mach_type, tr.dest_mach_num);
    if (src_tile_id == -1 || dest_tile_id == -1)
        goto TRACE_ERROR;
    // -- 03/22/08 new structure (for reconfig work)
    g_sim.m_num_instr_executed = tr.instr_executed;
#ifdef _DEBUG_TRACE_TILED_CMP
    printf("DEBUG_TILED_CMP: src_tile_id=%d dest_tile_id=%d #flits=%d cycle=%lld cur_cycle=%.0lf\n", src_tile_id, dest_tile_id, num_flits, tr.cycle, simtime());
#endif

    // NI pos
    int NI_in_pos, NI_out_pos;
    if (g_cfg.NI_port_mux) { // port multiplexing ?
        if (g_cfg.core_num_NIs == 1) {
            NI_in_pos = 0;
            NI_out_pos = 0;
        } else if (g_cfg.core_num_NIs == 2) {
            switch ((int) tr.src_mach_type) {
            case MachineType_L1Cache: NI_in_pos = 0; break;
            case MachineType_L2Cache: NI_in_pos = 1; break;
            case MachineType_Directory: NI_in_pos = 1; break;
            default: err_str = "src_mach_type is invalid"; goto TRACE_ERROR;
            }
            switch ((int) tr.dest_mach_type) {
            case MachineType_L1Cache: NI_out_pos = 0; break;
            case MachineType_L2Cache: NI_out_pos = 1; break;
            case MachineType_Directory: NI_out_pos = 1; break;
            default: err_str = "dest_mach_type is invalid"; goto TRACE_ERROR;
            }
        } else {
            assert(0);
        }
    } else {
        switch ((int) tr.src_mach_type) {
        case MachineType_L1Cache: NI_in_pos = 0; break;
        case MachineType_L2Cache: NI_in_pos = 1; break;
        case MachineType_Directory: NI_in_pos = 0; break;
        default: err_str = "src_mach_type is invalid"; goto TRACE_ERROR;
        }
        switch ((int) tr.dest_mach_type) {
        case MachineType_L1Cache: NI_out_pos = 0; break;
        case MachineType_L2Cache: NI_out_pos = 1; break;
        case MachineType_Directory: NI_out_pos = 0; break;
        default: err_str = "dest_mach_type is invalid"; goto TRACE_ERROR;
        }
    }

    // packet type
    unsigned int packet_type;
    switch (MessageSizeType_to_int((MessageSizeType) tr.msg_sz_type)) {
    case CONTROL_MESSAGE_SIZE:
        packet_type = PACKET_TYPE_UNICAST_SHORT;
        break;
    case DATA_MESSAGE_SIZE:
        packet_type = PACKET_TYPE_UNICAST_LONG;
        break;
    default:
        err_str = "tr.msg_sz_type is invalid";
        goto TRACE_ERROR;
    }

#ifdef _DEBUG_TRACE_TILED_CMP
    printf("DEBUG_TILED_CMP: NI_in_pos=%d NI_out_pos=%d\n", NI_in_pos, NI_out_pos);
#endif


    // make one packet
    if (tr.multicast) {	// multicast packet ?
        int num_multicast_dest = 0;
        for (int pos=0; pos<64; pos++) {
            unsigned long long mask = 0x1;
            mask <<= pos;
// printf("tr.multicast=%llX pos=%d\n", tr.multicast, pos);
            if (tr.multicast & mask) {	// set this pos as a destination ?
                dest_tile_id = getTileID(tr.dest_mach_type, pos);
                if (dest_tile_id == -1) {
                    err_str = "multicast: dest_tile_id is invalid";
                    goto TRACE_ERROR;
                }
                num_multicast_dest++;

                Packet* p_pkt = g_PacketPool.alloc();
                assert(p_pkt);
                p_pkt->setID(g_sim.m_next_pkt_id++);
                p_pkt->m_start_flit_id = g_sim.m_next_flit_id;
                p_pkt->m_num_flits = num_flits;
                g_sim.m_next_flit_id += num_flits;
                if (g_cfg.net_topology == TOPOLOGY_FAT_TREE || 
                    g_cfg.net_topology == TOPOLOGY_FLBFLY) {
                    pair< int, int > src_pair = g_Topology->core2router(src_tile_id);
                    int src_router_id = src_pair.first;
                    int src_port_pos = src_pair.second;

                    pair< int, int > dest_pair = g_Topology->core2router(dest_tile_id);
                    int dest_router_id = dest_pair.first;
                    int dest_port_pos = dest_pair.second;
                    p_pkt->setSrcRouterID(src_router_id);
                    p_pkt->addDestRouterID(dest_router_id);
                    NI_out_pos = dest_port_pos;
                } else {
                    p_pkt->setSrcRouterID(src_tile_id);
                    p_pkt->addDestRouterID(dest_tile_id);
                }
                p_pkt->setSrcCoreID(src_tile_id);
                p_pkt->addDestCoreID(dest_tile_id);
                p_pkt->m_clk_gen = (double) tr.cycle;
                p_pkt->m_NI_in_pos = NI_in_pos;
                p_pkt->m_NI_out_pos = NI_out_pos;
                p_pkt->m_packet_type = PACKET_TYPE_MULTICAST_SHORT;

                pkt_vec.push_back(p_pkt);

                // spatial distribution
                m_spat_pattern_pkt_vec[src_tile_id][dest_tile_id]++;
                m_spat_pattern_flit_vec[src_tile_id][dest_tile_id] += num_flits;

                if (g_cfg.profile_perf)
                    g_sim.m_periodic_inj_flit += num_flits;
// printf("pos=%d\n", pos);
            }
        } // for (int pos=0; pos<64; pos++) {
// printf("multicast: %d\n", num_multicast_dest);

        g_sim.m_num_pkt_inj_multicast++;
        g_sim.m_sum_multicast_dest += num_multicast_dest;
    } else { // unicast packet
        Packet* p_pkt = g_PacketPool.alloc();
        assert(p_pkt);
        p_pkt->setID(g_sim.m_next_pkt_id++);
        p_pkt->m_start_flit_id = g_sim.m_next_flit_id;
        p_pkt->m_num_flits = num_flits;
        g_sim.m_next_flit_id += num_flits;
        if (g_cfg.net_topology == TOPOLOGY_FAT_TREE || 
            g_cfg.net_topology == TOPOLOGY_FLBFLY) {

            pair< int, int > src_pair = g_Topology->core2router(src_tile_id);
            int src_router_id = src_pair.first;
            int src_port_pos = src_pair.second;

            pair< int, int > dest_pair = g_Topology->core2router(dest_tile_id);
            int dest_router_id = dest_pair.first;
            int dest_port_pos = dest_pair.second;

            p_pkt->setSrcRouterID(src_router_id);
            p_pkt->addDestRouterID(dest_router_id);
// printf("src(tile=%d router=%d) ", src_tile_id, g_Core_vec[src_tile_id]->m_attached_router_id);
// printf("dst(tile=%d router=%d)\n", dest_tile_id, g_Core_vec[dest_tile_id]->m_attached_router_id);
            NI_out_pos = dest_port_pos;
        } else {
            p_pkt->setSrcRouterID(src_tile_id);
            p_pkt->addDestRouterID(dest_tile_id);
        }
        p_pkt->setSrcCoreID(src_tile_id);
        p_pkt->addDestCoreID(dest_tile_id);
        p_pkt->m_clk_gen = (double) tr.cycle;
        p_pkt->m_NI_in_pos = NI_in_pos;
        p_pkt->m_NI_out_pos = NI_out_pos;
        p_pkt->m_packet_type = packet_type;

        pkt_vec.push_back(p_pkt);

        // spatial distribution
        m_spat_pattern_pkt_vec[src_tile_id][dest_tile_id]++;
        m_spat_pattern_flit_vec[src_tile_id][dest_tile_id] += num_flits;

        if (g_cfg.profile_perf)
            g_sim.m_periodic_inj_flit += num_flits;
    }

    if (g_cfg.profile_perf)
        g_sim.m_periodic_inj_pkt++;

    m_num_proc_traces++;

    return pkt_vec;

TRACE_ERROR:

    cerr << err_str << endl;
    printTrace(cerr, tr);

    pkt_vec.clear();

    m_num_unproc_traces++;

    return pkt_vec;
}

bool WorkloadTiledCMP::openTraceFile()
{
    char trace_path_name[256];
    sprintf(trace_path_name, "%s/%s.%04d", m_trace_dir_name.c_str(), m_benchmark_name.c_str(), m_trace_file_id);
// printf("openTraceFile(): trace_path_name='%s'\n", trace_path_name);

    // check the existence of file
    int fd = open(trace_path_name, O_RDONLY);
    if (fd == -1) {
        printf("no '%s' file.\n", trace_path_name);
        return false;
    }
    close(fd);
//    fprintf(stderr, "trace='%s' successfully open.\n", trace_path_name);

    m_trace_fp = igzstream_open(trace_path_name);
    assert(m_trace_fp);

    m_trace_file_id++;

    return true;
}

void WorkloadTiledCMP::closeTraceFile()
{
    if (m_trace_fp) {
        igzstream_close(m_trace_fp);
        m_trace_fp = 0;
    }
}

void WorkloadTiledCMP::skipTraceFile()
{
    if (m_trace_skip_cycles == 0.0) // no skip ?
        return;

    PktTrace tr;
    int num_skipped_trace_files = 0;

    assert(m_trace_fp != 0);

    // look at cycle of the first trace in each file
    while (1) {
        tr.cycle = 0;
        tr.msg_sz_type = 0;
        igzstream_read(m_trace_fp, (char*) &tr, sizeof(PktTrace));
        assert(tr.cycle!=0 && tr.msg_sz_type!=0);

        closeTraceFile();

        if (((double) tr.cycle) > m_trace_skip_cycles) {
            num_skipped_trace_files--;
            break;
        }

        // open the next file
        if (openTraceFile())
            num_skipped_trace_files++;
        else
            break;

#ifdef _DEBUG_TRACE_TILED_CMP
        printf("DEBUG_TRACE_TILED_CMP: skipping... tr.cycle=%lld tr.instr=%lld num_skipped_files=%d\n", tr.cycle, tr.instr_executed, num_skipped_trace_files);
#endif
    }

#ifdef _DEBUG_TRACE_TILED_CMP
        printf("DEBUG_TRACE_TILED_CMP: inspecting... file_id=%d\n", num_skipped_trace_files);
#endif

    // set the trace file index to open
    m_trace_file_id = num_skipped_trace_files;

    // locate the proper trace position
    bool rc_open = openTraceFile();
    assert(rc_open == true);

    int num_lines = 0;
    while (1) {
        tr.cycle = 0;
        tr.msg_sz_type = 0;
        igzstream_read(m_trace_fp, (char*) &tr, sizeof(PktTrace));
        if (tr.cycle==0 || ((int) tr.msg_sz_type) ==0) {
            fprintf(stderr, "skip_cycle=%0.lf is too big...\n", m_trace_skip_cycles);
            assert(0);
        }
        num_lines++;

        if (((double) tr.cycle) > m_trace_skip_cycles)
            break;
    }

    g_sim.m_num_instr_executed = tr.instr_executed;

#ifdef _DEBUG_TRACE_TILED_CMP
    printf("DEBUG_TRACE_TILED_CMP: positioned... file_id=%d num_lines=%d\n", m_trace_file_id-1, num_lines);
    printf("DEBUG_TRACE_TILED_CMP:               m_trace_skip_cycles=%.0lf tr.cycle=%lld\n", m_trace_skip_cycles, tr.cycle);
#endif
}

void WorkloadTiledCMP::printTrace(ostream& out, const PktTrace & pkt)
{
    out << "cycle: " << pkt.cycle << " ";
    out << "instr: " << pkt.instr_executed << " ";
    out << "src: [" << MachineType_to_string(pkt.src_mach_type) << ":" << pkt.src_mach_num << "] ";
    out << "dest:[" << MachineType_to_string(pkt.dest_mach_type) << ":" << pkt.dest_mach_num << "] ";
    out << "msg_sz_type:" << MessageSizeType_to_string((MessageSizeType) pkt.msg_sz_type) << " ";
    if (pkt.multicast)
        out << "mltc:" << hex << pkt.multicast << " ";

#ifdef MESI_SCMP_bankdirectory
    if (pkt.msg_format == 0) {
        out << "Cac-";
    } else if (pkt.msg_format == 1) {
        out << "Req-";
        switch (pkt.coh_type) {
        case CoherenceRequestType_GETX: out << "GETX "; break;
        case CoherenceRequestType_UPGRADE: out << "UPGRADE "; break;
        case CoherenceRequestType_GETS: out << "GETS "; break;
        case CoherenceRequestType_GET_INSTR: out << "GET_I "; break;
        case CoherenceRequestType_INV: out << "INV "; break;
        case CoherenceRequestType_PUTX: out << "PUTX "; break;
        default: assert(0);
        }
    } else if (pkt.msg_format == 2) {
        out << "Res-";
        switch (pkt.coh_type) {
        case CoherenceResponseType_MEMORY_ACK: out << "MEM_ACK "; break;
        case CoherenceResponseType_DATA: out << "DATA "; break;
        case CoherenceResponseType_DATA_EXCLUSIVE: out << "DATA_EX "; break;
        case CoherenceResponseType_MEMORY_DATA: out << "MEM_DATA "; break;
        case CoherenceResponseType_ACK: out << "ACK "; break;
        case CoherenceResponseType_WB_ACK: out << "WB_ACK "; break;
        case CoherenceResponseType_UNBLOCK: out << "UNBLOCK "; break;
        case CoherenceResponseType_EXCLUSIVE_UNBLOCK: out << "EX_UNBLOCK "; break;
        default: assert(0);
        }
    } else {
        assert(0);
    }
#endif // #ifdef MESI_SCMP_bankdirectory

    out << endl;
    out << dec << flush;
}

int WorkloadTiledCMP::getTileID(int mach_type, int mach_num)
{
    switch (mach_type) {
    case MachineType_L1Cache:
    case MachineType_L2Cache:
        if (mach_num >= g_cfg.core_num_tile_tiledCMP)
            goto TILE_ID_ERROR;
        return mach_num;
    case MachineType_Directory:
        switch (g_cfg.core_num_tile_tiledCMP)
        case 16:
            if (mach_num >= g_16p_TILED_Dir_num)
                goto TILE_ID_ERROR;
            return m_16p_TILED_Dir_map[mach_num];
        case 64:
            if (mach_num >= g_64p_TILED_Dir_num)
                goto TILE_ID_ERROR;
            return m_64p_TILED_Dir_map[mach_num];
        default:
            assert(0);
    }

TILE_ID_ERROR:
    return -1;
}

// Each tile has a private L1$ and a slice of shared L2$.
// Memory controllers are attached to some tiles.
// if g_cfg.core_num_tile_tiledCMP == g_cfg.core_num_mem_tiledCMP, each tile has a memory controller.
void config_tiledCMP_network()
{
    assert(g_cfg.core_num_tile_tiledCMP >= g_cfg.core_num_mem_tiledCMP);

    // simulation termination condition
    // end_clk is set when reading a trace file is done.
    g_cfg.sim_end_cond = SIM_END_BY_CYCLE;

    // Assume that a mesh network
    g_cfg.router_num_pc = 4 + g_cfg.core_num_NIs;

    // assume that #routers are the same as #tiles.
    // FIXME: This only supports mesh-like topologies
    g_cfg.router_num = g_cfg.core_num_tile_tiledCMP;
    // g_cfg.core_num = g_cfg.core_num_tile_tiledCMP*2 + g_cfg.core_num_mem_tiledCMP;
    g_cfg.core_num = g_cfg.core_num_tile_tiledCMP;

    // link length: 20mmx20mm chip area assumption
    g_cfg.link_length = 20.0/sqrt((double) g_cfg.core_num_tile_tiledCMP);
}
