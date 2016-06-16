#include "noc.h"
#include "Flit.h"

////////////////////////////////////////////////////////////////////
// common
////////////////////////////////////////////////////////////////////

void Flit::init_common()
{
    m_pkt_ptr = 0;
    m_clk_enter_stage = 0.0;
    m_clk_enter_router = 0.0;
}

void Flit::destroy_common()
{
}

void Flit::setRandomData()
{
    unsigned long long high, low;

    // FIXME: support non 64-multiple aligned and less 64-bit numbers 
    if (g_cfg.link_width < 64) {
        if (g_cfg.link_width == 32) {
            low = random(0x0, 0x7FFFFFFF);
            low = (low << 32) >> 32;
            m_flitData[0] = low;
        } else {
            assert(0);
        }
    } else {
        for (int i=0; i<g_cfg.flit_sz_64bit_multiple; i++) {
            high = random(0x0, 0x7FFFFFFF);
            low = random(0x0, 0x7FFFFFFF);
            high = (high << 32);
            low = (low << 32) >> 32;
            low |= high;

            m_flitData[i] = low;
        }
    }
}

void Flit::setZeroData()
{
    unsigned long long high, low;

    if (g_cfg.link_width < 64) {
        if (g_cfg.link_width == 32) {
            low = 0x0;
            low = (low << 32) >> 32;
            m_flitData[0] = low;
        } else {
            assert(0);
        }
    } else {
        for (int i=0; i<g_cfg.flit_sz_64bit_multiple; i++) {
            high = 0x0;
            low = 0x0;
            high = (high << 32);
            low = (low << 32) >> 32;
            low |= high;

            m_flitData[i] = low;
        }
    }
}

string Flit::convertData2Str()
{
    string str = "";

    for (unsigned int i=0; i<m_flitData.size(); i++)
        str += ulonglong2hstr(m_flitData[i]);

    return str;
}

void Flit::print()
{
    printf("flit[p=%-3lld %s f=%-5lld %c]",
           getPkt()->id(),
           get_pkttype_name(getPkt()->m_packet_type),
           m_id,
           FLIT_TYPE_STR(this));
}

////////////////////////////////////////////////////////////////////
// head flit
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// atom flit
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// middle flit
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// tail flit
////////////////////////////////////////////////////////////////////

