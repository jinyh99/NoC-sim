#ifndef _LINK_DVS_H_
#define _LINK_DVS_H_

#ifdef LINK_DVS

// #ifdef LINK_DVS_DEBUG

typedef enum {
    DVS_LINK_SPEEDUP,
    DVS_LINK_SLOWDOWN,
    DVS_LINK_NOCHANGE
} dvs_link_speedup_t;

#define SD_FLIT_RATE_PREDICT_DIR_NAME	"/home/simics/msahn/sd-avg-flit-inj-predict"
#define SD_FLIT_RATE_DIR_NAME "/home/yuho/OMP_trace/bin/zInterval-1M"

#define MAX_LINK_DVS_LEVELS     10
extern double g_link_dvs_freq[MAX_LINK_DVS_LEVELS];
extern double g_link_dvs_voltage[MAX_LINK_DVS_LEVELS];

void router_power_link_voltage_transition(double V1, double V2);
double router_power_link_voltage_transition_energy_report();
unsigned long long router_power_link_voltage_transition_count_report();


class LinkDVSer {
public:
    LinkDVSer();
    ~LinkDVSer();

    void init();

public:
    void link_dvs_select_vf();
    void link_dvs_update_voltage_speedup();
    void link_dvs_update_freq_speedup();
    void link_dvs_update_voltage_slowdown();
    void link_dvs_update_freq_slowdown();

    void open_flit_rate_predict_file();
    void close_flit_rate_predict_file();
    void skip_flit_rate_predict_file(int num_lines);
    void read_flit_rate_predict_interval();
    void estimate_link_utilz_from_flit_rate_predict();
    void link_dvs_select_vf_predicted_flit_rate();

    int predict_line_num() const { return m_cur_predict_line_num; };

private:
    vector< vector< double > > m_sd_predict_flit_rate_vec; // [src_router_id][dest_router_id]
    vector< vector< double > > m_predict_link_utilz_vec; // [router_id][out_pc]

    FILE* m_fp_sd_flit_rate_predict;

    // There is a difference between time for voltage change estimation 
    // and time for estimated voltage application. For the time gap,
    // we need to memorize the estimated voltage for application.
    vector< vector< dvs_link_speedup_t > > m_link_dvs_speedup_flag_vec; // [router_id][out_pc]
    vector< vector< int > > m_link_dvs_next_level_vec; // [router_id][out_pc];
    bool m_link_dvs_set_slowdown;
    bool m_link_dvs_set_speedup;

    bool m_use_highest_voltage;

    int m_cur_predict_line_num;

    // statistics
    int m_slowdown_link_count;
    int m_speedup_link_count;
    int m_nochange_link_count;
};
#endif

#endif // #ifndef _LINK_DVS_H_
