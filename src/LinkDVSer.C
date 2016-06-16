#include "noc.h"
#include "Router.h"
#include "RouterPower.h"
#include "Routing.h"
#include "LinkDVSer.h"

#ifdef LINK_DVS

LinkDVSer::LinkDVSer()
{
}

LinkDVSer::~LinkDVSer()
{
}

void LinkDVSer::init()
{
   unsigned int router_num = g_Router_vec.size();

   m_sd_predict_flit_rate_vec.resize(router_num);
   m_predict_link_utilz_vec.resize(router_num);
   m_link_dvs_speedup_flag_vec.resize(router_num);
   m_link_dvs_next_level_vec.resize(router_num);
   for (unsigned int r=0; r<router_num; r++) {
      m_sd_predict_flit_rate_vec[r].resize(router_num, 0.0);
      m_predict_link_utilz_vec[r].resize(g_Router_vec[r]->num_pc(), 0.0);
      m_link_dvs_speedup_flag_vec[r].resize(g_Router_vec[r]->num_pc());
      m_link_dvs_next_level_vec[r].resize(g_Router_vec[r]->num_pc());
   }

   m_fp_sd_flit_rate_predict = 0;

   m_link_dvs_set_slowdown = false;
   m_link_dvs_set_speedup = false;

   m_use_highest_voltage = false;

   m_cur_predict_line_num = 0;

   m_slowdown_link_count = 0;
   m_speedup_link_count = 0;
   m_nochange_link_count = 0;
}

void LinkDVSer::link_dvs_select_vf()
{
    m_slowdown_link_count = 0;
    m_speedup_link_count = 0;
    m_nochange_link_count = 0;

    // select the proper frequency for each link utilization
    for (unsigned int n=0; n<g_Router_vec.size(); n++) {
        Router* p_router = g_Router_vec[n];
        for (int pc=0; pc<p_router->num_pc(); pc++) {
            if (p_router->isEjectChannel(pc))
                continue;

            Link& router_link = p_router->getLink(pc);

            if (! router_link.m_valid )	// no link
                continue;

#ifdef LINK_DVS_DEBUG
printf("m_sim_pc_dvs_link_op_vec[%d]=%lld interval=%lf\n", pc, p_router->m_sim_pc_dvs_link_op_vec[pc], cfg_link_dvs_interval);
#endif
            double link_utilz = ((double) p_router->m_sim_pc_dvs_link_op_vec[pc]) / cfg_link_dvs_interval;
            assert(link_utilz >= 0.0 && link_utilz <= 1.0);
            // double weighted_link_utilz = sqrt(link_utilz)link_utilz_weight
            // double weighted_link_utilz = sqrt(sqrt(link_utilz));
            double const link_utilz_weight_const = 0.7;
            double weighted_link_utilz = cbrt(link_utilz/link_utilz_weight_const);
            int next_dvs_level = 0;
            double freq_rate = g_link_dvs_freq[0]/cfg_freq;
            assert(freq_rate <= 1.0);
            // double rate_diff_min = freq_rate - link_utilz;
            double rate_diff_min = freq_rate - weighted_link_utilz;

            for (int lv=1; lv<MAX_LINK_DVS_LEVELS; lv++) {
                freq_rate = g_link_dvs_freq[lv]/cfg_freq;
                assert(freq_rate <= 1.0);
                // double rate_diff = freq_rate - link_utilz;
                double rate_diff = freq_rate - weighted_link_utilz;
                if (rate_diff < 0.0)
                    break;

                if (rate_diff_min > rate_diff) {
                    rate_diff_min = rate_diff;
                    next_dvs_level = lv;
                }
            }

            double curr_link_freq = router_link.dvs_freq;
            double curr_link_voltage = router_link.dvs_voltage;
#ifdef LINK_DVS_DEBUG
printf("LINK_DVS: router=%d link=%d link_utilz=%lg", n, pc, link_utilz);
printf(" curr(F=%.2lf V=%.2lf), next(F=%.2lf V=%.2lf lv=%d)\n",
       curr_dvs_freq/UNIT_GIGA,
       curr_link_voltage,
       g_link_dvs_freq[next_dvs_level]/UNIT_GIGA,
       g_link_dvs_voltage[next_dvs_level],
       next_dvs_level);
#endif
            if (curr_link_voltage != g_link_dvs_voltage[next_dvs_level]) {
                // freq/voltage transition overhead energy
                router_power_link_voltage_transition(curr_link_voltage, g_link_dvs_voltage[next_dvs_level]);
                m_link_dvs_next_level_vec[n][pc] = next_dvs_level;

                if (g_link_dvs_freq[next_dvs_level] > curr_link_freq) {
                    m_link_dvs_speedup_flag_vec[n][pc] = DVS_LINK_SPEEDUP;
                    m_speedup_link_count++;
                } else {
                    assert(g_link_dvs_freq[next_dvs_level] < curr_link_freq);
                    m_link_dvs_speedup_flag_vec[n][pc] = DVS_LINK_SLOWDOWN;
                    m_slowdown_link_count++;
                }
            } else {
                m_link_dvs_speedup_flag_vec[n][pc] = DVS_LINK_NOCHANGE;
                m_nochange_link_count++;
            }
        }
    }
#ifdef LINK_DVS_DEBUG
printf("dvs set clk=%.0lf speedup=%d slowdown=%d nochange=%d\n",
       simtime(), 
       m_speedup_link_count, m_slowdown_link_count, m_nochange_link_count);
#endif

    // reset link utilization counter
    for (unsigned int n=0; n<cfg_router_num; n++) {
        Router* p_router = g_Router_vec[n];
        for (unsigned int pc=0; pc<p_router->num_pc(); pc++) {
            p_router->m_sim_pc_dvs_link_op_vec[pc] = 0;
        }
    }

    m_link_dvs_set_slowdown = true;
    m_link_dvs_set_speedup = true;
}

