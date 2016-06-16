#include "noc.h"
#include "main.h"
#include "Core.h"
#include "NIInput.h"
#include "NIOutput.h"
#include "Router.h"
#include "VCArb.h"
#include "SwArb.h"
#include "RouterPower.h"
#include "RouterPowerStats.h"
#include "profile.h"
#include "sim_process.h"
#include "Topology2DMesh.h"
#include "Topology2DTorus.h"
#include "TopologyFatTree.h"
#include "TopologyDMesh.h"
#include "TopologyHMesh.h"
#include "TopologyFlbfly.h"
#include "TopologySNUCA.h"
#include "RoutingMeshStatic.h"
#include "RoutingMeshMinAdaptive.h"
#include "RoutingFatTree.h"
#include "RoutingTFatTree.h"
#include "RoutingTorusStatic.h"
#include "RoutingDMeshStatic.h"
#include "RoutingHMeshStatic.h"
#include "RoutingFlbfly.h"
#include "WorkloadSynthetic.h"
#include "WorkloadSyntheticMatrix.h"
#include "WorkloadTRIPS.h"
#include "WorkloadTiledCMP.h"
#include "WorkloadTiledCMPValue.h"
#include "WorkloadSNUCAValue.h"
#ifdef LINK_DVS
  #include "LinkDVSer.h"
#endif
#include "CAMManager.h"

//////////////////////////////////////////////////////////////////
// global variables
event* g_ev_sim_done = 0;
bool g_EOS = false;

Config g_cfg;
SimOut g_sim;

Routing* g_Routing = 0;
Topology* g_Topology = 0;
vector< Router* > g_Router_vec;
vector< NIInput* > g_NIInput_vec;
vector< NIOutput* > g_NIOutput_vec;
vector< Core*> g_Core_vec;
Workload* g_Workload = 0;
Pool< Packet > g_PacketPool(1024*2, "packet");
Pool< FlitHead > g_FlitHeadPool(1024*2, "head_flit");
Pool< FlitMidl > g_FlitMidlPool(1024*4, "midl_flit");
Pool< FlitTail > g_FlitTailPool(1024*2, "tail_flit");
Pool< FlitAtom > g_FlitAtomPool(1024, "atom_flit");
Pool< Credit > g_CreditPool(1024*4, "credit");
#ifdef LINK_DVS
LinkDVSer g_LinkDVSer;
#endif // #ifdef LINK_DVS
CAMManager* g_CamManager = 0;
//////////////////////////////////////////////////////////////////

extern "C" void sim(int, char *[]);

#if 0
#define NARS 5000
#define IAR_TM 2.0
#define SRV_TM 1.0

event done("done");                     // the event named done
facility f("facility");                 // the facility named f
table tbl("resp tms");                  // table of response times
qhistogram qtbl("num in sys", 10l);     // qhistogram of number in system
int cnt;                                // count of remaining processes

void customer();
void theory();

extern "C" void sim(int, char **);

void sim(int argc, char *argv[])
{
        set_model_name("M/M/1 Queue");
        create("sim");
        cnt = NARS;
        for(int i = 1; i <= NARS; i++) {
                hold(expntl(IAR_TM));   // interarrival interval
                customer();             // generate next customer
                }
        done.wait();                    // wait for last customer to depart
        report();                       // model report
        theory();
        mdlstat();                      // model statistics
}

void customer()                         // arriving customer
{
        float t1;

        create("cust");
        t1 = clock;                     // record start time
        qtbl.note_entry();              // note arrival
        f.reserve();                    // reserve facility
                hold(expntl(SRV_TM));   // service interval
        f.release();                    // release facility
        tbl.record(clock - t1);         // record response time
        qtbl.note_exit();               // note departure
        if(--cnt == 0)
                done.set();             // if last customer, set done
}

void theory()                           // print theoretical results
{
        float rho, nbar, rtime, tput;

        printf("\n\n\n\t\t\tM/M/1 Theoretical Results\n");

        tput = 1.0/IAR_TM;
        rho = tput*SRV_TM;
        nbar = rho/(1.0 - rho);
        rtime = SRV_TM/(1.0 - rho);

        printf("\n\n");
        printf("\t\tInter-arrival time = %10.3f\n",IAR_TM);
        printf("\t\tService time       = %10.3f\n",SRV_TM);
        printf("\t\tUtilization        = %10.3f\n",rho);
        printf("\t\tThroughput rate    = %10.3f\n",tput);
        printf("\t\tMn nbr at queue    = %10.3f\n",nbar);
        printf("\t\tMn queue length    = %10.3f\n",nbar-rho);
        printf("\t\tResponse time      = %10.3f\n",rtime);
        printf("\t\tTime in queue      = %10.3f\n",rtime - SRV_TM);
}
#endif

void sim(int argc, char **argv)
{
    // This is the main function.
    set_model_name("NoC");

    // change CSIM maximum resources
    max_mailboxes(5000000L);
    max_processes(5000000L);
    max_events(50000000L);
    max_messages(50000000L);
    max_tables(50000000L);

    // configure simulator
    config_sim();

    // get configuration parameters from command line
    parse_options(argc, argv);

    // assign computable parameters from the given parameters
    assert(g_cfg.link_width > 0 && g_cfg.link_width % BITS_IN_BYTE == 0);
    g_cfg.flit_sz_byte = g_cfg.link_width/BITS_IN_BYTE;
    g_cfg.flit_sz_64bit_multiple = (int) ceil(g_cfg.flit_sz_byte / 8.0);
    assert(g_cfg.flit_sz_64bit_multiple > 0);
    // if (g_cfg.router_lookahead_RC)
    //    g_cfg.router_num_pipelines -= 1;

    // workload-customized configuration
    switch (g_cfg.wkld_type) {
    case WORKLOAD_TRIPS_TRACE:
        ::config_TRIPS_network();
        break;
    case WORKLOAD_TILED_CMP_TRACE:
        // FIXME: too much customized!!!
        if (g_cfg.core_num_tile_tiledCMP == 16) {
            g_cfg.NI_port_mux = false;
        } else {
            g_cfg.NI_port_mux = true;
        }
        g_cfg.profile_interval_cycle = false;	// profile for instrs
        // g_cfg.sim_clk_end = 1.0 * UNIT_TERA;
        ::config_tiledCMP_network();
        break;
    case WORKLOAD_TILED_CMP_VALUE_TRACE:
        g_cfg.NI_port_mux = true;
        g_cfg.cam_data_num_interleaved_tab = 8;
        g_cfg.core_num_NIs = 2;
        g_cfg.link_power_model = LINK_POWER_MODEL_DELAY_OPT_REPEATED_VALUE;
        g_cfg.core_num_tile_tiledCMP = 16;
        g_cfg.core_num_mem_tiledCMP = 16;
        ::config_tiledCMP_network();
        break;
    case WORKLOAD_SNUCA_CMP_VALUE_TRACE:
        g_cfg.cam_data_num_interleaved_tab = 16;
        g_cfg.link_power_model = LINK_POWER_MODEL_DELAY_OPT_REPEATED_VALUE;
        ::config_snucaCMP_network();
        break;
    default:
        if (g_cfg.core_num == -1)
            g_cfg.core_num = g_cfg.router_num;
    }

    // open output file
    if (g_cfg.out_file_name != "") {
        g_cfg.out_file_fp = fopen(g_cfg.out_file_name.c_str(), "w+t");
        assert(g_cfg.out_file_fp);
    }
    // open performance profile file
    if (g_cfg.profile_perf) {
        g_cfg.profile_perf_fp = fopen(g_cfg.profile_perf_file_name.c_str(), "w+t");
        assert(g_cfg.profile_perf_fp);
    }
    // open power profile file
    if (g_cfg.profile_power_file_name != "") {
        g_cfg.profile_power_fp = fopen(g_cfg.profile_power_file_name.c_str(), "w+t");
        assert(g_cfg.profile_power_fp);
    }

#ifdef ORION_MODEL
    // orion
    if (g_cfg.chip_tech_nano == 65) {
        // 70 nm in Orion
        orion::SCALE_T = 0.5489156157;
        orion::SCALE_M = 0.6566502462;
        orion::SCALE_S = 1.4088071075;
        orion::PARM_Freq = g_cfg.chip_freq;
    } else if (g_cfg.chip_tech_nano == 45) {
        // 50 nm in Orion
        orion::SCALE_T = 0.3251012552;
        orion::SCALE_M = 0.4426460239;
        orion::SCALE_S = 2.8667111607;
        orion::PARM_Freq = g_cfg.chip_freq;
    } else if (g_cfg.chip_tech_nano == 32) {
        // 35 nm in Orion
        orion::SCALE_T = 0.2016627474;
        orion::SCALE_M = 0.2489788586;
        orion::SCALE_S = 8.7726826878;
        orion::PARM_Freq = g_cfg.chip_freq;
    } else {
        // 100 nm in Orion
        orion::SCALE_T = 1.0;
        orion::SCALE_M = 1.0;
        orion::SCALE_S = 1.0;
        orion::PARM_Freq = g_cfg.chip_freq;
    }
#endif

    // choose topology
    g_Topology = 0;
    switch (g_cfg.net_topology) {
    case TOPOLOGY_MESH:	g_Topology = new Topology2DMesh(); break;
    case TOPOLOGY_TORUS: g_Topology = new Topology2DTorus(); break;
    case TOPOLOGY_FAT_TREE: g_Topology = new TopologyFatTree(); break;
    case TOPOLOGY_DMESH: g_Topology = new TopologyDMesh(); break;
    case TOPOLOGY_HMESH: g_Topology = new TopologyHMesh(); break;
    case TOPOLOGY_FLBFLY: g_Topology = new TopologyFlbfly(); break;
    case TOPOLOGY_SNUCA: g_Topology = new TopologySNUCA(); break;
    default: assert(0);
    }
    // build link connection between routers in Topology
    g_Topology->buildTopology(); fflush(stdout);

    // choose routing algorithm
    g_Routing = 0;
    switch (g_cfg.net_routing) {
    case ROUTING_XY:
    case ROUTING_YX:
        assert(g_cfg.net_topology == TOPOLOGY_MESH || g_cfg.net_topology == TOPOLOGY_SNUCA);
        g_Routing = new RoutingMeshStatic();
        break;

    case ROUTING_MIN_OBLIVIOUS:
    case ROUTING_MIN_ADAPTIVE_VC:
        assert(g_cfg.net_topology == TOPOLOGY_MESH || g_cfg.net_topology == TOPOLOGY_SNUCA);
        g_Routing = new RoutingMeshMinAdaptive();
        break;

    case ROUTING_TORUS_DALLY:
        assert(g_cfg.router_num_vc%2 == 0);	// #VC must be even.
        assert(g_cfg.net_topology == TOPOLOGY_TORUS);
        g_Routing = new RoutingTorusStatic();
        break;

    case ROUTING_FAT_TREE_ADAPTIVE:
    case ROUTING_FAT_TREE_RANDOM:
        assert(g_cfg.net_topology == TOPOLOGY_FAT_TREE);
        g_Routing = new RoutingFatTree();
        break;

    case ROUTING_DMESH_XY:
    case ROUTING_DMESH_YX:
        assert(g_cfg.net_topology == TOPOLOGY_DMESH);
        g_Routing = new RoutingDMeshStatic();
        break;

    case ROUTING_HMESH_ASCENT:
    case ROUTING_HMESH_DESCENT:
        assert(g_cfg.net_topology == TOPOLOGY_HMESH);
        g_Routing = new RoutingHMeshStatic();
        break;

    case ROUTING_FLBFLY_XY:
    case ROUTING_FLBFLY_YX:
        assert(g_cfg.net_topology == TOPOLOGY_FLBFLY);
        g_Routing = new RoutingFlbfly();
        break;

    default:
        assert(0);
    }
    // set topology for the routing algorithm 
    g_Routing->setTopology(g_Topology);

    // choose workload
    switch (g_cfg.wkld_type) {
    case WORKLOAD_SYNTH_SPATIAL:
        g_Workload = new WorkloadSynthetic();
        break;
    case WORKLOAD_SYNTH_TRAFFIC_MATRIX:
        g_Workload = new WorkloadSyntheticMatrix();
        break;
    case WORKLOAD_TRIPS_TRACE:
        g_Workload = new WorkloadTRIPS();
        break;
    case WORKLOAD_TILED_CMP_TRACE:
        g_Workload = new WorkloadTiledCMP();
        break;
    case WORKLOAD_TILED_CMP_VALUE_TRACE:
        g_Workload = new WorkloadTiledCMPValue();
        break;
    case WORKLOAD_SNUCA_CMP_VALUE_TRACE:
        g_Workload = new WorkloadSNUCAValue();
        break;
    default:
        assert(0);
    }
    // open the first (or N-th positioned after skipping lines) trace file,
    // if g_Workload is instantitated from WorkloadTrace class.
    if (! g_Workload->isSynthetic())
        assert(((WorkloadTrace*) g_Workload)->openTraceFile());

    // data compression
    g_CamManager = 0;
    if (g_cfg.cam_data_enable || g_cfg.cam_addr_enable) {
        assert(g_Workload->containData());
        g_CamManager = new CAMManager(g_cfg.cam_data_manage);
        if (g_cfg.flit_sz_byte <= g_cfg.cam_data_blk_byte)
            g_cfg.cam_data_num_interleaved_tab = 1;
    }

#ifdef LINK_DVS
    assert(g_cfg.link_dvs_interval > g_cfg.link_dvs_voltage_transit_delay + g_cfg.link_dvs_freq_transit_delay) ;
    g_LinkDVSer.init();
#endif

    // precompute bitcount for 16-bit
    if (g_cfg.link_power_model == LINK_POWER_MODEL_DELAY_OPT_REPEATED_VALUE)
        precompute_bitcount_16bit();

    // attach power models
    // NOTE: This job should be performed after topology setup,
    //       because topology can change router parameters.
    for (unsigned int n=0; n<g_Router_vec.size(); n++)
        g_Router_vec[n]->attachPowerModel();

    // simulation end event
    g_ev_sim_done = new event("ev-sim-done");
  
    // initialize all measurement variables
    g_sim.init();

    // init packet/flit/credit pools
    assert(g_cfg.router_num <= MAX_PACKET_NUM_DEST);
    assert(g_cfg.core_num <= MAX_PACKET_NUM_DEST);
    g_PacketPool.init();
    g_FlitHeadPool.init();
    g_FlitMidlPool.init();
    g_FlitTailPool.init();
    g_FlitAtomPool.init();
    g_CreditPool.init();

    // print configuration
    print_config(g_cfg.out_file_fp);

    create("ABCD");
    // run simulation
    process_main();

    // print simulation result
    print_stats(g_cfg.out_file_fp);

    // close files
    if (g_cfg.out_file_fp != stdout) fclose(g_cfg.out_file_fp);
    if (g_cfg.profile_perf_fp != stdout) fclose(g_cfg.profile_perf_fp);
    if (g_cfg.profile_power_fp != stdout) fclose(g_cfg.profile_power_fp);

    exit(0);
}

