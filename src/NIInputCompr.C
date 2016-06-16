#include "noc.h"
#include "Router.h"
#include "Routing.h"
#include "NIInputCompr.h"
#include "RouterPower.h"
#include "Workload.h"

// #define _DEBUG_NI_DATA_COMPRESS
// #define _DEBUG_NI_DATA_COMPRESS_STREAMLINE

#ifdef _DEBUG_NI_DATA_COMPRESS
static IntSet _debug_router_id_set;
static int _debug_ni_id = 26;
#endif
#ifdef _DEBUG_NI_DATA_COMPRESS_STREAMLINE
static IntSet _debug_router_id_set;
static int _debug_ni_id = 0;
#endif

NIInputCompr::NIInputCompr()
{
    assert(0);	// Don't call this constructor
}

NIInputCompr::NIInputCompr(Core* p_Core, int NI_id, int NI_pos, int num_vc) : NIInput(p_Core, NI_id, NI_pos, num_vc)
{
    if (g_cfg.cam_dyn_control)
        // FIXME: ONLY supports two traffic types and per-router CAM architecture
        m_CAM_sts_vec.resize(g_cfg.router_num, true);

#ifdef _DEBUG_NI_DATA_COMPRESS
_debug_router_id_set.insert(0);
_debug_router_id_set.insert(2);
_debug_router_id_set.insert(3);
_debug_router_id_set.insert(13);
_debug_router_id_set.insert(5);
#endif
#ifdef _DEBUG_NI_DATA_COMPRESS_STREAMLINE
_debug_router_id_set.insert(0);
#endif

    m_xxx_tab.moving_window(g_cfg.cam_dyn_control_window);
}

NIInputCompr::~NIInputCompr()
{
}

void NIInputCompr::printStats(ostream& out) const
{
}

void NIInputCompr::print(ostream& out) const
{
}

