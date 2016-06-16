using namespace std;
#include <bitset>
#include <vector>
#include <assert.h>
#include "noc_def.h"
#include "Packet.h"

void Packet::init()
{
    m_id = 0;
    m_src_router_id = INVALID_ROUTER_ID;
    m_dest_router_id = INVALID_ROUTER_ID;
    m_src_core_id = INVALID_CORE_ID;
    m_dest_core_id = INVALID_CORE_ID;
    m_dest_router_id_bs.reset();
    m_dest_core_id_bs.reset();

    m_start_flit_id = 0;
    m_num_flits = 0;
    m_hops = 0;
    m_wire_delay = 0;

    m_clk_gen = INVALID_CLK;
    m_clk_store_NIin = INVALID_CLK;
    m_clk_enter_net = INVALID_CLK;
    m_clk_out_net = INVALID_CLK;
    m_clk_store_NIout = INVALID_CLK;

    m_NI_in_pos = -1;
    m_NI_out_pos = -1;
    m_packet_type = PACKET_TYPE_UNDEFINED;

    // compression
    m_num_compr_flits = 0;
}

void Packet::destroy()
{
    m_packetData_vec.clear();

    // compression
    m_en_new_value_vec.clear();
    m_en_value_vec.clear();
}

int Packet::getSrcRouterID()
{
    return m_src_router_id;
}

void Packet::setSrcRouterID(int src_router_id)
{
    m_src_router_id = src_router_id;
}

int Packet::getDestRouterID()
{
    assert(m_dest_router_id_bs.count() == 1);
    return m_dest_router_id;
}

vector< int > Packet::getDestRouterIDVec()
{
    vector< int > dest_router_id_vec;

    for (unsigned int i=0; i<m_dest_router_id_bs.size(); ++i) {
        if (m_dest_router_id_bs[i])
            dest_router_id_vec.push_back(i);
    }

    return dest_router_id_vec;
}

int Packet::getDestRouterNum()
{
    return m_dest_router_id_bs.count();
}

void Packet::addDestRouterID(int dest_router_id)
{
    if (m_dest_router_id_bs.count() == 0)
        m_dest_router_id = dest_router_id;

    m_dest_router_id_bs[dest_router_id] = true;
}

void Packet::addDestRouterIDVec(vector< int > dest_router_id_vec)
{
    assert(dest_router_id_vec.size() > 0);

    m_dest_router_id = dest_router_id_vec[0];
    for (unsigned int i=0; i<dest_router_id_vec.size(); i++) {
        m_dest_router_id_bs[dest_router_id_vec[i]] = true;
    }
}

void Packet::delDestRouterID(int dest_router_id)
{
    assert(m_dest_router_id_bs[dest_router_id]);
    m_dest_router_id_bs[dest_router_id] = false;
}

bool Packet::testDestRouterID(int dest_router_id)
{
    return m_dest_router_id_bs[dest_router_id];
}

int Packet::getSrcCoreID()
{
    return m_src_core_id;
}

void Packet::setSrcCoreID(int src_core_id)
{
    m_src_core_id = src_core_id;
}

int Packet::getDestCoreID()
{
    assert(m_dest_core_id_bs.count() == 1);
    return m_dest_core_id;
}

vector< int > Packet::getDestCoreIDVec()
{
    vector< int > dest_core_id_vec;

    for (unsigned int i=0; i<m_dest_core_id_bs.size(); ++i) {
        if (m_dest_core_id_bs[i])
            dest_core_id_vec.push_back(i);
    }

    return dest_core_id_vec;
}

int Packet::getDestCoreNum()
{
    return m_dest_core_id_bs.count();
}

void Packet::addDestCoreID(int dest_core_id)
{
    if (m_dest_core_id_bs.count() == 0)
        m_dest_core_id = dest_core_id;

    m_dest_core_id_bs[dest_core_id] = true;
}

void Packet::addDestCoreIDVec(vector< int > dest_core_id_vec)
{
    assert(dest_core_id_vec.size() > 0);

    m_dest_core_id = dest_core_id_vec[0];
    for (unsigned int i=0; i<dest_core_id_vec.size(); i++) {
        m_dest_core_id_bs[dest_core_id_vec[i]] = true;
    }
}

void Packet::delDestCoreID(int dest_core_id)
{
    assert(m_dest_core_id_bs[dest_core_id]);
    m_dest_core_id_bs[dest_core_id] = false;
}

bool Packet::testDestCoreID(int dest_core_id)
{
    return m_dest_core_id_bs[dest_core_id];
}
