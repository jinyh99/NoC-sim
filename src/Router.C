#include "noc.h"
#include "Router.h"
#include "NIOutput.h"
#include "SwArb2Stage.h"
#include "VCArbFCFS.h"
#include "Routing.h"
#include "RouterPower.h"
#include "RouterPowerOrion.h"
#include "RouterPowerStats.h"

Router::Router()
{
    assert(0);
}

Router::Router(int router_id, int num_pc, int num_vc, int num_ipc, int num_epc, int inbuf_depth)
{
    // configuration
    m_id = router_id;
    m_num_pc = num_pc;
    m_num_vc = num_vc;
    m_inbuf_depth = inbuf_depth;

    assert(num_ipc == num_epc);
    m_num_ipc = num_ipc;
    m_num_epc = num_epc;

    // flit input buffers
    m_flitQ = new FlitQ(num_pc, num_vc, inbuf_depth);

    // input modules
    m_in_mod_vec.resize(num_pc);
    for (int in_pc=0; in_pc<num_pc; in_pc++) {
        m_in_mod_vec[in_pc].resize(num_vc);
        for (int in_vc=0; in_vc<num_vc; in_vc++) {
            RouterInputModule & in_mod = m_in_mod_vec[in_pc][in_vc];
            in_mod.m_state = IN_MOD_I;
            in_mod.m_out_pc = INVALID_PC;
            in_mod.m_out_vc = INVALID_VC;
        }
    }

    // output modules
    m_out_mod_vec.resize(num_pc);
    for (int pc=0; pc<num_pc; pc++) {
        m_out_mod_vec[pc].resize(num_vc);
        for (int vc=0; vc<num_vc; vc++) {
            RouterOutputModule & out_mod = m_out_mod_vec[pc][vc];
            out_mod.m_state = OUT_MOD_I;

            // credits
            // NOTE: Each router can have different number of VCs.
            //       Router connection is done in Topology class.
            //       Here, router does not know number of VCs for downstream router.
            // HACK: m_num_credit and m_num_credit_rsv have maximum
            //       possible VCs(=g_cfg.router_num_vc) in the network.

            out_mod.m_num_credit = inbuf_depth - g_cfg.router_num_rsv_credit;
            out_mod.m_num_credit_rsv = g_cfg.router_num_rsv_credit;

#ifdef _DEBUG_CREDIT
            printf("pc=%d vc=%d credit=%d credit_rsv=%d\n", pc, vc, out_mod.m_num_credit, out_mod.m_num_credit_rsv);
#endif

        }
    }

    // VC arbiter
    m_vc_arb = new VCArbFCFS(this);

    // switch arbiter
    m_sw_arb = new SwArb2Stage(this, g_cfg.router_sa_v1_type, g_cfg.router_sa_p1_type);

    // xbar
    m_xbar.m_waiting_in_vc_vec.resize(num_pc, INVALID_VC);
    m_xbar.m_outport_free_vec.resize(num_pc, true);

    // link
    m_link_vec.resize(num_pc);
    for (int pc=0; pc<num_pc; pc++) {
        m_link_vec[pc].m_valid = false;
        m_link_vec[pc].m_link_name = "X";
        m_link_vec[pc].m_delay_factor = 1;
        m_link_vec[pc].m_length_mm = g_cfg.link_length;

#ifdef LINK_DVS
        m_link_vec[pc].m_link_expected_utilz = 0.0;
        m_link_vec[pc].m_dvs_freq = g_cfg.chip_freq;
        m_link_vec[pc].m_dvs_voltage = g_cfg.dvs_voltage;
        m_link_vec[pc].m_last_sent_clk = 0.0;
        m_link_vec[pc].m_dvs_freq_set_clk = -1.0 * (g_cfg.link_dvs_freq_transit_delay + 1.0);
#endif
    }

    // power
    m_power_tmpl = 0;
    m_power_tmpl_profile = 0;

    // fast simulation
    m_num_flits_inside = 0;
    m_ev_wakeup = new event("ev_wk_router");

#ifdef _DEBUG_CHECK_BUFFER_INTEGRITY
    m_debug_pkt_chk_mid_vec.resize(num_pc);
    m_debug_pkt_chk_fid_vec.resize(num_pc);
    for (int in_pc=0; in_pc<num_pc; in_pc++) {
        m_debug_pkt_chk_mid_vec[in_pc].resize(num_vc);
        m_debug_pkt_chk_fid_vec[in_pc].resize(num_vc);
    }
#endif

    // for stalls of each pipeline stage
    m_pipe_lat_LT_tab = new table("pipe_lat_LT");
    m_pipe_lat_ST_tab = new table("pipe_lat_ST");
    m_pipe_lat_SA_tab = new table("pipe_lat_SA");
    m_pipe_lat_SSA_tab = new table("pipe_lat_SSA");
    m_pipe_lat_VA_tab = new table("pipe_lat_VA");
    m_pipe_lat_RC_tab = new table("pipe_lat_RC");
#ifdef LINK_DVS
    m_sim_pc_dvs_link_op_vec.resize(num_pc, 0);
#endif
    m_flit_lat_router_tab = new table("flit_intra_router");

    // for router load
    m_num_pkt_inj_from_core = 0;
    m_num_flit_inj_from_core = 0;
    m_num_pkt_inj_from_router = 0;
    m_num_flit_inj_from_router = 0;

    // tunneling: bypass pipeline
    m_tunnel_info_vec.resize(num_pc);
    m_num_tunnel_flit_vec.resize(num_pc);
    for (int i=0; i<num_pc; i++) {
        m_tunnel_info_vec[i].m_in_vc = INVALID_VC;
        m_tunnel_info_vec[i].m_out_pc = INVALID_PC;
        m_tunnel_info_vec[i].m_out_vc = INVALID_VC;
        m_tunnel_info_vec[i].m_flow = make_pair(INVALID_ROUTER_ID, INVALID_ROUTER_ID);
        m_tunnel_info_vec[i].m_dest_router_id = INVALID_ROUTER_ID;

        m_num_tunnel_flit_vec[i].resize(num_vc, 0);
    }
}