void LinkDVSer::link_dvs_update_voltage_speedup()
{
    if (!m_link_dvs_set_speedup)
        return;

    if (m_speedup_link_count == 0)
        return;

    for (unsigned int n=0; n<g_Router_vec.size(); n++) {
        Router* p_router = g_Router_vec[n];
        for (int pc=0; pc<p_router->num_pc(); pc++) {
            if (p_router->isEjectChannel(pc))
                continue;

            Link& router_link = p_router->getLink(pc);	// link for outpc

            if (! router_link.valid )	// no link
                continue;

            if (m_link_dvs_speedup_flag_vec[n][pc] != DVS_LINK_SPEEDUP)
                continue;

            int next_dvs_level = m_link_dvs_next_level_vec[n][pc];

            // set next voltage
            router_link.dvs_voltage = g_link_dvs_voltage[next_dvs_level];
        }
    }
}

void LinkDVSer::link_dvs_update_freq_speedup()
{
    if (!m_link_dvs_set_speedup)
        return;

    if (m_speedup_link_count == 0)
        return;

    for (unsigned int n=0; n<g_Router_vec.size(); n++) {
        Router* p_router = g_Router_vec[n];
        for (int pc=0; pc<p_router->num_pc(); pc++) {
            if (p_router->isEjectChannel(pc))
                continue;

            Link& router_link = p_router->getLink(pc);	// link for outpc

            if (! router_link.valid )	// no link
                continue;

            if (m_link_dvs_speedup_flag_vec[n][pc] != DVS_LINK_SPEEDUP)
                continue;

            int next_dvs_level = m_link_dvs_next_level_vec[n][pc];

            // set next freq
            router_link.dvs_freq = g_link_dvs_freq[next_dvs_level];
            router_link.dvs_freq_set_clk = simtime();
        }
    }

    m_link_dvs_set_speedup = false;
}

