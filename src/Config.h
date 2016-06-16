#ifndef _CONFIG_H_
#define _CONFIG_H_

// Configuration parameters

class Config {
public:

    // interconnection network
    int net_topology;		// topology
    int net_routing;		// routing algorithm
    int net_fattree_way;	// fat tree
    int net_networks;		// #networks: DMesh(=2)

    int flit_sz_byte;		// flit size in byte
    int flit_sz_64bit_multiple; // multiple of 64-bit

    // link
    double link_length;		// link length (mm)
    int link_width;		// link width (#wires = #bits)
    int link_latency;		// link latency (cycles)
    double link_voltage;	// link voltage
    int link_power_model;	// link power model
    int link_wire_type;		// wire type for link
    bool link_wire_pipelining;	// wire pipelining for multi-cycle link delay

    // router
    int router_num;		// #routers
    int router_num_pc;		// #PCs
    int router_num_vc;		// #VCs
    int router_inbuf_depth;	// flit input buffer depth
    bool router_lookahead_RC;	// lookahead routing
    bool router_bypass_IB;	// input buffer bypass
    bool router_spec_VA;	// speculative VC allocation 
    bool router_spec_SA;	// speculative switch allocation
    int router_extra_pipeline_depth;	// extra pipeline depth
    int router_sa_v1_type;	// V:1 SW allocator type
    int router_sa_p1_type;	// P:1 SW allocator type
    int router_power_model;	// router power model
    int router_buffer_type;     // buffer type
    int router_num_rsv_credit;  // #reserved credits for each PC
    int router_num_pipelines;	// mininum router pipelines
    bool router_tunnel;
    int router_tunnel_type;

    // NI
    int NIin_pktbuf_depth;	// packet buffer depth for input NI
    int NIin_type;
    bool NI_port_mux;		// injection/ejection port multiplexing

    // core
    int core_num;		// #cores
    int core_num_NIs;		// #attached NI's
    int core_num_tile_tiledCMP;	// #tiles for tiled CMP
    int core_num_mem_tiledCMP;	// #memory controlers for tiled CMP

    // off-chip memory
    int mem_access_latency;
    int mem_access_pipelined_latency;

    int chip_tech_nano;		// technology (DRAM pitch)
    double chip_freq;		// clock frequency
    double chip_voltage;	// router/link supply voltage

    // average switching activity for energy consumption
    double power_router_avf;	// only for RouterPowerStats model
    double power_link_avf;	// for LinkPowerRepeated model

    // simulation
    unsigned int sim_end_cond;		// simulation end condition
    unsigned int sim_num_inj_pkt;	// #packets for injections
    unsigned int sim_num_ejt_pkt_4warmup;	// #packets to reach warm-up state
    unsigned int sim_num_ejt_pkt;	// #packets to end simulation
    double sim_clk_start;		// clock for simulation start
    double sim_clk_end;			// clock for simulation termination

    bool sim_show_progress;		// show simulation progress
    int sim_progress_interval;		// simulation progress interval (#cycles)

    /////////////////////////////////////////////////////////////////////
    // profile
    bool profile_interval_cycle;
    double profile_interval;		// profile interval (#cycles if profile_interval_cycle=true)
    					// profile interval (#instrs if profile_interval_cycle=false)

    // performance profile (periodic report)
    bool profile_perf;			// performance profiling ? 
    string profile_perf_file_name;	// performance profile file name
    FILE* profile_perf_fp;		// performance profile file pointer

    // power profile (periodic report)
    bool profile_power;			// power profiling ?
    string profile_power_file_name;	// power profile file name
    FILE* profile_power_fp;		// power profile file pointer
    /////////////////////////////////////////////////////////////////////

    // simulation result file
    string out_file_name;
    FILE* out_file_fp;

#ifdef LINK_DVS
    int link_dvs_method;
    double link_dvs_interval;		// check DVS possibility every this interval
    double link_dvs_voltage_transit_delay;	// voltage transition delay
    double link_dvs_freq_transit_delay;		// frequency transition delay
    string link_dvs_rate_file_dir;		// rate file directory
#endif

    // injection throttling
    bool throttle_global_injection;
    bool throttle_local_injection;
    bool throttle_drop;	// if false, slowdown is applied.
    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////
    // workload
    int wkld_type;	                // workload type

    // synthetic workload
    int wkld_synth_spatial;		// spatial pattern
    double wkld_synth_load;		// injection load (flit/cycle/node)
    bool wkld_synth_ss;	                // self-similar traffic
    int wkld_synth_num_flits_pkt;	// #flits per packet
    string wkld_synth_matrix_filename;
    bool wkld_synth_bimodal;		// bimodal traffic
                                        // 1-flit & n-flit (n=wkld_synth_num_flits_pkt)
    double wkld_synth_bimodal_single_pkt_rate;
    double wkld_synth_multicast_ratio;
    int wkld_synth_multicast_destnum;

    // real workload from traces
    string wkld_trace_dir_name; 	// trace file directory
    string wkld_trace_benchmark_name;
    double wkld_trace_skip_cycles;
    unsigned long long wkld_trace_skip_instrs;
    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////
    // CAM
    int cam_data_manage;
    int cam_repl_policy;
    int cam_LFU_saturation;

    bool cam_data_enable;
    int cam_data_blk_byte;
    int cam_data_num_interleaved_tab;
    int cam_data_en_latency;
    int cam_data_en_num_sets;
    int cam_data_de_latency;
    int cam_data_de_num_sets;

    int cam_VLB_num_sets;	// VLB
    int cam_VLB_LFU_saturation;	// threshold for moving entry from VLB to CAM

    bool cam_addr_enable;
    int cam_addr_blk_byte;
    int cam_addr_en_latency;
    int cam_addr_de_latency;
    int cam_addr_num_sets;

    bool cam_streamline;	// streamlined flit injection with cam operations 

    // dynamic control of encoding
    bool cam_dyn_control;	// status for dynamic management
    int cam_dyn_control_window;	// moving window
    //////////////////////////////////////////////////////////////////
};

#endif // #ifndef _CONFIG_H_
