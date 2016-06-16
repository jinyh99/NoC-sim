#include "noc.h"
#include "main.h"
#include "Router.h"
#include "Core.h"
#include "NIInput.h"
#include "NIOutput.h"
#include "profile.h"
#include "Workload.h"
#include "WorkloadSynthetic.h"
#ifdef LINK_DVS
#include "LinkDVSer.h"
#endif
#include "CAMManager.h"
#include "sim_process.h"

static const int MAX_PROCESS_NAME_STR_LEN = 64;

void process_main()
{
    create("start_proc");
    g_sim.m_num_CSIM_process++;

    fprintf(stderr, "started simulation.\n"); 

    // simulation progress verbose
    if (g_cfg.sim_show_progress)
        process_sim_progress();

    // router
    for (unsigned int i=0; i<g_Router_vec.size(); i++) {
        process_router(g_Router_vec[i]);
    }

    // input/output NI
    for (unsigned int n=0; n<g_NIInput_vec.size(); n++) {
        switch (g_cfg.NIin_type) {
        case NI_INPUT_TYPE_PER_PC:
            process_NI_input(g_NIInput_vec[n], 0);
            break;
        case NI_INPUT_TYPE_PER_VC:
            for (int NI_vc=0; NI_vc<g_cfg.router_num_vc; NI_vc++)
                process_NI_input(g_NIInput_vec[n], NI_vc);
            break;
        default:
            assert(0);
        }
    }

    for (unsigned int n=0; n<g_NIOutput_vec.size(); n++) {
        process_NI_output(g_NIOutput_vec[n]);
    }

    // profile
    if (g_cfg.profile_perf || g_cfg.profile_power) {
        if (g_cfg.profile_interval_cycle)
            process_profile_cycle();
        else
            process_profile_instr();
    }

#ifdef LINK_DVS
    // link-dvs
    process_link_dvs_link_speedup();
    process_link_dvs_link_slowdown();
    process_link_dvs_set();
#endif

    // injection
    switch (g_cfg.wkld_type) {
    case WORKLOAD_TRIPS_TRACE:
    case WORKLOAD_TILED_CMP_TRACE:
    case WORKLOAD_TILED_CMP_VALUE_TRACE:
    case WORKLOAD_SNUCA_CMP_VALUE_TRACE:
        process_parse_trace();
        break;

    case WORKLOAD_SYNTH_SPATIAL:
    case WORKLOAD_SYNTH_TRAFFIC_MATRIX:
        for (unsigned int c=0; c<g_Core_vec.size(); c++)
            process_gen_synth_traffic(c);
        break;

    default:
        assert(0);
    }

    // control simulation for warmup and finalize
    process_control_sim();

    g_ev_sim_done->wait();

    // Now the simulation is done.
    fprintf(stderr, "finished simulation at clk=%.0lf.\n", simtime());

    // Find the simulation end time
    g_sim.m_end_time = time((time_t *)NULL);
    g_sim.m_elapsed_time = _MAX(g_sim.m_end_time - g_sim.m_start_time, 1);

#ifdef _DEBUG_ROUTER_PROCESS
    printf("PROCESS COMPLETE: process_main()\n");
#endif
}