void config_sim()
{
    // core
    g_cfg.core_num = 16;
    g_cfg.core_num_NIs = 1;
    g_cfg.core_num_tile_tiledCMP = 64; // #tiles for tiled CMP
    g_cfg.core_num_mem_tiledCMP = 8;  // #memory controlers for tiled CMP

    // network
    g_cfg.net_routing = ROUTING_XY;
    g_cfg.net_topology = TOPOLOGY_MESH;
    g_cfg.net_fattree_way = 4;
    g_cfg.net_networks = 1;

    // routers
    g_cfg.router_num = 16;

    // workload: traffic pattern & packet size
    g_cfg.wkld_type = WORKLOAD_SYNTH_SPATIAL;
    g_cfg.wkld_synth_spatial = WORKLOAD_SYNTH_SP_UR;
    g_cfg.wkld_synth_ss = false;
    // (injection load) = (#injection flits) / cycle / node
    g_cfg.wkld_synth_load = 0.1;
    // 
    // maximum injection load for k x k mesh network:
    //  If a network has bisection bandwidth B bits/s,
    //  each node in an N-node network can inject 2B/N bits/s
    //  at the maximum load.
    // bisection bandwidth: B = k
    // number of nodes: N = k^2
    // maximum injection load(#flits/cycle/node): = 2B/N = 2/k
    // 
    g_cfg.wkld_synth_num_flits_pkt = 4;		// #flits per packet
    g_cfg.wkld_synth_matrix_filename = "load.matrix";
    g_cfg.wkld_synth_bimodal = false;
    g_cfg.wkld_synth_bimodal_single_pkt_rate = 0.5;
    g_cfg.wkld_synth_multicast_ratio = 0.0;
    g_cfg.wkld_synth_multicast_destnum = 0;

    // router
    g_cfg.router_inbuf_depth = 4;
    g_cfg.router_num_pc = 5;
    g_cfg.router_num_vc = 2;
    g_cfg.router_spec_VA = false;
    g_cfg.router_spec_SA = true;
    g_cfg.router_lookahead_RC = true;
    g_cfg.router_bypass_IB = true;
    g_cfg.router_extra_pipeline_depth = 0;
    g_cfg.router_sa_v1_type = SW_ALLOC_RR;
    g_cfg.router_sa_p1_type = SW_ALLOC_RR;
    g_cfg.router_power_model = ROUTER_POWER_MODEL_STATS;
    g_cfg.router_buffer_type = ROUTER_BUFFER_SAMQ;
    g_cfg.router_num_rsv_credit = 0;
    g_cfg.router_num_pipelines = 2;	// minimum router pipelines
    g_cfg.router_tunnel = false;

    // input NI
    // NOTE: NI_INPUT_TYPE_PER_VC achieves better throughput than NI_INPUT_TYPE_PER_PC,
    //       because it better utilizes input buffers for ingress channels.
    //       This works well particularly for the large number of VCs.
    g_cfg.NIin_pktbuf_depth = 16;
    g_cfg.NIin_type = NI_INPUT_TYPE_PER_VC;
    // input & output NI
    g_cfg.NI_port_mux = false;

    // link
    g_cfg.link_length = 2.5;	// 2.5mm for 8x8 mesh and 20x20mm^2 chip
    g_cfg.link_width = 128;
    g_cfg.link_latency = 1;
    g_cfg.link_voltage = 1.0;	// 1.0V
    g_cfg.link_power_model = LINK_POWER_MODEL_DELAY_OPT_REPEATED;
    g_cfg.link_wire_type = LINK_WIRE_TYPE_GLOBAL;
    // g_cfg.link_wire_type = LINK_WIRE_TYPE_INTER;
    g_cfg.link_wire_pipelining = true;

    // off-chip memory
    g_cfg.mem_access_latency = 260;
    g_cfg.mem_access_pipelined_latency = 16;

    // chip
    g_cfg.chip_tech_nano = 45;	// 45nm
    g_cfg.chip_freq = 4.0 * UNIT_GIGA;
    g_cfg.chip_voltage = 1.0;

    // power activity factor
    g_cfg.power_router_avf = 1.0;
    g_cfg.power_link_avf = 0.15;	// IEEE TED 2004 Vol.51 No.2 Banerjee

    // simulation termination by #injected packets
#if 0
    g_cfg.sim_num_inj_pkt = 1;
    g_cfg.sim_num_ejt_pkt_4warmup = 0;
    g_cfg.sim_end_cond = SIM_END_BY_INJ_PKT;
#endif

    // simulation termination by #cycles
    g_cfg.sim_num_inj_pkt = 6000000;
    g_cfg.sim_clk_start = 100.0 * UNIT_KILO;
    g_cfg.sim_clk_end = 1.0 * UNIT_MEGA;
    g_cfg.sim_end_cond = SIM_END_BY_CYCLE;

    // simultaion progress
    g_cfg.sim_show_progress = true;
    g_cfg.sim_progress_interval = 50000;

    // profile interval
    g_cfg.profile_interval_cycle = true;
    g_cfg.profile_interval = 100.0 * UNIT_KILO;

    // performance profile
    g_cfg.profile_perf = false;
    g_cfg.profile_perf_file_name = "";
    g_cfg.profile_perf_fp = stdout;

    // power profile
    g_cfg.profile_power = false;
    g_cfg.profile_power_file_name = "";
    g_cfg.profile_power_fp = stdout;

    // simulation result file
    g_cfg.out_file_name = "";
    g_cfg.out_file_fp = stdout;

    // trace workload
    g_cfg.wkld_trace_dir_name = "";
    g_cfg.wkld_trace_benchmark_name = "";
    g_cfg.wkld_trace_skip_cycles = 0.0;
    g_cfg.wkld_trace_skip_instrs = 0;

#ifdef LINK_DVS
    // FIXME: supports only 1GHz
    g_cfg.chip_freq = 1.0 * UNIT_GIGA;

    g_cfg.link_dvs_method = LINK_DVS_NODVS;
    g_cfg.link_dvs_interval = UNIT_MEGA;
    g_cfg.link_dvs_voltage_transit_delay = 100.0;
    g_cfg.link_dvs_freq_transit_delay = 20.0;
#endif

    g_cfg.cam_data_manage = CAM_MT_PRIVATE_PER_ROUTER;
    g_cfg.cam_repl_policy = CAM_REPL_LRU;
    g_cfg.cam_LFU_saturation = 16;
    g_cfg.cam_data_enable = false;
    g_cfg.cam_data_blk_byte = 8;
    g_cfg.cam_data_num_interleaved_tab = 1;
    g_cfg.cam_data_en_latency = 1;
    g_cfg.cam_data_en_num_sets = 4;
    g_cfg.cam_data_de_latency = 1;
    g_cfg.cam_data_de_num_sets = 4;
    g_cfg.cam_addr_enable = false;

    g_cfg.cam_VLB_num_sets = 16;
    g_cfg.cam_VLB_LFU_saturation = 4;

    g_cfg.cam_streamline = false;

    g_cfg.cam_dyn_control = false;
    g_cfg.cam_dyn_control_window = 10;
}

void reset_stats()
{
    g_sim.resetStats();

    for (unsigned int r=0; r<g_Router_vec.size(); r++)
        g_Router_vec[r]->resetStats();

    for (unsigned int n=0; n<g_NIInput_vec.size(); n++)
        g_NIInput_vec[n]->resetStats();
    for (unsigned int n=0; n<g_NIOutput_vec.size(); n++)
        g_NIOutput_vec[n]->resetStats();

    g_Workload->resetStats();
}

