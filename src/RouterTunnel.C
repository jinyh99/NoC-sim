#include "noc.h"
#include "Router.h"
#include "NIOutput.h"
#include "SwArb2Stage.h"
#include "Routing.h"
#include "RouterPower.h"
#include "RouterPowerOrion.h"
#include "RouterPowerStats.h"

void Router::stageTN()
{
    for (int in_pc=0; in_pc<m_num_pc; in_pc++) {
        int tunnel_in_vc = m_tunnel_info_vec[in_pc].m_in_vc;
        int tunnel_out_pc = m_tunnel_info_vec[in_pc].m_out_pc;
        int tunnel_out_vc = m_tunnel_info_vec[in_pc].m_out_vc;

        if (tunnel_in_vc == INVALID_VC)	// support tunneling ?
            continue;

        if (m_flitQ->isEmpty(in_pc, tunnel_in_vc)) // has a flit ?
            continue;

        // peek one flit from the input buffer
        Flit* p_flit = m_flitQ->peek(in_pc, tunnel_in_vc);
        assert(p_flit);

        switch (p_flit->type()) {
        case HEAD_FLIT:
        case ATOM_FLIT:
          {
            FlitHead* p_head_flit = (FlitHead*) p_flit;
            switch(g_cfg.router_tunnel_type) {
            case TUNNELING_PER_FLOW:
                if (m_tunnel_info_vec[in_pc].m_flow != make_pair(p_head_flit->src_router_id(), p_head_flit->dest_router_id()))
                    goto NO_TUNNEL;
                break;
            case TUNNELING_PER_DEST:
                if (m_tunnel_info_vec[in_pc].m_dest_router_id != p_head_flit->dest_router_id())
                    goto NO_TUNNEL;
                break;
            case TUNNELING_PER_OUTPORT:
                break;
            default:
                assert(0);
            }

            // check input module status
            if (m_in_mod_vec[in_pc][tunnel_in_vc].m_state != IN_MOD_I)
                goto NO_TUNNEL;

            // Step 1.1: do routing
            int out_pc = g_Routing->selectOutPC(this, tunnel_in_vc, (FlitHead*) p_flit);
            assert(out_pc < (int) m_connNextRouter_vec.size());
            int next_router_id = m_connNextRouter_vec[out_pc].first;
            int next_in_pc = m_connNextRouter_vec[out_pc].second;

            if (tunnel_out_pc != out_pc)
                goto NO_TUNNEL;

            // Step 2.1: reserve VC
            if (! isEjectChannel(out_pc)) {
                if (m_out_mod_vec[out_pc][tunnel_out_vc].m_state != OUT_MOD_I ) // reserved ?
                    goto NO_TUNNEL;
            }

            // Step 3.1: check no request in SA for designated input and output ports
            // FIXME: do we need this?
            // if (! m_sw_alloc->hasNoReq(in_pc, out_pc))
            //    goto NO_TUNNEL;

            // 03/14/08 credit-based flow control
            if (! hasCredit(out_pc, tunnel_out_vc) ) // no credit?
                goto NO_TUNNEL;

            // Step 4.1: check no flit in xbar for designated input and output ports
            if (m_xbar.m_waiting_in_vc_vec[in_pc] != INVALID_VC || ! m_xbar.m_outport_free_vec[out_pc])
                goto NO_TUNNEL;

            // Step 5.1: check no flit in a link
            if (! isEjectChannel(out_pc)) {
                if (((int) m_link_vec[out_pc].m_w_pipeline.size()) >= m_link_vec[out_pc].m_delay_factor*g_cfg.link_latency)
                    goto NO_TUNNEL;
            }

            // Now checking is done for tunneling.

            // Step 2.2: reserve VC
            if (! isEjectChannel(out_pc)) {
                m_out_mod_vec[out_pc][tunnel_out_vc].m_state = OUT_MOD_V;
            }

            // create a credit and send it to upstream router
            int prev_router_id = m_connPrevRouter_vec[in_pc].first;
            int prev_out_pc = m_connPrevRouter_vec[in_pc].second;
            if (prev_router_id != INVALID_ROUTER_ID && prev_out_pc != INVALID_PC) {
                Credit* p_credit = g_CreditPool.alloc();

                p_credit->m_out_pc = prev_out_pc;
                p_credit->m_out_vc = tunnel_in_vc;
                p_credit->m_num_credits = 1;
                p_credit->m_clk_deposit = simtime() + g_Router_vec[prev_router_id]->getLink(prev_out_pc).m_delay_factor * g_cfg.link_latency;

                g_Router_vec[prev_router_id]->depositCredit(p_credit);
            }

            // 11/05/05: no stall at ST stage
            if (! isEjectChannel(out_pc)) {
                assert(next_router_id != INVALID_PC);
                assert(next_in_pc != INVALID_PC);
            }
            // pipeline stage latency
            m_pipe_lat_ST_tab->tabulate(simtime() - p_flit->m_clk_enter_stage);
            p_flit->m_clk_enter_stage = simtime();

            // assert(m_xbar.m_waiting_in_vc_vec[in_pc] == INVALID_VC);
            // m_xbar.m_waiting_in_vc_vec[in_pc] = tunnel_in_vc;

            // assert(m_xbar.m_outport_free_vec[out_pc] == true);
            // m_xbar.m_outport_free_vec[out_pc] = false;

            // Step 4.2: traverse a crossbar
            // move flit to the link
            if (isEjectChannel(out_pc)) {
                // select ejection port
                int epc = p_flit->getPkt()->m_NI_out_pos;
                NIOutput* p_ni_output = getNIOutput(epc);
                assert(p_ni_output);
                assert(m_id == p_flit->getPkt()->getDestRouterID());

                p_ni_output->writeFlit(p_flit);

                // 03/15/06 fast simulation
                m_num_flits_inside--;
            } else {
                m_link_vec[out_pc].m_w_pipeline.push_back(make_pair(p_flit, make_pair(tunnel_out_vc, simtime())));
                // decrease credit
                decCredit(out_pc, tunnel_out_vc, 1);
            }

            // read a flit
            m_flitQ->read(in_pc, tunnel_in_vc);

            // bypass pipeline
            m_num_tunnel_flit_vec[in_pc][tunnel_in_vc]++;
printf("here1\n");

#ifdef _DEBUG_CHECK_BUFFER_INTEGRITY
            assert(checkBufferIntegrity(in_pc, tunnel_in_vc, p_flit));
#endif
            // change input module status for only multi-flit packets
            assert(m_in_mod_vec[in_pc][tunnel_in_vc].m_state == IN_MOD_I);
            if (p_flit->type() != ATOM_FLIT) {
                m_in_mod_vec[in_pc][tunnel_in_vc].m_state = IN_MOD_S;
                m_in_mod_vec[in_pc][tunnel_in_vc].m_out_pc = out_pc;
                m_in_mod_vec[in_pc][tunnel_in_vc].m_out_vc = tunnel_out_vc;
            }
// printf("TN3 router=%d m_in_mod_vec[%d][%d]: state=%d, out_pc=%d, out_vc=%d\n", id(), in_pc, tunnel_in_vc, m_in_mod_vec[in_pc][tunnel_in_vc].m_state, m_in_mod_vec[in_pc][tunnel_in_vc].m_out_pc, m_in_mod_vec[in_pc][tunnel_in_vc].m_out_vc);


#ifdef _DEBUG_ROUTER
            debugTN(p_flit, in_pc, tunnel_in_vc, out_pc, tunnel_out_vc);
#endif

          }
            break;
        case MIDL_FLIT:
        case TAIL_FLIT:
          {
            assert(m_in_mod_vec[in_pc][tunnel_in_vc].m_state == IN_MOD_S);
            int out_pc = m_in_mod_vec[in_pc][tunnel_in_vc].m_out_pc;
            int out_vc = m_in_mod_vec[in_pc][tunnel_in_vc].m_out_vc;

            if (tunnel_out_pc != out_pc)
                goto NO_TUNNEL;

            if (tunnel_out_vc != out_vc)	// output VC for tunneling is determined at previous packet delivery.
                goto NO_TUNNEL;

            // Step 3.1: check no request in SA for designated input and output ports
            // FIXME: do we need this?
            // if (! m_sw_alloc->hasNoReq(in_pc, out_pc))
            //    goto NO_TUNNEL;

            // check no flit in xbar for designated input and output ports
            if (m_xbar.m_waiting_in_vc_vec[in_pc] != INVALID_VC || ! m_xbar.m_outport_free_vec[out_pc])
                goto NO_TUNNEL;

            // 03/14/08 credit-based flow control
            if (! hasCredit(out_pc, out_vc) ) // no credit?
                goto NO_TUNNEL;

            // check no flit in a link
            if (! isEjectChannel(out_pc)) {
                if (((int) m_link_vec[out_pc].m_w_pipeline.size()) >= m_link_vec[out_pc].m_delay_factor*g_cfg.link_latency)
                    goto NO_TUNNEL;
            }

            // Now checking is done for tunneling.

            // delete switch allocation status if reserved
            m_sw_arb->del(in_pc, tunnel_in_vc);

            // create a credit and send it to upstream router
            int prev_router_id = m_connPrevRouter_vec[in_pc].first;
            int prev_out_pc = m_connPrevRouter_vec[in_pc].second;
            if (prev_router_id != INVALID_ROUTER_ID && prev_out_pc != INVALID_PC) {
                Credit* p_credit = g_CreditPool.alloc();

                p_credit->m_out_pc = prev_out_pc;
                p_credit->m_out_vc = tunnel_in_vc;
                p_credit->m_num_credits = 1;
                p_credit->m_clk_deposit = simtime() + g_Router_vec[prev_router_id]->getLink(prev_out_pc).m_delay_factor * g_cfg.link_latency;

                g_Router_vec[prev_router_id]->depositCredit(p_credit);
            }

            // Step 4: traverse a crossbar
            // move flit to the link
            if (isEjectChannel(out_pc)) {
                // select ejection port
                int epc = p_flit->getPkt()->m_NI_out_pos;
                NIOutput* p_ni_output = getNIOutput(epc);
                assert(p_ni_output);
                assert(m_id == p_flit->getPkt()->getDestRouterID());

                p_ni_output->writeFlit(p_flit);

                // 03/15/06 fast simulation
                m_num_flits_inside--;
            } else {
                m_link_vec[out_pc].m_w_pipeline.push_back(make_pair(p_flit, make_pair(out_vc, simtime())));
                // decrease credit
                decCredit(out_pc, out_vc, 1);
            }

            // clear status of input module
            if (p_flit->isTail()) {
//              assert(eject_vc_sts[epc][out_vc] == false);
//              eject_vc_sts[epc][out_vc] = true;
                m_in_mod_vec[in_pc][tunnel_in_vc].m_state = IN_MOD_I;
                m_in_mod_vec[in_pc][tunnel_in_vc].m_out_pc = INVALID_PC;
                m_in_mod_vec[in_pc][tunnel_in_vc].m_out_vc = INVALID_VC;
            }

            // read a flit
            m_flitQ->read(in_pc, tunnel_in_vc);

            // bypass pipeline
            m_num_tunnel_flit_vec[in_pc][tunnel_in_vc]++;
printf("here2\n");

#ifdef _DEBUG_CHECK_BUFFER_INTEGRITY
            assert(checkBufferIntegrity(in_pc, tunnel_in_vc, p_flit));
#endif

#ifdef _DEBUG_ROUTER
            debugTN(p_flit, in_pc, tunnel_in_vc, out_pc, out_vc);
#endif
          }
            break;
        default:
            assert(0);
        } // switch (p_flit->type()) {

NO_TUNNEL:
        ;
    } // for (int in_pc=0; in_pc<m_num_pc; in_pc++) {

    return;
}
