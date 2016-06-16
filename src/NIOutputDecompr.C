#include "noc.h"
#include "NIOutputDecompr.h"
#include "NIInputCompr.h"
#include "CAMManager.h"
#include "Router.h"
#include "Pool.h"
#include "Topology.h"

// #define _DEBUG_CAM_DYN_CONTROL

NIOutputDecompr::NIOutputDecompr()
{
    assert(0);    // Don't call this constructor
}

NIOutputDecompr::NIOutputDecompr(Core* p_Core, int NI_id, int NI_pos) : NIOutput(p_Core, NI_id, NI_pos)
{
    // CAM
    m_lastCAM_clk = 0.0;

    if (g_cfg.cam_dyn_control) {
        char buf[64];
        m_cont_delay_src_tab_vec.resize(g_cfg.router_num);
        for (int r=0; r<g_cfg.router_num; r++) {
            sprintf(buf, "cam_dyn_ctrl_acct_%d", r);
            m_cont_delay_src_tab_vec[r] = new table (buf);
            m_cont_delay_src_tab_vec[r]->moving_window(g_cfg.cam_dyn_control_window);
        }
    }
}

NIOutputDecompr::~NIOutputDecompr()
{
    for (unsigned int r=0; r<m_cont_delay_src_tab_vec.size(); r++)
        delete m_cont_delay_src_tab_vec[r];
}

void NIOutputDecompr::assembleFlit()
{
    Flit* p_flit = readFlit();

    g_sim.m_num_flit_ejt++;
    g_sim.m_num_flit_in_network--;

    if (p_flit->isTail()) {

#ifdef _DEBUG_ROUTER
m_attached_router->debugEX(p_flit, m_router_pc, 0, this);
#endif

        Packet* p_pkt = p_flit->getPkt();
        double decode_lat = 0.0;

        if (g_cfg.cam_data_enable) {
            switch (p_pkt->m_packet_type) {
            case PACKET_TYPE_UNICAST_SHORT:
                break;
            case PACKET_TYPE_UNICAST_LONG:
                decompressDataByCAM(p_pkt);
                // decoding latency = (table access latency) * (# table accesses)
                if (g_cfg.flit_sz_byte <= g_cfg.cam_data_blk_byte) {
                    decode_lat = (double) (g_cfg.cam_data_de_latency * p_pkt->m_num_compr_flits/(g_cfg.cam_data_blk_byte/g_cfg.flit_sz_byte));
                } else {
                    // if (! g_cfg.cam_streamline) {
                        // no pipelining
                        decode_lat = (double) (p_pkt->m_num_compr_flits * g_cfg.cam_data_de_latency);
                    // } else {
                        // pipelining
                        // if (p_pkt->m_num_compr_flits > 0)
                            // decode_lat = (double) (p_pkt->m_num_compr_flits + (g_cfg.cam_data_de_latency - 1));
                    // }
                }

                if (g_cfg.cam_dyn_control)
                    controlCompression(p_pkt, (int) decode_lat);
                break;

            default:
                assert(0); 
            } // switch (p_pkt->m_packet_type) {
        }

        // record statistics
        g_sim.recordPkt(p_pkt, decode_lat, g_cfg.profile_perf);

        g_sim.m_num_pkt_ejt++;
        g_sim.m_num_pkt_in_network--;
        if (g_cfg.profile_perf) {
            g_sim.m_periodic_ejt_pkt++;
            g_sim.m_periodic_ejt_flit += p_pkt->m_num_flits;
        }

        // free packet
        g_PacketPool.reclaim(p_pkt);
        p_pkt = 0;

    } // if (p_flit->isTail())

    // free flit
    switch (p_flit->type()) {
    case HEAD_FLIT: g_FlitHeadPool.reclaim((FlitHead*) p_flit); break; 
    case MIDL_FLIT: g_FlitMidlPool.reclaim((FlitMidl*) p_flit); break; 
    case TAIL_FLIT: g_FlitTailPool.reclaim((FlitTail*) p_flit); break; 
    case ATOM_FLIT: g_FlitAtomPool.reclaim((FlitAtom*) p_flit); break; 
    default: assert(0);
    }
    p_flit = 0;
}

