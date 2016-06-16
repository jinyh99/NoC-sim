#include "noc.h"
#include "Router.h"
#include "RouterPower.h"
#include "SwArb2Stage.h"

#ifdef _DEBUG_ROUTER_SA
int _debug_sa_router_id = 0;
double _debug_sa_clk = 0.0; 
#endif

SwArb2Stage::SwArb2Stage() : SwArb()
{
    assert(0);
}

SwArb2Stage::SwArb2Stage(Router* p_router, int v1_arb_config, int p1_arb_config)
{
    assert(p_router);
    m_router = p_router;
    m_v1_arb_config = v1_arb_config;
    m_p1_arb_config = p1_arb_config;

    init();

    // set all zeros to round-robin counters
    m_RR_sa_v1a_vec.resize(2);
    m_RR_sa_p1a_vec.resize(2);
    for (int i=0; i<2; i++) {
        m_RR_sa_v1a_vec[i].resize(m_num_pc, 0);	// per input PC
        m_RR_sa_p1a_vec[i].resize(m_num_pc, 0);	// per output PC
    }

    // LRS order
    m_LRS_sa_v1a_vec.resize(2);
    m_LRS_sa_p1a_vec.resize(2);
    for (int i=0; i<2; i++) {
        m_LRS_sa_v1a_vec[i].resize(m_num_pc);
        for (int in_pc=0; in_pc<m_num_pc; in_pc++) {
            m_LRS_sa_v1a_vec[i][in_pc].resize(m_num_vc);
            for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
                m_LRS_sa_v1a_vec[i][in_pc][in_vc] = in_vc;
            }
        }

        m_LRS_sa_p1a_vec[i].resize(m_num_pc);
        for (int out_pc=0; out_pc<m_num_pc; out_pc++) {
            m_LRS_sa_p1a_vec[i][out_pc].resize(m_num_pc);
            for (int in_pc=0; in_pc<m_num_pc; in_pc++) {
                m_LRS_sa_p1a_vec[i][out_pc][in_pc] = in_pc;
            }
        }
    }
}

SwArb2Stage::~SwArb2Stage()
{
}

vector< pair< pair< int, int >, pair< int, int > > > SwArb2Stage::grantGeneric(bool spec)
{
#ifdef _DEBUG_ROUTER_SA
    if (m_router->id() == _debug_sa_router_id && simtime() > _debug_sa_clk) {
        fprintf(stdout, "-- SA%s status router=%d clk=%.0lf --\n", (spec ? "-S" : ""), m_router->id(), simtime());
        printCurStatus(stdout);
    }
#endif

    // grant vectors for final result
    vector< pair< pair< int, int >, pair< int, int > > > grant_vec;

    if (m_num_reqs == 0) // no request ?
        return grant_vec;

    const unsigned int cpos = (spec ? 1 : 0);	// counter position: regular(=0)/spec(=1)

    vector< SwReq* > selected_in_vc_vec;	// a selected VC per input PC after V:1 arbitration
    selected_in_vc_vec.resize(m_num_pc, 0);

    // Select one input VC in the same PC. (V:1 arb)
    switch (m_v1_arb_config) {
    case SW_ALLOC_RR:	// round-robin
        // find the next closest requesting VC to the previously chosen VC.
        for (int in_pc=0; in_pc<m_num_pc; in_pc++)
        for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
            int test_in_vc = (m_RR_sa_v1a_vec[cpos][in_pc] + in_vc) % m_num_vc;
            SwReq & req = m_swReq_vec[in_pc][test_in_vc];

            if (! req.m_requesting ) // no request for in_pc ?
                continue;

            if (spec) { // speculative SA ?
                Flit* p_flit = m_router->flitQ()->peek(in_pc, test_in_vc);
                if (! p_flit->isHead()) // not a head flit ?
                    continue;

                if (req.req_clk != simtime()) // not simultaneously with VA ?
                    continue;
            }

            selected_in_vc_vec[req.in_pc] = &req;
            assert(test_in_vc == req.in_vc);

            // increase RR counter
            m_RR_sa_v1a_vec[cpos][in_pc] = (test_in_vc + 1) % m_num_vc;

            break;
        }
        break;

    case SW_ALLOC_LRS: // least recently selected
        // matrix arbitration
        for (int in_pc=0; in_pc<m_num_pc; in_pc++)
        for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
            int test_in_vc = m_LRS_sa_v1a_vec[cpos][in_pc][in_vc];
            SwReq & req = m_swReq_vec[in_pc][test_in_vc];

            if (! req.m_requesting ) // no request for in_pc ?
                continue;

            if (spec) { // speculative SA ?
                Flit* p_flit = m_router->flitQ()->peek(in_pc, test_in_vc);
                if (! p_flit->isHead()) // not a head flit ?
                    continue;

                if (req.req_clk != simtime()) // not simultaneously with VA ?
                    continue;
            }

            selected_in_vc_vec[req.in_pc] = &req;
            assert(test_in_vc == req.in_vc);

            // update LRS counter
            int old_lrs_order = V1_LRS_ORDER(cpos, req.in_pc, req.in_vc);
            for (int new_lrs_order=old_lrs_order; new_lrs_order<m_num_vc-1; new_lrs_order++)
                m_LRS_sa_v1a_vec[cpos][in_pc][new_lrs_order] = m_LRS_sa_v1a_vec[cpos][in_pc][new_lrs_order+1];
            m_LRS_sa_v1a_vec[cpos][in_pc][m_num_vc-1] = req.in_vc;

#ifdef _DEBUG_ROUTER_SA
            // print LRS order
            if (m_router->id() == _debug_sa_router_id && simtime() > _debug_sa_clk) {
                printf("LRS_ORDER(VC) IN_PC=%d: ordered VCs\n", in_pc);
                for (int i=0; i<m_num_vc; i++) printf("%d ", m_LRS_sa_v1a_vec[cpos][in_pc][i]);
                printf("\n");
            }
#endif

            break;
        }
        break;

    default:
        assert(0);
    }
  

