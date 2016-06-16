#include "noc.h"
#include "Router.h"
#include "Routing.h"
#include "NIInput.h"
#include "RouterPower.h"
#include "Workload.h"
#include "Core.h"

// #define _DEBUG_NI_INPUT

#ifdef _DEBUG_NI_INPUT
int _debug_ni_input_router_id = 6;
int _debug_ni_input_router_pc = 4;
int _debug_ni_input_vc = 1;
#endif

NIInput::NIInput()
{
    assert(0);	// Don't call this constructor
}

NIInput::NIInput(Core* p_core, int NI_id, int NI_pos, int num_vc)
{
    m_attached_core = p_core;
    m_NI_id = NI_id;
    m_NI_pos = NI_pos;
    m_num_vc_router = num_vc;
    m_attached_router = 0;     // must initialize from attachRouter() call

    m_num_pktQ_full = 0;
    m_last_free_vc = 0;

    m_num_reads = 0;
    m_num_writes = 0;
    m_max_occupancy = 0;
    m_sum_occupancy = 0;
    m_last_update_buf_clk = 0.0;

    m_vc_free_vec.resize(num_vc, true);
}

NIInput::~NIInput()
{
    for (unsigned int i=0; i<m_ev_wakeup_vec.size(); i++)
        delete m_ev_wakeup_vec[i];
}

void NIInput::init()
{
    switch (g_cfg.NIin_type) {
    case NI_INPUT_TYPE_PER_PC:
        m_ev_wakeup_vec.push_back(new event("ev_wk_NIin"));
        break;
    case NI_INPUT_TYPE_PER_VC:
        for (int vc=0; vc<m_attached_router->num_vc(); vc++)
            m_ev_wakeup_vec.push_back(new event("ev_wk_NIin"));
        break;
    default:
        assert(0);
    }
}

void NIInput::printStats(ostream& out) const
{
}

void NIInput::print(ostream& out) const
{
}

vector< Flit* > NIInput::pkt2flit(Packet* p_pkt)
{
    vector< Flit* > flit_vec;

    int packet_data_pos = 1;	// 0th is for address
    for (int i=0; i<p_pkt->m_num_flits; i++) { // construct flit

        // select a flit type
        int alloc_flit_type = UNDEFINED_FLIT;
        if (i == 0) {
            alloc_flit_type = ((p_pkt->m_num_flits == 1) ? ATOM_FLIT : HEAD_FLIT);
        } else if (i == p_pkt->m_num_flits-1) {
            alloc_flit_type = TAIL_FLIT;
        } else {
            alloc_flit_type = MIDL_FLIT;
        }

        Flit* p_flit = 0;

        // allocate a flit
        switch (alloc_flit_type) {
        case HEAD_FLIT:
            p_flit = g_FlitHeadPool.alloc();

            // copy routing path if source routing
            // g_Routing->writeRouteInfoToHeadFlit((FlitHead*) p_flit);

            break;
        case MIDL_FLIT:
            p_flit = g_FlitMidlPool.alloc();
            break;
        case TAIL_FLIT:
            p_flit = g_FlitTailPool.alloc();
            ((FlitTail*) p_flit)->m_crc_check = 0;
            break;
        case ATOM_FLIT:
            p_flit = g_FlitAtomPool.alloc();

            // copy routing path if source routing
            // g_Routing->writeRouteInfoToHeadFlit((FlitHead*) p_flit);

            break;
        default:
            assert(0);
        }

        // set flit id
        p_flit->setID(p_pkt->m_start_flit_id + i);
        // set packet pointer
        p_flit->setPkt(p_pkt);

        // set payload
        if (g_Workload->containData()) {
            if (alloc_flit_type == HEAD_FLIT || alloc_flit_type == ATOM_FLIT) {
                p_flit->setZeroData();
                p_flit->m_flitData[0] = p_pkt->m_packetData_vec[0];
            } else {
                for (int n=0; n<g_cfg.flit_sz_64bit_multiple; n++) {
                    assert(n < (int) p_flit->m_flitData.size());
                    assert(packet_data_pos < (int) p_pkt->m_packetData_vec.size());
                    p_flit->m_flitData[n] = p_pkt->m_packetData_vec[packet_data_pos];
                    packet_data_pos++;
                }
            }
        } else {
            p_flit->setRandomData();
        }

        flit_vec.push_back(p_flit);
    } // for (int i=0; i<p_pkt->num_flits; i++) {

    assert(p_pkt->m_num_flits == (int) flit_vec.size());

    return flit_vec;
}