void NIInputCompr::segmentPacket(int NI_vc)
{
    Packet* p_pkt = readPacket(NI_vc);
    int router_in_pc = m_router_pc;     // router input PC
    FlitQ* fq = m_attached_router->flitQ();     // router flit queue
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
        // obtain one free VC of buffers in the attached router
        // check each VC in a round-robin way
        do {
            for (int vc=0; vc<m_attached_router->num_vc(); vc++) {
                int check_vc = (vc + m_last_free_vc + 1) % m_attached_router->num_vc();
                if (m_vc_free_vec[check_vc] &&
                    !fq->isFull(router_in_pc, check_vc)) { // free & not full ?
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
    printf("clk=%.1lf NI_id=%d router_pc=%d router_in_vc=%d p=%lld flit#=%d router(src=%d dest=%d) core(src=%d dest=%d)\n", simtime(), m_NI_id, m_router_pc, router_in_vc, p_pkt->id(), p_pkt->m_num_flits, p_pkt->getSrcRouterID(), p_pkt->getDestRouterID(), p_pkt->getSrcCoreID(), p_pkt->getDestCoreID());
#endif

    // build flits for the received packet
    vector< Flit* > flit_store_vec = pkt2flit(p_pkt);
    int debug_serialz_lat = 0;
    int debug_encode_lat = 0;
    int debug_misc_lat = 0;

    vector< int > cam_en_sts_vec;	// CAM encoded indices : (-1 => no encoding)
    vector< bool > flit_en_sts_vec;	// compression status of each (original) flit
    flit_en_sts_vec.resize(p_pkt->m_num_flits, false);
    bool compress_flag = false;
    p_pkt->m_num_compr_flits = 0;
    if (g_cfg.cam_data_enable && p_pkt->m_packet_type == PACKET_TYPE_UNICAST_LONG) {
        // mark compression status in a packet
        if (g_cfg.cam_dyn_control) {
            int queuing_delay = (int) (simtime() - p_pkt->m_clk_gen);
            m_xxx_tab.tabulate((double) queuing_delay);

            if (! m_CAM_sts_vec[p_pkt->getDestRouterID()])
                compress_flag = true;

            if (m_pktQ.size() > 0 ||
                fq->isFull(router_in_pc, router_in_vc) ||
                m_xxx_tab.mean() > 0.0)
                compress_flag = true;
#ifdef _DEBUG_CAM_DYN_CONTROL
printf("clk=%.0lf NI-in NI-%d %-2d->%-2d pkt_id=%lld comp=%c q_len=%-2ld q=%d (%.1lf)\n", simtime(), m_NI_id, p_pkt->getSrcRouterID(), p_pkt->getDestRouterID(), p_pkt->id(), compress_flag ? 'Y' : 'N', m_pktQ.size(), queuing_delay, m_xxx_tab.mean());
#endif
        } else {
            compress_flag = true;
        }
    }

    int encode_lat = 0;
    if (compress_flag) {
        int num_flit_org = flit_store_vec.size();
        compressDataByCAM(p_pkt, flit_store_vec, cam_en_sts_vec, flit_en_sts_vec);
        assert(((int) flit_en_sts_vec.size()) == num_flit_org);
// int reduced_flit_num = flit_store_vec.size();

        if (! g_cfg.cam_streamline) {
            // no pipelining
            encode_lat = (cam_en_sts_vec.size() / g_cfg.cam_data_num_interleaved_tab) * g_cfg.cam_data_en_latency;
        } else {
            // pipelining
            encode_lat = cam_en_sts_vec.size() / g_cfg.cam_data_num_interleaved_tab + (g_cfg.cam_data_en_latency - 1);
        }

        if (! g_cfg.cam_streamline) {
#ifdef _DEBUG_NI_DATA_COMPRESS
if (_debug_router_id_set.count(m_attached_router->id()) == 1 && m_NI_id == _debug_ni_id) {
  cout << "  NON_STR cam access count  :" << cam_en_sts_vec.size() << endl;
  cout << "  NON_STR cam access latency:" << encode_lat << endl;
}
#endif
            hold ( (double) encode_lat );
            debug_encode_lat += encode_lat;
        }

#ifdef _DEBUG_NI_DATA_COMPRESS
if (_debug_router_id_set.count(m_attached_router->id()) == 1 && m_NI_id == _debug_ni_id) {
  printf("  compression done clk: pkt=%lld pkt_gen_clk=%.0lf now=%.0lf\n", p_pkt->id(), p_pkt->m_clk_gen, simtime());
}
#endif
    }


#ifdef _DEBUG_NI_DATA_COMPRESS_STREAMLINE
if (_debug_router_id_set.count(m_attached_router->id()) == 1 && m_NI_id == _debug_ni_id) {
  if (p_pkt->m_packet_type == PACKET_TYPE_UNICAST_LONG) {
    printf("STRM_LINE: router_id=%d pkt_id=%lld pkt_gen_clk=%.0lf inject_clk=%.0lf\n", m_attached_router->id(), p_pkt->id(), p_pkt->m_clk_gen, simtime());
    printf("           flit_en_sts=");
    for (int i=0; i<flit_en_sts_vec.size(); i++)
      printf("%c ", flit_en_sts_vec[i] ? 'T' : 'F');
    printf("\n");
  }
}
#endif

    int total_hold_cycle = 0;
    // send flits into the router's injection channel
    for (unsigned int i=0; i<flit_store_vec.size(); i++) {
        Flit* p_flit = flit_store_vec[i];
        assert(p_flit);

        // check the fullness at next cycle again
        while (fq->isFull(router_in_pc, router_in_vc)) {
            hold(ONE_CYCLE);
            debug_misc_lat++;
        }

        // measure pipeline latency
        p_flit->m_clk_enter_stage = simtime();

        // measure the flit/packet traversal time for this router
        p_flit->m_clk_enter_router = simtime(); // set the router-enter-clock for this flit

        // record router load
        m_attached_router->m_num_flit_inj_from_core++;
        if (p_flit->isHead()) {
            m_attached_router->m_num_pkt_inj_from_core++;
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

        if (p_flit->isHead())
            p_pkt->m_clk_enter_net = simtime();

#ifdef _DEBUG_ROUTER
        m_attached_router->debugIB(p_flit, router_in_pc, router_in_vc);
#endif
#ifdef _DEBUG_NI_DATA_COMPRESS_STREAMLINE
if (_debug_router_id_set.count(m_attached_router->id()) == 1 && m_NI_id == _debug_ni_id) {
  if (p_pkt->m_packet_type == PACKET_TYPE_UNICAST_LONG) {
  printf("STRM_LINE: I router_id=%d pkt_id=%lld flit_pos=%d clk=%.0lf\n", m_attached_router->id(), p_pkt->id(), i, simtime());
  }
}
#endif

        g_sim.m_num_flit_inj++;
        g_sim.m_num_flit_in_network++;

        hold(ONE_CYCLE);
        debug_serialz_lat++;

        if (g_cfg.cam_streamline && compress_flag) {
            switch (p_flit->type()) {
            case ATOM_FLIT:
            case TAIL_FLIT:
                break;

            case HEAD_FLIT:
            case MIDL_FLIT:
              {
                int hold_cycle = 0;
                if (i == flit_store_vec.size()-2) { // last middle flit ? 
                    hold_cycle = encode_lat - total_hold_cycle;
                } else {
                    if (total_hold_cycle >= encode_lat) {
                        hold_cycle = 0;
                    } else {
                        hold_cycle = 1;
                    }
                }
                total_hold_cycle += hold_cycle;

                if (hold_cycle > 0)
                    hold( (double) hold_cycle );
                debug_encode_lat += hold_cycle;
#ifdef _DEBUG_NI_DATA_COMPRESS_STREAMLINE
if (_debug_router_id_set.count(m_attached_router->id()) == 1 && m_NI_id == _debug_ni_id)
  if (p_pkt->m_packet_type == PACKET_TYPE_UNICAST_LONG)
    printf("STRM_LINE: router_id=%d pkt_id=%lld flit_pos=%d hold_cycle=%d total_hold_cycle=%d encode_lat=%d clk=%.0lf\n", m_attached_router->id(), p_pkt->id(), i, hold_cycle, total_hold_cycle, encode_lat, simtime());
#endif
              }
                break;

            default:
                assert(0);
            } // switch (p_flit->type()) {
        } // if (g_cfg.cam_streamline) {
    } // for (unsigned int i=0; i<flit_store_vec.size(); i++) {

//    if (p_pkt->m_packet_type == PACKET_TYPE_UNICAST_LONG)
//        printf(" clk=%.0lf R=%-2d NI=%d pid=%lld serialz_lat=%d encode_lat=%d misc_lat=%d total=%2d #flits=%d q_lat=%0.lf q_sz=%d\n", 
//               simtime(), m_attached_router->id(), m_NI_id, p_pkt->id(), debug_serialz_lat, debug_encode_lat, debug_misc_lat, debug_serialz_lat+debug_encode_lat+debug_misc_lat, flit_store_vec.size(), p_pkt->m_clk_enter_net - p_pkt->m_clk_gen, m_pktQ.size());

#ifdef _DEBUG_NI_DATA_COMPRESS
if (_debug_router_id_set.count(m_attached_router->id()) == 1 && m_NI_id == _debug_ni_id && p_pkt->m_packet_type == PACKET_TYPE_UNICAST_LONG) {
  printf("  injection done clk:   pkt=%lld pkt_gen_clk=%.0lf now=%.0lf\n\n", p_pkt->id(), p_pkt->m_clk_gen, simtime());
}
#endif

    // set VC status as free
    m_vc_free_vec[router_in_vc] = true;
    g_sim.m_num_pkt_in_network++;
}

// returns number of CAM accesses.
void NIInputCompr::compressDataByCAM(Packet* p_pkt, vector<Flit*> & flit_store_vec,
                                     vector< int > & cam_en_sts_vec, vector< bool > & flit_en_sts_vec)
{
    // construct a value string for payload
    // FIXME: We assume that m_packetData_vec[0] contains only address.
    string payload_value_str = "";
    for (unsigned int i=1; i<(p_pkt->m_packetData_vec).size(); i++)
        payload_value_str += ulonglong2hstr(p_pkt->m_packetData_vec[i]);

    // pass payload_value_str to a compressor
    int encoder_pos = -1;
    switch (g_cfg.cam_data_manage) {
    case CAM_MT_PRIVATE_PER_ROUTER:
    case CAM_MT_PRIVATE_PER_CORE:
        encoder_pos = p_pkt->getDestRouterID();
        break;
    case CAM_MT_SHARED_PER_ROUTER:
        encoder_pos = 0;
        break;
    default:
        assert(0);
    }
    m_CAM_data_vec[encoder_pos]->compress(payload_value_str, cam_en_sts_vec, p_pkt->getDestRouterID());

    // construct a value string for compressed payload
    // count compressible flits
    string compr_payload_value_str = "";
    int num_compr_flits = 0;	// flits to be compressed in original flits
    int flit_pos = 0;
    flit_en_sts_vec[flit_pos++] = false;	// incompressible head flit
    if (g_cfg.flit_sz_byte <= g_cfg.cam_data_blk_byte) {
        int num_flits_for_blk = g_cfg.cam_data_blk_byte / g_cfg.flit_sz_byte;
        for (unsigned int i=0; i<cam_en_sts_vec.size(); ++i) {
            bool blk_compressible = (cam_en_sts_vec[i] >= 0) ? true : false;

            if (blk_compressible) {
                num_compr_flits += num_flits_for_blk;
            } else {
                compr_payload_value_str += payload_value_str.substr(BYTESZ2STRLEN(g_cfg.cam_data_blk_byte*i),
                                                                    BYTESZ2STRLEN(g_cfg.cam_data_blk_byte));
            }
            for (int j=0; j<num_flits_for_blk; j++)
                flit_en_sts_vec[flit_pos++] = blk_compressible;
        }

        if (num_compr_flits > 0) {
            // FIXME: I assume that all encoded indices fit an 8B flit.
            string encoded_index_str = "0000000000000000";
            compr_payload_value_str += encoded_index_str;
        }
    } else {
        int num_blks_for_flit = g_cfg.flit_sz_byte / g_cfg.cam_data_blk_byte;
        for (unsigned int i=0; i<cam_en_sts_vec.size(); i+=num_blks_for_flit ) {
            bool flit_compressible = false;
            for (int j=0; j<num_blks_for_flit; j++) {
                if (cam_en_sts_vec[i + j] < 0) {	// data compressed ?
                    if (flit_compressible)
                        flit_compressible = true;
                    compr_payload_value_str += payload_value_str.substr(BYTESZ2STRLEN(g_cfg.cam_data_blk_byte*(i+j)),
                                                                        BYTESZ2STRLEN(g_cfg.cam_data_blk_byte));
                }
            }

            if (flit_compressible)
                flit_en_sts_vec[flit_pos++] = true;
            else
                flit_en_sts_vec[flit_pos++] = false;
        }

        // pad zero bits to compr_payload_value_str
        int num_align_byte = STRLEN2BYTESZ(compr_payload_value_str.length()) % g_cfg.flit_sz_byte;
        if (num_align_byte)
            compr_payload_value_str += zeroStr(g_cfg.flit_sz_byte - num_align_byte);

        num_compr_flits = STRLEN2BYTESZ(payload_value_str.length() - compr_payload_value_str.length()) / g_cfg.flit_sz_byte;

        if (num_compr_flits > 0) {
            // FIXME: I assume that all encoded indices fit an 8B flit.
            string encoded_index_str = "0000000000000000";
            compr_payload_value_str += encoded_index_str;
        }
    }


#ifdef _DEBUG_NI_DATA_COMPRESS
if (_debug_router_id_set.count(m_attached_router->id()) == 1 && m_NI_id == _debug_ni_id) {
  assert(cam_en_sts_vec.size() == payload_value_str.length()/BYTESZ2STRLEN(g_cfg.cam_data_blk_byte));

  // original string
  cout << "  org=";
  for (int i=0; i<payload_value_str.length()/BYTESZ2STRLEN(g_cfg.cam_data_blk_byte); i++)
    cout << payload_value_str.substr(BYTESZ2STRLEN(g_cfg.cam_data_blk_byte*i),
                                     BYTESZ2STRLEN(g_cfg.cam_data_blk_byte)) << " ";
  cout << endl;

  // converted string
  cout << "  com=";
  for (int i=0; i<compr_payload_value_str.length()/BYTESZ2STRLEN(g_cfg.cam_data_blk_byte); i++)
    cout << compr_payload_value_str.substr(BYTESZ2STRLEN(g_cfg.cam_data_blk_byte*i),
                                     BYTESZ2STRLEN(g_cfg.cam_data_blk_byte)) << " ";
  cout << endl;

  // encoding status
  cout << "  cam_en_sts_vec: ";
  for (int i=0; i<cam_en_sts_vec.size(); i++) {
    cout << ((cam_en_sts_vec[i] >= 0) ? "Y ": "N ");
    if (i%4 == 3)	// 2B table assumption
      cout << " ";
  }
  cout << endl;

  cout << "  org=" << payload_value_str << " byte=" << STRLEN2BYTESZ(payload_value_str.length()) << endl;
  cout << "  com=" << compr_payload_value_str << " byte=" << STRLEN2BYTESZ(compr_payload_value_str.length()) << endl;

  cout << endl;
}
#endif

    // successfully encoded values
    p_pkt->m_en_value_vec = m_CAM_data_vec[encoder_pos]->getEnValueVec();
    // newly encoded values
    p_pkt->m_en_new_value_vec = m_CAM_data_vec[encoder_pos]->getEnNewValueVec();
    // # of flits to be compressed
    p_pkt->m_num_compr_flits = num_compr_flits;

    if (payload_value_str.compare(compr_payload_value_str) != 0) {	// at least one compression?
        // rebuild flits from the compressed packet data
        vector<Flit*> flit_store_org_vec(flit_store_vec);

        // # of flits for a compressed packet
        int num_conv_flits = STRLEN2BYTESZ(compr_payload_value_str.size())/g_cfg.flit_sz_byte;
        assert(num_conv_flits > 0 && num_conv_flits <= (int) flit_store_vec.size());

        vector< unsigned long long > compressed_64bit_vec = str2ullv(compr_payload_value_str);

#ifdef _DEBUG_NI_DATA_COMPRESS
if (_debug_router_id_set.count(m_attached_router->id()) == 1 && m_NI_id == _debug_ni_id) {
  printf("   #compressed_flits=%d m_en_value_vec.size()=%d m_en_new_value_vec.size()=%d\n", p_pkt->m_num_compr_flits, (p_pkt->m_en_value_vec).size(), (p_pkt->m_en_new_value_vec).size());
  printf("   num_conv_flits=%d compressed_64bit_vec.size()=%d\n", num_conv_flits, compressed_64bit_vec.size());
}
#endif

        flit_store_vec.clear();

        // NOTE: we reuse generated flits.
        // head flit : no change
        flit_store_vec.push_back(flit_store_org_vec[0]);
        flit_store_org_vec[0] = 0;

        // middle flits
        unsigned int compr_data_64bit_pos = 0;
        for (int f=0; f<num_conv_flits-1; f++) {
            Flit* p_midl_flit = flit_store_org_vec[f+1];
            flit_store_org_vec[f+1] = 0;
            assert(p_midl_flit->type() == MIDL_FLIT);
            flit_store_vec.push_back(p_midl_flit);

            // change filt data as encoded value.
            for (int n=0; n<g_cfg.flit_sz_64bit_multiple; n++) {
                if (compr_data_64bit_pos < compressed_64bit_vec.size()) {
                    p_midl_flit->m_flitData[n] = compressed_64bit_vec[compr_data_64bit_pos];
                    compr_data_64bit_pos++;
                } else {
                    p_midl_flit->m_flitData[n] = 0x0;
                }
            }
        }

        // tail flit
        Flit* p_tail_flit = flit_store_org_vec[flit_store_org_vec.size()-1];
        flit_store_org_vec[flit_store_org_vec.size()-1] = 0;
        assert(p_tail_flit->type() == TAIL_FLIT);
        flit_store_vec.push_back(p_tail_flit);
        for (int n=0; n<g_cfg.flit_sz_64bit_multiple; n++) {
            if (compr_data_64bit_pos < compressed_64bit_vec.size()) {
                p_tail_flit->m_flitData[n] = compressed_64bit_vec[compr_data_64bit_pos];
                compr_data_64bit_pos++;
            } else {
                p_tail_flit->m_flitData[n] = 0x0;
            }
        }
// printf("compr_data_64bit_pos=%d compressed_64bit_vec.size()=%d\n", compr_data_64bit_pos, compressed_64bit_vec.size());
        assert(compr_data_64bit_pos == compressed_64bit_vec.size());

#ifdef _DEBUG_NI_DATA_COMPRESS
if (_debug_router_id_set.count(m_attached_router->id()) == 1 && m_NI_id == _debug_ni_id) {
  cout << "  COM: reduced_flit_num=" << flit_store_org_vec.size() - flit_store_vec.size() << endl;
  cout << "       value_str=" << compr_payload_value_str << endl;

  cout << "       flit_value: ";
  for (int i=0; i<flit_store_vec.size(); i++) {
    Flit* p_flit = flit_store_vec[i];
    cout << p_flit->convertData2Str();
    switch (p_flit->type()) {
    case ATOM_FLIT: cout << ":A "; break;
    case HEAD_FLIT: cout << ":H "; break;
    case MIDL_FLIT: cout << ":M "; break;
    case TAIL_FLIT: cout << ":T "; break;
    default: assert(0);
    }
  }
  cout << endl;
}
#endif

        // reclaim generated flits that are not used for reduction.
        for (unsigned int n=0; n<flit_store_org_vec.size(); n++) {
            if (flit_store_org_vec[n] == 0)
                continue;

            switch (flit_store_org_vec[n]->type()) {
            case HEAD_FLIT: g_FlitHeadPool.reclaim((FlitHead*) flit_store_org_vec[n]); break;
            case MIDL_FLIT: g_FlitMidlPool.reclaim((FlitMidl*) flit_store_org_vec[n]); break;
            case TAIL_FLIT: g_FlitTailPool.reclaim((FlitTail*) flit_store_org_vec[n]); break;
            case ATOM_FLIT: g_FlitAtomPool.reclaim((FlitAtom*) flit_store_org_vec[n]); break;
            default: assert(0);
            }
        }
    } else {
#ifdef _DEBUG_NI_DATA_COMPRESS
if (_debug_router_id_set.count(m_attached_router->id()) == 1 && m_NI_id == _debug_ni_id) {
  cout << "  CMP: no compression" << endl;
}
#endif
    } // switch (g_cfg.cam_data_manage) {

    // update the number of flits for this packet
    p_pkt->m_num_flits = flit_store_vec.size();

    int num_cam_access = (payload_value_str.size()/2) / m_CAM_data_vec[encoder_pos]->blk_byte();
    assert(num_cam_access == ((int) cam_en_sts_vec.size()));
// printf("payload_value_str.size()=%d m_CAM_data_vec[encoder_pos]->blk_byte()=%d num_cam_access=%d \n", payload_value_str.size(), m_CAM_data_vec[encoder_pos]->blk_byte(), num_cam_access);
}

void NIInputCompr::disableCAM(int dest_router_id, int ni_out_id)
{
// printf("CAM disable clk=%0.lf src=%d dest=%d\n", simtime(), m_attached_router->id(), dest_router_id); fflush(stdout);
    // assert(! m_CAM_sts_vec[dest_router_id]);
    m_CAM_sts_vec[dest_router_id] = true;
}

void NIInputCompr::enableCAM(int dest_router_id, int ni_out_id)
{
// printf("CAM enable clk=%0.lf src=%d dest=%d\n", simtime(), m_attached_router->id(), dest_router_id); fflush(stdout);
    // assert(m_CAM_sts_vec[dest_router_id]);
    m_CAM_sts_vec[dest_router_id] = false;
}