void LinkDVSer::link_dvs_update_voltage_slowdown()
{
    if (!m_link_dvs_set_slowdown)
        return;

    if (m_slowdown_link_count == 0)
        return;

    for (unsigned int n=0; n<g_Router_vec.size(); n++) {
        Router* p_router = g_Router_vec[n];
        for (int pc=0; pc<p_router->num_pc(); pc++) {
            if (p_router->isEjectChannel(pc))
                continue;

            Link& router_link = p_router->getLink(pc);	// link for outpc

            if (! router_link.valid )	// no link
                continue;

            if (m_link_dvs_speedup_flag_vec[n][pc] != DVS_LINK_SLOWDOWN)
                continue;

            int next_dvs_level = m_link_dvs_next_level_vec[n][pc];

            // set next voltage
            router_link.dvs_voltage = g_link_dvs_voltage[next_dvs_level];
        }
    }

    m_link_dvs_set_slowdown = false;
}

void LinkDVSer::link_dvs_update_freq_slowdown()
{
    if (!m_link_dvs_set_slowdown)
        return;

    if (m_slowdown_link_count == 0)
        return;

    for (unsigned int n=0; n<g_Router_vec.size(); n++) {
        Router* p_router = g_Router_vec[n];
        for (int pc=0; pc<p_router->num_pc(); pc++) {
            if (p_router->isEjectChannel(pc))
                continue;

            Link& router_link = p_router->getLink(pc);	// link for outpc

            if (! router_link.valid )	// no link
                continue;

            if (m_link_dvs_speedup_flag_vec[n][pc] != DVS_LINK_SLOWDOWN)
                continue;

            int next_dvs_level = m_link_dvs_next_level_vec[n][pc];

            // set next freq
            router_link.dvs_freq = g_link_dvs_freq[next_dvs_level];
            router_link.dvs_freq_set_clk = simtime();
        }
    }
}

void LinkDVSer::open_flit_rate_predict_file()
{
    char filename[256];

    // prediction result
//    sprintf(filename, "%s/%s-sd-avg-flit-inj-predict-string-100.txt",
//            SD_FLIT_RATE_PREDICT_DIR_NAME, cfg_trace_benchmark_name); 
    // estimation result
    sprintf(filename, "%s/%s/%s-sd-avg-flit-inj.txt",
            cfg_link_dvs_rate_file_dir.c_str(), cfg_trace_benchmark_name,
            cfg_trace_benchmark_name); 
printf("filename=%s\n", filename);

    m_fp_sd_flit_rate_predict = fopen(filename, "r");
    assert(m_fp_sd_flit_rate_predict);
}

void LinkDVSer::close_flit_rate_predict_file()
{
    if (m_fp_sd_flit_rate_predict)
        fclose(m_fp_sd_flit_rate_predict);
}

void LinkDVSer::skip_flit_rate_predict_file(int num_lines)
{
    char buf[8192];
    int skipped_lines = 0;

    assert(m_fp_sd_flit_rate_predict);

// printf("DVS rate num_lines=%d\n", num_lines);
    while (skipped_lines < num_lines) {
        fgets(buf, 8192, m_fp_sd_flit_rate_predict);
        skipped_lines++;
    }
// printf("%d prediction results skipped...\n", skipped_lines);
// printf("buf=%s\n", buf);
    m_cur_predict_line_num = skipped_lines;
}

void LinkDVSer::read_flit_rate_predict_interval()
{
    double sum_of_flit_rate = 0.0;
    double highest_voltage_threshold = 6.0;

    assert(m_fp_sd_flit_rate_predict);

    for (unsigned int s=0; s<cfg_router_num; s++) {
        for (unsigned int d=0; d<cfg_router_num; d++) {
            double flow_flit_rate;
            int rc = fscanf(m_fp_sd_flit_rate_predict, "%lf", &flow_flit_rate);
            m_sd_predict_flit_rate_vec[s][d] = flow_flit_rate;
            sum_of_flit_rate += m_sd_predict_flit_rate_vec[s][d];
            assert(rc == 1);
        }
    }
    m_cur_predict_line_num++;
// printf("prediction line=%d read... at %.0lf\n", m_cur_predict_line_num, simtime());

#ifdef LINK_DVS_DEBUG
    for (unsigned int s=0; s<cfg_router_num; s++) {
        for (unsigned int d=0; d<cfg_router_num; d++) {
            printf("%lf ", m_sd_predict_flit_rate_vec[s][d]);
        }
        printf("\n");
    }
#endif

    // 06/08/06 yuho: go to the highest voltage level
    // when the sum of flit rate is above threshold
    if (sum_of_flit_rate > highest_voltage_threshold)
        m_use_highest_voltage = true;
    else
        m_use_highest_voltage = false;
}

