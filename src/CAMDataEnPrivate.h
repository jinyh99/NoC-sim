#ifndef _CAM_DATA_EN_PRIVATE_H_
#define _CAM_DATA_EN_PRIVATE_H_

#include "CAMDataEn.h"

struct CAMDataEnPrivateEntry {
    struct CAMDataEnPrivateEntry* m_next;
    struct CAMDataEnPrivateEntry* m_prev;

    bool m_valid;
    int m_index;
    string m_value_str;

    unsigned int m_LFU_counter;	// for LFU replacement
    unsigned int m_num_access;
};

typedef map<string, CAMDataEnPrivateEntry*> Data2BufMap;

class CAMDataEnPrivate : public CAMDataEn {
public:
    // Constructors
    CAMDataEnPrivate();

    // Destructors
    ~CAMDataEnPrivate();

    // Public Methods
    void printStats(ostream& out) const;
    void clearStats();

    void alloc(int encoder_id, CAMReplacePolicy repl_policy, int num_sets, int blk_byte);
    void dealloc();
    void compress(string datablk_value_str, vector< int > & en_sts_vec, int dest_id);

    void print(ostream& out) const;
    void print(FILE* stat_fp);

private:
    // Private Methods

    // unit access
    int access_by_LRU(const string value_str);
    int access_by_LFU(const string value_str);

    CAMDataEnPrivateEntry* m_buf_head;
    CAMDataEnPrivateEntry* m_buf_tail;
    CAMDataEnPrivateEntry* m_buf;
    Data2BufMap m_datablk2buf_map;
};

// Output operator declaration
ostream& operator<<(ostream& out, const CAMDataEnPrivate& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const CAMDataEnPrivate& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _CAM_DATA_EN_PRIVATE_H_
