#include "CAMDataDePrivate.h"

CAMDataDePrivate::CAMDataDePrivate()
{
}

CAMDataDePrivate::~CAMDataDePrivate()
{
    dealloc();
}

void CAMDataDePrivate::print(ostream& out) const
{
    printStats(out);
}

void CAMDataDePrivate::printStats(ostream& out) const
{

    out << "CAMDataDePrivate:"
        << " id=" << m_decoder_id
        << " entries=" << m_num_sets << endl;
    out << "total accesses: " << m_num_access << endl;
    out << "access rate (access/cycle): " << ((double) m_num_access) / simtime() << endl;
    out << endl;
    out << endl;
}

void CAMDataDePrivate::alloc(int decoder_id, CAMReplacePolicy repl_policy, int num_sets, int blk_byte, int VLB_num_sets)
{
    m_decoder_id = decoder_id;

    m_repl_policy = repl_policy; 

    m_num_sets = num_sets;
    assert(is_power2(blk_byte));
    m_blk_byte = blk_byte;

    clearStats();

    assert(num_sets > 0);

    m_buf_alloc = true;
}

void CAMDataDePrivate::dealloc()
{
}
