////////////////////////////////////////////////////////////////////////////////
// CSIM process creation functions
void process_main();
void process_control_sim();
void process_sim_progress();
void process_router(Router* p_router);
void process_gen_synth_traffic(int core_id);
void process_parse_trace();
void process_NI_input(NIInput* p_ni_input, int vc);
void process_NI_output(NIOutput* p_ni_output);
#ifdef LINK_DVS
void process_link_dvs_set();
void process_link_dvs_link_speedup();
void process_link_dvs_link_slowdown();
#endif
void process_profile_cycle();
void process_profile_instr();

// others
void print_sim_progress();
void take_network_snapshot(FILE* fp);
////////////////////////////////////////////////////////////////////////////////
