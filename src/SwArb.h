#ifndef _SW_ARB_H_
#define _SW_ARB_H_

class Router;

class SwReq {
public:
    bool m_requesting;
    double req_clk;
    int in_pc;          // for reverse reference
    int in_vc;          // for reverse reference
    int out_pc;         // for reverse reference
    int out_vc;         // for reverse reference
};

const int max_sz_sw_arb = 32;

class SwArb {
public:
    // Constructors
    SwArb();
    SwArb(Router* p_router, int v1_arb_config=SW_ALLOC_RR, int p1_arb_config=SW_ALLOC_RR);

    // Destructors
    virtual ~SwArb();

    void init();

    // return free input PCs
    // element format: <in_pc, in_vc>
    vector< pair< int, int > > getFreeInPorts();

    void add(int in_pc, int in_vc, int out_pc, int out_vc);
    void del(int in_pc, int in_vc);

    // return a grant vector
    // element format : <in_pc, in_vc>, <out_pc, out_vc>
    virtual vector< pair< pair< int, int >, pair< int, int > > > grantRegular() = 0;
    virtual vector< pair< pair< int, int >, pair< int, int > > > grantSpec() = 0;

    // statistic reset when warmup period ends.
    void resetStats();

    void printCurStatus(FILE* fp);
    void printStats(ostream& out) const;
    void print(ostream& out) const;

    double getGrantRate() const { return m_grant_rate_tab->mean(); };
    double getSpecGrantRate() const { return m_spec_grant_rate_tab->mean(); };
    bool hasNoReq(int check_in_pc, int check_out_pc);

protected:
    void del(SwReq& req);
    bool isReqValid(SwReq& req);

protected:
    // arbiter type configuration
    int m_v1_arb_config;
    int m_p1_arb_config;

    Router* m_router;
    int m_num_pc;
    int m_num_vc;

    vector< vector< SwReq > > m_swReq_vec;	// X[in_pc][in_vc]
    int m_num_reqs;		// valid #entries in m_swReq_vec

    // stats for grant rate (max=1)
    table* m_grant_rate_tab;		// #grants/#requests
    table* m_spec_grant_rate_tab;	// #grants/#requests
};

// Output operator declaration
ostream& operator<<(ostream& out, const SwArb& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const SwArb& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _SW_ARB_H_