// control simulation for warmup and finalize
void process_control_sim()
{
    create("sim-control-proc");
    g_sim.m_num_CSIM_process++;

    double num_cycles_skip_trace = 0.0;
    double num_cycles_warmup_sim = 0.0;
    double num_cycles_end_sim = 0.0;

#if 0
    printf("g_cfg.wkld_trace_skip_cycles=%.1lf\n", g_cfg.wkld_trace_skip_cycles);
    printf("g_cfg.clk_start=             %.1lf\n", g_cfg.sim_clk_start);
    printf("g_cfg.clk_end=               %.1lf\n", g_cfg.sim_clk_end);
    fflush(stdout);
#endif
    num_cycles_skip_trace = g_cfg.wkld_trace_skip_cycles;
    num_cycles_warmup_sim = (g_cfg.sim_clk_start == 0.0) ? 0.0 : (g_cfg.sim_clk_start - num_cycles_skip_trace);
    if (num_cycles_warmup_sim < 0.0) {
        fprintf(stderr, "process_control_sim(): negative num_cycles_warmup_sim=%lf\n", num_cycles_warmup_sim);
        assert(0);
    }
    num_cycles_end_sim = g_cfg.sim_clk_end - num_cycles_skip_trace - num_cycles_warmup_sim;
    if (num_cycles_end_sim < 0.0) {
        fprintf(stderr, "process_control_sim(): negative num_cycles_end_sim\n");
        fprintf(stderr, "  g_cfg.sim_clk_end=      %.1lf\n", g_cfg.sim_clk_end);
        fprintf(stderr, "  num_cycles_skip_trace=  %.1lf\n", num_cycles_skip_trace);
        fprintf(stderr, "  num_cycles_warmup_sim=  %.1lf\n", num_cycles_warmup_sim);
        fprintf(stderr, "  num_cycles_end_sim=     %.1lf\n", num_cycles_end_sim);
        assert(0);
    }

    // skip cycles for trace
    if (num_cycles_skip_trace != 0.0)
        hold(num_cycles_skip_trace);
// printf("SKIP TRACE END:  %lf\n", simtime());

    const double cycles_check = 4.0;
    // start warm-up
    switch (g_cfg.sim_end_cond) {
    case SIM_END_BY_INJ_PKT:
    case SIM_END_BY_EJT_PKT:
        while (g_sim.m_num_pkt_ejt < g_cfg.sim_num_ejt_pkt_4warmup)
           hold (cycles_check);
        break;
    case SIM_END_BY_CYCLE:
        if (num_cycles_warmup_sim != 0.0)
            hold(num_cycles_warmup_sim);
        break;
    default:
        assert(0);
    }
    // finished warm-up
    g_sim.m_warmup_phase = false;
    g_sim.m_clk_warmup_end = simtime();
    g_sim.m_num_pkt_inj_warmup = g_sim.m_num_pkt_inj;
    g_sim.m_num_pkt_ejt_warmup = g_sim.m_num_pkt_ejt;
    g_sim.m_num_flit_inj_warmup = g_sim.m_num_flit_inj;
    g_sim.m_num_flit_ejt_warmup = g_sim.m_num_flit_ejt;
    reset_stats();

    fprintf(stderr, "finished warm-up at clk=%.0lf.\n", simtime());

    // start simulation
    switch (g_cfg.sim_end_cond) {
    case SIM_END_BY_INJ_PKT:
        while (g_sim.m_num_pkt_ejt <= g_cfg.sim_num_inj_pkt)
            hold(cycles_check);
        break;
    case SIM_END_BY_EJT_PKT:
        while (g_sim.m_num_pkt_ejt <= g_cfg.sim_num_ejt_pkt)
            hold(cycles_check);
        break;
    case SIM_END_BY_CYCLE:
        if (num_cycles_end_sim != 0.0) {
            if (g_cfg.profile_perf || g_cfg.profile_power)
                num_cycles_end_sim += 0.5;	// for the last profile
            hold(num_cycles_end_sim);
        }
        break;
    }
    // finished simulation

    if (!g_EOS) {
        g_ev_sim_done->set();
        g_sim.m_clk_sim_end = simtime();
        g_EOS = true;
    }

#ifdef _DEBUG_ROUTER_PROCESS
    printf("PROCESS COMPLETE: process_control_sim() clk=%.0lf\n", simtime());
#endif
}

void process_sim_progress()
{
    create("sim-progress-proc");
    g_sim.m_num_CSIM_process++;

    if (g_cfg.wkld_trace_skip_cycles != 0.0)
        hold(g_cfg.wkld_trace_skip_cycles);

    while (!g_EOS) {
        hold((double) g_cfg.sim_progress_interval);
        print_sim_progress();
    }

#ifdef _DEBUG_ROUTER_PROCESS
    printf("PROCESS COMPLETE: process_sim_progress\n");
#endif
}

void process_router(Router* p_router)
{
    char proc_name[MAX_PROCESS_NAME_STR_LEN];

    sprintf(proc_name, "r_%d proc", p_router->id());
    create(proc_name);
    g_sim.m_num_CSIM_process++;

    while (1) {
#ifdef _DEBUG_ROUTER_SNAPSHOT
        if (simtime() >= _DEBUG_ROUTER_SNAPSHOT_CLK)
            take_network_snapshot(stdout);
#endif

        // 03/15/06 fast simulation
        if (p_router->hasNoFlitsInside() && p_router->hasNoCreditDepositsInside()) {
            p_router->sleep();
        }

        p_router->router_sim();
        hold(ONE_CYCLE);
    }

#ifdef _DEBUG_ROUTER_PROCESS
    printf("PROCESS COMPLETE: process_router(router=%d)\n", p_router->id());
#endif
}

