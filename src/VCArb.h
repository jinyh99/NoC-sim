#ifndef _VC_ARB_H_
#define _VC_ARB_H_

class Router;

// Reference: A Delay Model and Speculative Architecture for Pipelined Routers, HPCA 2001
// Figure 8(b): Given a routing function which returns virtual channels
//              of a single physical channel (R->p), the virtual-channel
//              allocator needs the first stage of v:1 arbiters for each
//              input virtual channel, followed by a second stage of P_iV:1
//              arbiters for each output virtual channel.

const int max_sz_vc_arb = 32;

class VCArb {
public:
    // Constructors
    VCArb();
    VCArb(Router* p_router);

    // Destructors
    virtual ~VCArb();

    virtual void add(int in_pc, int in_vc, int out_pc) = 0;
    virtual void del(int in_pc, int in_vc) = 0;
    // return a grant vector: <in_pc, in_vc>, out_vc>
    virtual vector< pair< pair< int, int >, int > > grant() = 0;
    virtual bitset< max_sz_vc_arb > getReqBitVector(int out_pc) { return 0; };
    virtual bool hasNoReq() = 0;

    // statistic reset when warmup period ends.
    void resetStats();

    virtual void printCurStatus(FILE* fp) = 0;
    void printStats(ostream& out) const;
    void print(ostream& out) const;

    double getAvgGrantRate() const { return m_grant_rate_tab->mean(); };
    double getAvgReq() const { return m_num_req_tab->mean(); };

protected:
    Router* m_router;
    int m_num_pc;
    int m_num_vc;

    // stats
    table* m_grant_rate_tab;	// #grants/#requests (max=1)
    table* m_num_req_tab;	// #requests
};

// Output operator declaration
ostream& operator<<(ostream& out, const VCArb& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const VCArb& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _VC_ALLOC_H_