void print_config(FILE* stat_fp)
{
    fprintf(stat_fp, "------------ configuration (BEGIN) ----------\n");
    fprintf(stat_fp, "Core:\n");
    fprintf(stat_fp, "  #cores: %d\n", g_cfg.core_num);
    fprintf(stat_fp, "  #attached NIs: %d\n", g_cfg.core_num_NIs);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "Network:\n");
    fprintf(stat_fp, "  #routers: %d\n", g_cfg.router_num);
    fprintf(stat_fp, "  topology: %s\n", g_Topology->getName().c_str());
    fprintf(stat_fp, "  routing: %s\n", g_Routing->getName().c_str());
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "Workload:\n");
    fprintf(stat_fp, "  flit size(bytes): %d\n", g_cfg.flit_sz_byte);
    fprintf(stat_fp, "  type: %s\n", g_Workload->getName().c_str());
    fprintf(stat_fp, "%s", g_Workload->getConfig().c_str());
    fprintf(stat_fp, "\n");

    // analyze how many m x n routers
    map< pair< int, int >, int > router_org_map;
    for (unsigned int i=0; i<g_Router_vec.size(); i++) {
        int num_pc = g_Router_vec[i]->num_pc();
        router_org_map[make_pair(num_pc, num_pc)]++;
    }

    // analyze how many different lengths / delays among links
    int num_total_links = 0;
    map< double, int > link_length_org_map;
    map< int, int > link_delay_org_map;
    for (unsigned int i=0; i<g_Router_vec.size(); i++) {
        for (int pc=0; pc<g_Router_vec[i]->num_pc(); pc++) {
            Link& link = g_Router_vec[i]->getLink(pc);
            if (link.m_valid) {
                link_length_org_map[link.m_length_mm]++;
                link_delay_org_map[link.m_delay_factor]++;
                num_total_links++;
            }
        }
    }

    fprintf(stat_fp, "Router:\n");
    fprintf(stat_fp, "  input buffer depth(#flits): %d\n", g_cfg.router_inbuf_depth);
    fprintf(stat_fp, "  #PCs: %d\n", g_cfg.router_num_pc);
    fprintf(stat_fp, "  #VCs per PC: %d\n", g_cfg.router_num_vc);
    fprintf(stat_fp, "  speculative switch allocation: %s\n", g_cfg.router_spec_SA ? "Yes" : "No");
    fprintf(stat_fp, "  lookahead routing: %s\n", g_cfg.router_lookahead_RC ? "Yes" : "No");
    fprintf(stat_fp, "  input buffer bypass: %s\n", g_cfg.router_bypass_IB ? "Yes" : "No");
    fprintf(stat_fp, "  extra pipeline depth: %d\n", g_cfg.router_extra_pipeline_depth);
    fprintf(stat_fp, "  switch allocator V:1=%s P:1=%s\n", get_sa_type_name(g_cfg.router_sa_v1_type), get_sa_type_name(g_cfg.router_sa_p1_type));
    fprintf(stat_fp, "  buffer type: %s\n", get_router_buffer_type_name(g_cfg.router_buffer_type));
    fprintf(stat_fp, "  tunneling: %s type=%d\n", g_cfg.router_tunnel ? "Yes" : "No", g_cfg.router_tunnel_type);
    fprintf(stat_fp, "  per-VC #reserved credits: %d\n", g_cfg.router_num_rsv_credit);
    fprintf(stat_fp, "  structure: ");
    for (map< pair< int, int >, int >::iterator pos = router_org_map.begin();
         pos != router_org_map.end(); ++pos) {
         fprintf(stat_fp, "%dx%d - %d (%.0lf%%), ", pos->first.first, pos->first.second, pos->second, ((((double) pos->second))/g_Router_vec.size())*100.0);
    }
    fprintf(stat_fp, "\n");
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "Link:\n");
    fprintf(stat_fp, "  #links: %d\n", num_total_links);
    fprintf(stat_fp, "  width: %d\n", g_cfg.link_width);
    fprintf(stat_fp, "  base latency(cycle): %d\n", g_cfg.link_latency);
    fprintf(stat_fp, "  latency distribution: ");
    for (map< int, int >::iterator pos = link_delay_org_map.begin(); pos != link_delay_org_map.end(); ++pos)
         fprintf(stat_fp, "%d - %d (%.0lf%%), ", pos->first, pos->second, ((((double) pos->second))/num_total_links)*100.0);
    fprintf(stat_fp, "\n");
    fprintf(stat_fp, "  wire type: %d\n", g_cfg.link_wire_type);
    fprintf(stat_fp, "  wire pipelining: %s\n", g_cfg.link_wire_pipelining ? "Yes" : "No");
    fprintf(stat_fp, "  base length(mm): %.2lf\n", g_cfg.link_length);
    fprintf(stat_fp, "  length distribution: ");
    for (map< double, int >::iterator pos = link_length_org_map.begin(); pos != link_length_org_map.end(); ++pos)
         fprintf(stat_fp, "%.2lf - %d (%.0lf%%), ", pos->first, pos->second, ((((double) pos->second))/num_total_links)*100.0);
    fprintf(stat_fp, "\n");
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "NI:\n");
    fprintf(stat_fp, "  #inputNIs: %d\n", g_NIInput_vec.size());
    fprintf(stat_fp, "  #outputNIs: %d\n", g_NIInput_vec.size());
    fprintf(stat_fp, "  input NI packet buffer depth: %d\n", g_cfg.NIin_pktbuf_depth);
    fprintf(stat_fp, "  input NI model: %s\n", (g_cfg.NIin_type == NI_INPUT_TYPE_PER_PC) ? "Per PC" : "Per VC");
    fprintf(stat_fp, "  input NI port multiplexing: %s\n", g_cfg.NI_port_mux ? "Yes" : "No");
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "Off-chip Memory:\n");
    fprintf(stat_fp, "  access latency(cycle): %d\n", g_cfg.mem_access_latency);
    fprintf(stat_fp, "  pipelined access latency(cycle): %d\n", g_cfg.mem_access_pipelined_latency);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "Physical Environment:\n");
    fprintf(stat_fp, "  technology(mm): %d\n", g_cfg.chip_tech_nano);
    fprintf(stat_fp, "  clock frequency(GHz): %.2lf\n", g_cfg.chip_freq/UNIT_GIGA);
    fprintf(stat_fp, "  supply voltage(V): %lf\n", g_cfg.chip_voltage);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "Power Model:\n");
    fprintf(stat_fp, "  router power model: %s\n", get_router_power_model_name());
    fprintf(stat_fp, "  link power model: %s\n", get_link_power_model_name());
    fprintf(stat_fp, "  flit_sz_64bit_multiple: %d\n", g_cfg.flit_sz_64bit_multiple);
    fprintf(stat_fp, "  router switching activity factor: %lf\n", g_cfg.power_router_avf);
    fprintf(stat_fp, "  link switching activity factor: %lf\n", g_cfg.power_link_avf);
#ifdef LINK_DVS
    fprintf(stat_fp, "  link DVS: ");
    switch (g_cfg.link_dvs_method) {
    case LINK_DVS_NODVS:
        fprintf(stat_fp, " No\n"); break;
    case LINK_DVS_HISTORY:
        fprintf(stat_fp, " History\n"); break;
    case LINK_DVS_FLIT_RATE_PREDICT:
        fprintf(stat_fp, " Flit Rate Prediction\n"); break;
    default:
        assert(0);
    }
    if (g_cfg.link_dvs_method != LINK_DVS_NODVS)
        fprintf(stat_fp, "  link DVS interval(cycle): %.0lf\n",
                g_cfg.link_dvs_interval);
        fprintf(stat_fp, "  link DVS voltage transition delay(cycle): %.0lf\n",
                g_cfg.link_dvs_voltage_transit_delay);
        fprintf(stat_fp, "  link DVS frequency transition delay(cycle): %.0lf\n",
                g_cfg.link_dvs_freq_transit_delay);
#endif
    fprintf(stat_fp, "\n");


    fprintf(stat_fp, "Profile:\n");
    fprintf(stat_fp, "  profile interval (cycle): %.0lf\n", g_cfg.profile_interval);
    fprintf(stat_fp, "\n");


    fprintf(stat_fp, "Misc:\n");
    fprintf(stat_fp, "  simulation end condition: ");
    switch (g_cfg.sim_end_cond) {
    case SIM_END_BY_INJ_PKT:
        fprintf(stat_fp, "By #injected pkts\n");
        fprintf(stat_fp, "    #injected pkts: %d\n", g_cfg.sim_num_inj_pkt);
        fprintf(stat_fp, "    #ejected pkts: %d\n", g_cfg.sim_num_ejt_pkt);
        fprintf(stat_fp, "    #ejected pkts for warmup: %d\n", g_cfg.sim_num_ejt_pkt_4warmup);
        break;
    case SIM_END_BY_EJT_PKT:
        fprintf(stat_fp, "By #ejected pkts\n");
        break;
    case SIM_END_BY_CYCLE:
        fprintf(stat_fp, "By #cycle (%.0lf cycle)\n", g_cfg.sim_clk_end);
        break;
    }
    if (g_cfg.sim_show_progress) {
        fprintf(stat_fp, "  simulation progress interval: %d\n", g_cfg.sim_progress_interval);
    }
    fprintf(stat_fp, "\n");


    if (g_cfg.throttle_global_injection) {
        fprintf(stat_fp, "Injection Throttling:\n");
        fprintf(stat_fp, "  mechanism : %s\n", g_cfg.throttle_drop ? "DROP" : "SLOWDOWN");
        fprintf(stat_fp, "\n");
    }


    fprintf(stat_fp, "CAM data:\n");
    fprintf(stat_fp, "  enable: %s\n", g_cfg.cam_data_enable ? "Yes" : "No");
    if (g_cfg.cam_data_enable) {
        fprintf(stat_fp, "  management scheme:  %s\n", get_compression_scheme_name(g_cfg.cam_data_manage));
        fprintf(stat_fp, "  replacement policy: %d\n", g_cfg.cam_repl_policy);
        fprintf(stat_fp, "  LFU saturation:     %d\n", g_cfg.cam_LFU_saturation);
        fprintf(stat_fp, "  streamlined encoding: %s\n", g_cfg.cam_streamline ? "Yes" : "No");
        fprintf(stat_fp, "  dynamic management: %s\n", g_cfg.cam_dyn_control ? "Yes" : "No");
        fprintf(stat_fp, "    window size:      %d\n", g_cfg.cam_dyn_control_window);
        fprintf(stat_fp, "  block bytes:        %d\n", g_cfg.cam_data_blk_byte);
        fprintf(stat_fp, "  #interleaved tabs:  %d\n", g_cfg.cam_data_num_interleaved_tab);
        fprintf(stat_fp, "  encode latency:     %d\n", g_cfg.cam_data_en_latency);
        fprintf(stat_fp, "  encode sets:        %d\n", g_cfg.cam_data_en_num_sets);
        fprintf(stat_fp, "  decode latency:     %d\n", g_cfg.cam_data_de_latency);
        fprintf(stat_fp, "  decode sets:        %d\n", g_cfg.cam_data_de_num_sets);
        fprintf(stat_fp, "  VLB sets:           %d\n", g_cfg.cam_VLB_num_sets);
        fprintf(stat_fp, "  VLB saturation:     %d\n", g_cfg.cam_VLB_LFU_saturation);
    }
    fprintf(stat_fp, "\n");


    fprintf(stat_fp, "------------ configuration (END) ------------\n");
    fflush(stat_fp);
}