void LinkDVSer::estimate_link_utilz_from_flit_rate_predict()
{
    // reset predicted link utilization
    for (int i=0; i<cfg_router_num; i++)
    for (int j=0; j<g_Router_vec[i]->num_pc(); j++)
        m_predict_link_utilz_vec[i][j] = 0.0;

    for (int src=0; src<cfg_router_num; src++) {
        for (int dest=0; dest<cfg_router_num; dest++) {
            if (src == dest)
                continue;

            double inj_rate = m_sd_predict_flit_rate_vec[src][dest];

            // path: src, next1, next2, ..., dest
            vector<Router*> path = g_Routing->getPathVector(g_Router_vec[src], g_Router_vec[dest]);
            assert(path.size() >= 2);

// printf("src=%d dest=%d path(len=%d):", src, dest, path.size());

            for (unsigned int n=0; n<path.size()-1; n++) {

              FlitHead head_flit;
              head_flit.src_addr = path[n]->id();
              head_flit.dest_addr = dest;
              RouteInfo route_info = g_Routing->selectOutPC(path[n], 0, &head_flit);
// printf("%d(%d), ", path[n]->id(), route_info.m_cur_out_pc);

              m_predict_link_utilz_vec[path[n]->id()][route_info.m_cur_out_pc] += inj_rate;
            }
// printf("\n");
        }
    }

#ifdef LINK_DVS_DEBUG
    printf("Expected Link Utilization:\n");
    for (int i=0; i<cfg_router_num; i++) {
        Router* p_router = g_Router_vec[i];
        printf("R-%02d: ", p_router->id());
        for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
            Link& link = p_router->getLink(out_pc);
            link.link_expected_utilz = m_predict_link_utilz_vec[i][out_pc];
            printf("PC%d=%lg ", out_pc, m_predict_link_utilz_vec[i][out_pc]);
        }
        printf("\n");
    }
#endif

    for (int i=0; i<cfg_router_num; i++) {
        Router* p_router = g_Router_vec[i];
        for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
            Link& link = p_router->getLink(out_pc);
            link.link_expected_utilz = m_predict_link_utilz_vec[i][out_pc];
        }
    }
}

