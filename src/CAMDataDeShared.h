#ifndef _CAM_DATA_DE_SHARED_H_
#define _CAM_DATA_DE_SHARED_H_

#include "noc.h"
#include "CAMDataDe.h"

struct CAMDataDeSharedEntry {
    struct CAMDataDeSharedEntry* m_next;
    struct CAMDataDeSharedEntry* m_prev;

    int m_index;
    bool m_valid;
    string m_value_str;
    unsigned int m_LFU_counter;     // for LFU replacement
    unsigned int m_access;

    vector< bool > m_src_valid_vec;
};

//////////////////////////////////////////////////////////////////
// entry for value locality buffer 
struct VLBEntry {
    bool m_valid;
    string m_value_str;
    int m_LFU_counter;     // for LFU replacement
};  
        
typedef map<string, VLBEntry*> Value2VLBEntryMap;
////////////////////////////////////////////////////////////////////

typedef map<string, CAMDataDeSharedEntry*> DeSharedValueMap;

class CAMDataDeShared : public CAMDataDe {
public:
    // Constructors
    CAMDataDeShared();

    // Destructors
    ~CAMDataDeShared();

    // Public Methods
    void printStats(ostream& out) const;
    void clearStats();

    void alloc(int decoder_id, CAMReplacePolicy repl_policy,
               int num_sets, int blk_byte, int VLB_num_sets);
    void dealloc();
    bool decompress(string value_str, Packet* p_pkt);
    void update(string value_str, Packet* p_pkt);

    void print(ostream& out) const;
    void print(FILE* stat_fp);

    void set_num_peer(int num_peer) { m_num_peer = num_peer; };

    const int hits() { return m_num_hit; };
    const int misses() { return m_num_miss; };
    const int hits_zero() { return m_num_hit_zero; };
    const int accesses() { return m_num_hit + m_num_miss; };
    const double hit_rate() { return (accesses() == 0) ? 0.0 : ((double) hits()) / accesses(); };

    // for VLB
    int VLB_hits() const { return m_num_VLB_hit; };
    int VLB_misses() const { return m_num_VLB_miss; };
    int VLB2CAM_moves() const { return m_num_VLB2CAM_move; } ;
    int replace_reqs() const { return m_num_replace_req; } ;

private:
    // Private Methods

    // unit access
    bool access_by_LRU(const string value_str, Packet* p_pkt);
    bool access_by_LFU(const string value_str, Packet* p_pkt);
    void sendReplacePkt(string value_str, int value_index, int cur_router_id, int src_router_id);

private:
    int m_num_peer;

    CAMDataDeSharedEntry* m_buf_head;
    CAMDataDeSharedEntry* m_buf_tail;
    CAMDataDeSharedEntry* m_buf;

    DeSharedValueMap m_value2entry_map;

    // statistics
    int m_num_hit;
    int m_num_miss;
    int m_num_hit_zero;       // zero string
    int m_num_replace_req;

    ////////////////////////////////////////////////
    // VLB
    Value2VLBEntryMap m_value2VLB_map;
    int m_VLB_num_sets;
    VLBEntry* m_VLB;

    int m_num_VLB2CAM_move;
    int m_num_VLB_hit;
    int m_num_VLB_miss;
    ////////////////////////////////////////////////
};

// Output operator declaration
ostream& operator<<(ostream& out, const CAMDataDeShared& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const CAMDataDeShared& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _CAM_DATA_DE_SHARED_H_