void NIInput::writePacket(Packet* p_pkt)
{
    assert(p_pkt);

    if (((int) m_pktQ.size()) < g_cfg.NIin_pktbuf_depth) {	// not full ?
        m_sum_occupancy += m_pktQ.size()*(simtime() - m_last_update_buf_clk);
        m_last_update_buf_clk = simtime();

        // set clock: packet is stored to input NI packet buffer
        p_pkt->m_clk_store_NIin = simtime();

        int Q_sz_before_push = m_pktQ.size();
        m_pktQ.push_back(p_pkt);
        int Q_sz_after_push = m_pktQ.size();

        // wakeup input NI process
        if (Q_sz_before_push == 0 && Q_sz_after_push == 1) {
            switch (g_cfg.NIin_type) {
            case NI_INPUT_TYPE_PER_PC:
                m_ev_wakeup_vec[0]->set();
                break;
            case NI_INPUT_TYPE_PER_VC:
                m_ev_wakeup_vec[m_last_free_vc]->set();
                m_last_free_vc = (m_last_free_vc + 1) % m_attached_router->num_vc();
                break;
            }
        }

        if (m_max_occupancy < ((int) m_pktQ.size()))
            m_max_occupancy = m_pktQ.size();

#ifdef _DEBUG_ROUTER
        printf("NIin::writePacket p=%lld NI_id=%d NI_pos=%d core=%d router=%d stored_at_m_pktQ\n", p_pkt->id(), m_NI_id, m_NI_pos, m_attached_core->id(), m_attached_router->id());
#endif
    } else {
        m_num_pktQ_full++;
        m_extra_pktQ.push_back(p_pkt);

#ifdef _DEBUG_ROUTER
        printf("NIin::writePacket p=%lld NI_id=%d NI_pos=%d core=%d router=%d m_pktQ.size()=%d stored_at_m_extra_pktQ\n", p_pkt->id(), m_NI_id, m_NI_pos, m_attached_core->id(), m_attached_router->id(), m_pktQ.size());
#endif
    }

    m_num_writes++;
}

Packet* NIInput::readPacket(int NI_vc)
{
    while (m_pktQ.size() == 0) {
        m_ev_wakeup_vec[NI_vc]->wait();
    }

    Packet* p_pkt = m_pktQ.front();
    m_pktQ.pop_front();

    m_sum_occupancy += m_pktQ.size()*(simtime() - m_last_update_buf_clk);
    m_last_update_buf_clk = simtime();
    m_num_reads++;

#ifdef _DEBUG_ROUTER
    printf("NIin::readPacket clk=%.0lf p=%lld NI_id=%d NI_pos=%d core=%d router=%d\n", simtime(), p_pkt->id(), m_NI_id, m_NI_pos, m_attached_core->id(), m_attached_router->id());
#endif

    if (m_extra_pktQ.size() > 0) {
        Packet* p_move_pkt = m_extra_pktQ.front();
        m_extra_pktQ.pop_front();

        // set clock: packet is stored to input NI packet buffer
        p_move_pkt->m_clk_store_NIin = simtime();

        m_pktQ.push_back(p_move_pkt);
    }

    return p_pkt;
}

