#include "noc.h"
#include "Router.h"
#include "Topology.h"
#include "VCArb.h"
#include "SwArb.h"

#ifndef _DEBUG_ROUTER_START_CLK
static double _debug_router_start_clk = 0.0;
#else
static double _debug_router_start_clk = _DEBUG_ROUTER_START_CLK;
#endif

void
Router::debugStage(Flit* p_flit, const char* stage, int in_pc, int in_vc)
{
    if (simtime() < _debug_router_start_clk)
        return;

    fprintf(stdout, "clk=%-5.0lf router=%-2d: ", simtime(), m_id);
    p_flit->print();
    fprintf(stdout, "%s (in_pc=%d in_vc=%d)", stage, in_pc, in_vc);
    fprintf(stdout, "\n");
    fflush(stdout);
}

void
Router::debugIB(Flit* p_flit, int in_pc, int in_vc)
{
    if (simtime() < _debug_router_start_clk)
        return;

    fprintf(stdout, "clk=%-5.0lf router=%-2d: ", simtime(), m_id);
    p_flit->print();
    fprintf(stdout, "IB (router=%d in_pc=%d in_vc=%d size=%d)", m_id, in_pc, in_vc, m_flitQ->size(in_pc, in_vc));
    fprintf(stdout, "\n");
    fflush(stdout);
}

// debug routing stage
void
Router::debugRC(Flit* p_flit, int next_router_id, int in_pc, int in_vc, int out_pc, int next_in_pc)
{
    if (simtime() < _debug_router_start_clk)
        return;

    fprintf(stdout, "clk=%-5.0lf router=%-2d: ", simtime(), m_id);
    p_flit->print();

    fprintf(stdout, "RC %d=>%d(dest: core=%d router=%d) (in pc=%d:vc=%d)->(out pc=%d)(next_in pc=%d) ",
            m_id, next_router_id, ((FlitHead*) p_flit)->dest_core_id(), ((FlitHead*) p_flit)->dest_router_id(), in_pc, in_vc, out_pc, next_in_pc);

    switch (out_pc) {
    case DIR_WEST:    fprintf(stdout, "dir=W "); break;
    case DIR_EAST:    fprintf(stdout, "dir=E "); break;
    case DIR_NORTH:   fprintf(stdout, "dir=N "); break;
    case DIR_SOUTH:   fprintf(stdout, "dir=S "); break;
    default:
      if (out_pc < m_num_pc) {
        int external_pc_index = m_num_pc - out_pc;
        assert(external_pc_index > 0);
        fprintf(stdout, "dir=P%d ", external_pc_index-1);
      } else {
        assert(0);
      }
    }

    fprintf(stdout, "\n");
    fflush(stdout);
}

// debug VC allocation stage
void
Router::debugVA(Flit* p_flit, int in_pc, int in_vc, int out_pc, int out_vc, bool is_precompute)
{
    if (simtime() < _debug_router_start_clk)
        return;

    fprintf(stdout, "clk=%-5.0lf router=%-2d: ", simtime(), m_id);
    p_flit->print();
    fprintf(stdout, "VA (in_pc=%d in_vc=%d)->(out_pc=%d out_vc=%d) ", in_pc, in_vc, out_pc, out_vc);
    fprintf(stdout, "\n");
    fflush(stdout);
}

// debug SA allocation stage
void
Router::debugSA(Flit* p_flit, int xbar_in_pc, int xbar_in_vc, int xbar_out_pc, int xbar_out_vc, bool is_speculative, bool is_precompute)
{
    if (simtime() < _debug_router_start_clk)
        return;

    fprintf(stdout, "clk=%-5.0lf router=%-2d: ", simtime(), m_id);
    p_flit->print();
    if (is_speculative)
        fprintf(stdout, "S");
    fprintf(stdout, "SA (pc=%d vc=%d)->(pc=%d vc=%d) ",
            xbar_in_pc, xbar_in_vc, xbar_out_pc, xbar_out_vc);
    fprintf(stdout, "\n");
    fflush(stdout);
}