Router::~Router()
{
    if (m_flitQ) delete m_flitQ;
    if (m_ev_wakeup) delete m_ev_wakeup;
    if (m_vc_arb) delete m_vc_arb;
    if (m_sw_arb) delete m_sw_arb;
    if (m_power_tmpl) delete m_power_tmpl;
    if (m_power_tmpl_profile) delete m_power_tmpl_profile;

    if (m_pipe_lat_LT_tab) delete m_pipe_lat_LT_tab;
    if (m_pipe_lat_ST_tab) delete m_pipe_lat_ST_tab;
    if (m_pipe_lat_SA_tab) delete m_pipe_lat_SA_tab;
    if (m_pipe_lat_SSA_tab) delete m_pipe_lat_SSA_tab;
    if (m_pipe_lat_VA_tab) delete m_pipe_lat_VA_tab;
    if (m_pipe_lat_RC_tab) delete m_pipe_lat_RC_tab;
    if (m_flit_lat_router_tab) delete m_flit_lat_router_tab;
}

bool Router::isEjectChannel(int out_pc)
{
    return (out_pc >= m_num_pc - m_num_epc) ? true : false;
}

bool Router::isInjectChannel(int in_pc)
{
    return (in_pc >= m_num_pc - m_num_ipc) ? true : false;
}

void Router::attachPowerModel()
{
    if (m_power_tmpl)
        delete m_power_tmpl;

    // select router power model
    // NOTE: When RouterPower class is instantiated, link power model is selected and instantiated.
    //       We don't have to select link power model.
    switch (g_cfg.router_power_model) {
    case ROUTER_POWER_MODEL_STATS:
        m_power_tmpl = new RouterPowerStats(this);
        break;
    case ROUTER_POWER_MODEL_ORION_CALL:
        m_power_tmpl = new RouterPowerOrion(this);
        break;
    default:
        assert(0);
    }

    // select router power model for periodic report
    // When the power value for router is reset at the end of one period,
    // the power value for links of that router is automatically reset.
    if (m_power_tmpl_profile)
        delete m_power_tmpl_profile;
    switch (g_cfg.router_power_model) {
    case ROUTER_POWER_MODEL_STATS:
        m_power_tmpl_profile = new RouterPowerStats(this);
        break;
    case ROUTER_POWER_MODEL_ORION_CALL:
        m_power_tmpl_profile = new RouterPowerOrion(this);
        break;
    default:
        assert(0);
    }
}

unsigned long long Router::getTotalLinkOp() const
{
    unsigned long long num_total_link_op = 0;

    assert(m_power_tmpl);
    for (unsigned int out_pc=0; out_pc<m_link_vec.size(); out_pc++) {
        if (! m_link_vec[out_pc].m_valid)
            continue;

        LinkPower* p_link_power = m_power_tmpl->getLinkPower(out_pc);
        assert(p_link_power);

        num_total_link_op += p_link_power->op();
    }

    return num_total_link_op;
}

//router pipeline stages
void Router::router_sim()
{
/*
    // FIXME: do subclassing later
    ///////////////////////////////////////////////////////
    // 4-stage pipeline
    stageLT(); // stage 5: link traversal
    stageST(); // stage 4: xbar traversal
    stageSA(); // stage 3: switch arbitration
    stageVA(); // stage 2: VC arbitration
    stageRC(); // stage 1: routing computation
    ///////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////
    // 3-stage pipeline (speculative-SA)
    stageLT();  // stage 4
    stageST();  // stage 3
    stageSA();  // stage 2 (for middle/tail flits)
    stageVA();  // stage 2.2 (for head flits)
    stageSSA(); // stage 2.1 (for head flits)
    stageRC();  // stage 1
    ///////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////
    // 2-stage pipeline (speculative-SA + lookahead routing)
    stageLT();  // stage 3
    stageST();  // stage 2
    stageSA();	// stage 1 (for middle/tail flits)
    stageRC();  // stage 1 (for head flits)
    stageVA();  // stage 1 (for head flits)
    stageSSA(); // stage 1 (for head flits)
    ///////////////////////////////////////////////////////
*/

    incCredit();

    stageLT();
    if (g_cfg.router_tunnel)
        stageTN();
    stageST();
    stageSA();
    if (g_cfg.router_lookahead_RC)
        stageRC();
    stageVA();
    if (g_cfg.router_spec_SA)
        stageSSA();
    if (! g_cfg.router_lookahead_RC)
        stageRC();
}

void Router::resetStats()
{
    m_pipe_lat_LT_tab->reset();
    m_pipe_lat_ST_tab->reset();
    m_pipe_lat_SA_tab->reset();
    m_pipe_lat_SSA_tab->reset();
    m_pipe_lat_VA_tab->reset();
    m_pipe_lat_RC_tab->reset();

    m_flit_lat_router_tab->reset();

    m_num_pkt_inj_from_core = 0;
    m_num_flit_inj_from_core = 0;
    m_num_pkt_inj_from_router = 0;
    m_num_flit_inj_from_router = 0;

    m_vc_arb->resetStats();
    m_sw_arb->resetStats();

    m_flitQ->resetStats();

    for (int in_pc=0; in_pc<m_num_pc; in_pc++)
    for (int in_vc=0; in_vc<m_num_vc; in_vc++)
        m_num_tunnel_flit_vec[in_pc][in_vc] = 0;
}

