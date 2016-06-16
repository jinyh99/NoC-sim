#include "noc.h"
#include "CAMDataDeShared.h" 
#include "CAMDataEnShared.h"
#include "NIInputCompr.h"
#include "Router.h"
#include "CAMManager.h"

// #define CAM_DATA_SHARED_DEBUG

#ifdef CAM_DATA_SHARED_DEBUG
static IntSet _debug_decoder_id_set;
#endif

CAMDataDeShared::CAMDataDeShared()
{
    m_buf_alloc = false;

#ifdef CAM_DATA_SHARED_DEBUG
    _debug_decoder_id_set.insert(0);
//  _debug_decoder_id_set.insert(1);
//  _debug_decoder_id_set.insert(2);
//  _debug_decoder_id_set.insert(3);
//  _debug_decoder_id_set.insert(4);
//  _debug_decoder_id_set.insert(13);
#endif
}

CAMDataDeShared::~CAMDataDeShared()
{
    dealloc();
}

void CAMDataDeShared::print(ostream& out) const
{
    printStats(out);
}

void CAMDataDeShared::printStats(ostream& out) const
{
    out << "CAMDataDeShared:"
        << " id=" << m_decoder_id
        << " entries=" << m_num_sets << endl;
    out << "total accesses: " << m_num_access << endl;
    out << "access rate (access/cycle): " << ((double) m_num_access) / simtime() << endl;
    out << endl;
    out << endl;
}

void CAMDataDeShared::clearStats()
{
    m_num_hit = 0;
    m_num_miss = 0;
    m_num_hit_zero = 0;
    m_num_replace_req = 0;

    m_num_VLB2CAM_move = 0;
    m_num_VLB_hit = 0;
    m_num_VLB_miss = 0;
}

void CAMDataDeShared::alloc(int decoder_id, CAMReplacePolicy repl_policy, int num_sets,
                            int blk_byte, int VLB_num_sets)
{
    m_decoder_id = decoder_id;

    m_repl_policy = repl_policy; 

    m_num_sets = num_sets;
    assert(is_power2(blk_byte));
    m_blk_byte = blk_byte;

    clearStats();

    assert(num_sets > 0);

    m_buf = new CAMDataDeSharedEntry [m_num_sets];
    assert(m_buf);
    for (int i=0; i<m_num_sets; i++) {
        m_buf[i].m_prev = (i == 0) ? 0 : &m_buf[i-1];
        m_buf[i].m_next = (i == m_num_sets-1) ? 0 : &m_buf[i+1];

        m_buf[i].m_index = i;
        m_buf[i].m_valid = false;
        m_buf[i].m_value_str = "";
        m_buf[i].m_LFU_counter = 0;
        m_buf[i].m_access = 0;

        m_buf[i].m_src_valid_vec.resize(m_num_peer, false);
    }
    m_buf_head = &m_buf[0];
    m_buf_tail = &m_buf[m_num_sets-1];

    // VLB
    m_VLB_num_sets = VLB_num_sets;
    m_VLB = new VLBEntry [m_VLB_num_sets];
    assert(m_VLB);
    for (int i=0; i<m_VLB_num_sets; i++) {
        m_VLB[i].m_valid = false;
        m_VLB[i].m_value_str = "";
        m_VLB[i].m_LFU_counter = 0;
    }

/*
cout << "m_decoder_id=" << m_decoder_id << " m_buf_tail->next=" << m_buf_tail->next << endl;
for (CAMDataDeSharedEntry* ptr = m_buf_head; ptr; ptr = ptr->next)
cout << (ptr->valid ? "X" : "-");
cout << endl;
*/

    m_buf_alloc = true;
}

void CAMDataDeShared::dealloc()
{
    if (m_buf) {
        delete [] m_buf;
        m_buf = 0;
        m_num_sets = 0;

        m_buf_alloc = false;
    }

    if (m_VLB) {
        delete [] m_VLB;
        m_VLB = 0;
        m_VLB_num_sets = 0;
    }
}


bool CAMDataDeShared::decompress(string value_str, Packet* p_pkt)
{
    if (m_value2entry_map.find(value_str) != m_value2entry_map.end()) {
        // decompression successful
        m_num_access++;
#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_decoder_id_set.count(m_decoder_id) == 1) printf("DeCAM-%02d: decomp:hit  clk=%.0lf src_id=%2d value=%s\n", m_decoder_id, simtime(), p_pkt->getSrcRouterID(), value_str.c_str());
#endif

        return true;
    }

    return false;
}

