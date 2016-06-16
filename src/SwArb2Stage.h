#ifndef _SW_ARB_2STAGE_H_
#define _SW_ARB_2STAGE_H_

/**
 * Switch(crossbar) arbitration has two stages.
 * stage 1 - V:1 arbiters: one input VC for each input PC
 * stage 2 - P:1 arbiters: one input PC for each output PC
 */

#include "SwArb.h"

class SwArb2Stage : public SwArb {
public:
    // Constructors
    SwArb2Stage();
    SwArb2Stage(Router* p_router, int v1_arb_config=SW_ALLOC_LRS, int p1_arb_config=SW_ALLOC_LRS);

    // Destructor
    ~SwArb2Stage();

    // Public Methods
    vector< pair< pair< int, int >, pair< int, int > > > grantRegular() { return grantGeneric(false); };
    vector< pair< pair< int, int >, pair< int, int > > > grantSpec() { return grantGeneric(true); };

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    vector< pair< pair< int, int >, pair< int, int > > > grantGeneric(bool spec);

    int RR_ORDER(int cur_val, int start_val, int max_val);
    int V1_LRS_ORDER(int cpos, int in_pc, int in_vc);
    int P1_LRS_ORDER(int cpos, int out_pc, int in_pc);

private:
    /////////////////////////////////////////////////////////////////////////////
    // NOTE: The 0-th D counters are used for regular switch arbitration.
    //       The 1-th D counters are used for speculative switch arbitration.
    /////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////
    // round-robin(RR) counters
    vector< vector< int > > m_RR_sa_v1a_vec;    // X[type][in_pc]
    vector< vector< int > > m_RR_sa_p1a_vec;    // X[type][out_pc]
    /////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////
    // LRS counters: order value for least recently selected
    //               0 - highest order, n - lowest order
    // e.g. for type=0 inPC-0 V(=3):1 arbitration
    //      format: v1a_vec[type][in_pc][order] = in_vc
    //      v1a_vec[0][0][0] = 1
    //      v1a_vec[0][0][1] = 2
    //      v1a_vec[0][0][2] = 0 
    //      LRS order (highest -> lowest) : inVC-1, inVC-2, inVC-0
    // e.g. for type=0 outPC-2 P(=5):1 arbitration
    //      format: p1a_vec[type][out_pc][order] = in_pc
    //      p1a_vec[0][2][0] = 2
    //      p1a_vec[0][2][1] = 1
    //      p1a_vec[0][2][2] = 0 
    //      p1a_vec[0][2][3] = 4 
    //      p1a_vec[0][2][4] = 3 
    //      LRS order (highest -> lowest) : inPC-2, inPC-1, inPC-0, inPC-4, inPC-3
    vector< vector< vector< int > > > m_LRS_sa_v1a_vec;	// X[type][in_pc][order] = in_vc
    vector< vector< vector< int > > > m_LRS_sa_p1a_vec;	// X[type][out_pc][order] = in_pc
    /////////////////////////////////////////////////////////////////////////////
};

// Output operator declaration
ostream& operator<<(ostream& out, const SwArb2Stage& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const SwArb2Stage& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

inline int SwArb2Stage::RR_ORDER(int cur_val, int start_val, int max_val)
{
    int diff = cur_val - start_val;
    int order = (diff < 0) ? (diff + max_val) : diff;
    assert(order >= 0);

    return order;
}

inline int SwArb2Stage::V1_LRS_ORDER(int cpos, int in_pc, int in_vc)
{
    int order = -1;
    for (int test_order=0; test_order<m_num_vc; test_order++) {
        if (m_LRS_sa_v1a_vec[cpos][in_pc][test_order] == in_vc) {
            order = test_order;
            break;
        }
    }
    assert(order >= 0);

    return order;
}

inline int SwArb2Stage::P1_LRS_ORDER(int cpos, int out_pc, int in_pc)
{
    int order = -1;
    for (int test_order=0; test_order<m_num_pc; test_order++) {
        if (m_LRS_sa_p1a_vec[cpos][out_pc][test_order] == in_pc) {
            order = test_order;
            break;
        }
    }
    assert(order >= 0);

    return order;
}
#endif // #ifndef _SW_ARB_2STAGE_H_