void Router::stageLT()
{
    for (int out_pc=0; out_pc<m_num_pc; out_pc++) {
        if (m_link_vec[out_pc].m_w_pipeline.size() == 0) // no flits to traverse a link?
            continue;

        Link & link = m_link_vec[out_pc];
        pair< Flit*, pair< int, double > > & front_flit = link.m_w_pipeline.front();
        Flit* p_flit = front_flit.first;
        int next_in_vc = front_flit.second.first;
        double store_clk = front_flit.second.second;
        assert(next_in_vc != INVALID_VC);

        // 03/06/06: support for multi-cycle link
        int link_lat = link.m_delay_factor * g_cfg.link_latency;
        if (link_lat > 1) {
            int link_traverse_time = (int) (simtime() - store_clk);
            if (link_traverse_time < link_lat) {
                continue;
            }
        }

#ifdef LINK_DVS
        // When link frequency is changed, link prevents flit traversal
        // for g_cfg.link_dvs_freq_transit_delay time.
        if (simtime() >= link.dvs_freq_set_clk &&
            simtime() < link.dvs_freq_set_clk + g_cfg.link_dvs_freq_transit_delay) {
// printf("router-%d link-%d at clk=%.0lf stall\n", m_id, out_pc, simtime());
            continue;
        }

        double DVS_lat = g_cfg.chip_freq / link.dvs_freq;
        assert(DVS_lat >= 1.0);

/*
if (m_id == 5 && out_pc == 0 && simtime() > 21000001.0) {
printf("router=%d out_pc=%d DVS_lat=%.1lf utilz=%lg freq=%lg voltage=%lg\n",
m_id, out_pc, DVS_lat, link.link_expected_utilz, link.dvs_freq, link.dvs_voltage);
}
*/

        // FIXME: support for multi-cycle link
        double DVS_ready_clk;
        if (link.last_sent_clk > link.store_clk) {
            DVS_ready_clk = link.last_sent_clk;
        } else {
            DVS_ready_clk = link.store_clk;
        }
        DVS_ready_clk += DVS_lat;

        if (DVS_ready_clk > simtime())
            continue;
#endif

        // get router ID and input PC of the downstream router for out_pc
        int next_router_id = m_connNextRouter_vec[out_pc].first;
        int next_in_pc = m_connNextRouter_vec[out_pc].second;
        assert(next_router_id != INVALID_ROUTER_ID);
        assert(next_in_pc != INVALID_PC);
        Router* p_next_router = g_Router_vec[next_router_id];
        FlitQ* p_next_flitQ = p_next_router->flitQ();
        assert(! p_next_flitQ->isFull(next_in_pc, next_in_vc) ); // not full in downstream router's buffer

        // pop flit from link
        link.m_w_pipeline.pop_front();

        // write flit to the downstream router's buffer
        p_next_flitQ->write(next_in_pc, next_in_vc, p_flit);

        p_next_router->m_num_flit_inj_from_router++;

        if (p_flit->isHead()) { // per-packet accounting
            p_next_router->m_num_pkt_inj_from_router++;

            p_flit->getPkt()->m_wire_delay += link_lat;	// wire delay (T_w) for this packet
        }

        // intra-router flit latency
        p_flit->m_clk_enter_router = simtime();

        // pipeline stage latency
        m_pipe_lat_LT_tab->tabulate(simtime() - p_flit->m_clk_enter_stage);
        p_flit->m_clk_enter_stage = simtime();

        // 03/15/06 fast simulation
        m_num_flits_inside--;
        if (p_next_router->hasNoFlitsInside()) {
            p_next_router->wakeup();
// printf("WAKEUP r_%d process at clk=%.0lf\n", p_next_router->id, simtime());
        }
        p_next_router->incFlitsInside();

        // record power
        if (!g_sim.m_warmup_phase) {
            m_power_tmpl->record_link_trav(p_flit, out_pc);
            p_next_router->m_power_tmpl->record_buffer_write(p_flit, next_in_pc);

            if (g_cfg.profile_power) {
                m_power_tmpl_profile->record_link_trav(p_flit, out_pc);
                p_next_router->m_power_tmpl_profile->record_buffer_write(p_flit, next_in_pc);
            }
        }

#ifdef _DEBUG_ROUTER
        debugLT(p_flit, out_pc);
        p_next_router->debugIB(p_flit, next_in_pc, next_in_vc);
#endif

#ifdef LINK_DVS
        if (!g_sim.m_warmup_phase)
            m_sim_pc_dvs_link_op_vec[out_pc]++;

        if (link.m_store_clk > link.m_last_sent_clk)
            link.m_last_sent_clk = link.m_store_clk;
        link.m_last_sent_clk += DVS_lat;
#endif
    }
}