void LinkDVSer::link_dvs_select_vf_predicted_flit_rate()
{
    m_slowdown_link_count = 0;
    m_speedup_link_count = 0;
    m_nochange_link_count = 0;
    int dvs_level_count[MAX_LINK_DVS_LEVELS];

    memset(dvs_level_count, 0, sizeof(int)*MAX_LINK_DVS_LEVELS);

    // select the proper frequency for each link utilization
    for (unsigned int n=0; n<g_Router_vec.size(); n++) {
        Router* p_router = g_Router_vec[n];
        for (int p=0; p<p_router->num_pc(); p++) {
            if (p_router->isEjectChannel(p))
                continue;

            Link& router_link = p_router->getLink(p);

            if (! router_link.valid )	// no link
                continue;

// printf("%lf\n", m_predict_link_utilz_vec[n][p]);
// printf("m_predict_link_utilz_vec[%d][%d]=%lf interval=%lf\n", n, p, m_predict_link_utilz_vec[n][p], cfg_link_dvs_interval);
            double link_utilz = m_predict_link_utilz_vec[n][p];
            assert(link_utilz >= 0.0);
            // double weighted_link_utilz = sqrt(link_utilz)link_utilz_weight
            // double weighted_link_utilz = sqrt(sqrt(link_utilz));
            double const link_utilz_weight_const = 0.7;
            double weighted_link_utilz = cbrt(link_utilz/link_utilz_weight_const);
            int next_dvs_level = 0;
            double freq_rate = g_link_dvs_freq[0]/cfg_freq;
            assert(freq_rate <= 1.0);
            // double rate_diff_min = freq_rate - link_utilz;
            double rate_diff_min = freq_rate - weighted_link_utilz;

            for (int lv=1; lv<MAX_LINK_DVS_LEVELS; lv++) {
                freq_rate = g_link_dvs_freq[lv]/cfg_freq;
                assert(freq_rate <= 1.0);
                // double rate_diff = freq_rate - link_utilz;
                double rate_diff = freq_rate - weighted_link_utilz;
                if (rate_diff < 0.0)
                    break;

                if (rate_diff_min > rate_diff) {
                    rate_diff_min = rate_diff;
                    next_dvs_level = lv;
                }
            }

            if (m_use_highest_voltage)
                next_dvs_level = 0;

            double curr_link_freq = router_link.dvs_freq;
            double curr_link_voltage = router_link.dvs_voltage;
            dvs_level_count[next_dvs_level]++;
#ifdef LINK_DVS_DEBUG
      printf("LINK_DVS: router=%d link=%d link_utilz=%lg", n, p, link_utilz);
      printf(" curr(F=%.2lf V=%.2lf), next(F=%.2lf V=%.2lf lv=%d)\n",
            curr_link_freq/UNIT_GIGA,
            curr_link_voltage,
            g_link_dvs_freq[next_dvs_level]/UNIT_GIGA,
            g_link_dvs_voltage[next_dvs_level],
            next_dvs_level);
#endif
            if (curr_link_voltage != g_link_dvs_voltage[next_dvs_level]) {
                // record freq/voltage transition overhead energy
                router_power_link_voltage_transition(curr_link_voltage, g_link_dvs_voltage[next_dvs_level]);

                m_link_dvs_next_level_vec[n][p] = next_dvs_level;

                if (g_link_dvs_freq[next_dvs_level] > curr_link_freq) {
                    m_link_dvs_speedup_flag_vec[n][p] = DVS_LINK_SPEEDUP;
                    m_speedup_link_count++;
                } else {

if (g_link_dvs_freq[next_dvs_level] >= curr_link_freq) {
printf("g_link_dvs_freq[%d]=%lg, curr_link_freq=%lg clock=%.0lf\n",
next_dvs_level, g_link_dvs_freq[next_dvs_level], curr_link_freq, simtime());
}

                    assert(g_link_dvs_freq[next_dvs_level] < curr_link_freq);
                    m_link_dvs_speedup_flag_vec[n][p] = DVS_LINK_SLOWDOWN;
                    m_slowdown_link_count++;
                }
            } else {
                m_link_dvs_speedup_flag_vec[n][p] = DVS_LINK_NOCHANGE;
                m_nochange_link_count++;
            }
        }
    }
#ifdef LINK_DVS_DEBUG
printf("dvs set clk=%.0lf speedup=%d slowdown=%d nochange=%d\n", simtime(),
       m_speedup_link_count, m_slowdown_link_count, m_nochange_link_count);
for (int lv=0; lv<MAX_LINK_DVS_LEVELS; lv++)
    printf("lv-%02d:%d ", lv, dvs_level_count[lv]);
printf("\n");
#endif

    m_link_dvs_set_slowdown = true;
    m_link_dvs_set_speedup = true;
}

////////////////////////////////////////////////////////////////////////////
// Link DVS (HPCA 02)
void router_power_link_voltage_transition(double V1, double V2)
{
    double transition_energy;
    double efficiency = 0.9;
    double filter_capacitance = 5.0 * UNIT_PICO;

    transition_energy = (1.0 - efficiency) * filter_capacitance
                      * fabs(V1*V1 - V2*V2);
 
    g_DVS_transition_energy += transition_energy;
    g_DVS_transition_count++;
}

double router_power_link_voltage_transition_energy_report()
{
    return g_DVS_transition_energy;
}

unsigned long long router_power_link_voltage_transition_count_report()
{
    return g_DVS_transition_count;
}
////////////////////////////////////////////////////////////////////////////

#endif // #ifdef LINK_DVS