void NIOutputDecompr::decompressDataByCAM(Packet* p_pkt)
{
    int cam_pos = -1;
    switch (g_cfg.cam_data_manage) {
    case CAM_MT_PRIVATE_PER_ROUTER:
        cam_pos = p_pkt->getSrcRouterID();
        break;
    case CAM_MT_SHARED_PER_ROUTER:
        cam_pos = 0;
        break;
    case CAM_MT_PRIVATE_PER_CORE:
    default:
        assert(0);
        break;
    }

    // check decompression works correctly
    // for (unsigned int i=0; i<p_pkt->m_en_value_vec.size(); i++) {
    //    bool decomp_sts = m_CAM_data_vec[cam_pos]->decompress(p_pkt->m_en_value_vec[i], p_pkt);
    //    assert(decomp_sts);
    // }

    // update decoder with new values
    switch (g_cfg.cam_data_manage) {
    case CAM_MT_PRIVATE_PER_ROUTER:
        for (unsigned int i=0; i<p_pkt->m_en_new_value_vec.size(); i++)
            m_CAM_data_vec[cam_pos]->update(p_pkt->m_en_new_value_vec[i], p_pkt);
        break;

    case CAM_MT_SHARED_PER_ROUTER:
      {
        string payload_value_str = "";
        for (unsigned int i=1; i<(p_pkt->m_packetData_vec).size(); i++)
            payload_value_str += ulonglong2hstr(p_pkt->m_packetData_vec[i]);

        int str_skip_sz = m_CAM_data_vec[cam_pos]->blk_byte()*2;    // A bytes = 2A chars
// cout << "str_skip_sz=" << str_skip_sz << endl;
        for (unsigned int i=0; i<payload_value_str.length(); i += str_skip_sz) {
            string value_str = payload_value_str.substr(i, str_skip_sz);
// cout << "i=" << i << ":" << value_str << endl;
            m_CAM_data_vec[cam_pos]->update(value_str, p_pkt);
        }
      }
        break;

    default:
        assert(0); 
    }
}

void NIOutputDecompr::controlCompression(Packet* p_pkt, const int decode_lat)
{
    int src_router_id = p_pkt->getSrcRouterID();

    // network contention delay for packet (not including contention at NI)
    int hop_count = g_Topology->getMinHopCount(src_router_id, m_attached_router->id());
    const int per_hop_delay = 3;
    int pkt_0_load_delay = (hop_count*per_hop_delay - 1) + (p_pkt->m_num_flits - 1);
    int net_cont_delay = ((int) (simtime() - p_pkt->m_clk_enter_net)) - pkt_0_load_delay;
    assert(net_cont_delay >= 0);

#ifdef _DEBUG_CAM_DYN_CONTROL
    int pkt_delay = (int) (simtime() - p_pkt->m_clk_gen);
printf("NIOutDecompr: clk=%0.lf NI-%d %-2d->%-2d pid=%lld cont=%d (mean=%.1lf cnt=%ld) pkt=%d zero=%d\n", simtime(), m_NI_id, p_pkt->getSrcRouterID(), p_pkt->getDestRouterID(), p_pkt->id(),
net_cont_delay, 
m_cont_delay_src_tab_vec[src_router_id]->mean(),
m_cont_delay_src_tab_vec[src_router_id]->cnt(),
pkt_delay, pkt_0_load_delay);
#endif

    // accounting
    m_cont_delay_src_tab_vec[src_router_id]->tabulate((double) net_cont_delay);

    // control actions
    if (m_cont_delay_src_tab_vec[src_router_id]->mean() > 2.0) {
        // enable
        Router* srcRouter = g_Router_vec[src_router_id];
        for (unsigned int ni=0; ni<srcRouter->getNIInputVec().size(); ni++) {
            NIInputCompr* p_NI_comp = (dynamic_cast<NIInputCompr*> (srcRouter->getNIInputVec()[ni]));

            if (! p_NI_comp->CAMsts(m_attached_router->id())) {	// disable ?
                p_NI_comp->enableCAM(m_attached_router->id(), 0);

                if (ni==0) {
                    g_sim.m_num_pkt_spurious++;
                    g_CamManager->m_pkt_dyn_control++;
                }
            }
        }
    } else {
        // disable
        Router* srcRouter = g_Router_vec[src_router_id];
        for (unsigned int ni=0; ni<srcRouter->getNIInputVec().size(); ni++) {
            NIInputCompr* p_NI_comp = (dynamic_cast<NIInputCompr*> (srcRouter->getNIInputVec()[ni]));

            if (p_NI_comp->CAMsts(m_attached_router->id())) {
                p_NI_comp->disableCAM(m_attached_router->id(), 0);

                if (ni==0) {
                    g_sim.m_num_pkt_spurious++;
                    g_CamManager->m_pkt_dyn_control++;
                }
            }
        }
    }
}

void NIOutputDecompr::printStats(ostream& out) const
{
}

void NIOutputDecompr::print(ostream& out) const
{
}