void Router::stageST()
{
#ifdef _DEBUG_ROUTER_ST
    int _st_debug_router = 3;
    double _st_debug_clk = 141000012.0;
    if (m_id==_st_debug_router && simtime() > _st_debug_clk) {
        printf("ST_status router=%d, clk=%.0lf\n  ", m_id, simtime());
        for (int out_pc=0; out_pc<m_num_pc; out_pc++) {
            if (isEjectChannel(out_pc)) {
            } else {
                if (m_link_vec[out_pc].m_w_pipeline.size() == 0) {
                    printf("out_pc=%d(free) ", out_pc);
                } else {
                    printf("out_pc=%d", out_pc);
                    for (unsigned int i=0; i<m_link_vec[out_pc].m_w_pipeline.size(); i++)
                        printf("(fid=%lld) ",  m_link_vec[out_pc].m_w_pipeline[i].first->id());
                }
            }
        }
        printf("\n");
    }
#endif

    int num_xbar_passes = 0;

    for (int in_pc=0; in_pc<m_num_pc; in_pc++) {
        int in_vc = m_xbar.m_waiting_in_vc_vec[in_pc];
        if (in_vc == INVALID_VC)
            continue;

        Flit* read_flit = m_flitQ->read(in_pc, in_vc);
        assert(read_flit);

#ifdef _DEBUG_CHECK_BUFFER_INTEGRITY
        assert(checkBufferIntegrity(in_pc, in_vc, read_flit));
#endif

        // create a credit and send it to upstream router
        int prev_router_id = m_connPrevRouter_vec[in_pc].first;
        int prev_out_pc = m_connPrevRouter_vec[in_pc].second;
        if (prev_router_id != INVALID_ROUTER_ID && prev_out_pc != INVALID_PC) {
            Credit* p_credit = g_CreditPool.alloc();

            p_credit->m_out_pc = prev_out_pc;
            p_credit->m_out_vc = in_vc;
            p_credit->m_num_credits = 1;
            p_credit->m_clk_deposit = simtime() + g_Router_vec[prev_router_id]->getLink(prev_out_pc).m_delay_factor * g_cfg.link_latency;

            g_Router_vec[prev_router_id]->depositCredit(p_credit);
        }

        assert(m_in_mod_vec[in_pc][in_vc].m_state == IN_MOD_S);
        int out_pc = m_in_mod_vec[in_pc][in_vc].m_out_pc;
        int out_vc = m_in_mod_vec[in_pc][in_vc].m_out_vc;
        int next_router_id = m_connNextRouter_vec[out_pc].first;
        int next_in_pc = m_connNextRouter_vec[out_pc].second;

        if (isEjectChannel(out_pc)) {
            // select ejection port
            int epc = out_pc - num_internal_pc();
// printf("epc=%d out_pc=%d\n", epc, out_pc);
            NIOutput* p_ni_output = getNIOutput(epc);
            assert(p_ni_output);
            // FIXME: The following assert() is not valid for DMesh topology,
            //        because destination is encoded at injection function.
            // assert(m_id == read_flit->getPkt()->getDestRouterID());

            p_ni_output->writeFlit(read_flit);

            // 03/15/06 fast simulation
            m_num_flits_inside--;

#ifdef _DEBUG_ROUTER
            debugST(read_flit, in_pc, out_pc, INVALID_ROUTER_ID, INVALID_PC, out_vc, true);
#endif
        } else {
            // 11/05/05: no stall at ST stage
            assert(next_router_id != INVALID_PC);
            assert(next_in_pc != INVALID_PC);

#ifdef _DEBUG_CREDIT
            if(g_Router_vec[next_router_id]->flitQ()->isFull(next_in_pc, out_vc)) {
                printf("router=%d out_pc=%d next_router=%d next_in_pc=%d\n", m_id, out_pc, next_router_id, next_in_pc);
                for (int x_vc=0; x_vc<m_num_vc; x_vc++)
                    printf("  vc=%d: credit=%d credit_rsv=%d\n", x_vc, m_out_mod_vec[out_pc][x_vc].m_num_credit, m_out_mod_vec[out_pc][x_vc].m_num_credit_rsv);
                for (int x_vc=0; x_vc<m_num_vc; x_vc++)
                    printf("  vc=%d: Q_sz=%d\n", x_vc, g_Router_vec[next_router_id]->flitQ()->size(next_in_pc, x_vc));
            }
#endif

            // assert(! g_Router_vec[next_router_id]->flitQ()->isFull(next_in_pc, out_vc) );

            // move flit to the link
            // NOTE: For wire pipelining,
            //       # of traversing flits on the link must be less than link latency.
            assert(((int) m_link_vec[out_pc].m_w_pipeline.size()) < m_link_vec[out_pc].m_delay_factor*g_cfg.link_latency);
            m_link_vec[out_pc].m_w_pipeline.push_back(make_pair(read_flit, make_pair(out_vc, simtime())));

            // pipeline stage latency
            m_pipe_lat_ST_tab->tabulate(simtime() - read_flit->m_clk_enter_stage);
            read_flit->m_clk_enter_stage = simtime();

#ifdef _DEBUG_ROUTER
            debugST(read_flit, in_pc, out_pc, next_router_id, next_in_pc, out_vc, false);
#endif
        } // if (isEjectChannel(out_pc)) {

        // remove a flit from xbar
        m_xbar.m_waiting_in_vc_vec[in_pc] = INVALID_VC;

        // update xbar outport status
        assert(m_xbar.m_outport_free_vec[out_pc] == false);
        m_xbar.m_outport_free_vec[out_pc] = true;

        if (read_flit->isTail()) {
            // change input module status (S->I)
            m_in_mod_vec[in_pc][in_vc].m_state = IN_MOD_I;
	    m_in_mod_vec[in_pc][in_vc].m_out_pc = INVALID_PC;
            m_in_mod_vec[in_pc][in_vc].m_out_vc = INVALID_VC;

            // change output module status (V->I)
            assert(m_out_mod_vec[out_pc][out_vc].m_state == OUT_MOD_V);
            m_out_mod_vec[out_pc][out_vc].m_state = OUT_MOD_I;
        }

        num_xbar_passes++;

        // intra-router flit latency
        m_flit_lat_router_tab->tabulate(simtime() - read_flit->m_clk_enter_router);

        // record power
        if (!g_sim.m_warmup_phase) {
            m_power_tmpl->record_buffer_read(read_flit, in_pc);
            m_power_tmpl->record_xbar_trav(read_flit, in_pc, out_pc);

            if (g_cfg.profile_power) {
                m_power_tmpl_profile->record_buffer_read(read_flit, in_pc);
                m_power_tmpl_profile->record_xbar_trav(read_flit, in_pc, out_pc);
            }
        }
    }

    // record power
    if (!g_sim.m_warmup_phase) {
        if (num_xbar_passes > 0) {
            m_power_tmpl->record_xbar_trav_num(num_xbar_passes);

            if (g_cfg.profile_power)
                m_power_tmpl_profile->record_xbar_trav_num(num_xbar_passes);
        }
    }
}

