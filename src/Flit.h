#ifndef _FLIT_H_
#define _FLIT_H_

enum {
    UNDEFINED_FLIT = 0x0,
    HEAD_FLIT = 0x1,
    MIDL_FLIT = 0x2,
    TAIL_FLIT = 0x4,
    ATOM_FLIT = 0x8,	// for single-flit packet
};

class Packet;

class Flit {
public:
    Flit() {};
    virtual ~Flit() {};

    virtual void init() = 0;
    virtual void destroy() = 0;
    void setID(unsigned long long id) { m_id = id; };
    const unsigned long long id() { return m_id; };

    const bool isHead() { return (m_flitType & (ATOM_FLIT | HEAD_FLIT)) ? true : false; };
    const bool isTail() { return (m_flitType & (ATOM_FLIT | TAIL_FLIT)) ? true : false; };
    const int type() { return m_flitType; };
    void setRandomData();
    void setZeroData();
    string convertData2Str();

    void setPkt(Packet* pkt) { m_pkt_ptr = pkt; };
    Packet* getPkt() { return m_pkt_ptr; };
    void print();

public:
    vector< unsigned long long > m_flitData;	// flit data

    double m_clk_enter_stage;	// clock when this flit is in a router pipeline stage
    double m_clk_enter_router;	// clock when this flit is in a router

protected:
    void init_common();
    void destroy_common();

    unsigned long long m_id;	// flit id
    int m_flitType;	// flit type
    Packet* m_pkt_ptr;		// pointer to the packet
};

class FlitHead : public Flit {
public:
    FlitHead() {
        m_flitType = HEAD_FLIT;
        m_flitData.resize(g_cfg.flit_sz_64bit_multiple);
    };
    ~FlitHead() {};

    void init() {
        init_common();
        m_fattree_up_dir = true;
    };
    void destroy() { destroy_common(); };

public:
    int src_router_id() const { return m_pkt_ptr->getSrcRouterID(); };
    int dest_router_id() const { return m_pkt_ptr->getDestRouterID(); };

    int src_core_id() const { return m_pkt_ptr->getSrcCoreID(); };
    int dest_core_id() const { return m_pkt_ptr->getDestCoreID(); };

    // source routing
    deque< int > m_source_routing_vec;	// path vector

    bool m_fattree_up_dir;
};

class FlitAtom : public FlitHead {
public:
    FlitAtom() {
        m_flitType = ATOM_FLIT;
        m_flitData.resize(g_cfg.flit_sz_64bit_multiple);
    };
    ~FlitAtom() {};

    void init() {
        init_common();
        m_fattree_up_dir = true;
        m_crc_check = 0;
    };
    void destroy() { destroy_common(); };

public:
    unsigned long long m_crc_check;
};

class FlitMidl : public Flit {
public:
    FlitMidl() {
        m_flitType = MIDL_FLIT;
        m_flitData.resize(g_cfg.flit_sz_64bit_multiple);
    };
    ~FlitMidl() {};

    void init() { init_common(); };
    void destroy() { destroy_common(); };
};

class FlitTail : public Flit {
public:
    FlitTail() {
        m_flitType = TAIL_FLIT;
        m_flitData.resize(g_cfg.flit_sz_64bit_multiple);
    };
    ~FlitTail() {};

    void init() {
        init_common();
        m_crc_check = 0;
    };
    void destroy() { destroy_common(); };

public:
    unsigned long long m_crc_check;
};


#define FLIT_TYPE_STR(flit) (((flit) == 0) ? 'I' : \
        ((flit)->type() == HEAD_FLIT) ? 'H' : \
        ((flit)->type() == MIDL_FLIT) ? 'M' : \
        ((flit)->type() == TAIL_FLIT) ? 'T' : \
        ((flit)->type() == ATOM_FLIT) ? 'A' : \
        'U')

#endif // #ifndef _FLIT_H_
