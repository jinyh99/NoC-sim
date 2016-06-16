#include "CAMDataEnPrivate.h"

// #define CAM_DATA_PRIVATE_DEBUG

#ifdef CAM_DATA_PRIVATE_DEBUG
static IntSet _debug_encoder_id_set;
#endif

CAMDataEnPrivate::CAMDataEnPrivate()
{
    m_buf_alloc = false;
}

CAMDataEnPrivate::~CAMDataEnPrivate()
{
    dealloc();
}

void CAMDataEnPrivate::print(ostream& out) const
{
    printStats(out);
}

void CAMDataEnPrivate::printStats(ostream& out) const
{
    out << "CAMDataEnPrivate:"
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

void CAMDataEnPrivate::clearStats()
{
    m_num_hit = 0;
    m_num_miss = 0;
    m_num_hit_zero = 0;
}

void CAMDataEnPrivate::alloc(int encoder_id, CAMReplacePolicy repl_policy, int num_sets, int blk_byte)
{
    m_encoder_id = encoder_id;

    m_repl_policy = repl_policy; 
    m_buf = 0;
    m_buf_head = 0;
    m_buf_tail = 0;

    m_num_sets = num_sets;
    assert(is_power2(blk_byte));
    m_blk_byte = blk_byte;

    clearStats();

    m_buf = new CAMDataEnPrivateEntry [m_num_sets];
    assert(m_buf);
    for (int i=0; i<m_num_sets; i++) {
        m_buf[i].m_prev = (i == 0) ? 0 : &m_buf[i-1];
        m_buf[i].m_next = (i == m_num_sets-1) ? 0 : &m_buf[i+1];

        m_buf[i].m_index = i;
        m_buf[i].m_valid = false;
        m_buf[i].m_value_str = "";
        m_buf[i].m_LFU_counter = 0;
        m_buf[i].m_num_access = 0;
    }

    m_buf_head = &m_buf[0];
    m_buf_tail = &m_buf[m_num_sets-1];

/*
cout << "m_encoder_id=" << m_encoder_id << " m_buf_tail->next=" << m_buf_tail->next << endl;
for (CAMDataEnPrivateBuffer* ptr = m_buf_head; ptr; ptr = ptr->next)
cout << (ptr->valid ? "X" : "-");
cout << endl;
*/

    m_buf_alloc = true;
}

void CAMDataEnPrivate::dealloc()
{
    if (m_buf) {
        delete [] m_buf;
        m_buf = 0;
        m_num_sets = 0;

        m_buf_alloc = false;
    }
}

// return size of bytes of reduced data
void CAMDataEnPrivate::compress(string datablk_value_str, vector< int > & en_sts_vec, int dest_id)
{
    assert(m_buf_alloc);

    // reset m_en_new_value_vec and m_en_value_vec
    m_en_new_value_vec.clear();
    m_en_value_vec.clear();

    int str_skip_sz = m_blk_byte*2;	// A bytes = 2A chars
    string encoded_datablk_value_str = "";
// cout << "str_skip_sz=" << str_skip_sz << endl;
    for (unsigned int i=0; i<datablk_value_str.length(); i += str_skip_sz) {
        string value_str = datablk_value_str.substr(i, str_skip_sz);
// cout << "i=" << i << ":" << value_str << endl;

        int hit_index;
        switch (m_repl_policy) {
        case CAM_REPL_LRU:
            hit_index = access_by_LRU(value_str);
            break;
        case CAM_REPL_LFU:
            hit_index = access_by_LFU(value_str);
            break;
        default:
            assert(0);
        }

        en_sts_vec.push_back(hit_index);
    }
}

// return value: index (non-negative) of bits if hit
//               negative if miss
int CAMDataEnPrivate::access_by_LRU(const string value_str)
{
    // 1 char => 4 bits, 0.5 byte; N chars => N/2 byte
    assert(value_str.length()/2 == (unsigned int) m_blk_byte);

    int hit_index = -1;

    Data2BufMap::iterator pos = m_datablk2buf_map.find(value_str);

    if (pos == m_datablk2buf_map.end()) {
        // not found in m_data2buf_map

        CAMDataEnPrivateEntry* tail_buf = m_buf_tail;
        assert(tail_buf->m_next == 0);

        // update m_buf_tail
        tail_buf->m_prev->m_next = 0;
        m_buf_tail = tail_buf->m_prev;

        // set tail_buf as the head
        tail_buf->m_next = m_buf_head;
        tail_buf->m_prev = 0;
        m_buf_head->m_prev = tail_buf;
        m_buf_head = tail_buf;

        if (tail_buf->m_valid) {
            // invalidate data in this entry
            m_datablk2buf_map.erase(tail_buf->m_value_str);
        }

        // update content
        tail_buf->m_valid = true;
        tail_buf->m_value_str = value_str;

        // insert data into map
        m_datablk2buf_map[value_str] = tail_buf;
// if (m_data2buf_map.size() > m_num_sets) cout << "m_encoder_id=" << m_encoder_id << " m_num_sets=" << m_num_sets << endl;
        assert(m_datablk2buf_map.size() <= (unsigned int) m_num_sets);

        // store new encoding values
        bool isNew = true;
        for (unsigned int i=0; i<m_en_new_value_vec.size(); i++) {
            if (m_en_new_value_vec[i] == value_str) {
                isNew = false;
                break;
            }
        }
        if (isNew)
            m_en_new_value_vec.push_back(value_str);

        m_num_miss++;

        hit_index = -1;
// cout << "miss m_encoder_id=" << m_encoder_id << endl;
    } else {
        // found entry in m_data2buf_map
        // just change the order in the list
        CAMDataEnPrivateEntry* found_buf = m_datablk2buf_map[value_str];

        if (found_buf == m_buf_head) {
            // already at the head
        } else if (found_buf == m_buf_tail) { // found at tail
            assert(found_buf->m_next == 0);

            // update m_buf_tail
            found_buf->m_prev->m_next = 0;
            m_buf_tail = found_buf->m_prev;

            // set found_buf as the head
            found_buf->m_next = m_buf_head;
            found_buf->m_prev = 0;
            m_buf_head->m_prev = found_buf;
            m_buf_head = found_buf;
        } else {	// found in the middle of list (not head or tail)
            found_buf->m_prev->m_next = found_buf->m_next;
            found_buf->m_next->m_prev = found_buf->m_prev;

            // set found_buf as the head
            found_buf->m_next = m_buf_head;
            found_buf->m_prev = 0;
            m_buf_head->m_prev = found_buf;
            m_buf_head = found_buf;
        }

        // store successfully encoded values
        m_en_value_vec.push_back(value_str);

        m_num_hit++;
        if (isZeroStr(value_str))
            m_num_hit_zero++;
// cout << "hit m_encoder_id=" << m_encoder_id << endl;

        hit_index = found_buf->m_index;
    }

    return hit_index;
}

// return value: index (non-negative) of bits if hit
//               negative if miss
int CAMDataEnPrivate::access_by_LFU(const string value_str)
{
    // 4-bit is represented as one letter.
    assert(value_str.length()/2 == (unsigned int) m_blk_byte);

    int hit_index;

    Data2BufMap::iterator pos = m_datablk2buf_map.find(value_str);

    if (pos == m_datablk2buf_map.end()) {
        // not found in m_data2buf_map

        // find a entry in CAM for replacement
        CAMDataEnPrivateEntry* repl_buf = m_buf_head;
        CAMDataEnPrivateEntry* cur_buf = m_buf_head;
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
            m_datablk2buf_map.erase(repl_buf->m_value_str);
        }

        // update content
        repl_buf->m_valid = true;
        repl_buf->m_value_str = value_str;
        repl_buf->m_LFU_counter = 1;
        repl_buf->m_num_access = 1;

        // insert data into map
        m_datablk2buf_map[value_str] = repl_buf;
// if (m_data2buf_map.size() > m_num_sets) cout << "m_encoder_id=" << m_encoder_id << " m_num_sets=" << m_num_sets << endl;
        assert(m_datablk2buf_map.size() <= (unsigned int) m_num_sets);

        // store new encoding values
        bool isNew = true;
        for (unsigned int i=0; i<m_en_new_value_vec.size(); i++) {
            if (m_en_new_value_vec[i] == value_str) {
                isNew = false;
                break;
            }
        }
        if (isNew)
            m_en_new_value_vec.push_back(value_str);

        m_num_miss++;

        hit_index = -1;
// cout << "miss m_encoder_id=" << m_encoder_id << endl;
    } else {
        // found entry in m_data2buf_map (hit)
        // just increase its frequency
        CAMDataEnPrivateEntry* found_buf = m_datablk2buf_map[value_str];

        assert(value_str.compare(found_buf->m_value_str) == 0);
        (found_buf->m_LFU_counter)++;
        (found_buf->m_num_access)++;

        // store successfully encoded values
        m_en_value_vec.push_back(value_str);

        // saturation (4-bit counter)
        if (found_buf->m_LFU_counter >= ((unsigned int) g_cfg.cam_LFU_saturation)) {
            // decrease all counters by right shift.
            for (int i=0; i<m_num_sets; i++) {
                if (m_buf[i].m_LFU_counter > 0)
                    m_buf[i].m_LFU_counter >>= 1;
            }
        }

        m_num_hit++;
        if (isZeroStr(value_str))
            m_num_hit_zero++;
// cout << "hit m_encoder_id=" << m_encoder_id << endl;

        hit_index = found_buf->m_index;
    }

    return hit_index;
}

void CAMDataEnPrivate::print(FILE* stat_fp)
{
    fprintf(stat_fp, " EnPv=%d\n", m_encoder_id);
    for (CAMDataEnPrivateEntry* ptr=m_buf_head; ptr!=0; ptr=ptr->m_next)
        fprintf(stat_fp, "  %2d %c %s %4d %8d\n", ptr->m_index, ptr->m_valid ? 'T':'F', ptr->m_value_str.c_str(), ptr->m_LFU_counter, ptr->m_num_access);
    fprintf(stat_fp, "\n");
}