void Router::stageSA()
{
    // make switch arbiter requests for middle/tail flits if head flit completed VA.
    // FIXME: can we do this better?
    vector< pair< int, int > > free_in_port_vec = m_sw_arb->getFreeInPorts();
    for (unsigned int n=0; n<free_in_port_vec.size(); n++) {
        int in_pc = free_in_port_vec[n].first;
        int in_vc = free_in_port_vec[n].second;

        if (m_flitQ->isEmpty(in_pc, in_vc))
            continue;

        // flit type should be MIDL or TAIL.
        Flit* peek_flit = m_flitQ->peek(in_pc, in_vc);
        if (peek_flit->isHead())
            continue;

        // head flit that belongs to the same packet for peek_flit
        // should complete switch allocation.
        if (m_in_mod_vec[in_pc][in_vc].m_state != IN_MOD_S)
            continue;

        // get output PC/VC
        int out_pc = m_in_mod_vec[in_pc][in_vc].m_out_pc;
        int out_vc = m_in_mod_vec[in_pc][in_vc].m_out_vc;

        // add a request to SW arbiter
        m_sw_arb->add(in_pc, in_vc, out_pc, out_vc);
    }

    // get granted requests
    vector< pair< pair< int, int >, pair< int, int > > > grant_vec = m_sw_arb->grantRegular();

    // move granted requests to xbar
    for (unsigned int n=0; n<grant_vec.size(); n++) {
        int in_pc = grant_vec[n].first.first;
        int in_vc = grant_vec[n].first.second;
        int out_pc = grant_vec[n].second.first;
        assert(in_pc != INVALID_PC);
        assert(in_vc != INVALID_VC);

        Flit* peek_flit = m_flitQ->peek(in_pc, in_vc);
        assert(peek_flit);

#ifdef _DEBUG_ROUTER
        int out_vc = grant_vec[n].second.second;
        debugSA(peek_flit, in_pc, in_vc, out_pc, out_vc, false, false);
#endif

        assert(m_xbar.m_waiting_in_vc_vec[in_pc] == INVALID_VC);
        m_xbar.m_waiting_in_vc_vec[in_pc] = in_vc;

        assert(m_xbar.m_outport_free_vec[out_pc] == true);
        m_xbar.m_outport_free_vec[out_pc] = false;

        // pipeline stage latency
        m_pipe_lat_SA_tab->tabulate(simtime() - peek_flit->m_clk_enter_stage);
        peek_flit->m_clk_enter_stage = simtime();

        // change input module status (V->S) if a head flit completes SW allocation.
        if (peek_flit->isHead()) {
            assert(m_in_mod_vec[in_pc][in_vc].m_state == IN_MOD_V);
            m_in_mod_vec[in_pc][in_vc].m_state = IN_MOD_S;
        }
    }
}

void Router::stageVA()
{
    if (m_vc_arb->hasNoReq())
        return;

    vector< pair< pair< int, int >, int > > grant_vec = m_vc_arb->grant();  // return value: <in_pc, in_vc>, out_vc>

    for (unsigned int n=0; n<grant_vec.size(); ++n) {
        int in_pc = grant_vec[n].first.first;
        int in_vc = grant_vec[n].first.second;
        int out_pc = m_in_mod_vec[in_pc][in_vc].m_out_pc;
        int out_vc = grant_vec[n].second;
        Flit* peek_flit = m_flitQ->peek(in_pc, in_vc);

        // add a request to SW arbiter
        m_sw_arb->add(in_pc, in_vc, out_pc, out_vc);

        // delete granted request
        m_vc_arb->del(in_pc, in_vc);

#ifdef _DEBUG_ROUTER
        debugVA(peek_flit, in_pc, in_vc, out_pc, out_vc, false);
#endif

        // pipeline stage latency
        m_pipe_lat_VA_tab->tabulate((simtime() - peek_flit->m_clk_enter_stage));
        peek_flit->m_clk_enter_stage = simtime();

        // tunneling: bypass pipeline
        if (g_cfg.router_tunnel) {
            FlitHead* p_head_flit = (FlitHead*) m_flitQ->peek(in_pc, in_vc);
            switch(g_cfg.router_tunnel_type) {
            case TUNNELING_PER_FLOW:
                if (m_tunnel_info_vec[in_pc].m_flow != make_pair(p_head_flit->src_router_id(), p_head_flit->dest_router_id())) {
                    m_tunnel_info_vec[in_pc].m_flow = make_pair(p_head_flit->src_router_id(), p_head_flit->dest_router_id());
                }
                break;
            case TUNNELING_PER_DEST:
                if (m_tunnel_info_vec[in_pc].m_dest_router_id != p_head_flit->dest_router_id()) {
                    m_tunnel_info_vec[in_pc].m_dest_router_id = p_head_flit->dest_router_id();
                }
                break;
            case TUNNELING_PER_OUTPORT:
                break;
            default:
                assert(0);
            }

            // common properties
            m_tunnel_info_vec[in_pc].m_in_vc = in_vc;
            m_tunnel_info_vec[in_pc].m_out_pc = out_pc;
            m_tunnel_info_vec[in_pc].m_out_vc = out_vc;
        }
    }

    // VA power consumption in Orion - assumption: per output PC organization
    // record power
    if (!g_sim.m_warmup_phase) {
        vector< bitset< max_sz_vc_arb > > vc_req_orion_vec;
        vector< bitset< max_sz_vc_arb > > vc_grant_orion_vec;
        vc_req_orion_vec.resize(m_num_pc, 0);
        vc_grant_orion_vec.resize(m_num_pc, 0);

        for (unsigned int n=0; n<grant_vec.size(); ++n) {
            int in_pc = grant_vec[n].first.first;
            int in_vc = grant_vec[n].first.second;
            int out_pc = m_in_mod_vec[in_pc][in_vc].m_out_pc;

            // NOTE: Orion limitation
            int arb_bit_pos = in_pc*m_num_vc + in_vc;
            if (arb_bit_pos > max_sz_vc_arb)
                arb_bit_pos %= max_sz_vc_arb;

            vc_req_orion_vec[out_pc][arb_bit_pos] = true;
            vc_grant_orion_vec[out_pc][arb_bit_pos] = true;
        }

        for (int out_pc=0; out_pc<m_num_pc; ++out_pc) {
            vc_req_orion_vec[out_pc] |= m_vc_arb->getReqBitVector(out_pc);

            if (vc_req_orion_vec[out_pc].any()) {
                m_power_tmpl->record_vc_arb(out_pc, (unsigned int) vc_req_orion_vec[out_pc].to_ulong() , (unsigned int) vc_grant_orion_vec[out_pc].to_ulong());

                if (g_cfg.profile_power) {
                    m_power_tmpl_profile->record_vc_arb(out_pc, (unsigned int) vc_req_orion_vec[out_pc].to_ulong(), (unsigned int) vc_grant_orion_vec[out_pc].to_ulong());
                }
            }
        }
    }
}

