#ifndef _PROFILE_H_

void profile_set_start_clk();

// performance
void profile_perf_header_print();
void profile_perf_print();
void profile_perf_reset();

// power
void profile_power_header_print();
void profile_power_print();
void profile_power_reset();

#endif