void print_stats(FILE* stat_fp)
{
    double eff_num_cycle;	// #cycles for measurement
    double eff_time;
    unsigned long long eff_num_pkt_inj;
    unsigned long long eff_num_pkt_ejt;
    unsigned long long eff_num_flit_inj;
    unsigned long long eff_num_flit_ejt;
    double max_load_4mesh;	// from analytical model (only for mesh)
    const int network_radix = (int) sqrt(g_cfg.core_num);

    eff_num_pkt_inj = g_sim.m_num_pkt_inj - g_sim.m_num_pkt_inj_warmup;
    eff_num_pkt_ejt = g_sim.m_num_pkt_ejt - g_sim.m_num_pkt_ejt_warmup;
    eff_num_flit_inj = g_sim.m_num_flit_inj - g_sim.m_num_flit_inj_warmup;
    eff_num_flit_ejt = g_sim.m_num_flit_ejt - g_sim.m_num_flit_ejt_warmup;

    eff_num_cycle = g_sim.m_clk_sim_end - g_sim.m_clk_warmup_end;
    eff_time = eff_num_cycle/g_cfg.chip_freq;
    max_load_4mesh = 2.0 / sqrt(g_cfg.router_num);

    fprintf(stat_fp, "------------ simulation result (BEGIN) -------------\n");
    fprintf(stat_fp, "  simulator execution time (sec/min/hour): %d/%d/%.1lf\n",
            g_sim.m_elapsed_time, g_sim.m_elapsed_time/60, g_sim.m_elapsed_time/3600.0);
    fprintf(stat_fp, "  simulator speed (cycle/sec): %lg\n",
            eff_num_cycle/(g_sim.m_elapsed_time/3600.0));
    fprintf(stat_fp, "  simulated time (cycle): %lg\n", eff_num_cycle);
    fprintf(stat_fp, "  simulated time (sec): %lg\n", eff_time);
    fprintf(stat_fp, "  simulation end clock: %.2lf\n", g_sim.m_clk_sim_end);
    fprintf(stat_fp, "  warmup end clock: %.2lf\n", g_sim.m_clk_warmup_end);
    fprintf(stat_fp, "  #CSIM processes: %d\n", g_sim.m_num_CSIM_process);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "  #pkts injected:             %lld\n", g_sim.m_num_pkt_inj);
    fprintf(stat_fp, "  #pkts ejected:              %lld\n", g_sim.m_num_pkt_ejt);
    fprintf(stat_fp, "  #pkts injected for warmup:  %lld\n", g_sim.m_num_pkt_inj_warmup);
    fprintf(stat_fp, "  #pkts ejected for warmup:   %lld\n", g_sim.m_num_pkt_ejt_warmup);
    fprintf(stat_fp, "  #pkts measured:             %lld\n", eff_num_pkt_inj);
    fprintf(stat_fp, "  #pkts spurious generated:   %lld\n", g_sim.m_num_pkt_spurious);
    fprintf(stat_fp, "  #pkts multicast injected:   %lld %lg%%\n", g_sim.m_num_pkt_inj_multicast, g_sim.m_num_pkt_inj_multicast/(g_sim.m_num_pkt_inj*100.0));
    fprintf(stat_fp, "  avg multicast destinations: %.2lf\n", (g_sim.m_num_pkt_inj_multicast==0) ? 0.0 : ((double) g_sim.m_sum_multicast_dest)/g_sim.m_num_pkt_inj_multicast);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "  #flits injected:            %lld\n", g_sim.m_num_flit_inj);
    fprintf(stat_fp, "  #flits ejected:             %lld\n", g_sim.m_num_flit_ejt);
    fprintf(stat_fp, "  #flits injected for warmup: %lld\n", g_sim.m_num_flit_inj_warmup);
    fprintf(stat_fp, "  #flits ejected for warmup:  %lld\n", g_sim.m_num_flit_ejt_warmup);
    fprintf(stat_fp, "  #flits measured:            %lld\n", eff_num_flit_inj);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "  offered load(flits/cycle/core):      %.4lf\n",
            (((double) eff_num_flit_inj)/eff_num_cycle)/g_cfg.core_num);
    fprintf(stat_fp, "  offered load(pkts/cycle/core):       %.4lf\n",
            (((double) eff_num_pkt_inj)/eff_num_cycle)/g_cfg.core_num);
    fprintf(stat_fp, "  offered load(flits/cycle/router):    %.4lf\n",
            (((double) eff_num_flit_inj)/eff_num_cycle)/g_cfg.router_num);
    fprintf(stat_fp, "  offered load(pkts/cycle/router):     %.4lf\n",
            (((double) eff_num_pkt_inj)/eff_num_cycle)/g_cfg.router_num);
    fprintf(stat_fp, "  normalized offered load (for mesh):  %.4lf\n",
            ((((double) eff_num_flit_inj)/eff_num_cycle)/g_cfg.core_num)/max_load_4mesh);
    fprintf(stat_fp, "  accepted traffic(flit/cycle/core):   %.4lf\n",
            (((double) eff_num_flit_ejt)/eff_num_cycle)/g_cfg.core_num);
    fprintf(stat_fp, "  accepted traffic(flit/cycle/router): %.4lf\n",
            (((double) eff_num_flit_ejt)/eff_num_cycle)/g_cfg.router_num);
    fprintf(stat_fp, "  accepted traffic(pkts/cycle/core):   %.4lf\n",
            (((double) eff_num_pkt_ejt)/eff_num_cycle)/g_cfg.core_num);
    fprintf(stat_fp, "  accepted traffic(pkts/cycle/router): %.4lf\n",
            (((double) eff_num_pkt_ejt)/eff_num_cycle)/g_cfg.router_num);
    fprintf(stat_fp, "  normalized accepted traffic:         %.4lf\n",
            ((((double) eff_num_flit_ejt)/eff_num_cycle)/g_cfg.core_num)/max_load_4mesh);
    fprintf(stat_fp, "\n");

    // hop count
    fprintf(stat_fp, "  total hop count: %.2lf\n", g_sim.m_hop_count_tab->mean()*g_sim.m_hop_count_tab->cnt());
    fprintf(stat_fp, "  avg hop count:   %.2lf\n", g_sim.m_hop_count_tab->mean());
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "packet latency (cycle):\n");
    fprintf(stat_fp, "  T_t:         avg %.2lf max %.0lf min %.0lf stddev %.2lf\n",
            g_sim.m_pkt_T_t_tab->mean(),
            g_sim.m_pkt_T_t_tab->max(),
            g_sim.m_pkt_T_t_tab->min(),
            g_sim.m_pkt_T_t_tab->stddev());
    fprintf(stat_fp, "    T_q:       avg %.2lf max %.0lf min %.0lf stddev %.2lf\n",
            g_sim.m_pkt_T_q_tab->mean(),
            g_sim.m_pkt_T_q_tab->max(),
            g_sim.m_pkt_T_q_tab->min(),
            g_sim.m_pkt_T_q_tab->stddev());
    fprintf(stat_fp, "    T_n:       avg %.2lf max %.0lf min %.0lf stddev %.2lf\n",
            g_sim.m_pkt_T_n_tab->mean(),
            g_sim.m_pkt_T_n_tab->max(),
            g_sim.m_pkt_T_n_tab->min(),
            g_sim.m_pkt_T_n_tab->stddev());
    fprintf(stat_fp, "  T_ni:        avg %.2lf max %.0lf min %.0lf stddev %.2lf\n",
            g_sim.m_pkt_T_ni_tab->mean(),
            g_sim.m_pkt_T_ni_tab->max(),
            g_sim.m_pkt_T_ni_tab->min(),
            g_sim.m_pkt_T_ni_tab->stddev());
    fprintf(stat_fp, "  T_e:         avg %.2lf max %.0lf min %.0lf stddev %.2lf\n",
            g_sim.m_pkt_T_e_tab->mean(),
            g_sim.m_pkt_T_e_tab->max(),
            g_sim.m_pkt_T_e_tab->min(),
            g_sim.m_pkt_T_e_tab->stddev());
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "packet latency breakdown (cycle):\n");
    fprintf(stat_fp, "  T_h:         avg %.2lf max %.0lf min %.0lf stddev %.2lf\n",
            g_sim.m_pkt_T_h_tab->mean(),
            g_sim.m_pkt_T_h_tab->max(),
            g_sim.m_pkt_T_h_tab->min(),
            g_sim.m_pkt_T_h_tab->stddev());
    fprintf(stat_fp, "  T_w:         avg %.2lf max %.0lf min %.0lf stddev %.2lf\n",
            g_sim.m_pkt_T_w_tab->mean(),
            g_sim.m_pkt_T_w_tab->max(),
            g_sim.m_pkt_T_w_tab->min(),
            g_sim.m_pkt_T_w_tab->stddev());
    fprintf(stat_fp, "  T_s:         avg %.2lf max %.0lf min %.0lf stddev %.2lf\n",
            g_sim.m_pkt_T_s_tab->mean(),
            g_sim.m_pkt_T_s_tab->max(),
            g_sim.m_pkt_T_s_tab->min(),
            g_sim.m_pkt_T_s_tab->stddev());
    fprintf(stat_fp, "  T_c:         avg %.2lf\n",
            g_sim.m_pkt_T_t_tab->mean() - g_sim.m_pkt_T_h_tab->mean() - g_sim.m_pkt_T_w_tab->mean() - g_sim.m_pkt_T_s_tab->mean() + 1.0);	// FIXME: +1.0??? Flits eject one cycle ahead.
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "per-type packet latency (cycle):\n");
    fprintf(stat_fp, "  type      count      T_t    T_h    T_w    T_s    T_c  (T_q)\n");
    fprintf(stat_fp, "  ------------------------------------------------------------\n");
    for (map< unsigned int, table* >::iterator pos = g_sim.m_pkttype_T_t_map.begin();
         pos != g_sim.m_pkttype_T_t_map.end(); ++pos) {
    
        unsigned int pkt_type = pos->first;
        fprintf(stat_fp, "  %4s", get_pkttype_name(pkt_type));
        fprintf(stat_fp, "  %9ld", g_sim.m_pkttype_T_t_map[pkt_type]->cnt());
        fprintf(stat_fp, "  %7.2lf", g_sim.m_pkttype_T_t_map[pkt_type]->mean());
        fprintf(stat_fp, "  %5.2lf", g_sim.m_pkttype_T_h_map[pkt_type]->mean());
        fprintf(stat_fp, "  %5.2lf", g_sim.m_pkttype_T_w_map[pkt_type]->mean());
        fprintf(stat_fp, "  %5.2lf", g_sim.m_pkttype_T_s_map[pkt_type]->mean());
        fprintf(stat_fp, "  %5.2lf", g_sim.m_pkttype_T_t_map[pkt_type]->mean() - g_sim.m_pkttype_T_h_map[pkt_type]->mean() - g_sim.m_pkttype_T_w_map[pkt_type]->mean() - g_sim.m_pkttype_T_s_map[pkt_type]->mean() + 1.0); // FIXME: +1.0??? Flits eject one cycle ahead.
        fprintf(stat_fp, "  (%.2lf)", g_sim.m_pkttype_T_q_map[pkt_type]->mean());
        fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, "\n");

    // router pipeline stalls
    table tab_router_pipeRC = table ("router_pipeRC");
    table tab_router_pipeVA = table ("router_pipeVA");
    table tab_router_pipeSSA = table ("router_pipeSSA");
    table tab_router_pipeSA = table ("router_pipeSA");
    table tab_router_pipeST = table ("router_pipeST");
    table tab_router_pipeLT = table ("router_pipeLT");
    for (int i=0; i<g_cfg.router_num; i++) {
        Router* p_router = g_Router_vec[i];

        if (p_router->m_pipe_lat_RC_tab->cnt() > 0)
            tab_router_pipeRC.tabulate(p_router->m_pipe_lat_RC_tab->mean());
        if (p_router->m_pipe_lat_VA_tab->cnt() > 0)
            tab_router_pipeVA.tabulate(p_router->m_pipe_lat_VA_tab->mean());
        if (p_router->m_pipe_lat_SSA_tab->cnt() > 0)
            tab_router_pipeSSA.tabulate(p_router->m_pipe_lat_SSA_tab->mean());
        if (p_router->m_pipe_lat_SA_tab->cnt() > 0)
            tab_router_pipeSA.tabulate(p_router->m_pipe_lat_SA_tab->mean());
        if (p_router->m_pipe_lat_ST_tab->cnt() > 0)
            tab_router_pipeST.tabulate(p_router->m_pipe_lat_ST_tab->mean());
        if (p_router->m_pipe_lat_LT_tab->cnt() > 0)
            tab_router_pipeLT.tabulate(p_router->m_pipe_lat_LT_tab->mean());
    }
    fprintf(stat_fp, "pipeline stage latency: \n");
    fprintf(stat_fp, "  RC:  %.3lf\n", tab_router_pipeRC.mean());
    fprintf(stat_fp, "  VA:  %.3lf\n", tab_router_pipeVA.mean());
    fprintf(stat_fp, "  SSA: %.3lf\n", tab_router_pipeSSA.mean());
    fprintf(stat_fp, "  SA:  %.3lf\n", tab_router_pipeSA.mean());
    fprintf(stat_fp, "  ST:  %.3lf\n", tab_router_pipeST.mean());
    fprintf(stat_fp, "  LT:  %.3lf\n", tab_router_pipeLT.mean());
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "VA-stage avg. latency distribution:\n");
    unsigned long long total_VA_ops = 0;
    for (int i=0; i<g_cfg.router_num; i++) {
        Router* p_router = g_Router_vec[i];
        total_VA_ops += p_router->m_pipe_lat_VA_tab->cnt();

        if (p_router->m_pipe_lat_VA_tab->cnt() == 0) {
            fprintf(stat_fp, "     -");
        } else {
            fprintf(stat_fp, "%6.2lf", p_router->m_pipe_lat_VA_tab->mean());
        }

        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, " VA-stage avg %.4lf stddev %.4lf min %.4lf max %.4lf\n",
            tab_router_pipeVA.mean(), tab_router_pipeVA.stddev(),
            tab_router_pipeVA.min(), tab_router_pipeVA.max());
    fprintf(stat_fp, " VA ops: %lld\n", total_VA_ops);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "SA-stage avg. latency distribution:\n");
    unsigned long long total_SA_ops = 0;
    unsigned long long total_SSA_ops = 0;
    for (int i=0; i<g_cfg.router_num; i++) {
        Router* p_router = g_Router_vec[i];
        total_SA_ops += p_router->m_pipe_lat_SA_tab->cnt();
        total_SSA_ops += p_router->m_pipe_lat_SSA_tab->cnt();

        if (p_router->m_pipe_lat_SA_tab->cnt() == 0) {
            fprintf(stat_fp, "     -");
        } else {
            fprintf(stat_fp, "%6.2lf", p_router->m_pipe_lat_SA_tab->mean());
        }

        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, " SA-stage avg %.4lf stddev %.4lf min %.4lf max %.4lf\n",
            tab_router_pipeSA.mean(), tab_router_pipeSA.stddev(),
            tab_router_pipeSA.min(), tab_router_pipeSA.max());
    fprintf(stat_fp, " SA-ops: %lld, SSA-ops: %lld, SSA-ops/VA-ops: %.2lf%%\n",
            total_SA_ops, total_SSA_ops, (100.0*total_SSA_ops)/total_VA_ops);
    fprintf(stat_fp, "\n");


    // intra-router flit latency
    table flit_intra_router_tab("flit_intra_router");
    fprintf(stat_fp, "intra-router flit latency distribution:\n");
    for (int i=0; i<g_cfg.router_num; i++) {
        fprintf(stat_fp, "%6.2f", g_Router_vec[i]->m_flit_lat_router_tab->mean());
        flit_intra_router_tab.tabulate(g_Router_vec[i]->m_flit_lat_router_tab->mean());
        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, " intra-router flit latency avg %.4lf stddev %.4lf min %.4lf max %.4lf\n",
            flit_intra_router_tab.mean(), flit_intra_router_tab.stddev(),
            flit_intra_router_tab.min(), flit_intra_router_tab.max());
    fprintf(stat_fp, "\n");

    // link utilization
    fprintf(stat_fp, "link utilization (#op/cycle):\n");
    table link_utilz_tab("link_utilz_tab");
    vector< table* > perPC_link_utilz_tab_vec;
    const int num_perPC_link_utilz_tab = 8;	// FIXME: more than 8 PCs
    for (int i=0; i<num_perPC_link_utilz_tab; i++)
        perPC_link_utilz_tab_vec.push_back(new table());
    for (int i=0; i<g_cfg.router_num; i++) {
        fprintf(stat_fp, "  R%02d:", g_Router_vec[i]->id());
        for (int out_pc=0; out_pc<g_Router_vec[i]->num_pc(); out_pc++) {
            Link& link = g_Router_vec[i]->getLink(out_pc);

            if (link.m_valid) {
                LinkPower* p_link_power = g_Router_vec[i]->m_power_tmpl->getLinkPower(out_pc);
                assert(p_link_power);

                double link_utilz = ((double) p_link_power->op())/eff_num_cycle;
                if (p_link_power->op() == 0) {
                    fprintf(stat_fp, " %s 0    ", link.m_link_name.c_str());
                } else {
                    fprintf(stat_fp, " %s %5.3lf", link.m_link_name.c_str(), link_utilz);
                }

                link_utilz_tab.tabulate(link_utilz);
                if (out_pc<(int) perPC_link_utilz_tab_vec.size())
                    perPC_link_utilz_tab_vec[out_pc]->tabulate(link_utilz);
            } else {
                fprintf(stat_fp, " %s -    ", link.m_link_name.c_str());
            }
        }
        fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, "  link-utilization avg %lg stddev %lg min %lg max %lg\n",
            link_utilz_tab.mean(), link_utilz_tab.stddev(),
            link_utilz_tab.min(), link_utilz_tab.max());
    for (int i=0; i<(int) perPC_link_utilz_tab_vec.size(); ++i) {
        if (perPC_link_utilz_tab_vec[i]->cnt() == 0)
            continue;

        fprintf(stat_fp, "  PC%d link-utilization avg %lg stddev %lg min %lg max %lg\n",
                i,
                perPC_link_utilz_tab_vec[i]->mean(), perPC_link_utilz_tab_vec[i]->stddev(),
                perPC_link_utilz_tab_vec[i]->min(), perPC_link_utilz_tab_vec[i]->max());
        delete perPC_link_utilz_tab_vec[i];
    }
    fprintf(stat_fp, "\n");

    // Per-PC Buffer occupancy
    fprintf(stat_fp, "per-PC buffer occupancy:\n");
    table pcbuf_avg_occupancy_tab = table("pcbuf_avg_occupancy_tab");
    table pcbuf_max_occupancy_tab = table("pcbuf_max_occupancy_tab");
    for (int i=0; i<g_cfg.router_num; i++) {
        int num_pc = g_Router_vec[i]->num_pc();
        int num_vc = g_Router_vec[i]->num_vc();
        FlitQ* fq = g_Router_vec[i]->flitQ();

        fprintf(stat_fp, " R%02d:", g_Router_vec[i]->id());
        for (int pc=0; pc<num_pc; pc++) {
            fprintf(stat_fp, " P%d", pc);
            double pcbuf_avg_occupancy = 0.0;
            int pcbuf_max_occupancy = 0;

            for (int vc=0; vc<num_vc; vc++) {
                pcbuf_avg_occupancy += fq->getAvgOccupancy(pc, vc, eff_num_cycle);
                pcbuf_max_occupancy += fq->getMaxOccupancy(pc, vc);
            }

            pcbuf_avg_occupancy_tab.tabulate(pcbuf_avg_occupancy);
            pcbuf_max_occupancy_tab.tabulate(pcbuf_max_occupancy);

            fprintf(stat_fp, " %.02lf (%2d)", pcbuf_avg_occupancy, pcbuf_max_occupancy);
        }
        fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, "  perPC-buffer-avg-occupancy: avg %lg stddev %lg min %lg max %lg #pcbuf %ld\n",
            pcbuf_avg_occupancy_tab.mean(), pcbuf_avg_occupancy_tab.stddev(),
            pcbuf_avg_occupancy_tab.min(), pcbuf_avg_occupancy_tab.max(),
            pcbuf_avg_occupancy_tab.cnt());
    fprintf(stat_fp, "  perPC-buffer-max-occupancy: avg %lg stddev %lg min %lg max %lg\n",
            pcbuf_max_occupancy_tab.mean(), pcbuf_max_occupancy_tab.stddev(),
            pcbuf_max_occupancy_tab.min(), pcbuf_max_occupancy_tab.max());
    fprintf(stat_fp, "\n");

    // Per-VC Buffer occupancy
    fprintf(stat_fp, "per-VC buffer occupancy:\n");
    table vcbuf_avg_occupancy_tab = table("vcbuf_avg_occupancy_tab");
    table vcbuf_max_occupancy_tab = table("vcbuf_max_occupancy_tab");
    for (int i=0; i<g_cfg.router_num; i++) {
        int num_pc = g_Router_vec[i]->num_pc();
        int num_vc = g_Router_vec[i]->num_vc();
        FlitQ* fq = g_Router_vec[i]->flitQ();

        fprintf(stat_fp, " R%02d:", g_Router_vec[i]->id());
        for (int pc=0; pc<num_pc; pc++) {
            fprintf(stat_fp, " %s%d", ((pc==0) ? "P" : "     P"), pc);

            for (int vc=0; vc<num_vc; vc++) {
                double vcbuf_avg_occupancy = fq->getAvgOccupancy(pc, vc, eff_num_cycle);
                int vcbuf_max_occupancy = fq->getMaxOccupancy(pc, vc);

                fprintf(stat_fp, " V%d %.4lf (%2d)", vc, vcbuf_avg_occupancy, vcbuf_max_occupancy);

                // NOTE: Some PCs in routers at periphery of mesh network are never used.
                vcbuf_avg_occupancy_tab.tabulate(vcbuf_avg_occupancy);
                vcbuf_max_occupancy_tab.tabulate(vcbuf_max_occupancy);
            }
            fprintf(stat_fp, "\n");
        }
    }
    fprintf(stat_fp, "  perVC-buffer-avg-occupancy: avg %lg stddev %lg min %lg max %lg #vcbuf %ld\n",
            vcbuf_avg_occupancy_tab.mean(), vcbuf_avg_occupancy_tab.stddev(),
            vcbuf_avg_occupancy_tab.min(), vcbuf_avg_occupancy_tab.max(),
            vcbuf_avg_occupancy_tab.cnt());
    fprintf(stat_fp, "  perVC-buffer-max-occupancy: avg %lg stddev %lg min %lg max %lg\n",
            vcbuf_max_occupancy_tab.mean(), vcbuf_max_occupancy_tab.stddev(),
            vcbuf_max_occupancy_tab.min(), vcbuf_max_occupancy_tab.max());
    fprintf(stat_fp, "\n");

    // PC buffer operation
    fprintf(stat_fp, "perPC buffer read/write operations: (ops/cycle):\n");
    table tab_avg_op_buf_rd("op_buf_rd");
    table tab_avg_op_buf_wr("op_buf_wr");
    for (int i=0; i<g_cfg.router_num; i++) {
        double avg_op_buf_rd = g_Router_vec[i]->m_power_tmpl->total_op_buf_rd() / eff_num_cycle;
        double avg_op_buf_wr = g_Router_vec[i]->m_power_tmpl->total_op_buf_wr() / eff_num_cycle;

        fprintf(stat_fp, " R%02d: %-8lg / %-8lg\n", i, avg_op_buf_rd, avg_op_buf_wr);
        tab_avg_op_buf_rd.tabulate(avg_op_buf_rd);
        tab_avg_op_buf_wr.tabulate(avg_op_buf_wr);
    }
    fprintf(stat_fp, "  perPC buffer read-ops  avg %lg stddev %lg max %lg\n",
                     tab_avg_op_buf_rd.mean(), tab_avg_op_buf_rd.stddev(), tab_avg_op_buf_rd.max());
    fprintf(stat_fp, "  perPC buffer write-ops avg %lg stddev %lg max %lg\n",
                     tab_avg_op_buf_wr.mean(), tab_avg_op_buf_wr.stddev(), tab_avg_op_buf_wr.max());
    fprintf(stat_fp, "\n");

    // stat for packet buffers at input NIs
    fprintf(stat_fp, "NI packet buffer occupancy:\n");
    table tab_pkt_buf_avg("pkt_buf_avg_occ");
    table tab_pkt_buf_max("pkt_buf_max_occ");
    for (unsigned int i=0; i<g_NIInput_vec.size(); i++) {
        fprintf(stat_fp, " NI%02d: avg %.2lf max %.0lf\n", g_NIInput_vec[i]->id(), g_NIInput_vec[i]->getAvgOccupancy(eff_num_cycle), g_NIInput_vec[i]->getMaxOccupancy());
        tab_pkt_buf_avg.tabulate(g_NIInput_vec[i]->getAvgOccupancy(eff_num_cycle));
        tab_pkt_buf_max.tabulate(g_NIInput_vec[i]->getMaxOccupancy());
    }
    fprintf(stat_fp, "  avg. packet buffer occupancy avg %lg stddev %lg max %lg\n",
                     tab_pkt_buf_avg.mean(), tab_pkt_buf_avg.stddev(), tab_pkt_buf_avg.max());
    fprintf(stat_fp, "  max. packet buffer occupancy avg %lg stddev %lg max %lg\n",
                     tab_pkt_buf_max.mean(), tab_pkt_buf_max.stddev(), tab_pkt_buf_max.max());
    fprintf(stat_fp, "\n");

    // xbar throughput
    fprintf(stat_fp, "xbar throughput (avg. #requests) (accounting for only duty cycles):\n");
    table tab_avg_xbar_trav("xbar_trav_parallel");
    for (int i=0; i<g_cfg.router_num; i++) {
        double avg_xbar_trav = g_Router_vec[i]->m_power_tmpl->avg_xbar_trav();
        tab_avg_xbar_trav.tabulate(avg_xbar_trav);

        fprintf(stat_fp, " %-7lg", avg_xbar_trav);

        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, "  xbar_throughput avg %lg stddev %lg min %lg max %lg\n",
            tab_avg_xbar_trav.mean(), tab_avg_xbar_trav.stddev(),
            tab_avg_xbar_trav.min(), tab_avg_xbar_trav.max());
    fprintf(stat_fp, "\n");

    // xbar utilization rate
    fprintf(stat_fp, "xbar utilization rate:\n");
    table tab_avg_xbar_utilz("xbar_utilz");
    for (int i=0; i<g_cfg.router_num; i++) {
        double avg_xbar_utilz = g_Router_vec[i]->m_power_tmpl->cnt_xbar_trav()/eff_num_cycle;
        tab_avg_xbar_utilz.tabulate(avg_xbar_utilz);

        fprintf(stat_fp, " %.4lf", avg_xbar_utilz);

        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, "  xbar_utilz avg %lg stddev %lg min %lg max %lg\n",
            tab_avg_xbar_utilz.mean(), tab_avg_xbar_utilz.stddev(),
            tab_avg_xbar_utilz.min(), tab_avg_xbar_utilz.max());
    fprintf(stat_fp, "\n");

    // switch arbitration efficiency
    fprintf(stat_fp, "SA grant rate (#grants/#requests):\n");
    table tab_sa_eff_rate("sa_eff");
    for (int i=0; i<g_cfg.router_num; i++) {
        const double sa_eff_rate = g_Router_vec[i]->getSwArb()->getGrantRate();
        tab_sa_eff_rate.tabulate(sa_eff_rate);

        fprintf(stat_fp, " %.4lf", sa_eff_rate);

        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, "  SA_grant_rate avg %lg stddev %lg min %lg max %lg\n",
            tab_sa_eff_rate.mean(), tab_sa_eff_rate.stddev(),
            tab_sa_eff_rate.min(), tab_sa_eff_rate.max());
    fprintf(stat_fp, "\n");

    // VC arbitration request
    fprintf(stat_fp, "VA request (#requests):\n");
    table tab_va_req("va_req");
    for (int i=0; i<g_cfg.router_num; i++) {
        const double avg_req = g_Router_vec[i]->getVCArb()->getAvgReq();
        tab_va_req.tabulate(avg_req);

        fprintf(stat_fp, " %.4lf", avg_req);

        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, "  VA_num_req avg %lg stddev %lg min %lg max %lg\n",
            tab_va_req.mean(), tab_va_req.stddev(),
            tab_va_req.min(), tab_va_req.max());
    fprintf(stat_fp, "\n");

    // VC arbitration efficiency
    fprintf(stat_fp, "VA grant rate (#grants/#requests):\n");
    table tab_va_eff_rate("va_eff");
    for (int i=0; i<g_cfg.router_num; i++) {
        const double va_eff_rate = g_Router_vec[i]->getVCArb()->getAvgGrantRate();
        tab_va_eff_rate.tabulate(va_eff_rate);

        fprintf(stat_fp, " %.4lf", va_eff_rate);

        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, "  VA_grant_rate avg %lg stddev %lg min %lg max %lg\n",
            tab_va_eff_rate.mean(), tab_va_eff_rate.stddev(),
            tab_va_eff_rate.min(), tab_va_eff_rate.max());
    fprintf(stat_fp, "\n");

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

    calc_network_energy(total_dyn_energy_buffer, total_dyn_energy_vc_arb,
                        total_dyn_energy_sw_arb, total_dyn_energy_xbar,
                        total_dyn_energy_router, total_dyn_energy_link,
                        total_dyn_energy_network,

                        total_stat_energy_buffer, total_stat_energy_vc_arb,
                        total_stat_energy_sw_arb, total_stat_energy_xbar,
                        total_stat_energy_router, total_stat_energy_link,
                        total_stat_energy_network,

                        router_dyn_energy_vec, router_stat_energy_vec,

                        eff_num_cycle,
                        false);
