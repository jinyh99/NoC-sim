#include "noc.h"
#include "Router.h" 
#include "SwArb.h"

SwArb::SwArb()
{
    m_grant_rate_tab = new table("grant_rate");
    m_spec_grant_rate_tab = new table("grant_rate");
}

SwArb::SwArb(Router* p_router, int v1_arb_config, int p1_arb_config)
{
    // abstract class
    assert(0);
}

SwArb::~SwArb()
{
    delete m_grant_rate_tab;
    delete m_spec_grant_rate_tab;
}

void SwArb::init()
{
    assert(m_router);
    m_num_pc = m_router->num_pc();
    m_num_vc = m_router->num_vc();

    m_swReq_vec.resize(m_num_pc);
    for (int in_pc=0; in_pc<m_num_pc; in_pc++) {
        m_swReq_vec[in_pc].resize(m_num_vc);
        for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
            del(m_swReq_vec[in_pc][in_vc]);
        }
    }

    m_num_reqs = 0;
}

// returns the vector of free (input PC, input VC) pairs
vector< pair<int, int> > SwArb::getFreeInPorts()
{
    vector< pair<int, int> > free_inport_vec;

    if (m_num_reqs < m_num_pc * m_num_vc) {
        for (int in_pc=0; in_pc<m_num_pc; in_pc++) {
            for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
                if (! m_swReq_vec[in_pc][in_vc].m_requesting)
                    free_inport_vec.push_back(make_pair(in_pc, in_vc));
            }
        }
    }

    return free_inport_vec;
}

bool SwArb::hasNoReq(int check_in_pc, int check_out_pc)
{
    // check input
    for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
        if (m_swReq_vec[check_in_pc][in_vc].m_requesting)
            return false;
    }

    // check output
    for (int in_pc=0; in_pc<m_num_vc; in_pc++)
    for (int in_vc=0; in_vc<m_num_vc; in_vc++) {
        if (m_swReq_vec[in_pc][in_vc].m_requesting && m_swReq_vec[in_pc][in_vc].out_pc == check_out_pc)
            return false;
    }

    return true;
}

void SwArb::del(SwReq& req)
{
    req.m_requesting = false;
    req.req_clk = INVALID_CLK;
    req.in_pc = INVALID_PC;
    req.in_vc = INVALID_VC;
    req.out_pc = INVALID_PC;
    req.out_vc = INVALID_VC;

    m_num_reqs--;
}

void SwArb::del(int in_pc, int in_vc)
{
    if (m_swReq_vec[in_pc][in_vc].m_requesting) {
        del(m_swReq_vec[in_pc][in_vc]);
    }
}

bool SwArb::isReqValid (SwReq& req)
{
    return ((! req.m_requesting) &&
            req.req_clk == INVALID_CLK &&
            req.in_pc == INVALID_PC &&
            req.in_vc == INVALID_VC &&
            req.out_pc == INVALID_PC &&
            req.out_vc == INVALID_VC) ?
            false : true;
}

void SwArb::add(int in_pc, int in_vc, int out_pc, int out_vc)
{
    SwReq & req = m_swReq_vec[in_pc][in_vc];

    assert(! isReqValid(req) );

    req.m_requesting = true;
    req.req_clk = simtime();
    req.in_pc = in_pc;
    req.in_vc = in_vc;
    req.out_pc = out_pc;
    req.out_vc = out_vc;

    m_num_reqs++;
}

// statistic reset when warmup period ends.
void SwArb::resetStats()
{
    m_grant_rate_tab->reset();
    m_spec_grant_rate_tab->reset();
}

void SwArb::printCurStatus(FILE* fp)
{
    fprintf(fp, "Current status:\n");
    for (int pc=0; pc<m_num_pc; pc++) {
        for (int vc=0; vc<m_num_vc; vc++) {
            SwReq & req = m_swReq_vec[pc][vc];
            if (req.m_requesting) {
                Flit* p_flit = m_router->flitQ()->peek(pc, vc);
                fprintf(fp, "  IN(pc=%d vc=%d) OUT(pc=%d vc=%d) p=%lld f=%lld req_clk=%.1lf\n",
                     req.in_pc, req.in_vc, req.out_pc, req.out_vc,
                     p_flit->getPkt()->id(), p_flit->id(),
                     req.req_clk);
            }
        }
    }
}
