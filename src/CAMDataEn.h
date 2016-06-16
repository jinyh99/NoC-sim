#ifndef _CAM_DATA_EN_H_
#define _CAM_DATA_EN_H_

#include "noc.h"
#include "WorkloadGEMSType.h"

// encoder
class CAMDataEn {
public:
    // Constructors
    CAMDataEn() {};

    // Destructors
    virtual ~CAMDataEn() {};

    // Public Methods
    virtual void printStats(ostream& out) const = 0;
    virtual void clearStats() = 0;

    virtual void alloc(int encoder_id, CAMReplacePolicy repl_policy, int num_sets, int blk_byte) = 0;
    virtual void dealloc() = 0;
    virtual void compress(string datablk_value_str, vector< int > & en_sts_vec, int dest_id) = 0;

    const int id() { return m_encoder_id; };
    const int blk_byte() { return m_blk_byte; };
    const int sets() { return m_num_sets; };
    const int hits() { return m_num_hit; };
    const int misses() { return m_num_miss; };
    const int accesses() { return m_num_hit + m_num_miss; };
    const int hits_zero() { return m_num_hit_zero; };
    const double hit_rate() { return (accesses() == 0) ? 0.0 : ((double) hits()) / accesses(); };
    const double miss_rate() { return (1.0 - hit_rate()); };

    vector< string > getEnNewValueVec() { return m_en_new_value_vec; };
    vector< string > getEnValueVec() { return m_en_value_vec; };

    virtual void print(ostream& out) const = 0;
    virtual void print(FILE* stat_fp) = 0;

protected:
    int m_encoder_id;
    bool m_buf_alloc;
    int m_num_sets;
    int m_blk_byte;

    enum CAMReplacePolicy m_repl_policy;

    // statistics
    int	m_num_hit;
    int	m_num_miss;
    int	m_num_hit_zero;	// zero values

    // newly encoded values (CAM miss)
    vector< string > m_en_new_value_vec;
    // successfuly encoded values (CAM hit)
    vector< string > m_en_value_vec;
};

// Output operator declaration
ostream& operator<<(ostream& out, const CAMDataEn& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const CAMDataEn& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