#ifdef LINK_DVS
    double total_dyn_energy_dvs_trans_link = router_power_link_voltage_transition_energy_report();
    total_dyn_energy_link += total_dyn_energy_dvs_trans_link;
    total_dyn_energy_network += total_dyn_energy_dvs_trans_link;
#endif

    // total network energy (dynamic + static)
    double total_energy_network = total_dyn_energy_network + total_stat_energy_network;

#ifdef ORION_MODEL
    if (g_cfg.router_power_model == ROUTER_POWER_MODEL_ORION_CALL) {
        fprintf(stat_fp, "Orion static power values\n");
        fprintf(stat_fp, "  in_buf.I_static: %lg\n", g_Router_vec[0]->m_power_tmpl->staticE_buffer());
        fprintf(stat_fp, "  xbar.I_static: %lg\n", g_Router_vec[0]->m_power_tmpl->staticE_xbar());
        fprintf(stat_fp, "  vc_arb.I_static: %lg\n", g_Router_vec[0]->m_power_tmpl->staticE_vc_arb());
        fprintf(stat_fp, "  xbar_v1_arb.I_static: %lg xbar_p1_arb.I_static: %lg\n", g_Router_vec[0]->m_power_tmpl->staticE_xbar_v1_arb(), g_Router_vec[0]->m_power_tmpl->staticE_xbar_p1_arb());
        fprintf(stat_fp, "\n");
    }