void CAMDataDeShared::update(string value_str, Packet* p_pkt)
{
    switch (m_repl_policy) {
    case CAM_REPL_LRU:
        access_by_LRU(value_str, p_pkt);
        break;
    case CAM_REPL_LFU:
        access_by_LFU(value_str, p_pkt);
        break;
    default:
        assert(0);
    }

    m_num_access++;
}

bool CAMDataDeShared::access_by_LRU(const string value_str, Packet* p_pkt)
{
    assert(0);	// not implemented yet

    return false;
}

// return value: true if hit
//               false if miss
bool CAMDataDeShared::access_by_LFU(const string value_str, Packet* p_pkt)
{
    // 4-bit is represented as one letter.
    assert(STRLEN2BYTESZ(value_str.length()) == (unsigned int) m_blk_byte);

    int src_id = p_pkt->getSrcRouterID();
    bool hit;

    DeSharedValueMap::iterator pos = m_value2entry_map.find(value_str);

    if (pos == m_value2entry_map.end()) {
#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_decoder_id_set.count(m_decoder_id) == 1) printf("DeCAM-%02d: update:miss clk=%.0lf src_id=%2d value=%s\n", m_decoder_id, simtime(), p_pkt->getSrcRouterID(), value_str.c_str());
#endif
        // not found in m_data2buf_map (CAM miss)

        Value2VLBEntryMap::iterator VLBpos = m_value2VLB_map.find(value_str);
        if (VLBpos == m_value2VLB_map.end()) { // VLB miss ?
            // not found in m_value2VLB_map (VLB miss)
#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_decoder_id_set.count(m_decoder_id) == 1) printf("VLB miss\n");
#endif

            // find the buffer (VLB_repl_pos) which is invalid or has the least freq
            int VLB_repl_pos = -1;
            for (int i=0; i<m_VLB_num_sets; i++) {
                if (!m_VLB[i].m_valid) {
                    VLB_repl_pos = i;
                    break;
                }
            }
            if (VLB_repl_pos == -1) { // all entries in VLB are valid ?
                int min_freq = INT_MAX;
                for (int i=0; i<m_VLB_num_sets; i++) {
                    if (m_VLB[i].m_LFU_counter < min_freq) {
                        min_freq = m_VLB[i].m_LFU_counter;
                        VLB_repl_pos = i;
                    }
                }
            }

            assert(VLB_repl_pos >= 0 && VLB_repl_pos < m_VLB_num_sets);

            // invalidate VLB
            if (m_VLB[VLB_repl_pos].m_valid)
                m_value2VLB_map.erase(m_VLB[VLB_repl_pos].m_value_str);

            // replace with a new value
            m_VLB[VLB_repl_pos].m_valid = true;
            m_VLB[VLB_repl_pos].m_value_str = value_str;
            m_VLB[VLB_repl_pos].m_LFU_counter = 1;

            // insert new value into map
            m_value2VLB_map[value_str] = &(m_VLB[VLB_repl_pos]);

            m_num_VLB_miss++;
        } else { // VLB hit
            VLBEntry* found_VLBbuf = m_value2VLB_map[value_str];
            (found_VLBbuf->m_LFU_counter)++;

#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_decoder_id_set.count(m_decoder_id) == 1) printf("VLB hit freq=%d", found_VLBbuf->freq);
#endif

            if (found_VLBbuf->m_LFU_counter > g_cfg.cam_VLB_LFU_saturation) {
                // move this entry from VLB to CAM
                assert(m_value2entry_map.find(value_str) == m_value2entry_map.end());

                // find a entry in CAM for replacement
                CAMDataDeSharedEntry* repl_buf = m_buf_head;
                CAMDataDeSharedEntry* cur_buf = m_buf_head;
                unsigned int cur_least_freq = m_buf_head->m_LFU_counter;
                while (1) {
                    if (! cur_buf->m_valid ) {
                        repl_buf = cur_buf;
                        break;
                    }

                    if (cur_buf->m_LFU_counter < cur_least_freq) {
                        repl_buf = cur_buf;
                        cur_least_freq = cur_buf->m_LFU_counter;
                    }

                    if (cur_buf == m_buf_tail)
                        break;

                    cur_buf = cur_buf->m_next;
                }
                assert(repl_buf);

                if (repl_buf->m_valid) {
                    // invalidate data in this entry
                    m_value2entry_map.erase(repl_buf->m_value_str);
                    repl_buf->m_src_valid_vec.resize(m_num_peer, false);
                }

                // update content
                repl_buf->m_valid = true;
                repl_buf->m_value_str = value_str;
                repl_buf->m_LFU_counter = 1;
                repl_buf->m_access = 1;
                repl_buf->m_src_valid_vec[src_id] = true;

                // insert data into map
                m_value2entry_map[value_str] = repl_buf;
// if (m_data2buf_map.size() > m_num_sets) cout << "m_decoder_id=" << m_decoder_id << " m_num_sets=" << m_num_sets << endl;
                assert(m_value2entry_map.size() <= (unsigned int) m_num_sets);

                // notify source CAM (replace source CAM)
                sendReplacePkt(value_str, repl_buf->m_index, p_pkt->getDestRouterID(), p_pkt->getSrcRouterID());

                // invalidate VLB entry
                m_value2VLB_map.erase(value_str);
                found_VLBbuf->m_valid = false;
                found_VLBbuf->m_value_str = "";
                found_VLBbuf->m_LFU_counter = 0;

#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_decoder_id_set.count(m_decoder_id) == 1) printf("  moved to CAM\n");
#endif

                m_num_VLB2CAM_move++;
            } else {
#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_decoder_id_set.count(m_decoder_id) == 1) printf("  no movement to CAM\n");
#endif
            }  // if (found_VLBbuf->freq > g_cfg.VLB2CAM_move_thr) {

            m_num_VLB_hit++;
        }

        hit = false;
        m_num_miss++;
// cout << "miss m_decoder_id=" << m_decoder_id << endl;
    } else {
        // found entry in m_data2buf_map
#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_decoder_id_set.count(m_decoder_id) == 1) printf("DeCAM-%02d: update:hit  clk=%.0lf src_id=%2d value=%s\n", m_decoder_id, simtime(), p_pkt->getSrcRouterID(), value_str.c_str());
#endif
        // just increase its frequency
        CAMDataDeSharedEntry* found_buf = m_value2entry_map[value_str];
        assert(value_str.compare(found_buf->m_value_str) == 0);

        if (found_buf->m_src_valid_vec[src_id] == false) {
            // notify source CAM (replace source CAM)
            sendReplacePkt(found_buf->m_value_str, found_buf->m_index, p_pkt->getDestRouterID(), p_pkt->getSrcRouterID());

            // set valid for this source
            found_buf->m_src_valid_vec[src_id] = true;
            hit = false;
            m_num_miss++;
        } else {
            hit = true;
            m_num_hit++;
            if (isZeroStr(value_str))
                m_num_hit_zero++;
        }

        (found_buf->m_LFU_counter)++;
        (found_buf->m_access)++;

        // saturation (4-bit counter)
        if (found_buf->m_LFU_counter >= ((unsigned int) g_cfg.cam_LFU_saturation)) {
            // decrease all counters by right shift.
            for (int i=0; i<m_num_sets; i++) {
                if (m_buf[i].m_LFU_counter > 0)
                    m_buf[i].m_LFU_counter >>= 1;
            }
        }

// cout << "hit m_decoder_id=" << m_decoder_id << endl;
    }

    return hit;
}