void NIInput::segmentPacket(int NI_vc)
{
    Packet* p_pkt = readPacket(NI_vc);
    int router_in_pc = m_router_pc;	// router input PC
    FlitQ* fq = m_attached_router->flitQ();	// router flit queue
    int router_in_vc = INVALID_VC;

    switch (g_cfg.NIin_type) {
    case NI_INPUT_TYPE_PER_PC:
        router_in_vc = NI_vc;
        while (1) {
            if (m_vc_free_vec[NI_vc] && !fq->isFull(router_in_pc, NI_vc)) { // free & not full ?
                goto FREE_VC_FOUND;
            }

            // check again at next cycle
            hold(ONE_CYCLE);
        }
        break;
    case NI_INPUT_TYPE_PER_VC:
        // obtain a free VC in PC of the attached router
        do {
            // check each VC in round-robin
            for (int vc=0; vc<m_attached_router->num_vc(); vc++) {
                int check_vc = (vc + m_last_free_vc + 1) % m_attached_router->num_vc();
                if (m_vc_free_vec[check_vc] && !fq->isFull(router_in_pc, check_vc)) { // free & not full ?
                    router_in_vc = check_vc;
                    m_last_free_vc = check_vc;
                    goto FREE_VC_FOUND;
                }
            }

            // check again at next cycle
            hold(ONE_CYCLE);
        } while (router_in_vc == INVALID_VC);
        break;
    }

FREE_VC_FOUND:
    // set VC status as reserved
    m_vc_free_vec[router_in_vc] = false;

#ifdef _DEBUG_ROUTER
    printf("NI::segmentPacket clk=%.0lf NIin(id=%d pos=%d) router(pc=%d vc=%d) [p=%lld flit#=%d] router(%d=>%d) core(%d=>%d)\n", simtime(), m_NI_id, m_NI_pos, router_in_pc, router_in_vc, p_pkt->id(), p_pkt->m_num_flits, p_pkt->getSrcRouterID(), p_pkt->getDestRouterID(), p_pkt->getSrcCoreID(), p_pkt->getDestCoreID());
#endif

    // build flits for the received packet
    vector< Flit* > flit_store_vec = pkt2flit(p_pkt);

    // send flits into the router's injection channel
    for (unsigned int i=0; i<flit_store_vec.size(); i++) {
        Flit* p_flit = flit_store_vec[i];
        assert(p_flit);

        // check the fullness at next cycle again
        while (fq->isFull(router_in_pc, router_in_vc))
            hold(ONE_CYCLE);

        // pipeline stage latency
        p_flit->m_clk_enter_stage = simtime();

        // intra-router flit/packet latency
        p_flit->m_clk_enter_router = simtime();
        m_attached_router->m_num_flit_inj_from_core++;
        if (p_flit->isHead()) {
            m_attached_router->m_num_pkt_inj_from_core++;
            p_pkt->m_clk_enter_net = simtime();
        }

        // write a flit to input buffer
        fq->write(router_in_pc, router_in_vc, p_flit);

        // 03/15/06 fast simulation
        if (m_attached_router->hasNoFlitsInside()) {
            m_attached_router->wakeup();
// printf("WAKEUP r_%d process at clk=%.0lf\n", m_attached_router->id, simtime());
        }
        m_attached_router->incFlitsInside();

        // record power
        if (!g_sim.m_warmup_phase) {
            m_attached_router->m_power_tmpl->record_buffer_write(p_flit, router_in_pc);

            if (g_cfg.profile_power)
                m_attached_router->m_power_tmpl_profile->record_buffer_write(p_flit, router_in_pc);
        }

#ifdef _DEBUG_ROUTER
        m_attached_router->debugIB(p_flit, router_in_pc, router_in_vc);
#endif

        g_sim.m_num_flit_inj++;
        g_sim.m_num_flit_in_network++;

        hold(ONE_CYCLE);
    }

    // set VC status as free
    m_vc_free_vec[router_in_vc] = true;
    g_sim.m_num_pkt_in_network++;
}

void NIInput::resetStats()
{
    m_num_pktQ_full = 0;
    m_num_reads = 0;
    m_num_writes = 0;
    m_max_occupancy = 0;
    m_sum_occupancy = 0.0;
    m_last_update_buf_clk = simtime();
}

void NIInput::takeSnapshot(FILE* fp)
{
    fprintf(fp, "----- NIin=%d -----------------------------\n", m_NI_id);

    fprintf(fp, "Buffer:\n");
    // head, ..., tail
    for (deque< Packet* >::iterator pos = m_pktQ.begin(); pos != m_pktQ.end(); ++pos) {
        Packet* p_pkt = *pos;

        fprintf(fp, "[p=%lld (s=%d d=%d)] ", p_pkt->id(), p_pkt->getSrcCoreID(), p_pkt->getDestCoreID());
    }
    fprintf(fp, "\n");
}