#endif

    fprintf(stat_fp, "network dynamic energy:\n");
    fprintf(stat_fp, "  d-total(J):    %lg %lg (J/cycle)\n",
            total_dyn_energy_network, total_dyn_energy_network/eff_num_cycle);
    fprintf(stat_fp, "  d-router(J):   %lg\t%.2lf%%\n",
            total_dyn_energy_router, total_dyn_energy_router/total_dyn_energy_network*100.0);
    fprintf(stat_fp, "    d-buffer(J): %lg\t%.2lf%%\n",
            total_dyn_energy_buffer, total_dyn_energy_buffer/total_dyn_energy_network*100.0);
    fprintf(stat_fp, "    d-VCarb(J):  %lg\t%.2lf%%\n",
            total_dyn_energy_vc_arb, total_dyn_energy_vc_arb/total_dyn_energy_network*100.0);
    fprintf(stat_fp, "    d-SWarb(J):  %lg\t%.2lf%%\n",
            total_dyn_energy_sw_arb, total_dyn_energy_sw_arb/total_dyn_energy_network*100.0);
    fprintf(stat_fp, "    d-xbar(J):   %lg\t%.2lf%%\n",
            total_dyn_energy_xbar, total_dyn_energy_xbar/total_dyn_energy_network*100.0);
    fprintf(stat_fp, "  d-link(J):     %lg\t%.2lf%%\n",
            total_dyn_energy_link, total_dyn_energy_link/total_dyn_energy_network*100.0);
