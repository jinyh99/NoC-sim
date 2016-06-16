#include "CAMDataEnShared.h"

// #define CAM_DATA_SHARED_DEBUG

#ifdef CAM_DATA_SHARED_DEBUG
static IntSet _debug_encoder_id_set;
#endif

CAMDataEnShared::CAMDataEnShared()
{
    m_buf_alloc = false;

#ifdef CAM_DATA_SHARED_DEBUG
  // _debug_encoder_id_set.insert(1);
  // _debug_encoder_id_set.insert(2);
  _debug_encoder_id_set.insert(3);
  // _debug_encoder_id_set.insert(4);
  // _debug_encoder_id_set.insert(9);
//  _debug_encoder_id_set.insert(15);
#endif
}

CAMDataEnShared::~CAMDataEnShared()
{
    dealloc();
}

void CAMDataEnShared::print(ostream& out) const
{
    printStats(out);
}

void CAMDataEnShared::printStats(ostream& out) const
{
    out << "CAMDataEnShared:"
        << " id=" << m_encoder_id
        << " entries=" << m_num_sets << endl;
    out << "total accesses: " << m_num_hit + m_num_miss << endl;
    out << "access rate (access/cycle): " << ((double) m_num_hit + m_num_miss) / simtime() << endl;
    out << "hit count: " << m_num_hit
        << " " << ((double) m_num_hit / (m_num_hit + m_num_miss)) * 100.0 << "%"
        << endl;
    out << "miss count: " << m_num_miss
        << " " << ((double) m_num_miss / (m_num_hit + m_num_miss)) * 100.0 << "%"
        << endl;
    out << endl;
    out << endl;
}

void CAMDataEnShared::clearStats()
{
    m_num_hit = 0;
    m_num_miss = 0;
    m_num_hit_zero = 0;
}

void CAMDataEnShared::alloc(int encoder_id, CAMReplacePolicy repl_policy, int num_sets, int blk_byte)
{
    m_encoder_id = encoder_id;

    m_repl_policy = repl_policy; 
    m_buf = 0;
    m_buf_head = 0;
    m_buf_tail = 0;

    assert(num_sets > 0);
    m_num_sets = num_sets;
    assert(is_power2(blk_byte));
    m_blk_byte = blk_byte;

    clearStats();

    m_buf = new CAMDataEnSharedEntry [m_num_sets];
    assert(m_buf);
    for (int i=0; i<m_num_sets; i++) {
        m_buf[i].m_prev = (i == 0) ? 0 : &m_buf[i-1];
        m_buf[i].m_next = (i == m_num_sets-1) ? 0 : &m_buf[i+1];

        m_buf[i].m_valid = false;
        m_buf[i].m_value_str = "";
        m_buf[i].m_LFU_counter = 0;
        m_buf[i].m_num_access = 0;

        m_buf[i].m_dest_valid_vec.resize(m_num_peer, false);
        m_buf[i].m_dest_index_vec.resize(m_num_peer, -1);
    }

    m_buf_head = &m_buf[0];
    m_buf_tail = &m_buf[m_num_sets-1];

/*
cout << "m_encoder_id=" << m_encoder_id << " m_buf_tail->next=" << m_buf_tail->next << endl;
for (CAMDataEnSharedEntry* ptr = m_buf_head; ptr; ptr = ptr->next)
cout << (ptr->valid ? "X" : "-");
cout << endl;
*/

    m_buf_alloc = true;
}

void CAMDataEnShared::dealloc()
{
    if (m_buf) {
        delete [] m_buf;
        m_buf = 0;
        m_num_sets = 0;

        m_buf_alloc = false;
    }
}

// return size of bytes of reduced data
void CAMDataEnShared::compress(string datablk_value_str, vector< int > & en_sts_vec, int dest_id)
{
    assert(m_buf_alloc);

    // reset m_en_new_value_vec and m_en_value_vec
    m_en_new_value_vec.clear();
    m_en_value_vec.clear();

#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_encoder_id_set.count(m_encoder_id) == 1) {
  cout << "datablk_value_str=" << datablk_value_str << endl;
}
#endif

    int str_skip_sz = m_blk_byte*2;	// A bytes = 2A chars
    string encoded_datablk_value_str = "";
// cout << "str_skip_sz=" << str_skip_sz << endl;
    for (unsigned int i=0; i<datablk_value_str.length(); i += str_skip_sz) {
        string value_str = datablk_value_str.substr(i, str_skip_sz);
// cout << "i=" << i << ":" << value_str << endl;

        int hit_index;
        switch (m_repl_policy) {
        case CAM_REPL_LRU:
            hit_index = access_by_LRU(value_str, dest_id);
            break;
        case CAM_REPL_LFU:
            hit_index = access_by_LFU(value_str, dest_id);
            break;
        default:
            assert(0);
        }

        en_sts_vec.push_back(hit_index);
    }
}