void Router::stageRC()
{
    for (int in_pc=0; in_pc<m_num_pc; in_pc++)
    for (int in_vc=0; in_vc<m_num_vc; in_vc++) {

#ifdef _DEBUG_ROUTER_RC
    int _rc_debug_router_id = 20;
    double _rc_debug_clk = 0.0;
    int _rc_in_pc = 4;
    int _rc_in_vc = 0;

    if (m_id == _rc_debug_router_id && simtime() > _rc_debug_clk && in_pc == _rc_in_pc && in_vc == _rc_in_vc ) {
        printf("buf_Status: clk=%.0lf router=%d pc=%d vc=%d sz=%d\n", simtime(), m_id, in_pc, in_vc, m_flitQ->size(in_pc, in_vc));
        if (! m_flitQ->isEmpty(in_pc, in_vc))  {
            printf("  ");
            m_flitQ->print(stdout, in_pc, in_vc);
        }
    }
#endif

        if (m_in_mod_vec[in_pc][in_vc].m_state != IN_MOD_I)
            continue;

        if (m_flitQ->isEmpty(in_pc, in_vc)) // has a flit ?
            continue;

        // peek one flit in the input buffer
        Flit* peek_flit = m_flitQ->peek(in_pc, in_vc);
        assert(peek_flit);

        // flit type must be HEAD.
        if (! peek_flit->isHead())
            continue;

        // CSIM process synchronization problem:
        //   The order of process creation may pre-determine the priority of processes.
        //   If process for router X is created earlier than process for router Y
        //   (i.e. X has higher priority than Y) in sim_process.C,
        //   router Y does not wait for one cycle when router X sends a flit to router Y.
        if (peek_flit->m_clk_enter_router == simtime())
            continue;

        // do routing (decide out_pc)
        int out_pc = g_Routing->selectOutPC(this, in_vc, (FlitHead*) peek_flit);
        assert(out_pc < (int) m_connNextRouter_vec.size());
        int next_router_id = m_connNextRouter_vec[out_pc].first;
        int next_in_pc = m_connNextRouter_vec[out_pc].second;

        assert(isEjectChannel(out_pc) || (! isEjectChannel(out_pc) && next_router_id != INVALID_ROUTER_ID));
        assert(isEjectChannel(out_pc) || (! isEjectChannel(out_pc) && next_in_pc != INVALID_PC));

        // change input module status
        assert(m_in_mod_vec[in_pc][in_vc].m_state == IN_MOD_I);
        m_in_mod_vec[in_pc][in_vc].m_state = IN_MOD_R; // I->R
        m_in_mod_vec[in_pc][in_vc].m_out_pc = out_pc;

        // make VC arbiter request for this packet
        m_vc_arb->add(in_pc, in_vc, out_pc);

#ifdef _DEBUG_ROUTER
        debugRC(peek_flit, next_router_id, in_pc, in_vc, out_pc, next_in_pc);
#endif

        // pipeline stage latency
        m_pipe_lat_RC_tab->tabulate(simtime() - peek_flit->m_clk_enter_stage);
        peek_flit->m_clk_enter_stage = simtime();

        // increase hop count for this packet
        peek_flit->getPkt()->m_hops++;
    }
}

void Router::stageSSA()
{
    // speculative-SW allocator does grant.
    vector< pair< pair< int, int >, pair< int, int > > > grant_vec = m_sw_arb->grantSpec();

    // move granted requests to xbar
    for (unsigned int n=0; n<grant_vec.size(); n++) {
        int in_pc = grant_vec[n].first.first;
        int in_vc = grant_vec[n].first.second;
        int out_pc = grant_vec[n].second.first;
        int out_vc = grant_vec[n].second.second;
        assert(in_pc != INVALID_PC);
        assert(in_vc != INVALID_VC);
        Flit* peek_flit = m_flitQ->peek(in_pc, in_vc);
        assert(peek_flit->isHead());

#ifdef _DEBUG_ROUTER
        debugSA(peek_flit, in_pc, in_vc, out_pc, out_vc, true, false);
#endif

        assert(m_xbar.m_waiting_in_vc_vec[in_pc] == INVALID_VC);
        m_xbar.m_waiting_in_vc_vec[in_pc] = in_vc;

        assert(m_xbar.m_outport_free_vec[out_pc] == true);
        m_xbar.m_outport_free_vec[out_pc] = false;

        // pipeline stage latency
        m_pipe_lat_SSA_tab->tabulate(simtime() - peek_flit->m_clk_enter_stage);
        peek_flit->m_clk_enter_stage = simtime();

        // change input module status: a header flit completes SA.
        assert(m_in_mod_vec[in_pc][in_vc].m_state == IN_MOD_V);
        m_in_mod_vec[in_pc][in_vc].m_state = IN_MOD_S;	// V->S
    }
}


