#include "noc.h"
#include "main.h"

int is_number(char* p)
{
    char* pp = p;

    if (*pp == '-')
        pp++;

    while (*pp) {
        if (!isdigit(*pp))
            return 0;
        pp++;
    }

    return 1;
}

int is_real_number(char* p)
{
    char* pp = p;

    if (*pp == '-')
        pp++;

    while (*pp) {
        if (!isdigit(*pp) && *pp != '.')
            return 0;
        pp++;
    }

    return 1;
}

// DESCRIPTION
//        Parse command arguments.
// INPUT PARAMETER
//        argc:        number of command arguments
//        argv:        array of command arguments
void parse_options(int argc, char** argv)
{
    int n = 1;
    int follow = 0;

    while (n<argc) {
        if (follow) {
            follow = 0;

            char* param_name = argv[n-1];
            char* param_value = argv[n];

            // workload
            if (!strcasecmp(param_name, "-WKLD:TYPE")) {
                if (!strcasecmp(param_value, "SYNTH")) {
                    g_cfg.wkld_type = WORKLOAD_SYNTH_SPATIAL;
                } else if (!strcasecmp(param_value, "TRAFFIC_MATRIX")) {
                    g_cfg.wkld_type = WORKLOAD_SYNTH_TRAFFIC_MATRIX;
                } else if (!strcasecmp(param_value, "TRIPS")) {
                    g_cfg.wkld_type = WORKLOAD_TRIPS_TRACE;
                } else if (!strcasecmp(param_value, "TILED_CMP")) {
                    g_cfg.wkld_type = WORKLOAD_TILED_CMP_TRACE;
                } else if (!strcasecmp(param_value, "TILED_CMP_VALUE")) {
                    g_cfg.wkld_type = WORKLOAD_TILED_CMP_VALUE_TRACE;
                } else if (!strcasecmp(param_value, "SNUCA_CMP_VALUE")) {
                    g_cfg.wkld_type = WORKLOAD_SNUCA_CMP_VALUE_TRACE;
                } else {
                    fprintf(stdout, "invalid workload type '%s'\n", param_value);
                    assert(0);
                }
            } else if (!strcasecmp(param_name, "-WKLD:SYNTH:SPATIAL")) {
                if (!strcasecmp(param_value, "UR")) {
                    g_cfg.wkld_synth_spatial = WORKLOAD_SYNTH_SP_UR;
                } else if (!strcasecmp(param_value, "NN")) {
                    g_cfg.wkld_synth_spatial = WORKLOAD_SYNTH_SP_NN;
                } else if (!strcasecmp(param_value, "BC")) {
                    g_cfg.wkld_synth_spatial = WORKLOAD_SYNTH_SP_BC;
                } else if (!strcasecmp(param_value, "TP")) {
                    g_cfg.wkld_synth_spatial = WORKLOAD_SYNTH_SP_TP;
                } else if (!strcasecmp(param_value, "TOR")) {
                    g_cfg.wkld_synth_spatial = WORKLOAD_SYNTH_SP_TOR;
                } else {
                    fprintf(stdout, "invalid synthetic traffic pattern '%s'\n", param_value);
                    assert(0);
                }
            } else if (!strcasecmp(param_name, "-WKLD:SYNTH:LOAD")) {
                assert(is_real_number(param_value));
                g_cfg.wkld_synth_load = atof(param_value);
                assert(g_cfg.wkld_synth_load >= 0.0 && g_cfg.wkld_synth_load <= 1.0);
            } else if (!strcasecmp(param_name, "-WKLD:SYNTH:FLITS_PER_PKT")) {
                assert(is_number(param_value));
                g_cfg.wkld_synth_num_flits_pkt = atoi(param_value);
            } else if (!strcasecmp(param_name, "-WKLD:SYNTH:BIMODAL")) {
                g_cfg.wkld_synth_bimodal = strcasecmp(param_value, "Y") ? false : true;
            } else if (!strcasecmp(param_name, "-WKLD:SYNTH:SS")) {
                g_cfg.wkld_synth_ss = strcasecmp(param_value, "Y") ? false : true;
            } else if (!strcasecmp(param_name, "-WKLD:SYNTH:MULTICAST:RATIO")) {
                g_cfg.wkld_synth_multicast_ratio = atof(param_value);
                assert(g_cfg.wkld_synth_multicast_ratio >= 0.0 && g_cfg.wkld_synth_multicast_ratio <= 1.0);
            } else if (!strcasecmp(param_name, "-WKLD:SYNTH:MULTICAST:DESTNUM")) {
                g_cfg.wkld_synth_multicast_destnum = atoi(param_value);
            } else if (!strcasecmp(param_name, "-WKLD:TRACE:DIR")) {
                g_cfg.wkld_trace_dir_name = param_value;
            } else if (!strcasecmp(param_name, "-WKLD:TRACE:BENCHMARK")) {
                g_cfg.wkld_trace_benchmark_name = param_value;
            } else if (!strcasecmp(param_name, "-WKLD:TRACE:SKIP_MCYCLES")) {
                assert(is_number(param_value));
                g_cfg.wkld_trace_skip_cycles = ((double) atoi(param_value)) * UNIT_MEGA;
            } else if (!strcasecmp(param_name, "-WKLD:TRACE:SKIP_KCYCLES")) {
                assert(is_number(param_value));
                g_cfg.wkld_trace_skip_cycles = ((double) atoi(param_value)) * UNIT_KILO;

            // network
            } else if (!strcasecmp(param_name, "-NET:TOPOLOGY")) {
                if (!strcasecmp(param_value, "MESH")) {
                    g_cfg.net_topology = TOPOLOGY_MESH;
                } else if (!strcasecmp(param_value, "TORUS")) {
                    g_cfg.net_topology = TOPOLOGY_TORUS;
                } else if (!strcasecmp(param_value, "HMESH")) {
                    g_cfg.net_topology = TOPOLOGY_HMESH;
                } else if (!strcasecmp(param_value, "FTREE")) {
                    g_cfg.net_topology = TOPOLOGY_FAT_TREE;
                } else if (!strcasecmp(param_value, "FLBFLY")) {
                    g_cfg.net_topology = TOPOLOGY_FLBFLY;
                } else if (!strcasecmp(param_value, "DMESH")) {
                    g_cfg.net_topology = TOPOLOGY_DMESH;
                } else if (!strcasecmp(param_value, "SNUCA")) {
                    g_cfg.net_topology = TOPOLOGY_SNUCA;
                } else if (!strcasecmp(param_value, "TILED_CMP_MESH")) {
                    g_cfg.net_topology = TOPOLOGY_TILED_CMP_MESH;
                } else {
                    fprintf(stdout, "invalid topology of network\n");
                    assert(0);
                }
            } else if (!strcasecmp(param_name, "-NET:CORE_NUM")) {
                assert(is_number(param_value));
                g_cfg.core_num = atoi(param_value);
                assert(g_cfg.core_num > 0);
            } else if (!strcasecmp(param_name, "-NET:CORE_TILE_NUM")) {
                assert(is_number(param_value));
                g_cfg.core_num_tile_tiledCMP = atoi(param_value);
                assert(g_cfg.core_num_tile_tiledCMP > 0);
            } else if (!strcasecmp(param_name, "-NET:ROUTING")) {
                if (!strcasecmp(param_value, "XY")) {
                    g_cfg.net_routing = ROUTING_XY;
                } else if (!strcasecmp(param_value, "YX")) {
                    g_cfg.net_routing = ROUTING_YX;
                } else if (!strcasecmp(param_value, "MIN_OBLIVIOUS")) {
                    g_cfg.net_routing = ROUTING_MIN_OBLIVIOUS;
                } else if (!strcasecmp(param_value, "MIN_ADAPTIVE_VC")) {
                    g_cfg.net_routing = ROUTING_MIN_ADAPTIVE_VC;
                } else if (!strcasecmp(param_value, "MIN_SCHEDULED")) {
                    g_cfg.net_routing = ROUTING_MIN_SCHEDULED;
                } else if (!strcasecmp(param_value, "HMESH_ASCENT")) {
                    g_cfg.net_routing = ROUTING_HMESH_ASCENT;
                } else if (!strcasecmp(param_value, "HMESH_DESCENT")) {
                    g_cfg.net_routing = ROUTING_HMESH_DESCENT;
                } else if (!strcasecmp(param_value, "TORUS_DALLY")) {
                    g_cfg.net_routing = ROUTING_TORUS_DALLY;
                } else if (!strcasecmp(param_value, "FTREE_ADAPTIVE")) {
                    g_cfg.net_routing = ROUTING_FAT_TREE_ADAPTIVE;
                } else if (!strcasecmp(param_value, "FTREE_RANDOM")) {
                    g_cfg.net_routing = ROUTING_FAT_TREE_RANDOM;
                } else if (!strcasecmp(param_value, "DMESH_XY")) {
                    g_cfg.net_routing = ROUTING_DMESH_XY;
                } else if (!strcasecmp(param_value, "DMESH_YX")) {
                    g_cfg.net_routing = ROUTING_DMESH_YX;
                } else if (!strcasecmp(param_value, "FLBFLY_XY")) {
                    g_cfg.net_routing = ROUTING_FLBFLY_XY;
                } else if (!strcasecmp(param_value, "FLBFLY_YX")) {
                    g_cfg.net_routing = ROUTING_FLBFLY_YX;
                } else {
                    fprintf(stdout, "invalid routing algorithm.\n");
                    assert(0);
                }

            // NI
            } else if (!strcasecmp(param_name, "-NI:INPUT_TYPE")) {
                if (!strcasecmp(param_value, "PER_PC")) {
                    g_cfg.NIin_type = NI_INPUT_TYPE_PER_PC;
                } else if (!strcasecmp(param_value, "PER_VC")) {
                    g_cfg.NIin_type = NI_INPUT_TYPE_PER_VC;
                } else {
                    assert(0);
                }
            } else if (!strcasecmp(param_name, "-NI:BUF_DEPTH")) {
                assert(is_number(param_value));
                g_cfg.NIin_pktbuf_depth = atoi(param_value);
            } else if (!strcasecmp(param_name, "-NI:PORT_MUX")) {
                g_cfg.NI_port_mux = strcasecmp(param_value, "Y") ? false : true;

            // core
            } else if (!strcasecmp(param_name, "-CORE:NUM_NI")) {
                assert(is_number(param_value));
                g_cfg.core_num_NIs = atoi(param_value);

            // router
            } else if (!strcasecmp(param_name, "-ROUTER:PC")) {
                assert(is_number(param_value));
                g_cfg.router_num_pc = atoi(param_value);
            } else if (!strcasecmp(param_name, "-ROUTER:VC")) {
                assert(is_number(param_value));
                g_cfg.router_num_vc = atoi(param_value);
            } else if (!strcasecmp(param_name, "-ROUTER:INBUF_DEPTH")) {
                assert(is_number(param_value));
                g_cfg.router_inbuf_depth = atoi(param_value);
            } else if (!strcasecmp(param_name, "-ROUTER:SPEC_SA")) {
                g_cfg.router_spec_SA = strcasecmp(param_value, "Y") ? false : true;
            } else if (!strcasecmp(param_name, "-ROUTER:LOOKAHEAD_RC")) {
                g_cfg.router_lookahead_RC = strcasecmp(param_value, "Y") ? false : true;
            } else if (!strcasecmp(param_name, "-ROUTER:EXTRA_PIPE_DEPTH")) {
                assert(is_number(param_value));
                g_cfg.router_extra_pipeline_depth = atoi(param_value);
            } else if (!strcasecmp(param_name, "-ROUTER:SA_TYPE")) {
                if (!strcasecmp(param_value, "MAXIMAL")) {
                    g_cfg.router_sa_v1_type = SW_ALLOC_MAXIMAL;
                    g_cfg.router_sa_p1_type = SW_ALLOC_MAXIMAL;
                    assert(0);
                } else if (!strcasecmp(param_value, "LRS")) {
                    g_cfg.router_sa_v1_type = SW_ALLOC_LRS;
                    g_cfg.router_sa_p1_type = SW_ALLOC_LRS;
                } else if (!strcasecmp(param_value, "RR")) {
                    g_cfg.router_sa_v1_type = SW_ALLOC_RR;
                    g_cfg.router_sa_p1_type = SW_ALLOC_RR;
		// add more combinations
                } else {
                    assert(0);
                }
            } else if (!strcasecmp(param_name, "-ROUTER:POWER_MODEL")) {
                if (!strcasecmp(param_value, "ORION_CALL")) {
                    g_cfg.router_power_model = ROUTER_POWER_MODEL_ORION_CALL;
                } else if (!strcasecmp(param_value, "STATS")) {
                    g_cfg.router_power_model = ROUTER_POWER_MODEL_STATS;
                } else {
                    assert(0);
                }
            } else if (!strcasecmp(param_name, "-ROUTER:BUFFER")) {
                if (!strcasecmp(param_value, "SAMQ")) {
                    g_cfg.router_buffer_type = ROUTER_BUFFER_SAMQ;
                } else if (!strcasecmp(param_value, "DAMQ_P")) {
                    g_cfg.router_buffer_type = ROUTER_BUFFER_DAMQ_P;
                    g_cfg.router_num_rsv_credit = 1;
                } else if (!strcasecmp(param_value, "DAMQ_R")) {
                    g_cfg.router_buffer_type = ROUTER_BUFFER_DAMQ_R;
                    g_cfg.router_num_rsv_credit = 1;
                } else {
                    assert(0);
                }
            } else if (!strcasecmp(param_name, "-ROUTER:TUNNEL")) {
                if (!strcasecmp(param_value, "FLOW")) {
                    g_cfg.router_tunnel_type = TUNNELING_PER_FLOW;
                    g_cfg.router_tunnel = true;
                } else if (!strcasecmp(param_value, "DEST")) {
                    g_cfg.router_tunnel_type = TUNNELING_PER_DEST;
                    g_cfg.router_tunnel = true;
                } else if (!strcasecmp(param_value, "OUTPORT")) {
                    g_cfg.router_tunnel_type = TUNNELING_PER_OUTPORT;
                    g_cfg.router_tunnel = true;
                } else {
                    assert(0);
                }

            // link
            } else if (!strcasecmp(param_name, "-LINK:WIDTH")) {
                assert(is_number(param_value));
                g_cfg.link_width = atoi(param_value);
            } else if (!strcasecmp(param_name, "-LINK:LENGTH")) {
                assert(is_real_number(param_value));
                g_cfg.link_length = atof(param_value);
            } else if (!strcasecmp(param_name, "-LINK:LATENCY")) {
                assert(is_number(param_value));
                g_cfg.link_latency = atoi(param_value);
            } else if (!strcasecmp(param_name, "-LINK:POWER_MODEL")) {
                if (!strcasecmp(param_value, "ORION")) {
                    g_cfg.link_power_model = LINK_POWER_MODEL_ORION;
                } else if (!strcasecmp(param_value, "OPT_REPEATED")) {
                    g_cfg.link_power_model = LINK_POWER_MODEL_DELAY_OPT_REPEATED;
                } else if (!strcasecmp(param_value, "OPT_REPEATED_VALUE")) {
                    g_cfg.link_power_model = LINK_POWER_MODEL_DELAY_OPT_REPEATED_VALUE;
                } else {
                    assert(0);
                }
#ifdef LINK_DVS
            } else if (!strcasecmp(param_name, "-LINK:DVS:METHOD")) {
                if (!strcasecmp(param_value, "NO")) {
                    g_cfg.link_dvs_method = LINK_DVS_NODVS;
                } else if (!strcasecmp(param_value, "HISTORY")) {
                    g_cfg.link_dvs_method = LINK_DVS_HISTORY;
                } else if (!strcasecmp(param_value, "FLIT_RATE_PREDICT")) {
                    g_cfg.link_dvs_method = LINK_DVS_FLIT_RATE_PREDICT;
                } else {
                    assert(0);
                }
            } else if (!strcasecmp(param_name, "-LINK:DVS:INTERVAL")) {
                g_cfg.link_dvs_interval = atof(param_value);
            } else if (!strcasecmp(param_name, "-LINK:DVS:VOLTAGE_DELAY")) {
                g_cfg.link_dvs_voltage_transit_delay = atof(param_value);
            } else if (!strcasecmp(param_name, "-LINK:DVS:FREQ_DELAY")) {
                g_cfg.link_dvs_freq_transit_delay = atof(param_value);
            } else if (!strcasecmp(param_name, "-LINK:DVS:RATE_FILE_DIR")) {
                g_cfg.link_dvs_rate_file_dir = param_value;
#endif

            // simulation
            } else if (!strcasecmp(param_name, "-SIM:OUTFILE")) {
                g_cfg.out_file_name = param_value;
            } else if (!strcasecmp(param_name, "-SIM:POWER_PFILE")) {
                g_cfg.profile_power_file_name = param_value;
                g_cfg.profile_power = true;
            } else if (!strcasecmp(param_name, "-SIM:PERF_PFILE")) {
                g_cfg.profile_perf_file_name = param_value;
                g_cfg.profile_perf = true;
            } else if (!strcasecmp(param_name, "-SIM:WARM_CYCLE")) {
                assert(is_number(param_value));
                g_cfg.sim_clk_start = (double) atol(param_value);
            } else if (!strcasecmp(param_name, "-SIM:END_CYCLE")) {
                assert(is_number(param_value));
                g_cfg.sim_clk_end = (double) atol(param_value);
                // negative value is used for trace simulation to process all the traces
                if (g_cfg.sim_clk_end < 0.0)
                    g_cfg.sim_clk_end = 1.0 * UNIT_TERA;
                g_cfg.sim_end_cond = SIM_END_BY_CYCLE;
            } else if (!strcasecmp(param_name, "-SIM:PROFILE_INTERVAL")) {
                assert(is_number(param_value));
                g_cfg.profile_interval = (double) atol(param_value);
            } else if (!strcasecmp(param_name, "-SIM:PROGRESS")) {
                g_cfg.sim_show_progress = strcasecmp(param_value, "Y") ? false : true;
            } else if (!strcasecmp(param_name, "-SIM:PROGRESS_INTERVAL")) {
                assert(is_number(param_value));
                g_cfg.sim_progress_interval = atoi(param_value);

            // etc
            } else if (!strcasecmp(param_name, "-ETC:INJECT_THROTTLE")) {
                if (!strcasecmp(param_value, "DROP")) {
                    g_cfg.throttle_global_injection = true;
                    g_cfg.throttle_local_injection = true;
                    g_cfg.throttle_drop = true;
                } else if (!strcasecmp(param_value, "SLOWDOWN")) {
                    g_cfg.throttle_global_injection = true;
                    g_cfg.throttle_local_injection = true;
                    g_cfg.throttle_drop = false;
                } else {
                    g_cfg.throttle_global_injection = false;
                    g_cfg.throttle_local_injection = false;
                }

            // CAM (packet compression)
            } else if (!strcasecmp(param_name, "-CAM:DATA_ENABLE")) {
                g_cfg.cam_data_enable = strcasecmp(param_value, "Y") ? false : true;
            } else if (!strcasecmp(param_name, "-CAM:DATA_MANAGE")) {
                if (!strcasecmp(param_value, "PER_CORE")) {
                    g_cfg.cam_data_manage = CAM_MT_PRIVATE_PER_CORE;
                } else if (!strcasecmp(param_value, "PER_ROUTER")) {
                    g_cfg.cam_data_manage = CAM_MT_PRIVATE_PER_ROUTER;
                } else if (!strcasecmp(param_value, "PER_ROUTER_SHARED")) {
                    g_cfg.cam_data_manage = CAM_MT_SHARED_PER_ROUTER;
                } else {
                    assert(0);
                }
            } else if (!strcasecmp(param_name, "-CAM:REPL")) {
                if (!strcasecmp(param_value, "LRU")) {
                    g_cfg.cam_repl_policy = CAM_REPL_LRU;
                } else if (!strcasecmp(param_value, "LFU")) {
                    g_cfg.cam_repl_policy = CAM_REPL_LFU;
                } else {
                    assert(0);
                }
            } else if (!strcasecmp(param_name, "-CAM:DATA_BLK_BYTE")) {
                assert(is_number(param_value));
                g_cfg.cam_data_blk_byte = atoi(param_value);
            } else if (!strcasecmp(param_name, "-CAM:INTERLEAVE_TAB")) {
                assert(is_number(param_value));
                g_cfg.cam_data_num_interleaved_tab = atoi(param_value);
            } else if (!strcasecmp(param_name, "-CAM:DATA_EN_LATENCY")) {
                assert(is_number(param_value));
                g_cfg.cam_data_en_latency = atoi(param_value);
            } else if (!strcasecmp(param_name, "-CAM:DATA_EN_NUM_SETS")) {
                assert(is_number(param_value));
                g_cfg.cam_data_en_num_sets = atoi(param_value);
            } else if (!strcasecmp(param_name, "-CAM:DATA_DE_LATENCY")) {
                assert(is_number(param_value));
                g_cfg.cam_data_de_latency = atoi(param_value);
            } else if (!strcasecmp(param_name, "-CAM:DATA_DE_NUM_SETS")) {
                assert(is_number(param_value));
                g_cfg.cam_data_de_num_sets = atoi(param_value);
            } else if (!strcasecmp(param_name, "-CAM:VLB_NUM_SETS")) {
                assert(is_number(param_value));
                g_cfg.cam_VLB_num_sets = atoi(param_value);
            } else if (!strcasecmp(param_name, "-CAM:STREAMLINE")) {
                g_cfg.cam_streamline = strcasecmp(param_value, "Y") ? false : true;
            } else if (!strcasecmp(param_name, "-CAM:DYN_CONTROL")) {
                g_cfg.cam_dyn_control = strcasecmp(param_value, "Y") ? false : true;
            } else if (!strcasecmp(param_name, "-CAM:DYN_CONTROL_WINDOW")) {
                g_cfg.cam_dyn_control_window = atoi(param_value);
            }
        } else {
            follow = 0;

            if (!strcasecmp(argv[n], "-WKLD:TYPE")
                || !strcasecmp(argv[n], "-WKLD:SYNTH:LOAD")
                || !strcasecmp(argv[n], "-WKLD:SYNTH:FLITS_PER_PKT")
                || !strcasecmp(argv[n], "-WKLD:SYNTH:BIMODAL")
                || !strcasecmp(argv[n], "-WKLD:SYNTH:SS")
                || !strcasecmp(argv[n], "-WKLD:SYNTH:SPATIAL")
                || !strcasecmp(argv[n], "-WKLD:SYNTH:MULTICAST:RATIO")
                || !strcasecmp(argv[n], "-WKLD:SYNTH:MULTICAST:DESTNUM")
                || !strcasecmp(argv[n], "-WKLD:TRACE:DIR")
                || !strcasecmp(argv[n], "-WKLD:TRACE:BENCHMARK")
                || !strcasecmp(argv[n], "-WKLD:TRACE:SKIP_MCYCLES")
                || !strcasecmp(argv[n], "-WKLD:TRACE:SKIP_KCYCLES")

                || !strcasecmp(argv[n], "-NET:TOPOLOGY")
                || !strcasecmp(argv[n], "-NET:CORE_NUM")
                || !strcasecmp(argv[n], "-NET:ROUTING")
                || !strcasecmp(argv[n], "-NET:CORE_TILE_NUM")

                || !strcasecmp(argv[n], "-NI:INPUT_TYPE")
                || !strcasecmp(argv[n], "-NI:BUF_DEPTH")
                || !strcasecmp(argv[n], "-NI:PORT_MUX")

                || !strcasecmp(argv[n], "-CORE:NUM_NI")

                || !strcasecmp(argv[n], "-ROUTER:PC")
                || !strcasecmp(argv[n], "-ROUTER:VC")
                || !strcasecmp(argv[n], "-ROUTER:INBUF_DEPTH")
                || !strcasecmp(argv[n], "-ROUTER:SPEC_SA")
                || !strcasecmp(argv[n], "-ROUTER:LOOKAHEAD_RC")
                || !strcasecmp(argv[n], "-ROUTER:EXTRA_PIPE_DEPTH")
                || !strcasecmp(argv[n], "-ROUTER:SA_TYPE")
                || !strcasecmp(argv[n], "-ROUTER:POWER_MODEL")
                || !strcasecmp(argv[n], "-ROUTER:BUFFER")
                || !strcasecmp(argv[n], "-ROUTER:TUNNEL")

                || !strcasecmp(argv[n], "-LINK:WIDTH")
                || !strcasecmp(argv[n], "-LINK:LENGTH")
                || !strcasecmp(argv[n], "-LINK:LATENCY")
                || !strcasecmp(argv[n], "-LINK:POWER_MODEL")
#ifdef LINK_DVS
                || !strcasecmp(argv[n], "-LINK:DVS:METHOD")
                || !strcasecmp(argv[n], "-LINK:DVS:INTERVAL")
                || !strcasecmp(argv[n], "-LINK:DVS:VOLTAGE_DELAY")
                || !strcasecmp(argv[n], "-LINK:DVS:FREQ_DELAY")
                || !strcasecmp(argv[n], "-LINK:DVS:RATE_FILE_DIR")
#endif
                || !strcasecmp(argv[n], "-SIM:OUTFILE")
                || !strcasecmp(argv[n], "-SIM:POWER_PFILE")
                || !strcasecmp(argv[n], "-SIM:PERF_PFILE")
                || !strcasecmp(argv[n], "-SIM:WARM_CYCLE")
                || !strcasecmp(argv[n], "-SIM:END_CYCLE")
                || !strcasecmp(argv[n], "-SIM:PROFILE_INTERVAL")
                || !strcasecmp(argv[n], "-SIM:PROGRESS")
                || !strcasecmp(argv[n], "-SIM:PROGRESS_INTERVAL")

                || !strcasecmp(argv[n], "-ETC:INJECT_THROTTLE")

                || !strcasecmp(argv[n], "-CAM:DATA_ENABLE")
                || !strcasecmp(argv[n], "-CAM:REPL")
                || !strcasecmp(argv[n], "-CAM:DATA_MANAGE")
                || !strcasecmp(argv[n], "-CAM:DATA_BLK_BYTE")
                || !strcasecmp(argv[n], "-CAM:INTERLEAVE_TAB")
                || !strcasecmp(argv[n], "-CAM:DATA_EN_LATENCY")
                || !strcasecmp(argv[n], "-CAM:DATA_EN_NUM_SETS")
                || !strcasecmp(argv[n], "-CAM:DATA_DE_LATENCY")
                || !strcasecmp(argv[n], "-CAM:DATA_DE_NUM_SETS")
                || !strcasecmp(argv[n], "-CAM:VLB_NUM_SETS")
                || !strcasecmp(argv[n], "-CAM:STREAMLINE")
                || !strcasecmp(argv[n], "-CAM:DYN_CONTROL")
                || !strcasecmp(argv[n], "-CAM:DYN_CONTROL_WINDOW")
               ) {
                follow = 1;
            } else if (!strcasecmp(argv[n], "-v")) {
                fprintf(stdout, " version %s\n", "0.9");
                exit(0);
            } else if (!strcasecmp(argv[n], "-h")) {
		print_usage(argv[0]);
                exit(0);
            } else if (!strcasecmp(argv[n], "--help")) {
		print_usage(argv[0]);
                exit(0);
            } else {
                fprintf(stdout, "invalid option '%s'\n", argv[n]);
                fprintf(stdout, "Try '%s --help' for more information.\n", argv[0]);
                exit(0);
            }
        }

        n++;
    }
}