// debug ST(xbar traversal) stage
void
Router::debugST(Flit* p_flit, int xbar_in_pc, int xbar_out_pc, int next_router_id, int next_in_pc, int next_in_vc, bool isLastRouter)
{
    if (simtime() < _debug_router_start_clk)
        return;

    fprintf(stdout, "clk=%-5.0lf router=%-2d: ", simtime(), m_id);
    p_flit->print();
    if (isLastRouter) {
        fprintf(stdout, "ST pc=%d->pc=%d eject(vc=%d)",
                xbar_in_pc, xbar_out_pc, next_in_vc);
    } else {
        fprintf(stdout, "ST pc=%d->pc=%d next(router=%d pc=%d vc=%d)",
                xbar_in_pc, xbar_out_pc, next_router_id, next_in_pc, next_in_vc);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

// debug LT(link traversal) stage
void
Router::debugLT(Flit* p_flit, int out_pc)
{
    if (simtime() < _debug_router_start_clk)
        return;

    fprintf(stdout, "clk=%-5.0lf router=%-2d: ", simtime(), m_id);
    p_flit->print();
    fprintf(stdout, "LT out_pc=%d", out_pc);
    fprintf(stdout, "\n");
    fflush(stdout);
}

// debug EX(exit from router) stage
void
Router::debugEX(Flit* p_flit, int eject_pc, int eject_vc, NIOutput* NI_out)
{
    if (simtime() < _debug_router_start_clk)
        return;

    Packet* p_pkt = p_flit->getPkt();
    // latency
    int T_t = (int) ((simtime() - p_pkt->m_clk_gen));
    int T_n = (int) ((simtime() - p_pkt->m_clk_enter_net));
    int T_q = (int) (p_pkt->m_clk_enter_net - p_pkt->m_clk_gen);
    int T_h = p_pkt->m_hops*g_cfg.router_num_pipelines;
    int T_s = p_pkt->m_num_flits;
    int T_w = p_pkt->m_wire_delay;
    int T_c = T_t - T_h - T_w - T_s + 1; // FIXME: +1

    fprintf(stdout, "clk=%-5.0lf router=%-2d: ", simtime(), m_id);
    p_flit->print();
    fprintf(stdout, "EX (ec=%d vc=%d) ", eject_pc, eject_vc);
    if (p_flit->isTail()) {
        fprintf(stdout, "%2d->%2d ", p_pkt->getSrcRouterID(), p_pkt->getDestRouterID());
        fprintf(stdout, "hop=%-2d ", p_pkt->m_hops);
    }
    // fprintf(stdout, "NIout[id=%d pos=%d] ", NI_out->id(), NI_out->pos());
    fprintf(stdout, "T_t=%d T_c=%d ", T_t, T_c);
    fprintf(stdout, "T_h=%d T_s=%d T_w=%d T_q=%d T_n=%d ", T_h, T_s, T_w, T_q, T_n);
    // fprintf(stdout, "clk(gen=%.0lf enter_net=%.0lf) ", p_pkt->m_clk_gen, p_pkt->m_clk_enter_net);
    fprintf(stdout, "\n");
    fflush(stdout);
}

void
Router::debugTN(Flit* p_flit, int in_pc, int in_vc, int out_pc, int out_vc)
{
    if (simtime() < _debug_router_start_clk)
        return;

    fprintf(stdout, "clk=%-5.0lf router=%-2d: ", simtime(), m_id);
    p_flit->print();
    fprintf(stdout, "TN IN(pc=%d vc=%d) OUT(pc=%d vc=%d)", in_pc, in_vc, out_pc, out_vc);
    fprintf(stdout, "\n");
    fflush(stdout);
}

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// packet integrity check at the input buffer
#ifdef _DEBUG_CHECK_BUFFER_INTEGRITY
bool Router::checkBufferIntegrity(int pc, int vc, Flit* p_flit)
{
    switch (p_flit->type()) {
    case HEAD_FLIT:
    case ATOM_FLIT:
        m_debug_pkt_chk_mid_vec[pc][vc] = p_flit->getPkt()->id();
        m_debug_pkt_chk_fid_vec[pc][vc] = p_flit->id();
        break;

    case MIDL_FLIT:
    case TAIL_FLIT:
        if (m_debug_pkt_chk_mid_vec[pc][vc] != p_flit->getPkt()->id()) {
            fprintf(stdout, "m_debug_pkt_chk_mid[%d][%d][%d]=%lld p_flit->mid=%lld\n", m_id, pc, vc, m_debug_pkt_chk_mid_vec[pc][vc], p_flit->getPkt()->id());
            return false;
        }
        if (m_debug_pkt_chk_fid_vec[pc][vc] != (p_flit->id() - 1)) {
            fprintf(stdout, "m_debug_pkt_chk_fid[%d][%d][%d]=%lld p_flit->id()-1=%lld\n", m_id, pc, vc, m_debug_pkt_chk_fid_vec[pc][vc], p_flit->id()-1);
            return false;
        }
        m_debug_pkt_chk_fid_vec[pc][vc] = p_flit->id();
        break;
    default:
        assert(0);
    }

    return true;
}
#endif // #ifdef _DEBUG_CHECK_BUFFER_INTEGRITY
//////////////////////////////////////////////////////////////////

#ifdef _DEBUG_ROUTER_SNAPSHOT
void take_network_snapshot(FILE* fp)
{
    // to avoid multiple calls
    static double snapshot_clk = 0.0;
    if (snapshot_clk == simtime())
        return;
    snapshot_clk = simtime();

    fprintf(fp, "----- Snapshot begin clk=%.0lf -------------\n", simtime());

    for (unsigned int i=0; i<g_NIInput_vec.size(); i++)
        g_NIInput_vec[i]->takeSnapshot(fp);

    for (unsigned int i=0; i<g_Router_vec.size(); i++)
        g_Router_vec[i]->takeSnapshot(fp);

    fprintf(fp, "----- Snapshot end -------------------------\n");
}

void Router::takeSnapshot(FILE* fp)
{
    fprintf(fp, "----- router=%d -----------------------------\n", m_id);

    fprintf(fp, "Buffer:\n");
    for (int in_pc=0; in_pc<m_num_pc; in_pc++)
    for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
        fprintf(fp, "  PC%d VC%d: sz=%d: ", in_pc, in_vc, m_flitQ->size(in_pc, in_vc));
        m_flitQ->print(fp, in_pc, in_vc);
    }

    fprintf(fp, "Routing:\n");
    for (int in_pc=0; in_pc<m_num_pc; in_pc++)
    for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
        RouterInputModule & in_mod = m_in_mod_vec[in_pc][in_vc];
        if (in_mod.m_state != IN_MOD_I) {
            fprintf(fp, "  PC%d VC%d: ", in_pc, in_vc);

            switch (in_mod.m_out_pc) {
            case DIR_WEST:  fprintf(fp, "dir=W"); break;
            case DIR_EAST:  fprintf(fp, "dir=E"); break;
            case DIR_NORTH: fprintf(fp, "dir=N"); break;
            case DIR_SOUTH: fprintf(fp, "dir=S"); break;
            default:
              if (in_mod.m_out_pc < m_num_pc) {
                  int external_pc_index = m_num_pc - m_num_ipc;
                  assert(external_pc_index > 0);
                  fprintf(fp, "dir=P%d", external_pc_index-1);
              } else {
                  assert(0);
              }
            }
            fprintf(fp, "\n");
        }
    }

    fprintf(fp, "VC requests:\n");
    for (int in_pc=0; in_pc<m_num_pc; in_pc++)
    for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
        if (m_in_mod_vec[in_pc][in_vc].m_state == IN_MOD_R) {
            Flit* p_flit = m_flitQ->peek(in_pc, in_vc);
            fprintf(fp, "  PC%d VC%d: p=%lld f=%lld\n", in_pc, in_vc, p_flit->getPkt()->id(), p_flit->id());
        }
    }

    fprintf(fp, "VC allocator\n"); fprintf(fp, "  ");
    m_vc_arb->printCurStatus(fp);

    fprintf(fp, "VC allocation status:\n");
    for (int in_pc=0; in_pc<m_num_pc; in_pc++)
    for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
        int cur_out_pc = m_in_mod_vec[in_pc][in_vc].m_out_pc;
        int cur_out_vc = m_in_mod_vec[in_pc][in_vc].m_out_vc;
        int next_in_pc = m_connNextRouter_vec[cur_out_pc].second;
        if (cur_out_pc != INVALID_PC) {
            fprintf(fp, "  PC%d VC%d: out PC%d VC%d, next PC%d\n", in_pc, in_vc, cur_out_pc, cur_out_vc, next_in_pc);
        }
    }

    fprintf(fp, "SA requests:\n");
    m_sw_arb->printCurStatus(fp);

    fprintf(fp, "Crossbar:\n");
    for (int in_pc=0; in_pc<m_num_pc; in_pc++) {
        fprintf(fp, "  PC%d: ", in_pc);
        int in_vc = m_xbar.m_waiting_in_vc_vec[in_pc];
        if (in_vc != INVALID_VC) {
            Flit* p_flit = m_flitQ->peek(in_pc, in_vc);
            fprintf(fp, "p=%lld f=%lld in_vc=%d out_pc=%d out_vc=%d", p_flit->getPkt()->id(), p_flit->id(), in_vc, m_in_mod_vec[in_pc][in_vc].m_out_pc, m_in_mod_vec[in_pc][in_vc].m_out_vc);
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "Output Module: sts m_num_credit m_num_credit_rsv\n");
    for (int out_pc=0; out_pc<m_num_pc; out_pc++) {
        fprintf(fp, "  PC%d: ", out_pc);
        for (int out_vc=0; out_vc<m_num_vc; out_vc++) {
            fprintf(fp, "  VC%d:", out_vc);
            fprintf(fp, "  %c ",  m_out_mod_vec[out_pc][out_vc].m_state == OUT_MOD_I ? 'I' : 'V');
            fprintf(fp, "(%d,%d) ", m_out_mod_vec[out_pc][out_vc].m_num_credit, m_out_mod_vec[out_pc][out_vc].m_num_credit_rsv);
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "Link:\n");
    for (int out_pc=0; out_pc<m_num_pc; out_pc++) {
        fprintf(fp, "  OUT_PC%d: ", out_pc);
        Link& link = m_link_vec[out_pc];
        for (unsigned int i=0; i<link.m_w_pipeline.size(); i++) {
            Flit* p_flit = link.m_w_pipeline[i].first;
            int next_in_vc = link.m_w_pipeline[i].second.first;
            fprintf(fp, "[p=%lld f=%lld next_in_vc=%d], ", p_flit->getPkt()->id(), p_flit->id(), next_in_vc);
        }
        fprintf(fp, "\n");
    }

    fflush(fp);
}
#endif // #ifdef _DEBUG_ROUTER_SNAPSHOT