// FIXME: This code is only for DMesh topology.
//        Specialize or generalize it later.
void select_network(Packet* p_pkt)
{
    // choose one network if multiple networks exist.
    // select randomly
/*
    int net_id = core_stream.random (0L, g_cfg.net_networks-1);
    p_pkt->m_NI_in_pos += g_cfg.core_num_ipc * net_id;
    printf("%d*%d pos=%d\n", g_cfg.core_num_ipc, net_id, p_pkt->m_NI_in_pos);
*/

    // select NI with minimum pkt queue size
    int min_NI_in_pos = -1;
    int min_pktQ_sz = INT_MAX;
    int min_net_id = -1;
    for (int net_id=0; net_id<g_cfg.net_networks; net_id++) {
        int NI_in_pos = p_pkt->m_NI_in_pos + g_cfg.core_num_NIs * net_id;
        if (g_Core_vec[p_pkt->getSrcCoreID()]->getNIInput(NI_in_pos)->pktQ_sz() < min_pktQ_sz) {
            min_NI_in_pos = NI_in_pos;
            min_pktQ_sz = g_Core_vec[p_pkt->getSrcCoreID()]->getNIInput(NI_in_pos)->pktQ_sz();
            min_net_id = net_id;
        }
    }
    assert(min_NI_in_pos >= 0);
    assert(min_net_id >= 0);
    p_pkt->m_NI_in_pos = min_NI_in_pos;

    // update source and destination routers
    p_pkt->setSrcRouterID(p_pkt->getSrcRouterID() + (g_cfg.router_num/g_cfg.net_networks)*min_net_id);
    int old_dest_router_id = p_pkt->getDestRouterID();
    p_pkt->delDestRouterID(old_dest_router_id);
    p_pkt->addDestRouterID(old_dest_router_id + (g_cfg.router_num/g_cfg.net_networks)*min_net_id);
}

void process_gen_synth_traffic(int core_id)
{
    char proc_name[MAX_PROCESS_NAME_STR_LEN];
    WorkloadSynthetic* workloadSynth = (WorkloadSynthetic*) g_Workload;

    if (workloadSynth->noInjection(core_id))
        return;

    sprintf(proc_name, "mgen-%d proc", core_id);
    create(proc_name);
    g_sim.m_num_CSIM_process++;

    while (!g_EOS) {
        double hold_time = workloadSynth->getWaitingTime(core_id);
        assert(hold_time >= 0.0);
        hold(hold_time);

        // finish simulation for the number of injected packets.
        if (g_cfg.sim_end_cond == SIM_END_BY_INJ_PKT) {
            if (g_sim.m_num_pkt_inj >= g_cfg.sim_num_inj_pkt)
                break;
        }

        Packet* p_pkt = workloadSynth->genPacket(core_id);

        // choose one network if multiple networks exist.
        if (g_cfg.net_networks > 1)
            select_network(p_pkt);

        assert(p_pkt->getSrcCoreID() == core_id);
        assert(p_pkt->m_NI_in_pos < g_Core_vec[p_pkt->getSrcCoreID()]->num_NIInput());
        g_Core_vec[p_pkt->getSrcCoreID()]->forwardPkt2NI(p_pkt->m_NI_in_pos, p_pkt);
        g_sim.m_num_pkt_inj++;
    }

    // NOTE: CSIM process synchronization problem...
    //       workaround: Wait for a sufficiently long time.
    hold(100.0);

#ifdef _DEBUG_ROUTER_PROCESS
    printf("PROCESS COMPLETE: process_gen_synth_traffic(core=%d)\n", core_id);
#endif
}