#ifdef _DEBUG_ROUTER_SA
    if (m_router->id() == _debug_sa_router_id && simtime() > _debug_sa_clk) {
        printf("V:1 arbitration result:\n");
        for (int in_pc=0; in_pc<m_num_pc; in_pc++) {
            if (selected_in_vc_vec[in_pc]==0) continue;
            SwReq & req = *(selected_in_vc_vec[in_pc]);
            Flit* p_flit = m_router->flitQ()->peek(req.in_pc, req.in_vc);
            printf("  IN(pc=%d vc=%d) OUT(pc=%d vc=%d) p=%lld f=%lld req_clk=%.1lf\n",
            req.in_pc, req.in_vc, req.out_pc, req.out_vc,
            p_flit->getPkt()->id(), p_flit->id(),
            req.req_clk);
        }
    }
#endif


    vector< SwReq* > selected_in_pc_vec;	// a selected input PC (SwReq) per output PC after P:1 arbitration
    selected_in_pc_vec.resize(m_num_pc, 0);
#ifdef _DEBUG_ROUTER_SA
    vector< string > sa_fail_str_vec;
    char sa_debug_msg[128];
#endif

    // Select one input PC among input PCs that compete the same output PC. (P:1 arb)
    for (int in_pc=0; in_pc<m_num_pc; in_pc++) {
        if (selected_in_vc_vec[in_pc] == 0)	// no selected input VC from V:1 arb ?
            continue;

        SwReq & req = *(selected_in_vc_vec[in_pc]);	// a request in input PC
        assert(in_pc == req.in_pc);
        int in_vc = req.in_vc;
        int out_pc = req.out_pc;

        // check xbar availability for input/output ports
        const XBar& xbar = m_router->getXBar();
        if (xbar.m_waiting_in_vc_vec[in_pc] != INVALID_VC) { // input port reserved?
#ifdef _DEBUG_ROUTER_SA
        if (m_router->id() == _debug_sa_router_id && simtime() > _debug_sa_clk) {
            sprintf(sa_debug_msg, "IN(pc=%d vc=%d): input port reserved", in_pc, in_vc);
            sa_fail_str_vec.push_back(string(sa_debug_msg));
        }
#endif
            continue;
        }

        if (! xbar.m_outport_free_vec[out_pc] ) { // output port reserved?
#ifdef _DEBUG_ROUTER_SA
            if (m_router->id() == _debug_sa_router_id && simtime() > _debug_sa_clk) {
                sprintf(sa_debug_msg, "IN(pc=%d vc=%d) OUT(pc=%d): output port reserved", in_pc, in_vc, out_pc);
                sa_fail_str_vec.push_back(string(sa_debug_msg));
            }
#endif
            continue;
        }

        // check an available credit for downstream router
        // NOTE: If out_pc is an ejection channel,
        //       we do not check credit availability (infinite credit assumption).
        if (! m_router->isEjectChannel(out_pc)) { // not eject channel?
            int out_vc = m_router->inputModule(in_pc, in_vc).m_out_vc;

            // 03/14/08 credit-based flow control
            if (! m_router->hasCredit(out_pc, out_vc, 1) ) { // no credit?
#ifdef _DEBUG_ROUTER_SA
                if (m_router->id() == _debug_sa_router_id && simtime() > _debug_sa_clk) {
                    sprintf(sa_debug_msg, "IN(pc=%d vc=%d) OUT(router=%d out_pc=%d out_vc=%d hasCredit?=%c)", in_pc, in_vc, m_router->id(), out_pc, out_vc, m_router->hasCredit(out_pc, out_vc, 1) ? 'Y' : 'N');
                    sa_fail_str_vec.push_back(string(sa_debug_msg));
                }
#endif
                continue;
            }
        }

        if (selected_in_pc_vec[out_pc]) {	// outport contention?
            switch (m_p1_arb_config) {
            case SW_ALLOC_RR: // P:1 round-robin
                if (RR_ORDER(req.in_pc, m_RR_sa_p1a_vec[cpos][out_pc], m_num_pc-1) <
                    RR_ORDER(selected_in_pc_vec[out_pc]->in_pc, m_RR_sa_p1a_vec[cpos][out_pc], m_num_pc-1)) { // closer to the index ?
                    selected_in_pc_vec[out_pc] = &req;    // replace selected_in_pc_vec[out_pc] with req
                }
                break;

            case SW_ALLOC_LRS: // P:1 LRS
              {
                int test_lrs_order = P1_LRS_ORDER(cpos, out_pc, req.in_pc);
                int selected_lrs_order = P1_LRS_ORDER(cpos, out_pc, selected_in_pc_vec[out_pc]->in_pc);
#ifdef _DEBUG_ROUTER_SA
                if (m_router->id() == _debug_sa_router_id && simtime() > _debug_sa_clk) {
                    printf(" OUT_PC=%d test_lrs_order(in_pc=%d)=%d selected_lrs_order(in_pc=%d)=%d\n", out_pc, req.in_pc, test_lrs_order, selected_in_pc_vec[out_pc]->in_pc, selected_lrs_order);
                }
#endif
                if (test_lrs_order < selected_lrs_order) {
                    selected_in_pc_vec[out_pc] = &req;    // replace selected_in_pc_vec[out_pc] with req
                }
              }
                break;

            default:
                assert(0);
            }
        } else {
            selected_in_pc_vec[out_pc] = &req;
        }
    }


    // move granted requests into selected_SA_vec and update counters.
    vector< SwReq* > selected_SA_vec;	// final SA flows to be allocated
    for (int out_pc=0; out_pc<m_num_pc; out_pc++) {
        if (selected_in_pc_vec[out_pc]) {
            selected_SA_vec.push_back(selected_in_pc_vec[out_pc]);

            SwReq & req = *selected_in_pc_vec[out_pc];

            switch (m_p1_arb_config) {
            case SW_ALLOC_RR: // P:1 round-robin
                // increase RR counter
                m_RR_sa_p1a_vec[cpos][req.out_pc] = (req.in_pc + 1) % m_num_pc;
                break;
            case SW_ALLOC_LRS: // P:1 LRS
#if 0
#ifdef _DEBUG_ROUTER_SA
                 // verify LRS order
                 if (m_router->id() == _debug_sa_router_id && simtime() > _debug_sa_clk) {
                     printf("before LRS_ORDER(PC) out_pc=%d in_pc=%d: \n", req.out_pc, req.in_pc);
                     for (int i=0; i<m_num_pc; i++) printf("%d ", m_LRS_sa_p1a_vec[cpos][req.out_pc][i]);
                     printf("\n");
                 }
#endif
#endif
              {
                // update LRS counter
                int old_lrs_order = P1_LRS_ORDER(cpos, req.out_pc, req.in_pc);
                for (int new_lrs_order=old_lrs_order; new_lrs_order<m_num_pc-1; new_lrs_order++)
                    m_LRS_sa_p1a_vec[cpos][req.out_pc][new_lrs_order] = m_LRS_sa_p1a_vec[cpos][req.out_pc][new_lrs_order+1];
                m_LRS_sa_p1a_vec[cpos][req.out_pc][m_num_pc-1] = req.in_pc; // MRS (=lowest priority)
              }
#if 0
#ifdef _DEBUG_ROUTER_SA
                 // verify LRS order
                 if (m_router->id() == _debug_sa_router_id && simtime() > _debug_sa_clk) {
                     printf("after LRS_ORDER(PC) out_pc=%d: \n", req.out_pc);
                     for (int i=0; i<m_num_pc; i++) printf("%d ", m_LRS_sa_p1a_vec[cpos][req.out_pc][i]);
                     printf("\n");
                 }
#endif
#endif

                break;
            default:
                assert(0);
            }

        }
    }


