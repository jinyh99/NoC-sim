#ifndef _CAM_DATA_DE_H_
#define _CAM_DATA_DE_H_

#include "noc.h"
#include "WorkloadGEMSType.h"

class Packet;

class CAMDataDe {
public:
    // Constructors
    CAMDataDe() {};

    // Destructors
    virtual ~CAMDataDe() {};

    // Public Methods
    virtual void printStats(ostream& out) const = 0;
    virtual void clearStats() {
        m_num_access = 0;
    };

    virtual void alloc(int decoder_id, CAMReplacePolicy repl_policy, int num_sets, int blk_byte, int VLB_num_sets) = 0;
    virtual void dealloc() = 0;
    virtual bool decompress(string payload_value_str, Packet* p_pkt) = 0;
    virtual void update(string value_str, Packet* p_pkt) = 0;

    virtual void print(ostream& out) const = 0;
    virtual void print(FILE* stat_fp) = 0;

    const int id() { return m_decoder_id; };
    const int blk_byte() { return m_blk_byte; };
    const int sets() { return m_num_sets; };
    const int accesses() { return m_num_access; };

protected:
    // Private Methods
    int m_decoder_id;
    bool m_buf_alloc;
    int m_num_sets;
    int m_blk_byte;

    enum CAMReplacePolicy m_repl_policy;

    // statistics
    int m_num_access;
};

// Output operator declaration
ostream& operator<<(ostream& out, const CAMDataDe& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const CAMDataDe& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _CAM_DATA_DE_H_