void process_parse_trace()
{
    char proc_name[MAX_PROCESS_NAME_STR_LEN];
    sprintf(proc_name, "trace proc");
    create(proc_name);
    g_sim.m_num_CSIM_process++;

    // stream net_stream;

    WorkloadTrace* wkldTrace = (WorkloadTrace*) g_Workload;

    ((WorkloadTrace*) g_Workload)->skipTraceFile();
    hold(g_cfg.wkld_trace_skip_cycles);
    fprintf(stderr, "skipped %.0lf cycles (trace_file_id=%d).\n", g_cfg.wkld_trace_skip_cycles, wkldTrace->trace_file_id());

    double last_pkt_inject_clk = simtime();

    while (!g_EOS) {
        vector< Packet* > pkt_vec = wkldTrace->readTrace();

        if (pkt_vec.size() == 0)	
            continue;

        if (pkt_vec.size() == 1 && pkt_vec[0] == 0) // no more trace?
            break;

        if (pkt_vec[0]->m_clk_gen > last_pkt_inject_clk) {
            double hold_tm = pkt_vec[0]->m_clk_gen - last_pkt_inject_clk;
            if (hold_tm > 0.0)
                hold(hold_tm);
            last_pkt_inject_clk = simtime();
        }

        for (unsigned int n=0; n<pkt_vec.size(); n++) {
            Packet* p_pkt = pkt_vec[n];
#ifdef _DEBUG_ROUTER
printf("clk=%0.lf GEN p=%lld C:%d->%d R:%d/%d->%d/%d #flits=%d gen_clk=%.0lf\n", simtime(), p_pkt->id(), p_pkt->getSrcCoreID(), p_pkt->getDestCoreID(), p_pkt->getSrcRouterID(), p_pkt->m_NI_in_pos, p_pkt->getDestRouterID(), p_pkt->m_NI_out_pos, p_pkt->m_num_flits, p_pkt->m_clk_gen);
#endif

            // choose one network if multiple networks exist.
            if (g_cfg.net_networks > 1)
                select_network(p_pkt);

            assert(p_pkt->m_NI_in_pos < g_Core_vec[p_pkt->getSrcCoreID()]->num_NIInput());
            g_Core_vec[p_pkt->getSrcCoreID()]->forwardPkt2NI(p_pkt->m_NI_in_pos, p_pkt);
            g_sim.m_num_pkt_inj++;
        }
    }

    // After the last trace is processed, terminate simulation.
    if (!g_EOS) {
        g_EOS = true;
        g_ev_sim_done->set();
        g_sim.m_clk_sim_end = simtime();
    }

#ifdef _DEBUG_ROUTER_PROCESS
    printf("PROCESS COMPLETE: process_parse_trace\n");
#endif
}

void process_NI_input(NIInput* p_ni_input, int NI_vc)
{
    char proc_name[MAX_PROCESS_NAME_STR_LEN];

    sprintf(proc_name, "NIin_%d_%d proc", p_ni_input->id(), NI_vc);
    create(proc_name);
    g_sim.m_num_CSIM_process++;

    while (!g_EOS) {
        p_ni_input->segmentPacket(NI_vc);
    }

#ifdef _DEBUG_ROUTER_PROCESS
    printf("PROCESS COMPLETE: process_NI_input (ni=%d vc=%d)\n", p_ni_input->id(), NI_vc);
#endif
}

void process_NI_output(NIOutput* p_ni_output)
{
    char proc_name[MAX_PROCESS_NAME_STR_LEN];

    sprintf(proc_name, "NIout_%d proc", p_ni_output->id());
    create(proc_name);
    g_sim.m_num_CSIM_process++;

    while (!g_EOS) {
        p_ni_output->assembleFlit();
        // hold(ONE_CYCLE);
    }

#ifdef _DEBUG_ROUTER_PROCESS
    printf("PROCESS COMPLETE: process_NI_output(ni=%d)\n", p_ni_output->id());
#endif
}

#ifdef LINK_DVS
void process_link_dvs_set()
{
    char proc_name[MAX_PROCESS_NAME_STR_LEN];
    double last_profile_clk = 0.0;

    sprintf(proc_name, "link-dvs proc");
    create(proc_name);
    g_sim.m_num_CSIM_process++;

    switch (g_cfg.link_dvs_method) {
    case LINK_DVS_NODVS:
        return;
    case LINK_DVS_HISTORY:
        break;
    case LINK_DVS_FLIT_RATE_PREDICT:
        g_LinkDVSer.open_flit_rate_predict_file();
        g_LinkDVSer.skip_flit_rate_predict_file((int) (g_cfg.wkld_trace_skip_cycles/UNIT_MEGA) + 1);
        break;
    }

    double hold_time = g_cfg.sim_clk_start + g_cfg.link_dvs_interval/2.0;
    assert(hold_time > 0.0);
    hold(hold_time);

    while (!g_EOS) {
        switch (g_cfg.link_dvs_method) {
        case LINK_DVS_HISTORY:
            g_LinkDVSer.link_dvs_select_vf();
            break;
        case LINK_DVS_FLIT_RATE_PREDICT:
            g_LinkDVSer.read_flit_rate_predict_interval();
            g_LinkDVSer.estimate_link_utilz_from_flit_rate_predict();
            g_LinkDVSer.link_dvs_select_vf_predicted_flit_rate();

#ifdef LINK_DVS_DEBUG
            printf("read (%.0lf~%.0lf) rate info for DVS at clk=%.0lf\n",
            (g_LinkDVSer.predict_line_num()-1)*g_cfg.link_dvs_interval,
            (g_LinkDVSer.predict_line_num())*g_cfg.link_dvs_interval,
            simtime());
#endif

            break;
        default:
            assert(0);
        }

        hold(g_cfg.link_dvs_interval);
    }

    switch (g_cfg.link_dvs_method) {
    case LINK_DVS_FLIT_RATE_PREDICT:
        g_LinkDVSer.close_flit_rate_predict_file();
        break;
    }
}