void Router::depositCredit(Credit* p_credit)
{
    assert(p_credit);
    m_credit_deposit_vec.push_back(p_credit);
}

void Router::decCredit(int out_pc, int out_vc, int num_credits)
{
    if (isEjectChannel(out_pc))
        return;	// no credit management

    switch (g_cfg.router_buffer_type) {
    case ROUTER_BUFFER_SAMQ:
        m_out_mod_vec[out_pc][out_vc].m_num_credit -= num_credits;
        assert(m_out_mod_vec[out_pc][out_vc].m_num_credit >= 0);

#ifdef _DEBUG_CREDIT
        printf("decCredit SAMQ: router=%d clk=%.0lf m_num_credit[out_pc=%d][out_vc=%d]=%d\n", id(), simtime(), out_pc, out_vc, m_out_mod_vec[out_pc][out_vc].m_num_credit);
#endif

        break;
    case ROUTER_BUFFER_DAMQ_P:
      {
        int num_out_vc = g_Router_vec[m_connNextRouter_vec[out_pc].first]->num_vc();
        int num_shared_credits = 0;    // total available credits for out_pc
        for (int vc=0; vc<num_out_vc; vc++)
            num_shared_credits += m_out_mod_vec[out_pc][vc].m_num_credit;

        if (num_shared_credits >= num_credits) { // can decrease shared credit?
            m_out_mod_vec[out_pc][out_vc].m_num_credit -= num_credits;
        } else {
            assert(m_out_mod_vec[out_pc][out_vc].m_num_credit_rsv >= num_credits);
            m_out_mod_vec[out_pc][out_vc].m_num_credit_rsv -= num_credits;
        }

#ifdef _DEBUG_CREDIT
        printf("decCredit DAMQ_P: router=%d clk=%.0lf out_pc=%d out_vc=%d\n", id(), simtime(), out_pc, out_vc);
        printf("    m_num_credit=%d m_num_credit_rsv=%d num_credits=%d num_shared_credits=%d\n", m_out_mod_vec[out_pc][out_vc].m_num_credit, m_out_mod_vec[out_pc][out_vc].m_num_credit_rsv, num_credits, num_shared_credits);
#endif

      }
        break;
    case ROUTER_BUFFER_DAMQ_R:
      {
        int num_shared_credits = 0;    // total available credits for router
        for (int pc=0; pc<num_internal_pc(); pc++) {
            int next_router_id = m_connNextRouter_vec[pc].first;
            int num_out_vc = (next_router_id == INVALID_ROUTER_ID) ? 0 : g_Router_vec[next_router_id]->num_vc();
            for (int vc=0; vc<num_out_vc; vc++)
                num_shared_credits += m_out_mod_vec[pc][vc].m_num_credit;
        }

        if (num_shared_credits >= num_credits) { // can decrease shared credit?
            m_out_mod_vec[out_pc][out_vc].m_num_credit -= num_credits;
        } else {
            assert(m_out_mod_vec[out_pc][out_vc].m_num_credit_rsv >= num_credits);
            m_out_mod_vec[out_pc][out_vc].m_num_credit_rsv -= num_credits;
        }

#ifdef _DEBUG_CREDIT
        printf("decCredit DAMQ_R: router=%d clk=%.0lf out_pc=%d out_vc=%d\n", id(), simtime(), out_pc, out_vc);
        printf("    m_num_credit=%d m_num_credit_rsv=%d num_credits=%d num_shared_credits=%d\n", m_out_mod_vec[out_pc][out_vc].m_num_credit, m_out_mod_vec[out_pc][out_vc].m_num_credit_rsv, num_credits, num_shared_credits);
#endif

      }
        break;
    default:
        assert(0);
    }

}