void print_usage(char* prog_name)
{
    string usage_str = prog_name;
    usage_str += "\n";
    usage_str += "\t-wkld:type                [synth|trips|tiled_cmp|tiled_cmp_value|tiled_snuca_value]\n";
    usage_str += "\t-wkld:synth:spatial       [ur|nn|bc|tp|tor]\n";
    usage_str += "\t-wkld:synth:load          <applied_load>\n";
    usage_str += "\t-wkld:synth:flits_per_pkt <#flits/packet>\n";
    usage_str += "\t-wkld:synth:bimodal       [Y|N]\n";
    usage_str += "\t-wkld:synth:ss            [Y|N]\n";
    usage_str += "\t-wkld:trace:dir           <trace directory>\n";
    usage_str += "\t-wkld:trace:benchmark     <benchmark name>\n";
    usage_str += "\t-wkld:trace:skip_mcycles  <cycles (1M unit)>\n";
    usage_str += "\t-wkld:trace:skip_kcycles  <cycles (1K unit)>\n";

    usage_str += "\t-net:topology             [mesh|torus|hmesh|ftree|dmesh|flbfly|snuca]\n";
    usage_str += "\t-net:core_num             <#cores>\n";
    usage_str += "\t-net:routing              [xy|yx|min_adaptive_vc|torus_dally|dmesh_xy|dmesh_yx]\n";

    usage_str += "\t-core:ipc                 <#injection PCs>\n";
    usage_str += "\t-core:epc                 <#ejection PCs>\n";

    usage_str += "\t-router:pc                <#PCs>\n";
    usage_str += "\t-router:vc                <#VCs>\n";
    usage_str += "\t-router:inbuf_depth       <input buffer depth>\n";
    usage_str += "\t-router:spec_sa           [Y|N]\n";
    usage_str += "\t-router:lookahead_rc      [Y|N]\n";
    usage_str += "\t-router:extra_pipe_depth  <#stages>\n";
    usage_str += "\t-router:sa_type           [maximal|lrs|rr]\n";
    usage_str += "\t-router:power_model       [orion_call|stats]\n";

    usage_str += "\t-link:width               <#wires>\n";
    usage_str += "\t-link:length              <mm unit>\n";
    usage_str += "\t-link:latency             <cycle>\n";
    usage_str += "\t-link:power_model         [orion|opt_repeated]\n";
#ifdef LINK_DVS
    usage_str += "\t-link:dvs:method          <no|history|flit-rate-predict>\n";
    usage_str += "\t-link:dvs:interval        <cycles>\n";
    usage_str += "\t-link:dvs:voltage_delay   <cycles>\n";
    usage_str += "\t-link:dvs:freq_delay      <cycles>\n";
#endif

    usage_str += "\t-NI:input_type            [per_vc|per_pc]\n";
    usage_str += "\t-NI:buf_depth             <depth>\n";
    usage_str += "\t-NI:port_mux              [Y|N]\n";

    usage_str += "\t-sim:outfile              <output_file_name>\n";
    usage_str += "\t-sim:power_pfile          <profile_file_name>\n";
    usage_str += "\t-sim:temp_pfile           <profile_file_name>\n";
    usage_str += "\t-sim:perf_pfile           <profile_file_name>\n";
    usage_str += "\t-sim:warm_cycle           <cycles for warmup>\n";
    usage_str += "\t-sim:end_cycle            <cycles for simulation>\n";
    usage_str += "\t-sim:profile_interval     <cycles>\n";
    usage_str += "\t-sim:progress             [Y|N]\n";
    usage_str += "\t-sim:progress_interval    <cycles>\n";

    usage_str += "\t-etc:inject_throttle      [Y|N]\n";

    usage_str += "\t-cam:data_enable          [Y|N]\n";
    usage_str += "\t-cam:repl                 [LRU|LFU]\n";
    usage_str += "\t-cam:data_manage          [per_router|per_core|per_router_shared]\n";
    usage_str += "\t-cam:data_blk_byte        <bytes>\n";
    usage_str += "\t-cam:interleave_tab       <#tabs>\n";
    usage_str += "\t-cam:data_en_latency      <cycles>\n";
    usage_str += "\t-cam:data_en_num_sets     <sets>\n";
    usage_str += "\t-cam:data_de_latency      <cycles>\n";
    usage_str += "\t-cam:data_de_num_sets     <sets>\n";
    usage_str += "\t-cam:VLB_num_sets         <sets>\n";
    usage_str += "\t-cam:streamline           [Y|N]\n";
    usage_str += "\t-cam:dyn_control          [Y|N]\n";
    usage_str += "\t-cam:dyn_control_window   <size>\n";

    cout << usage_str;
}