void process_link_dvs_link_speedup()
{
    char proc_name[MAX_PROCESS_NAME_STR_LEN];
    double last_profile_clk = 0.0;

    sprintf(proc_name, "link-dvs speedup proc");
    create(proc_name);
    g_sim.m_num_CSIM_process++;

    if (g_cfg.link_dvs_method == LINK_DVS_NODVS)
        return;

    double hold_time = g_cfg.sim_clk_start
                     + g_cfg.link_dvs_interval
                     - g_cfg.link_dvs_voltage_transit_delay 
                     - g_cfg.link_dvs_freq_transit_delay;
    assert(hold_time > 0.0);
    hold(hold_time);

    while (!g_EOS) {
        // voltage first, frequency second
        hold(g_cfg.link_dvs_voltage_transit_delay);
        g_LinkDVSer.link_dvs_update_voltage_speedup();
#ifdef LINK_DVS_DEBUG
        printf("link voltage speedup clk=%.0lf\n", simtime());
#endif

        hold(g_cfg.link_dvs_freq_transit_delay);
        g_LinkDVSer.link_dvs_update_freq_speedup();
#ifdef LINK_DVS_DEBUG
        printf("link freq speedup clk=%.0lf\n", simtime());
#endif

        hold(g_cfg.link_dvs_interval - g_cfg.link_dvs_voltage_transit_delay - g_cfg.link_dvs_freq_transit_delay);
    }
}

void process_link_dvs_link_slowdown()
{
    char proc_name[MAX_PROCESS_NAME_STR_LEN];
    double last_profile_clk = 0.0;

    sprintf(proc_name, "link-dvs slowdown proc");
    create(proc_name);
    g_sim.m_num_CSIM_process++;

    if (g_cfg.link_dvs_method == LINK_DVS_NODVS)
        return;

    double hold_time = g_cfg.sim_clk_start
                     + g_cfg.link_dvs_interval
                     - g_cfg.link_dvs_voltage_transit_delay 
                     - g_cfg.link_dvs_freq_transit_delay;
    assert(hold_time > 0.0);
    hold(hold_time);

    while (!g_EOS) {
        // frequency first, voltage second

        hold(g_cfg.link_dvs_freq_transit_delay);
        g_LinkDVSer.link_dvs_update_freq_slowdown();
#ifdef LINK_DVS_DEBUG
        printf("link freq slowdown clk=%.0lf\n", simtime());
#endif

        hold(g_cfg.link_dvs_voltage_transit_delay);
        g_LinkDVSer.link_dvs_update_voltage_slowdown();
#ifdef LINK_DVS_DEBUG
        printf("link voltage slowdown clk=%.0lf\n", simtime());
#endif

        hold(g_cfg.link_dvs_interval - g_cfg.link_dvs_voltage_transit_delay - g_cfg.link_dvs_freq_transit_delay);
    }
}
#endif

void process_profile_cycle()
{
    char proc_name[MAX_PROCESS_NAME_STR_LEN];
    sprintf(proc_name, "profile proc");
    create(proc_name);
    g_sim.m_num_CSIM_process++;

    double clk_start_profile = 0.0;

    if (g_cfg.sim_clk_start > 0.0) {
        clk_start_profile = g_cfg.sim_clk_start;
        assert(g_cfg.sim_clk_start >= g_cfg.wkld_trace_skip_cycles);
    } else if (g_cfg.wkld_trace_skip_cycles > 0.0) {
        clk_start_profile = g_cfg.wkld_trace_skip_cycles;
    }
    assert(clk_start_profile >= 0.0);
    
    hold(clk_start_profile);

    if (g_cfg.profile_perf) {
        profile_perf_reset();
        profile_perf_header_print();
    }

    if (g_cfg.profile_power) {
        profile_power_header_print();
    }

    while (!g_EOS) {
        hold(g_cfg.profile_interval);

        // performance
        if (g_cfg.profile_perf) {
            profile_perf_print();
            profile_perf_reset();
        }

        // power
        if (g_cfg.profile_power) {
            profile_power_print();
            profile_power_reset();
        }
    }

#ifdef _DEBUG_ROUTER_PROCESS
    printf("PROCESS COMPLETE: process_profile cycle\n");
#endif
}

