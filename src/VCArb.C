#include "noc.h"
#include "VCArb.h"

VCArb::VCArb()
{
    m_grant_rate_tab = new table("grant_rate");
    m_num_req_tab = new table("num_req");
}

VCArb::VCArb(Router* p_router)
{
    // abstract class
    assert(0);
}

VCArb::~VCArb()
{
}

// statistic reset when warmup period ends.
void VCArb::resetStats()
{
    m_grant_rate_tab->reset();
    m_num_req_tab->reset();
}
