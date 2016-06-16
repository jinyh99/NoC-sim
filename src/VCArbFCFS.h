#ifndef _VC_ARB_FCFS_H_
#define _VC_ARB_FCFS_H_

class Router;

#include "VCArb.h"

class VCArbFCFS : public VCArb {
public:
    // Constructors
    VCArbFCFS();
    VCArbFCFS(Router* p_router);

    // Destructors
    virtual ~VCArbFCFS();

    void add(int in_pc, int in_vc, int out_pc);
    void del(int in_pc, int in_vc);
    vector< pair< pair< int, int >, int > > grant();
    bitset< max_sz_vc_arb > getReqBitVector(int out_pc);
    bool hasNoReq() { return (m_reqQ.size() == 0) ? true: false; };

    void printCurStatus(FILE* fp) { printReqQ(fp); }

protected:
    void printReqQ(FILE* fp);

protected:
    /// input VCs that wait for VC allocation ordered by request time
    vector< pair< pair< int, int >, int>  > m_reqQ;	// < <in_pc, in_vc>, out_pc>
};

// Output operator declaration
ostream& operator<<(ostream& out, const VCArbFCFS& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const VCArbFCFS& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _VC_ARB_FCFS_H_