// return value: index (non-negative) of bits if hit
//               negative if miss
int CAMDataEnShared::access_by_LRU(const string value_str, int dest_id)
{
    assert(0);	// not implemented yet

    return 0;
}

// return value: index (non-negative) of bits if hit
//               negative if miss
int CAMDataEnShared::access_by_LFU(const string value_str, int dest_id)
{
    // 4-bit is represented as one letter.
    assert(STRLEN2BYTESZ(value_str.length()) == (unsigned int) m_blk_byte);

    int hit_index;

    EnSharedValueMap::iterator pos = m_value2entry_map.find(value_str);

    if (pos == m_value2entry_map.end()) { // CAM miss ?
#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_encoder_id_set.count(m_encoder_id) == 1) printf("EnCAM-%02d: value=%s dest=%d CAM miss ", m_encoder_id, value_str.c_str(), dest_id);
#endif
        // not found in m_data2buf_map (CAM miss)

        m_num_miss++;

        hit_index = -1;
// cout << "miss m_encoder_id=" << m_encoder_id << endl;
    } else {
        // found entry in m_data2buf_map (CAM hit)

        CAMDataEnSharedEntry* found_buf = m_value2entry_map[value_str];
        assert(value_str.compare(found_buf->m_value_str) == 0);

        if (found_buf->m_dest_valid_vec[dest_id] == false) {
            // (dest miss)
#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_encoder_id_set.count(m_encoder_id) == 1) printf("EnCAM-%02d: value=%s dest=%d CAM false hit (miss)\n", m_encoder_id, value_str.c_str(), dest_id);
#endif

            m_num_miss++;

            hit_index = -1;
        } else {
#ifdef CAM_DATA_SHARED_DEBUG
if (_debug_encoder_id_set.count(m_encoder_id) == 1) printf("EnCAM-%02d: value=%s dest=%d CAM hit\n", m_encoder_id, value_str.c_str(), dest_id);
#endif

            // store successfully encoded value
            m_en_value_vec.push_back(value_str);

            m_num_hit++;
            if (isZeroStr(value_str))
                m_num_hit_zero++;
// cout << "hit m_encoder_id=" << m_encoder_id << endl;

            hit_index = found_buf->m_dest_index_vec[dest_id];
            assert(hit_index >= 0);
        }

        found_buf->m_LFU_counter++;
        found_buf->m_num_access++;

        if (found_buf->m_LFU_counter >= ((unsigned int) g_cfg.cam_LFU_saturation)) { // counter saturated?
            // decrease all counters by right shift.
            for (int i=0; i<m_num_sets; i++) {
                if (m_buf[i].m_LFU_counter > 0)
                    m_buf[i].m_LFU_counter >>= 1;
            }
        }

    }

    return hit_index;
}

void CAMDataEnShared::replace(string value_str, int value_index, int dest_id)
{
    EnSharedValueMap::iterator pos = m_value2entry_map.find(value_str);
    if (pos == m_value2entry_map.end()) { // CAM miss ?
        // find a replacement entry in CAM
        CAMDataEnSharedEntry* repl_buf = m_buf_head;
        CAMDataEnSharedEntry* cur_buf = m_buf_head;
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
            // invalidate repl_buf
            m_value2entry_map.erase(repl_buf->m_value_str);
            repl_buf->m_dest_valid_vec.resize(m_num_peer, false);
            repl_buf->m_dest_index_vec.resize(m_num_peer, -1);
        }

        // update content
        repl_buf->m_valid = true;
        repl_buf->m_value_str = value_str;
        repl_buf->m_LFU_counter = 1;
        repl_buf->m_num_access = 1;
        repl_buf->m_dest_valid_vec[dest_id] = true;
        repl_buf->m_dest_index_vec[dest_id] = value_index;
        m_value2entry_map[value_str] = repl_buf;
    } else {	// CAM hit
        CAMDataEnSharedEntry* found_buf = m_value2entry_map[value_str];
        assert(value_str.compare(found_buf->m_value_str) == 0);

        assert(found_buf->m_valid);
        // assert(found_buf->dest_valid_vec[dest_id] == false);
        found_buf->m_dest_valid_vec[dest_id] = true;
        found_buf->m_dest_index_vec[dest_id] = value_index;
    }
}

void CAMDataEnShared::print(FILE* stat_fp)
{
    fprintf(stat_fp, " EnSh=%d\n", m_encoder_id);
    int index = 0;
    for (CAMDataEnSharedEntry* ptr=m_buf_head; ptr!=0; ptr=ptr->m_next) {
        fprintf(stat_fp, "  %2d %c %s %d\n", index, ptr->m_valid ? 'T':'F', ptr->m_value_str.c_str(), ptr->m_LFU_counter);
        index++;
    }
    fprintf(stat_fp, "\n");
}
