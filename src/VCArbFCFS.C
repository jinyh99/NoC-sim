#include "noc.h"
#include "Router.h" 
#include "VCArbFCFS.h"
#include "Routing.h"

#ifdef _DEBUG_ROUTER_VA
static double _va_debug_clk = 3.0;
static int _va_debug_router_id = 20;
#endif

VCArbFCFS::VCArbFCFS()
{
    assert(0);
}

VCArbFCFS::VCArbFCFS(Router* p_router) : VCArb()
{
    assert(p_router);
    m_router = p_router;
    m_num_pc = m_router->num_pc();
    m_num_vc = m_router->num_vc();
    m_reqQ.reserve(m_num_pc*m_num_vc);
}

VCArbFCFS::~VCArbFCFS()
{
    delete m_grant_rate_tab;
    delete m_num_req_tab;
}

void VCArbFCFS::add(int in_pc, int in_vc, int out_pc)
{
    m_reqQ.push_back(make_pair(make_pair(in_pc, in_vc), out_pc));
}

void VCArbFCFS::del(int in_pc, int in_vc)
{
    // already deleted when doing grant()
}

vector< pair< pair< int, int >, int > > VCArbFCFS::grant()
{
    vector< pair< pair< int, int >, int > > grant_vec;	// return value: < <in_pc, in_vc>, out_vc>

#ifdef _DEBUG_ROUTER_VA
    if (m_router->id() == _va_debug_router_id && simtime() > _va_debug_clk) {
        printf("VA_debug: router=%d clk=%.0lf m_reqQ.size()=%d\n", m_router->id(), simtime(), m_reqQ.size());
        printf("  m_reqQ: ");
        printReqQ(stdout);

        for (int out_pc=0; out_pc<m_num_pc; out_pc++) {
            printf("  OUT_PC%d: ", out_pc);
            int next_router_id = m_router->nextRouters()[out_pc].first;
            int next_in_pc = m_router->nextRouters()[out_pc].second;
            if (next_router_id == INVALID_ROUTER_ID) {
                printf("no-connection\n");
                continue;
            }
            for (int out_vc=0; out_vc<m_num_vc; out_vc++) {
                printf("VC%d=%c ", out_vc, (m_router->outputModule(out_pc, out_vc).m_state == OUT_MOD_I) ? 'f' : 'r');
            }
            printf("\n");
        }
        printf("\n");
    }
#endif // #ifdef _DEBUG_ROUTER_VA

    vector< int > del_vc_req_index_vec;
    del_vc_req_index_vec.reserve(m_reqQ.size());

    // check each request of the queue in the FIFO order
    for (unsigned int i=0; i<m_reqQ.size(); i++) {
        int in_pc = m_reqQ[i].first.first;
        int in_vc = m_reqQ[i].first.second;
        int out_pc = m_reqQ[i].second;
        int out_vc = INVALID_VC;	// arbitration result

        assert(out_pc != INVALID_PC);
        assert(out_pc == m_router->inputModule(in_pc, in_vc).m_out_pc);
        assert(m_router->inputModule(in_pc, in_vc).m_state == IN_MOD_R);

        Flit* p_flit = m_router->flitQ()->peek(in_pc, in_vc);
        assert(p_flit);

        // find a free output VC
        for (int search_out_vc=0; search_out_vc<m_num_vc; ++search_out_vc) {
            if (m_router->outputModule(out_pc, search_out_vc).m_state == OUT_MOD_I &&
                g_Routing->isOutVCDeadlockFree(search_out_vc, in_pc, in_vc, out_pc, m_router, (FlitHead*) p_flit)) { // free & deadlock-free ?

                out_vc = search_out_vc;
                break;
            }
        }

        if (out_vc == INVALID_VC) { // no free VC?
#ifdef _DEBUG_ROUTER_VA
            if (m_router->id() == _va_debug_router_id && simtime() > _va_debug_clk) {
                printf("  NO_FREE_VC: in_pc=%d in_vc=%d out_pc=%d\n", in_pc, in_vc, out_pc);
            }
#endif
            continue;
        }
        assert(out_vc >= 0 && out_vc < m_num_vc);

#ifdef _DEBUG_ROUTER_VA
        if (m_router->id() == _va_debug_router_id && simtime() > _va_debug_clk)
            printf("  RESERVED: in_pc=%d in_vc=%d => out_pc=%d out_vc=%d\n", in_pc, in_vc, out_pc, out_vc);
#endif

        // change input module status (R->V)
        assert(m_router->inputModule(in_pc, in_vc).m_state == IN_MOD_R);
        assert(m_router->inputModule(in_pc, in_vc).m_out_vc == INVALID_VC);
        m_router->inputModule(in_pc, in_vc).m_state = IN_MOD_V;
        m_router->inputModule(in_pc, in_vc).m_out_vc = out_vc;

        // change output module status (I->V)
        assert(m_router->outputModule(out_pc, out_vc).m_state == OUT_MOD_I);
        m_router->outputModule(out_pc, out_vc).m_state = OUT_MOD_V;

        // add granted request to return value
        grant_vec.push_back(make_pair(make_pair(in_pc, in_vc), out_vc));

        // store request's index to delete request later
        del_vc_req_index_vec.push_back(i);
    }

    // stats
    if (m_reqQ.size() > 0) {
        double grant_rate = ((double) del_vc_req_index_vec.size()) / m_reqQ.size();
        m_grant_rate_tab->tabulate(grant_rate);
        m_num_req_tab->tabulate(m_reqQ.size());
    }

    // delete allocated VC requests in m_reqQ 
    for (unsigned int n=0; n<del_vc_req_index_vec.size(); n++)
        m_reqQ.erase(m_reqQ.begin() + del_vc_req_index_vec[n] - n);

#ifdef _DEBUG_ROUTER_VA
    if (m_router->id() == _va_debug_router_id && simtime() > _va_debug_clk) {
        printf("  m_reqQ.size()=%d\n", m_reqQ.size());

        printf("  VC alloc status:\n");
        for (int in_pc=0; in_pc<m_num_pc; in_pc++)
        for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
            int next_in_vc = m_router->inputModule(in_pc, in_vc).m_out_vc;
            if (next_in_vc != INVALID_VC) {
                int cur_out_pc = m_router->inputModule(in_pc, in_vc).m_out_pc;
                int next_in_pc = m_router->nextRouters()[cur_out_pc].second;
                printf("  PC%d VC%d: ", in_pc, in_vc);
                printf("out PC%d, next PC%d VC%d", cur_out_pc, next_in_pc, next_in_vc);
                printf("\n");
            }
        }
    }
#endif

    return grant_vec;
}

bitset< max_sz_vc_arb > VCArbFCFS::getReqBitVector(int out_pc)
{
    bitset< max_sz_vc_arb > bs;

    for (unsigned int i=0; i<m_reqQ.size(); i++) {
        if (out_pc == m_reqQ[i].second) {
            int in_pc = m_reqQ[i].first.first;
            int in_vc = m_reqQ[i].first.second;

            // NOTE: Orion limitation
            int arb_bit_pos = in_pc*m_num_vc + in_vc;
            if (arb_bit_pos > max_sz_vc_arb)
                arb_bit_pos %= max_sz_vc_arb;

            bs[arb_bit_pos] = true;
        }
    }

    return bs;
}

void VCArbFCFS::printReqQ(FILE* fp)
{
   // print from head(oldest one) to tail(youngest one == arrived most recently)
    for (unsigned int i=0; i<m_reqQ.size(); i++) {
        int in_pc = m_reqQ[i].first.first;
        int in_vc = m_reqQ[i].first.second;
        fprintf(fp, "f=%lld ", m_router->flitQ()->peek(in_pc, in_vc)->id());
    }
    fprintf(fp, "\n");
}
