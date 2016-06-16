#ifndef _MAIN_H_
#define _MAIN_H_

void config_sim();
void reset_stats();
void print_config(FILE* stat_fp);
void print_stats(FILE* stat_fp);
void calc_network_energy(double & network_dyn_energy_buffer,
                         double & network_dyn_energy_vc_arb,
                         double & network_dyn_energy_sw_arb,
                         double & network_dyn_energy_xbar,
                         double & network_dyn_energy_router,
                         double & network_dyn_energy_link,
                         double & network_dyn_energy,
                         double & network_stat_energy_buffer,
                         double & network_stat_energy_vc_arb,
                         double & network_stat_energy_sw_arb,
                         double & network_stat_energy_xbar,
                         double & network_stat_energy_router,
                         double & network_stat_energy_link,
                         double & network_stat_energy,
                         vector< double > & router_dyn_energy_vec,
                         vector< double > & router_stat_energy_vec,
                         double num_cycles,
                         bool is_profile);

// parse_arg.c
void parse_options(int argc, char** argv);
void print_usage(char* prog_name);

#endif // #ifndef _MAIN_H_