#ifdef LINK_DVS
    fprintf(stat_fp, "    d-link DVS transition(J): %lg\n", total_dyn_energy_dvs_trans_link);
    fprintf(stat_fp, "    d-link DVS transition count: %lld\n", router_power_link_voltage_transition_count_report());
#endif
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "network dynamic power:\n");
    fprintf(stat_fp, "  d-total(W):    %lg %.2lf%%\n", total_dyn_energy_network/eff_time, total_dyn_energy_network/total_energy_network*100.0);
    fprintf(stat_fp, "  d-router(W):   %lg\n", total_dyn_energy_router/eff_time);
    fprintf(stat_fp, "    d-buffer(W): %lg\n", total_dyn_energy_buffer/eff_time);
    fprintf(stat_fp, "    d-VCarb(W):  %lg\n", total_dyn_energy_vc_arb/eff_time);
    fprintf(stat_fp, "    d-SWarb(W):  %lg\n", total_dyn_energy_sw_arb/eff_time);
    fprintf(stat_fp, "    d-xbar(W):   %lg\n", total_dyn_energy_xbar/eff_time);
    fprintf(stat_fp, "  d-link(W):     %lg\n", total_dyn_energy_link/eff_time);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "network static power:\n");
    fprintf(stat_fp, "  s-total(W):    %lg %.2lf%%\n", total_stat_energy_network/eff_time, total_stat_energy_network/total_energy_network*100.0);
    fprintf(stat_fp, "  s-router(W):   %lg\n", total_stat_energy_router/eff_time);
    fprintf(stat_fp, "    s-buffer(W): %lg\n", total_stat_energy_buffer/eff_time);
    fprintf(stat_fp, "    s-VCarb(W):  %lg\n", total_stat_energy_vc_arb/eff_time);
    fprintf(stat_fp, "    s-SWarb(W):  %lg\n", total_stat_energy_sw_arb/eff_time);
    fprintf(stat_fp, "    s-xbar(W):   %lg\n", total_stat_energy_xbar/eff_time);
    fprintf(stat_fp, "    s-router(W): %lg\n", total_stat_energy_router/eff_time);
    fprintf(stat_fp, "  s-link(W):     %lg\n", total_stat_energy_link/eff_time);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "network total power:\n");
    fprintf(stat_fp, "  t-total(W):    %lg\n", total_energy_network/eff_time);
    fprintf(stat_fp, "  t-router(W):   %lg\t%.2lf%%\n",
            (total_dyn_energy_router+total_stat_energy_router)/eff_time,
            (total_dyn_energy_router+total_stat_energy_router)/total_energy_network*100.0);
    fprintf(stat_fp, "    t-buffer(W): %lg\t%.2lf%%\n",
            (total_dyn_energy_buffer+total_stat_energy_buffer)/eff_time,
            (total_dyn_energy_buffer+total_stat_energy_buffer)/total_energy_network*100.0);
    fprintf(stat_fp, "    t-VCarb(W):  %lg\t%.2lf%%\n",
            (total_dyn_energy_vc_arb+total_stat_energy_vc_arb)/eff_time,
            (total_dyn_energy_vc_arb+total_stat_energy_vc_arb)/total_energy_network*100.0);
    fprintf(stat_fp, "    t-SWarb(W):  %lg\t%.2lf%%\n",
            (total_dyn_energy_sw_arb+total_stat_energy_sw_arb)/eff_time,
            (total_dyn_energy_sw_arb+total_stat_energy_sw_arb)/total_energy_network*100.0);
    fprintf(stat_fp, "    t-xbar(W):   %lg\t%.2lf%%\n",
            (total_dyn_energy_xbar+total_stat_energy_xbar)/eff_time,
            (total_dyn_energy_xbar+total_stat_energy_xbar)/total_energy_network*100.0);
    fprintf(stat_fp, "  t-link(W):     %lg\t%.2lf%%\n",
            (total_dyn_energy_link+total_stat_energy_link)/eff_time,
            (total_dyn_energy_link+total_stat_energy_link)/total_energy_network*100.0);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "router dynamic energy distribution (J): \n");
    table router_energy_tab("router_energy");
    for (int i=0; i<g_cfg.router_num; i++) {
        double router_energy = router_dyn_energy_vec[i] + router_stat_energy_vec[i];
        router_energy_tab.tabulate(router_energy);
        fprintf(stat_fp, " %lg", router_energy);

        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, " router-dyn-energy mean: %lg stddev: %lg min: %lg max:%lg\n",
            router_energy_tab.mean(), router_energy_tab.stddev(),
            router_energy_tab.min(), router_energy_tab.max());
    fprintf(stat_fp, "\n");


    // per-wire link switching activity
    if (g_cfg.link_power_model == LINK_POWER_MODEL_DELAY_OPT_REPEATED_VALUE) {
        fprintf(stat_fp, "link switching activity (per wire): \n");

        vector <unsigned long long> router_link_op_vec;
        vector <unsigned long long> router_wire_intra_switch_vec;
        vector <unsigned long long> router_wire_inter_switch_vec;
        int num_links = 0;

        router_link_op_vec.resize(g_cfg.router_num, 0);
        router_wire_intra_switch_vec.resize(g_cfg.router_num, 0);
        router_wire_inter_switch_vec.resize(g_cfg.router_num, 0);

        for (int i=0; i<g_cfg.router_num; i++) {
            Router* p_router = g_Router_vec[i];

            router_link_op_vec[i] = p_router->getTotalLinkOp();

            for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
                if (! p_router->getLink(out_pc).m_valid )
                    continue;

                LinkPowerRepeatedValue* link_power_model_ptr =
                    (LinkPowerRepeatedValue*) (p_router->m_power_tmpl->getLinkPower(out_pc));

                router_wire_intra_switch_vec[i] += link_power_model_ptr->intra_trans(); 
                router_wire_inter_switch_vec[i] += link_power_model_ptr->inter_trans();

                num_links++;
            }
        }

        fprintf(stat_fp, "router link-op intra-wire  (%%) inter-wire wire-op\n");
        for (int i=0; i<g_cfg.router_num; i++) {
            fprintf(stat_fp, "R%02d: %9lld %9lld (%2.0lf%%) %9lld\n",
                i,
                router_link_op_vec[i],
                router_wire_intra_switch_vec[i],
                router_wire_intra_switch_vec[i]/((double) router_link_op_vec[i]*g_cfg.link_width) * 100.0,
                router_wire_inter_switch_vec[i]);
        }
        fprintf(stat_fp, "SUM: %9lld %9lld (%2.0lf%%) %9lld\n",
                StatSum(router_link_op_vec),
                StatSum(router_wire_intra_switch_vec),
                StatSum(router_wire_intra_switch_vec)/((double) StatSum(router_link_op_vec)*g_cfg.link_width) * 100.0,
                StatSum(router_wire_inter_switch_vec));
        fprintf(stat_fp, "\n");
    } // if (g_cfg.link_power_model == LINK_POWER_MODEL_DELAY_OPT_REPEATED_VALUE) {

