#ifndef _CAM_DATA_EN_SHARED_H_
#define _CAM_DATA_EN_SHARED_H_

#include "CAMDataEn.h"

struct CAMDataEnSharedEntry {
    struct CAMDataEnSharedEntry* m_next;
    struct CAMDataEnSharedEntry* m_prev;

    bool m_valid;
    vector< bool > m_dest_valid_vec;	// valid
    vector< int > m_dest_index_vec;	// encoded index
    string m_value_str;

    unsigned int m_LFU_counter;		// LFU replacement
    unsigned int m_num_access;
};

typedef map< string, CAMDataEnSharedEntry* > EnSharedValueMap;

class CAMDataEnShared : public CAMDataEn {
public:
    // Constructors
    CAMDataEnShared();

    // Destructors
    ~CAMDataEnShared();

    // Public Methods
    void printStats(ostream& out) const;
    void clearStats();

    void alloc(int encoder_id, CAMReplacePolicy repl_policy, int num_sets, int blk_byte);
    void dealloc();
    void compress(string datablk_value_str, vector< int > & en_sts_vec, int dest_id);

    void print(ostream& out) const;
    void print(FILE* stat_fp);

    void set_num_peer(int num_peer) { m_num_peer = num_peer; };
    void replace(string value_str, int value_index, int dest_id);
  
private:
    // Private Methods

    // unit access
    int access_by_LRU(const string value_str, int dest_id);
    int access_by_LFU(const string value_str, int dest_id);

    int m_num_peer;

    CAMDataEnSharedEntry* m_buf_head;
    CAMDataEnSharedEntry* m_buf_tail;
    CAMDataEnSharedEntry* m_buf;

    EnSharedValueMap m_value2entry_map;
};

// Output operator declaration
ostream& operator<<(ostream& out, const CAMDataEnShared& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const CAMDataEnShared& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _CAM_DATA_EN_SHARED_H_