void process_profile_instr()
{
    char proc_name[MAX_PROCESS_NAME_STR_LEN];
    sprintf(proc_name, "profile core");
    create(proc_name);
    g_sim.m_num_CSIM_process++;

    double num_cycles_check = 5.0;
    double clk_start_profile = 0.0;

    if (g_cfg.sim_clk_start > 0.0) {
        clk_start_profile = g_cfg.sim_clk_start;
        assert(g_cfg.sim_clk_start >= g_cfg.wkld_trace_skip_cycles);
    } else if (g_cfg.wkld_trace_skip_cycles > 0.0) {
        clk_start_profile = g_cfg.wkld_trace_skip_cycles;
    }
    assert(clk_start_profile >= 0.0);
    
    hold(clk_start_profile);
    profile_set_start_clk();

    if (g_cfg.profile_perf) {
        profile_perf_reset();
        profile_perf_header_print();
    }

    if (g_cfg.profile_power) {
        profile_power_header_print();
    }

    unsigned long long next_profile_instr = g_sim.m_num_instr_executed + ((unsigned long long) g_cfg.profile_interval);
// printf("0 cycle=%.0lf instr=%lld\n", simtime(), next_profile_instr);
    // make next_profile_instr as multiple of g_cfg.profile_interval
    next_profile_instr /= ((unsigned long long) g_cfg.profile_interval);
    next_profile_instr *= ((unsigned long long) g_cfg.profile_interval);
// printf("1 cycle=%.0lf instr=%lld\n", simtime(), next_profile_instr);

    while (!g_EOS) {
        while (g_sim.m_num_instr_executed <= next_profile_instr)
            hold(num_cycles_check);

        // performance
        if (g_cfg.profile_perf) {
            profile_perf_print();
            profile_perf_reset();
        }

        // power
        if (g_cfg.profile_power) {
            profile_power_print();
            profile_power_reset();
        }

        profile_set_start_clk();

        next_profile_instr += ((unsigned long long) g_cfg.profile_interval);
// printf("cycle=%.0lf instr=%lld\n", simtime(), next_profile_instr);
    }

#ifdef _DEBUG_ROUTER_PROCESS
    printf("PROCESS COMPLETE: process_profile instr\n");
#endif
}

////////////////////////////////////////////////////////////////////////
// print simulation progress
void print_sim_progress()
{
    char buf[256];

    // format:
    //   clock,
    //   packe:   #injected pkts(I), #ejected pkts(E), #injected pkts - #ejected pkts(D),
    //            #in-transit pkts(N)
    //   flit:    #in-transit flits(N),
    //   trace:   #injected traces (if workload uses traces)
    //   latency: avg pkt latency(l), avg queuing latency(q), avg contention latency(c)

    sprintf(buf, "clk=%.0lf\tp:I=%lld E=%lld D=%lld N=%lld f:N=%lld",
            simtime(),
            g_sim.m_num_pkt_inj, g_sim.m_num_pkt_ejt, g_sim.m_num_pkt_inj-g_sim.m_num_pkt_ejt,
            g_sim.m_num_pkt_in_network, g_sim.m_num_flit_in_network);

    if (! g_Workload->isSynthetic()) {
        sprintf(buf+strlen(buf), " t:%lld",
                ((WorkloadTrace*) g_Workload)->getProcessedTraceCount());
    }

    sprintf(buf+strlen(buf), "\tl=%.2lf q=%.1lf c=%.1lf\n",
            g_sim.m_pkt_T_t_tab->mean(),
            g_sim.m_pkt_T_q_tab->mean(),
            g_sim.m_pkt_T_t_tab->mean() - g_sim.m_pkt_T_h_tab->mean() - g_sim.m_pkt_T_w_tab->mean() - g_sim.m_pkt_T_s_tab->mean() + 1.0); // FIXME 1.0+ cycle

    fprintf(stderr, "%s", buf);
    fflush(stderr);
}