void CAMDataDeShared::sendReplacePkt(string value_str, int value_index,
                                     int cur_router_id, int src_router_id)
{
    CAMDataEnShared* p_enSCAM = (dynamic_cast<CAMDataEnShared*> (((NIInputCompr*) g_Router_vec[src_router_id]->getNIInput(0))->getCAM(0)));

    p_enSCAM->replace(value_str, value_index, cur_router_id);

    m_num_replace_req++;
    g_CamManager->m_pkt_tab_sync += 2;	// request + ack
}

void CAMDataDeShared::print(FILE* stat_fp)
{
    fprintf(stat_fp, " DePv=%d\n", m_decoder_id);
    for (CAMDataDeSharedEntry* ptr=m_buf_head; ptr!=0; ptr=ptr->m_next) {
        string valid_str = "";
        for (unsigned int s=0; s<ptr->m_src_valid_vec.size(); s++)
            valid_str += ptr->m_src_valid_vec[s] ? 'O' : 'X';
      
        fprintf(stat_fp, "  %2d %c %s %4d %8d %s\n", ptr->m_index, ptr->m_valid ? 'T':'F', ptr->m_value_str.c_str(), ptr->m_LFU_counter, ptr->m_access, valid_str.c_str());
    }
    fprintf(stat_fp, "\n");

    for (int i=0; i<m_VLB_num_sets; i++)
        fprintf(stat_fp, "  %2d %c %s %8d\n", i, m_VLB[i].m_valid ? 'T':'F', m_VLB[i].m_value_str.c_str(), m_VLB[i].m_LFU_counter);

    fprintf(stat_fp, "\n");
}