void Router::incCredit()
{
    if (m_credit_deposit_vec.size() == 0) // no credits to deposit?
        return;

    int num_org_credit_reqs = m_credit_deposit_vec.size(); // for sanity check
    int num_complete_credit_reqs = 0;
    int num_deposited_credits = 0;	// total deposited credits

    // deposit a credit to the corresponding output module
    for (vector< Credit* >::iterator pos=m_credit_deposit_vec.begin(); pos != m_credit_deposit_vec.end(); ++pos) {
        Credit* p_credit = *pos;

        if (p_credit->m_clk_deposit > simtime())
            break;	// all other deposited credits must be increased later.

        switch (g_cfg.router_buffer_type) {
        case ROUTER_BUFFER_SAMQ:
            m_out_mod_vec[p_credit->m_out_pc][p_credit->m_out_vc].m_num_credit += p_credit->m_num_credits;
            assert(m_out_mod_vec[p_credit->m_out_pc][p_credit->m_out_vc].m_num_credit <= m_inbuf_depth);
            break;
        case ROUTER_BUFFER_DAMQ_P:
        case ROUTER_BUFFER_DAMQ_R:
          {
            int num_shared_credits = p_credit->m_num_credits;
            if (m_out_mod_vec[p_credit->m_out_pc][p_credit->m_out_vc].m_num_credit_rsv < g_cfg.router_num_rsv_credit) {
                int num_rsv_credits = _MIN(g_cfg.router_num_rsv_credit - m_out_mod_vec[p_credit->m_out_pc][p_credit->m_out_vc].m_num_credit_rsv, p_credit->m_num_credits);
                num_shared_credits = p_credit->m_num_credits - num_rsv_credits;
                assert(num_shared_credits >= 0);

                m_out_mod_vec[p_credit->m_out_pc][p_credit->m_out_vc].m_num_credit_rsv += num_rsv_credits;
            }

            m_out_mod_vec[p_credit->m_out_pc][p_credit->m_out_vc].m_num_credit += num_shared_credits;
          }
            break;
        default:
            assert(0);
        }

#ifdef _DEBUG_CREDIT
        printf("incCredit router=%d deposit_clk=%.0lf clk=%.0lf out_pc=%d out_vc=%d credits=%d\n", id(), p_credit->m_clk_deposit, simtime(), p_credit->m_out_pc, p_credit->m_out_vc, m_out_mod_vec[p_credit->m_out_pc][p_credit->m_out_vc].m_num_credit);
#endif

        num_complete_credit_reqs++;
        num_deposited_credits += p_credit->m_num_credits;
        g_CreditPool.reclaim(p_credit);
    }

    // delete successfully deposited credits
    if (num_complete_credit_reqs > 0)
        m_credit_deposit_vec.erase(m_credit_deposit_vec.begin(), m_credit_deposit_vec.begin()+num_complete_credit_reqs);

#ifdef _DEBUG_CREDIT
    printf("incCredit router=%d clk=%.0lf\n", m_id, simtime());
    printf("  num_org_credit_reqs=%d\n", num_org_credit_reqs);
    printf("  num_complete_credit_reqs=%d\n", num_complete_credit_reqs);
    printf("  m_credit_deposit_vec.size()=%d\n", m_credit_deposit_vec.size());
    printf("  num_deposited_credits=%d\n", num_deposited_credits);
#endif

    assert((num_org_credit_reqs - num_complete_credit_reqs) == ((int) m_credit_deposit_vec.size()));
}

bool Router::hasCredit(int out_pc, int out_vc, int num_credits)
{
    switch (g_cfg.router_buffer_type) {
    case ROUTER_BUFFER_SAMQ:
        return (m_out_mod_vec[out_pc][out_vc].m_num_credit >= num_credits) ? true: false;
    case ROUTER_BUFFER_DAMQ_P:
      {

        int num_out_vc = g_Router_vec[m_connNextRouter_vec[out_pc].first]->num_vc();
        int num_shared_credits = 0;	// total shared credits for out_pc
        int num_total_credits = 0;
        for (int vc=0; vc<num_out_vc; vc++) {
            num_shared_credits += m_out_mod_vec[out_pc][vc].m_num_credit;
            num_total_credits += m_out_mod_vec[out_pc][vc].m_num_credit + m_out_mod_vec[out_pc][vc].m_num_credit_rsv;
        }
        assert(num_shared_credits >= 0);

#ifdef _DEBUG_CREDIT
        printf("hasCredit DAMQ_P router=%d clk=%.0lf out_pc=%d out_vc=%d num_credits=%d\n", id(), simtime(), out_pc, out_vc, num_credits);
        for (int vc=0; vc<num_out_vc; vc++)
            printf("  VC=%d credits=%d %d\n", vc, m_out_mod_vec[out_pc][vc].m_num_credit, m_out_mod_vec[out_pc][vc].m_num_credit_rsv);
        printf("  num_shared_credits=%d num_total_credits=%d\n", num_shared_credits, num_total_credits);
#endif

        if (num_shared_credits >= num_credits) { // has enough shared credits?
            return true;
        } else {
            // check reserved credits
            return (m_out_mod_vec[out_pc][out_vc].m_num_credit_rsv >= num_credits) ? true: false;
        }
      }
        break;
    case ROUTER_BUFFER_DAMQ_R:
      {
        int num_shared_credits = 0;
        int num_total_credits = 0;
        for (int pc=0; pc<num_internal_pc(); pc++) {
            int next_router_id = m_connNextRouter_vec[pc].first;
            int num_out_vc = next_router_id == INVALID_ROUTER_ID ? 0 : g_Router_vec[next_router_id]->num_vc();
            for (int vc=0; vc<num_out_vc; vc++) {
                num_shared_credits += m_out_mod_vec[pc][vc].m_num_credit;
                num_total_credits += m_out_mod_vec[pc][vc].m_num_credit + m_out_mod_vec[pc][vc].m_num_credit_rsv;
            }
        }
        assert(num_shared_credits >= 0);

#ifdef _DEBUG_CREDIT
        printf("hasCredit DAMQ_R router=%d clk=%.0lf out_pc=%d out_vc=%d num_credits=%d\n", id(), simtime(), out_pc, out_vc, num_credits);
        for (int pc=0; pc<num_internal_pc(); pc++) {
            int next_router_id = m_connNextRouter_vec[pc].first;
            int num_out_vc = next_router_id == INVALID_ROUTER_ID ? 0 : g_Router_vec[next_router_id]->num_vc();
            for (int vc=0; vc<num_out_vc; vc++)
                printf("  PC=%d VC=%d credits=%d %d\n", pc, vc, m_out_mod_vec[pc][vc].m_num_credit, m_out_mod_vec[pc][vc].m_num_credit_rsv);
        }
    printf("  num_shared_credits=%d num_total_credits=%d\n", num_shared_credits, num_total_credits);
#endif

        if (num_shared_credits >= num_credits) { // has enough shared credits?
            return true;
        } else {
            // check reserved credits
            return (m_out_mod_vec[out_pc][out_vc].m_num_credit_rsv >= num_credits) ? true: false;
        }
      }
        break;
    default:
        ;
    }

    assert(0);	// never reached
    return false;
}
