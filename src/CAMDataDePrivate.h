#ifndef _CAM_DATA_DE_PRIVATE_H_
#define _CAM_DATA_DE_PRIVATE_H_

#include "noc.h"
#include "CAMDataDe.h"

class CAMDataDePrivate : public CAMDataDe {
public:
    // Constructors
    CAMDataDePrivate();

    // Destructors
    ~CAMDataDePrivate();

    // Public Methods
    void printStats(ostream& out) const;

    void alloc(int decoder_id, CAMReplacePolicy repl_policy, int num_sets, int blk_byte, int VLB_num_sets=0);
    void dealloc();
    bool decompress(string value_str, Packet* p_pkt)
         { m_num_access++; return true; };
    void update(string value_str, Packet* p_pkt) { m_num_access++; } ;

    void print(ostream& out) const;
    void print(FILE* stat_fp) { fprintf(stat_fp, " DePv=%d\n", m_decoder_id); };
};

// Output operator declaration
ostream& operator<<(ostream& out, const CAMDataDePrivate& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const CAMDataDePrivate& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _CAM_DATA_DE_PRIVATE_H_
