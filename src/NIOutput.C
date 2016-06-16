#include "noc.h"
#include "NIOutput.h"
#include "Router.h"

NIOutput::NIOutput()
{
    assert(0);    // Don't call this constructor
}

NIOutput::NIOutput(Core* p_core, int NI_id, int NI_pos)
{
    m_NI_id = NI_id;
    m_NI_pos = NI_pos;
    
    m_attached_core = p_core;
    m_attached_router = 0;	// must initialize from attachRouter() call
    m_ev_wakeup = new event("ev_wk_NIout");
}

NIOutput::~NIOutput()
{
    delete m_ev_wakeup;
    m_ev_wakeup = 0;
}

void NIOutput::printStats(ostream& out) const
{
}

void NIOutput::print(ostream& out) const
{
}

void NIOutput::writeFlit(Flit* p_flit)
{
    int Q_sz_before_push = m_NIout_flitQ.size();
    m_NIout_flitQ.push_back(p_flit);
    int Q_sz_after_push = m_NIout_flitQ.size();

    if (Q_sz_before_push == 0 && Q_sz_after_push == 1)
        m_ev_wakeup->set();

    if (p_flit->isTail()) {
        p_flit->getPkt()->m_clk_store_NIout = simtime();
    }
}

Flit* NIOutput::readFlit()
{
    while (m_NIout_flitQ.size() == 0)
        m_ev_wakeup->wait();

    // printf("m_NIout_flitQ.size()=%d\n", m_NIout_flitQ.size());
    Flit* p_flit = m_NIout_flitQ.front();
    assert(p_flit);
    m_NIout_flitQ.pop_front();

    return p_flit;
}

void NIOutput::assembleFlit()
{
    Flit* p_flit = readFlit();

    g_sim.m_num_flit_ejt++;
    g_sim.m_num_flit_in_network--;

    if (p_flit->isTail()) {

#ifdef _DEBUG_ROUTER
        // printf("m_NIout_flitQ.size()=%d\n", m_NIout_flitQ.size());
        m_attached_router->debugEX(p_flit, m_router_pc, 0, this);
#endif

// for deadlock debugging
#if 0
        printf("%08lld\n", p_flit->getPkt()->id());
#endif

        Packet* p_pkt = p_flit->getPkt();

        // record statistics
        g_sim.recordPkt(p_pkt, 0.0, g_cfg.profile_perf);
        g_sim.m_num_pkt_ejt++;
        g_sim.m_num_pkt_in_network--;
        assert(g_sim.m_num_pkt_in_network >= 0);
        if (g_cfg.profile_perf) {
            g_sim.m_periodic_ejt_pkt++;
            g_sim.m_periodic_ejt_flit += p_pkt->m_num_flits;
        }

        // free packet
        g_PacketPool.reclaim(p_pkt);

    } // if (p_flit->isTail())

    // free flit
    switch (p_flit->type()) {
    case HEAD_FLIT: g_FlitHeadPool.reclaim((FlitHead*) p_flit); break;
    case MIDL_FLIT: g_FlitMidlPool.reclaim((FlitMidl*) p_flit); break;
    case TAIL_FLIT: g_FlitTailPool.reclaim((FlitTail*) p_flit); break;
    case ATOM_FLIT: g_FlitAtomPool.reclaim((FlitAtom*) p_flit); break;
    default: assert(0);
    }
}
