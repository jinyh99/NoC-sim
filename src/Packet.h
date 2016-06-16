#ifndef _PACKET_H_
#define _PACKET_H_

enum PACKET_TYPE {
    PACKET_TYPE_UNDEFINED = 0,
    PACKET_TYPE_UNICAST_SHORT,
    PACKET_TYPE_UNICAST_LONG,
    PACKET_TYPE_MULTICAST_SHORT,
    PACKET_TYPE_MULTICAST_LONG,
};

// #define MAX_PACKET_NUM_DEST		128
#define MAX_PACKET_NUM_DEST		256
// #define MAX_PACKET_NUM_DEST		1024	// 1K-core chip

/**
 * <pre>
 * packet timestamp for latency measurement:
 * 
 *        = m_clk_gen
 *        |
 *     |  |
 *   t |  = m_clk_store_NIin (Packet is stored in NI-input buffer.)
 *   i |  |
 *   m |  |
 *   e |  = m_clk_enter_net (Head flit enters the network.)
 *     |  |
 *    \ / |
 *     .  |
 *        = m_clk_out_net (Head flit ejects from the network.)
 *        |
 *        |
 *        |
 *        = m_clk_store_NIout (write a tail flit (all flits of a packet) to NI-output buffer)
 *
 *   packet latency = m_clk_store_NIout - m_clk_gen
 *   queing latency = m_clk_enter_net - m_clk_gen
 *   network latency = m_clk_store_NIout - m_clk_enter_net
 *   warmhole-overhead latency = m_clk_store_NIout - m_clk_out_net
 *
 * </pre>
 */

class Packet {
public:
    unsigned long long m_start_flit_id;
    int	      m_num_flits;
    int       m_hops;		// hop count (increased at RC stage of every router)
    int       m_wire_delay;	// accumulated wire delay (T_w)

    // Timestamp
    double    m_clk_gen;	// clock when packet is generated
    double    m_clk_store_NIin;	// clock when packet is stored to NI input packet buffer
    double    m_clk_enter_net;	// clock when packet (head flit) enters network
    double    m_clk_out_net;	// clock when packet (head flit) goes out of network
    double    m_clk_store_NIout; // clock when packet (tail flit) is stored to output NI packet buffer

    vector< unsigned long long > m_packetData_vec;

    // multiple injections/ejections
    // e.g. CMP simulation: port-0 for L1$, port-1 for L2$, port-2 for Directory
    int m_NI_in_pos;	// input NI position
    int m_NI_out_pos;	// output NI position

    unsigned int m_packet_type;

    //////////////////////////////////////////////
    // compression
    vector< string > m_en_new_value_vec;// new values to make en/decoders synchorinize
    vector< string > m_en_value_vec;	// original values successfully encoded
    int m_num_compr_flits;		// #flits that are compressed
    //////////////////////////////////////////////

public:
    void init();
    void destroy();
    void setID(unsigned long long id) { m_id = id; };
    const unsigned long long id() { return m_id; };

    int getSrcRouterID();
    void setSrcRouterID(int src_router_id);
    int getDestRouterID();	// only for unicast packet
    vector< int > getDestRouterIDVec();
    int getDestRouterNum();
    void addDestRouterID(int dest_router_id);
    void addDestRouterIDVec(vector< int > dest_router_id_vec);
    void delDestRouterID(int dest_router_id);
    bool testDestRouterID(int dest_router_id);

    int getSrcCoreID();
    void setSrcCoreID(int src_core_id);
    int getDestCoreID();	// only for unicast packet
    vector< int > getDestCoreIDVec();
    int getDestCoreNum();
    void addDestCoreID(int dest_core_id);
    void addDestCoreIDVec(vector< int > dest_core_id_vec);
    void delDestCoreID(int dest_core_id);
    bool testDestCoreID(int dest_core_id);

protected:
    unsigned long long m_id;

    int m_src_router_id;	// source router id
    int m_src_core_id;		// source core id

    // NOTE: Unicast packet does not use bitset.
    //       Avoiding STL bitset makes simulation fast.
    int m_dest_router_id;	// destination router id
    int m_dest_core_id;		// destination core id

    // for multicast
    bitset< MAX_PACKET_NUM_DEST > m_dest_router_id_bs;
    bitset< MAX_PACKET_NUM_DEST > m_dest_core_id_bs;
};

#endif