/*
    // core power consumption
    double total_core_energy = 0.0;
    for (int i=0; i<g_cfg.router_num; i++) {
        total_core_energy += g_Core_vec[i]->getEnergy();
    }
    fprintf(stat_fp, "Core Total Power/Energy:\n");
    fprintf(stat_fp, "  c-total energy(J): %lg\n", total_core_energy);
    fprintf(stat_fp, "  c-total power(W): %lg\n", total_core_energy/eff_time);
    fprintf(stat_fp, "\n");
*/


    // average flit load of router
    fprintf(stat_fp, "average router load (flit/cycle):\n");
    table avg_router_flit_load_tab("avg_router_flit_load");
    for (int i=0; i<g_cfg.router_num; i++) {
        double avg_router_flit_load = (g_Router_vec[i]->m_num_flit_inj_from_core + g_Router_vec[i]->m_num_flit_inj_from_router)/eff_num_cycle;
        avg_router_flit_load_tab.tabulate(avg_router_flit_load);
        fprintf(stat_fp, " %.4lf", avg_router_flit_load);
        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, " router-flit-load avg %lf stddev %lf min %lf max %lf\n",
            avg_router_flit_load_tab.mean(), avg_router_flit_load_tab.stddev(),
            avg_router_flit_load_tab.min(), avg_router_flit_load_tab.max());
    fprintf(stat_fp, "\n");

    // average packet load of router
    fprintf(stat_fp, "average router load (pkt/cycle):\n");
    table avg_router_pkt_load_tab("avg_router_pkt_load");
    for (int i=0; i<g_cfg.router_num; i++) {
        double avg_router_pkt_load = (g_Router_vec[i]->m_num_pkt_inj_from_core + g_Router_vec[i]->m_num_pkt_inj_from_router)/eff_num_cycle;
        avg_router_pkt_load_tab.tabulate(avg_router_pkt_load);
        fprintf(stat_fp, " %.4lf", avg_router_pkt_load);
        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, " router-pkt-load avg %lf stddev %lf min %lf max %lf\n",
            avg_router_pkt_load_tab.mean(), avg_router_pkt_load_tab.stddev(),
            avg_router_pkt_load_tab.min(), avg_router_pkt_load_tab.max());
    fprintf(stat_fp, "\n");

    // average flit crossing (i.e., from only routers) load of router
    fprintf(stat_fp, "average router load (flit/cycle) except injection channels:\n");
    table avg_router_xing_flit_load_tab("avg_router_xing_flit_load");
    for (int i=0; i<g_cfg.router_num; i++) {
        double avg_router_xing_flit_load = g_Router_vec[i]->m_num_flit_inj_from_router/eff_num_cycle;
        avg_router_xing_flit_load_tab.tabulate(avg_router_xing_flit_load);
        fprintf(stat_fp, " %.4lf", avg_router_xing_flit_load);
        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, " router-xing-flit-load avg %lf stddev %lf min %lf max %lf\n",
            avg_router_xing_flit_load_tab.mean(), avg_router_xing_flit_load_tab.stddev(),
            avg_router_xing_flit_load_tab.min(), avg_router_xing_flit_load_tab.max());
    fprintf(stat_fp, "\n");

    // average packet crossing (i.e., from only routers) load of router
    fprintf(stat_fp, "average router load (pkt/cycle) except injection channels:\n");
    table avg_router_xing_pkt_load_tab("avg_router_xing_pkt_load");
    for (int i=0; i<g_cfg.router_num; i++) {
        double avg_router_xing_pkt_load = g_Router_vec[i]->m_num_pkt_inj_from_router/eff_num_cycle;
        avg_router_xing_pkt_load_tab.tabulate(avg_router_xing_pkt_load);
        fprintf(stat_fp, " %.4lf", avg_router_xing_pkt_load);
        if (i%network_radix == network_radix-1)
            fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, " router-xing-pkt-load avg %lf stddev %lf min %lf max %lf\n",
            avg_router_xing_pkt_load_tab.mean(), avg_router_xing_pkt_load_tab.stddev(),
            avg_router_xing_pkt_load_tab.min(), avg_router_xing_pkt_load_tab.max());
    fprintf(stat_fp, "\n");

    // source-destination distribution of traffic
    const double trf_pkt_dist_scale = 1000.0;
    fprintf(stat_fp, "traffic matrix: source-dest pair: (%.0lf pkts/cycle):\n", trf_pkt_dist_scale);
    unsigned long long num_self_inj_pkt = 0;
    for (int i=0; i<g_cfg.core_num; i++) {
        unsigned long long num_total_inj_pkt = 0;
        unsigned long long num_total_ejt_pkt = 0;
        for (int j=0; j<g_cfg.core_num; j++) {
            num_total_inj_pkt += g_Workload->getSpatialPattPkt(i, j);
            num_total_ejt_pkt += g_Workload->getSpatialPattPkt(j, i);

            if (i == j)
                num_self_inj_pkt += g_Workload->getSpatialPattPkt(i, j);
        }
        fprintf(stat_fp, " C%02d: %4.0lf(inj) %4.0lf(ejt)",
                i,
                num_total_inj_pkt/eff_num_cycle*trf_pkt_dist_scale,
                num_total_ejt_pkt/eff_num_cycle*trf_pkt_dist_scale);
        for (int j=0; j<g_cfg.core_num; j++)
            fprintf(stat_fp, " %.0lf", g_Workload->getSpatialPattPkt(i, j)/eff_num_cycle*trf_pkt_dist_scale);
        fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, "self comm. pkt percentage: %.2lf%%\n",
            ((double) num_self_inj_pkt / eff_num_pkt_inj) * 100.0);
    fprintf(stat_fp, "\n");

    const double trf_flit_dist_scale = 100.0;
    fprintf(stat_fp, "traffic matrix: source-dest pair: (%.0lf flits/cycle):\n", trf_flit_dist_scale);
    unsigned long long num_self_inj_flit = 0;
    for (int i=0; i<g_cfg.core_num; i++) {
        unsigned long long num_total_inj_flit = 0;
        unsigned long long num_total_ejt_flit = 0;
        for (int j=0; j<g_cfg.core_num; j++) {
            num_total_inj_flit += g_Workload->getSpatialPattFlit(i, j);
            num_total_ejt_flit += g_Workload->getSpatialPattFlit(j, i);

            if (i == j)
                num_self_inj_flit += g_Workload->getSpatialPattFlit(i, j);
        }
        fprintf(stat_fp, " C%02d: %.1lf(inj) %.1lf(ejt)",
                i,
                num_total_inj_flit/eff_num_cycle*trf_flit_dist_scale,
                num_total_ejt_flit/eff_num_cycle*trf_flit_dist_scale);
        for (int j=0; j<g_cfg.core_num; j++)
            fprintf(stat_fp, " %.2lf", g_Workload->getSpatialPattFlit(i, j)/eff_num_cycle*trf_flit_dist_scale);
        fprintf(stat_fp, "\n");
    }
    fprintf(stat_fp, "self comm. flit percentage: %.2lf%%\n",
            ((double) num_self_inj_flit / eff_num_flit_inj) * 100.0);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "per-source packet latency (cycle):\n");
    for (unsigned int i=0; i<g_sim.m_pkt_T_t_src_tab_vec.size(); i++)
        fprintf(stat_fp, " C%02d:  avg %.2lf stddev %.2lf min %.0lf max %4.0lf cnt %ld\n", i,
                (g_sim.m_pkt_T_t_src_tab_vec[i])->mean(),
                (g_sim.m_pkt_T_t_src_tab_vec[i])->stddev(),
                (g_sim.m_pkt_T_t_src_tab_vec[i])->min(),
                (g_sim.m_pkt_T_t_src_tab_vec[i])->max(),
                (g_sim.m_pkt_T_t_src_tab_vec[i])->cnt());
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "per-destination packet latency (cycle):\n");
    for (unsigned int i=0; i<g_sim.m_pkt_T_t_dest_tab_vec.size(); i++)
        fprintf(stat_fp, " C%02d:  avg %.2lf stddev %.2lf min %.0lf max %4.0lf cnt %ld\n", i,
                (g_sim.m_pkt_T_t_dest_tab_vec[i])->mean(),
                (g_sim.m_pkt_T_t_dest_tab_vec[i])->stddev(),
                (g_sim.m_pkt_T_t_dest_tab_vec[i])->min(),
                (g_sim.m_pkt_T_t_dest_tab_vec[i])->max(),
                (g_sim.m_pkt_T_t_dest_tab_vec[i])->cnt());
    fprintf(stat_fp, "\n");

#if 0
    // Too long output
    fprintf(stat_fp, "Per-flow Packet Latency (cycle):\n");
    for (unsigned int s=0; s<g_sim.m_pkt_T_t_c2c_tab_vec.size(); s++)
    for (unsigned int d=0; d<g_sim.m_pkt_T_t_c2c_tab_vec[s].size(); d++) {
        fprintf(stat_fp, " C%02d->C%02d:  avg %.2lf stddev %.2lf min %.0lf max %4.0lf cnt %ld\n",
                s, d,
                (g_sim.m_pkt_T_t_c2c_tab_vec[s][d])->mean(),
                (g_sim.m_pkt_T_t_c2c_tab_vec[s][d])->stddev(),
                (g_sim.m_pkt_T_t_c2c_tab_vec[s][d])->min(),
                (g_sim.m_pkt_T_t_c2c_tab_vec[s][d])->max(),
                (g_sim.m_pkt_T_t_c2c_tab_vec[s][d])->cnt());
    }
    fprintf(stat_fp, "\n");
#endif


    // CAM stats
    if (g_CamManager)
        g_CamManager->print_file(stat_fp);

    // pipeline bypass
    fprintf(stat_fp, "router pipeline bypassing: #bypass #pass bypass_ratio\n");
    unsigned long long num_total_pass = 0;
    unsigned long long num_total_bypass = 0;
    for (int i=0; i<g_cfg.router_num; i++) {
        unsigned long long num_bypass = 0;
        unsigned long long num_pass = g_Router_vec[i]->m_num_flit_inj_from_core + g_Router_vec[i]->m_num_flit_inj_from_router;
        // fprintf(stat_fp, " R%02d: ", i);
        for (int in_pc=0; in_pc<g_Router_vec[i]->num_pc(); in_pc++) {
            // fprintf(stat_fp, "PC%d: ", in_pc);
            for (int in_vc=0; in_vc<g_Router_vec[i]->num_vc(); in_vc++) {
                num_bypass += g_Router_vec[i]->numPipelineBypass(in_pc, in_vc);
                // fprintf(stat_fp, "%lld ", g_Router_vec[i]->numPipelineBypass(in_pc, in_vc));
            }
        }
        // fprintf(stat_fp, "\n");
        fprintf(stat_fp, " R%02d: %lld %lld %.4lf\n", i, num_bypass, num_pass, ((double) num_bypass)/num_pass);
        num_total_pass += num_pass;
        num_total_bypass += num_bypass;
    }
    fprintf(stat_fp, "bypass-pipeline-overall: %lld %lld %.4lf\n", num_total_bypass, num_total_pass, ((double) num_total_bypass)/num_total_pass);
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "------------- simulation result (END) --------------\n");
    fflush(stat_fp);
}

void calc_network_energy(double & total_dyn_energy_buffer,
                         double & total_dyn_energy_vc_arb,
                         double & total_dyn_energy_sw_arb,
                         double & total_dyn_energy_xbar,
                         double & total_dyn_energy_router,
                         double & total_dyn_energy_link,
                         double & total_dyn_energy_network,
                         double & total_stat_energy_buffer,
                         double & total_stat_energy_vc_arb,
                         double & total_stat_energy_sw_arb,
                         double & total_stat_energy_xbar,
                         double & total_stat_energy_router,
                         double & total_stat_energy_link,
                         double & total_stat_energy_network,
                         vector< double > & router_dyn_energy_vec,
                         vector< double > & router_stat_energy_vec,
                         double num_cycles,
                         bool is_profile)
{
    for (int i=0; i<g_cfg.router_num; i++) {
        double router_dyn_energy_buffer;
        double router_dyn_energy_vc_arb;
        double router_dyn_energy_sw_arb;
        double router_dyn_energy_xbar;
        double link_dyn_energy;
        double router_stat_energy_buffer;
        double router_stat_energy_vc_arb;
        double router_stat_energy_sw_arb;
        double router_stat_energy_xbar;
        double link_stat_energy;

        Router* p_router = g_Router_vec[i];
        RouterPower* p_router_power = is_profile ? p_router->m_power_tmpl_profile : p_router->m_power_tmpl;
        assert(p_router_power);

        // dynamic energy of router
        router_dyn_energy_buffer = p_router_power->dynamicE_buffer();
        router_dyn_energy_vc_arb = p_router_power->dynamicE_vc_arb();
        router_dyn_energy_sw_arb = p_router_power->dynamicE_sw_arb();
        router_dyn_energy_xbar = p_router_power->dynamicE_xbar();
        router_dyn_energy_vec[i] = router_dyn_energy_buffer + router_dyn_energy_vc_arb +
                               router_dyn_energy_sw_arb + router_dyn_energy_xbar;

        // dynamic energy of link
        link_dyn_energy = p_router_power->dynamicE_all_link();

        // dynamic energy of network
        total_dyn_energy_buffer += router_dyn_energy_buffer;
        total_dyn_energy_vc_arb += router_dyn_energy_vc_arb;
        total_dyn_energy_sw_arb += router_dyn_energy_sw_arb;
        total_dyn_energy_xbar += router_dyn_energy_xbar;
        total_dyn_energy_link += link_dyn_energy;
        total_dyn_energy_network += router_dyn_energy_vec[i] + link_dyn_energy;

        // static(leakage) energy of router
        router_stat_energy_buffer = p_router_power->staticE_buffer() * num_cycles;
        router_stat_energy_vc_arb = p_router_power->staticE_vc_arb() * num_cycles;
        router_stat_energy_sw_arb = (p_router_power->staticE_xbar_v1_arb() + p_router_power->staticE_xbar_p1_arb()) * num_cycles;
        router_stat_energy_xbar = p_router_power->staticE_xbar() * num_cycles;
        router_stat_energy_vec[i] = router_stat_energy_buffer + router_stat_energy_vc_arb +
                                    router_stat_energy_sw_arb + router_stat_energy_xbar;

        // static(leakage) energy of link
        link_stat_energy = 0.0;
        for (int out_pc=0; out_pc<g_Router_vec[i]->num_pc(); out_pc++) {
            if (p_router->getLink(out_pc).m_valid) {
                link_stat_energy += p_router_power->getLinkPower(out_pc)->staticE();
            }
        }
        link_stat_energy *= num_cycles;

        // static energy breakdown of router
        total_stat_energy_buffer += router_stat_energy_buffer;
        total_stat_energy_vc_arb += router_stat_energy_vc_arb;
        total_stat_energy_sw_arb += router_stat_energy_sw_arb;
        total_stat_energy_xbar += router_stat_energy_xbar;

        // static energy of network
        total_stat_energy_router += router_stat_energy_vec[i];
        total_stat_energy_link += link_stat_energy;
        total_stat_energy_network += router_stat_energy_vec[i] + link_stat_energy;
    }

    total_dyn_energy_router = StatSum(router_dyn_energy_vec);
}