#ifdef _DEBUG_ROUTER_SA
    if (m_router->id() == _debug_sa_router_id && simtime() > _debug_sa_clk) {
        printf("P:1 arbitration result:\n");
        for (unsigned int n=0; n<selected_SA_vec.size(); n++) {
            SwReq & req = *(selected_SA_vec[n]);
            Flit* p_flit = m_router->flitQ()->peek(req.in_pc, req.in_vc);
            printf("  IN(pc=%d vc=%d) OUT(pc=%d vc=%d) p=%lld f=%lld req_clk=%.1lf\n",
                   req.in_pc, req.in_vc, req.out_pc, req.out_vc,
                   p_flit->getPkt()->id(), p_flit->id(),
                   req.req_clk);
        }

        for (unsigned int n=0; n<sa_fail_str_vec.size(); n++)
            printf("  %s\n", sa_fail_str_vec[n].c_str());

        printf("----------------------------------------\n");
    }
#endif


    // build grant vectors ((in_pc, in_vc), (out_pc, out_vc))
    for (unsigned int n=0; n<selected_SA_vec.size(); n++) {
        SwReq & req = *(selected_SA_vec[n]);

        grant_vec.push_back(make_pair(make_pair(req.in_pc, req.in_vc), make_pair(req.out_pc, req.out_vc)));

        // decrease credit
        m_router->decCredit(req.out_pc, req.out_vc, 1);
    }

    // record power (V:1 arbiter)
    for (unsigned int n=0; n<selected_SA_vec.size(); n++) {
        SwReq & req = *(selected_SA_vec[n]);
        int grant_in_pc = req.in_pc;
        int grant_in_vc = req.in_vc;

        bitset< max_sz_sw_arb > sa_v1_request = 0;
        bitset< max_sz_sw_arb > sa_v1_grant = 0;

        // build a request
        for (int req_in_vc=0; req_in_vc<m_num_vc; req_in_vc++)
            if (m_swReq_vec[grant_in_pc][req_in_vc].m_requesting)
                sa_v1_request[req_in_vc] = true;

        // build a grant
        sa_v1_grant[grant_in_vc] = true;

        if (!g_sim.m_warmup_phase) {
            m_router->m_power_tmpl->record_v1_sw_arb(grant_in_pc, (unsigned int) sa_v1_request.to_ulong(), (unsigned int) sa_v1_grant.to_ulong());

            if (g_cfg.profile_power) {
                m_router->m_power_tmpl_profile->record_v1_sw_arb(grant_in_pc, (unsigned int) sa_v1_request.to_ulong(), (unsigned int) sa_v1_grant.to_ulong());
            }
        }
    }

    // record power (P:1 arbiter)
    for (unsigned int n=0; n<selected_SA_vec.size(); n++) {
        SwReq & req = *(selected_SA_vec[n]);
        int grant_in_pc = req.in_pc;
        int grant_out_pc = req.out_pc;

        bitset< max_sz_sw_arb > sa_p1_request = 0;
        bitset< max_sz_sw_arb > sa_p1_grant = 0;

        // build a request
        for (unsigned int m=0; m<selected_SA_vec.size(); m++) {
            SwReq & req = *(selected_SA_vec[m]);
            if (grant_out_pc == req.out_pc)
                sa_p1_request[req.in_pc] = true;
        }

        // build a grant
        sa_p1_grant[grant_in_pc] = true;

        if (!g_sim.m_warmup_phase) {
            m_router->m_power_tmpl->record_p1_sw_arb(grant_out_pc, (unsigned int) sa_p1_request.to_ulong(), (unsigned int) sa_p1_grant.to_ulong());

            if (g_cfg.profile_power) {
                m_router->m_power_tmpl_profile->record_p1_sw_arb(grant_out_pc, (unsigned int) sa_p1_request.to_ulong(), (unsigned int) sa_p1_grant.to_ulong());
            }
        }
    }

    // stats
    double grant_rate = ((double) selected_SA_vec.size()) / m_num_reqs;
    if (spec) 
        m_spec_grant_rate_tab->tabulate(grant_rate);
    else
        m_grant_rate_tab->tabulate(grant_rate);

    // delete all granted requests
    for (unsigned int n=0; n<selected_SA_vec.size(); n++)
        del(*(selected_SA_vec[n]));

    return grant_vec;
} // SwArb2Stage::grantGeneric(bool spec)
